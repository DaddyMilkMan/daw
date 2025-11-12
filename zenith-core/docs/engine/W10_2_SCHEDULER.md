## W10.2 — Sample-Accurate Scheduler (Skeleton)

**Scope**: compile-only, mixer-agnostic. No runtime cost when `ZENITH_ENABLE_PHASE1_AUDIO=OFF`.

### Transport

- `Engine::transportSamples()` — absolute timeline in samples.
- `play() / pause() / seekSamples()` — message-thread only.
- Transport advances in the audio callback **only when playing**.

### Events

- `TransportEvent` { StartClip, StopClip, trackIndex, clipId, whenSamples }.
- SPSC ring buffer (`rt::SpscRing<T,4096>`) for lock-free enqueue (UI→RT).
- Engine drains events due in current block `[start, end)`.
- Current hookup: debug log. Next step (W10.3) routes to Mixer.

### RT Rules

- No heap allocations in the callback.
- All buffers pre-allocated in `prepareToPlay()`.
- Single producer (message thread), single consumer (audio thread).

### Architecture

```
Message Thread                Audio Thread
─────────────────────        ──────────────────────
engine.play()                transport starts advancing
engine.scheduleClipStart()
  → eventQ_.push()           processAudio():
                               blockStart = transportSamples_
                               blockEnd = blockStart + numSamples

                               while (eventQ_.pop(ev)):
                                 if ev.whenSamples in [blockStart, blockEnd):
                                   DBG log event + offset
                                   (future: mixer hookup)
                                 if ev.whenSamples >= blockEnd:
                                   stash for re-queue

                               mixer.process(blockStart)

                               if (isPlaying_):
                                 transportSamples_ = blockEnd
```

### Implementation Details

**Lock-Free Ring Buffer:**
- Power-of-two capacity (4096 events)
- Cache-line aligned atomics (64 bytes)
- Acquire/release memory ordering
- Safe for SPSC (single producer, single consumer)

**Event Draining:**
1. Pop events from queue
2. Check if event is due in current block
3. If future (>= blockEnd), buffer locally and re-enqueue
4. If due, log (or in future: dispatch to mixer)
5. Continue draining until queue empty or future event found

**Transport Advancement:**
- Only advances when `isPlaying_` is true
- Atomic store with relaxed ordering (no synchronization needed)
- Sample-accurate: advances exactly by `numSamples` per block

### Testing

**Manual Test (Dev Build):**

```cpp
// In UI code or test:
auto& engine = getEngine();
engine.play();  // Start transport

// Schedule clip to start at 2 seconds (96000 samples @ 48kHz)
engine.scheduleClipStart(0, 1, 96000);

// Expected: DBG log appears at exactly 96000 samples
// "Scheduler: due event type=0 track=0 clip=1 when=96000 (offset N)"
```

**Verification:**
- Flag OFF: No scheduler code compiled, binary unchanged
- Flag ON: Events logged at correct sample offset
- No allocations in processAudio (check with instrumentation)
- Queue full handling (returns false from schedule* methods)

### Future Work (W10.3)

- Route events to Mixer: `mixer_.startClip(trackIndex, clipId, offsetInBlock)`
- Add event types: FadeIn, FadeOut, SetParam
- Priority queue for large event counts
- UI timeline integration (drag clips → auto-schedule)

### Files Modified

```
include/rt/SpscRing.h         (new) - Lock-free SPSC ring buffer
include/Engine.h              - Transport API, TransportEvent struct
src/Engine.cpp                - Transport implementation, event draining
docs/engine/W10_2_SCHEDULER.md (new) - This document
```

### Safety Guarantees

1. **No RT allocations:** All buffers pre-allocated in prepareToPlay()
2. **Lock-free:** SPSC ring uses only atomics (no mutexes)
3. **Graceful degradation:** Queue full returns false (caller can retry or log)
4. **Sample-accurate:** Event offset calculated precisely within block
5. **Zero cost when disabled:** All code gated by `ZENITH_ENABLE_PHASE1_AUDIO`
