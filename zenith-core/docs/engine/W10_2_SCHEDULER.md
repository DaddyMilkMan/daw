# W10.2: Sample-Accurate Transport Scheduler

## Overview

Lock-free sample-accurate event scheduling system for Phase 1 audio.

**Components:**
- `SpscRing<T>` - Single-producer single-consumer ring buffer with peek-then-pop
- `TransportEvent` - Clip start/stop events with sample-accurate timing
- Event scheduler API: `scheduleClipStart()`, `scheduleClipStop()`, `seekSamples()`
- `drainScheduledEvents()` - RT-safe consumption in audio callback

**Gating:** All code behind `ZENITH_ENABLE_PHASE1_AUDIO` (OFF in release-safe)

---

## Threading Model

### SPSC Contract

The event queue is **single-producer, single-consumer**:

- **Producer (Message Thread):** `scheduleClipStart/Stop()` → `eventQ_.tryPush()`
- **Consumer (Audio Thread):** `drainScheduledEvents()` → `eventQ_.tryPeek()/tryPop()`

### Critical Constraint: seekSamples()

`seekSamples()` **violates SPSC** by being both consumer and producer on the message thread:
- Calls `tryPop()` to drain stale events
- Calls `tryPush()` to re-queue future events

**Therefore:** `seekSamples()` can ONLY be called when `isPlaying_ == false`

**Enforcement:**
```cpp
void Engine::seekSamples(int64_t targetSample)
{
    jassert(!isPlaying_.load(std::memory_order_acquire));
    // ... drain and rewrite queue
}
```

This ensures audio thread is NOT draining events when we rewrite the queue.

---

## Usage Patterns

### Scheduling Events (Anytime)

Safe to call from message thread at any time:

```cpp
// Schedule clip to start at sample 48000
bool ok = engine.scheduleClipStart(trackIndex: 0, clipId: 123, startSample: 48000);
if (!ok) {
    // Queue full - check engine.getDroppedEvents()
}

// Schedule clip to stop at sample 96000
engine.scheduleClipStop(trackIndex: 0, clipId: 123, stopSample: 96000);
```

**Backpressure:** Returns `false` if queue is full, increments `droppedEvents_`

### Seeking (Stop Required)

**MUST** stop playback before seeking:

```cpp
engine.stop();                   // isPlaying_ = false, audio callback won't drain queue
engine.seekSamples(newPosition); // Safe to rewrite queue on message thread

// Optionally reschedule clips relative to new position
engine.scheduleClipStart(0, clipId, newPosition + offset);

engine.play();                   // Resume playback
```

**Why This Works:**
- `stop()` sets `isPlaying_ = false`
- Audio callback checks `isPlaying_` and skips `drainScheduledEvents()` when false
- Message thread has exclusive access to queue during seek
- SPSC contract maintained: never concurrent producers/consumers

**Live Scrubbing:**
Not supported in Phase 1. For scrubbing while playing, would need:
- MPMC queue (multiple consumers), OR
- Double-buffered event queues with atomic swap, OR
- Custom timeline structure (read-only on RT thread)

---

## Event Handling Semantics

### Block Interval: [blockStart, blockEnd)

Half-open interval where `blockEnd = blockStart + numSamples`

```cpp
void drainScheduledEvents(int64_t blockStart, int64_t blockEnd, int numSamples)
{
    TransportEvent ev;
    while (eventQ_.tryPeek(ev))
    {
        if (ev.whenSamples >= blockEnd)
            break;  // Future event - leave in queue for next block

        eventQ_.tryPop(ev);  // Consume now

        if (ev.whenSamples >= blockStart)
        {
            // Event is in this block - process at sample-accurate offset
            const int offset = static_cast<int>(ev.whenSamples - blockStart);
            jassert(offset >= 0 && offset < numSamples);
            // TODO (W10.3): mixer_.onScheduledEvent(ev, offset);
        }
        // else: Overdue event (< blockStart) - dropped silently
    }
}
```

### Event Categories

| Timing | Action | Notes |
|--------|--------|-------|
| `ev.whenSamples < blockStart` | Drop silently | Overdue (missed deadline) |
| `ev.whenSamples in [blockStart, blockEnd)` | Process at offset | Sample-accurate within block |
| `ev.whenSamples >= blockEnd` | Leave in queue | Future event for next block |

**Overdue Policy:** Drop silently (could alternatively clamp to offset 0)

### Seek Behavior

`seekSamples(targetSample)` purges stale events and preserves chronological order:

1. Drain entire queue to temporary vector
2. Filter: keep only events where `whenSamples >= targetSample`
3. Re-push filtered events in original chronological order

**Example:**
```
Queue before seek(50): [10, 30, 70, 90]
Queue after seek(50):  [70, 90]  // 10, 30 dropped as stale
```

**Ordering Guarantee:** Future events maintain relative time order after seek

---

## RT Safety

### Audio Thread (drainScheduledEvents)

**Allowed:**
- ✅ `tryPeek()` / `tryPop()` on event queue
- ✅ Reading atomics (`transportSamples_`, `isPlaying_`)
- ✅ Integer arithmetic for offset calculation
- ✅ `jassert()` (no-op in Release builds)

**Forbidden:**
- ❌ Memory allocation
- ❌ Locks / mutexes
- ❌ Logging (`DBG`, `printf`, etc.)
- ❌ System calls

### Message Thread (scheduleClip*, seekSamples)

**Allowed:**
- ✅ Memory allocation (std::vector in seekSamples)
- ✅ `tryPush()` on event queue (producer)
- ✅ `tryPop()` on event queue (ONLY in seekSamples when stopped)

**Constraints:**
- seekSamples() MUST only be called when `!isPlaying_`
- Enforced by `jassert()` (debug builds crash if violated)

---

## Queue Parameters

```cpp
static constexpr size_t kEventRingCap = 4096;
```

**Sizing:**
- Power-of-2 for fast modulo via bitmask
- 4096 events should handle dense editing at 48kHz for ~85ms worth of events
- Burst edits on timeline may exceed capacity → check `droppedEvents_`

**Performance:**
- Wait-free for single producer/consumer
- Cache-line aligned atomics (64-byte) prevent false sharing
- Lock-free: no contention, no priority inversion

---

## Error Handling

### Queue Full

When `eventQ_.tryPush()` fails:
```cpp
if (!eventQ_.tryPush(ev))
{
    droppedEvents_.fetch_add(1, std::memory_order_relaxed);
    return false;
}
```

**UI Feedback:**
```cpp
uint64_t dropped = engine.getDroppedEvents();
if (dropped > 0) {
    showWarning("Event queue overflow: " + String(dropped) + " events dropped");
}
```

**Mitigation:**
- Increase `kEventRingCap` if sustained drops occur
- Batch schedule events (schedule multiple clips in burst)
- Add telemetry: peak queue depth, drop rate

---

## Known Limitations (Phase 1)

### 1. No Live Scrubbing
- **Constraint:** `seekSamples()` requires `stop()` first
- **Reason:** SPSC contract doesn't allow concurrent rewrite
- **Future:** Upgrade to MPMC or double-buffered queues

### 2. Overdue Events Dropped
- **Behavior:** Events `< blockStart` are silently discarded
- **Alternative:** Could clamp to offset 0 (process immediately)
- **Trade-off:** Drop = predictable, clamp = may cause glitches

### 3. No Event Cancellation
- **Constraint:** Once pushed, events can't be individually cancelled
- **Workaround:** Call `stop()`, `seekSamples()` to rewrite queue
- **Future:** Add event ID tracking for targeted removal

### 4. Fixed Queue Size
- **Constraint:** 4096 events, no dynamic growth
- **Monitoring:** Track `droppedEvents_` counter
- **Mitigation:** Make `kEventRingCap` configurable via CMake

---

## Testing Checklist

### Unit Tests (TODO for W10.3)

- [ ] **Event ordering:** Schedule 3 events around block boundary
  - One before block (should drop)
  - One inside block (should process at correct offset)
  - One after block (should remain queued)

- [ ] **Seek purging:** Queue [10, 20, 30], seek(15) → [20, 30]

- [ ] **Chronological order:** Queue [10, 30, 20], seek(5) → [10, 20, 30]
  - Verifies sort isn't assumed, order preserved

- [ ] **Queue full:** Push 4097 events, verify 4096 succeed, 1 drops

- [ ] **Concurrency:** Stress test with producer/consumer hammering queue

### CI Verification

**release-safe preset:**
```bash
cmake --preset release-safe && cmake --build --preset release-safe
rg "SpscRing|TransportEvent|drainScheduledEvents" out/build/release-safe/compile_commands.json
# Expect: NO MATCHES (code gated out)
```

**dev-debug preset:**
```bash
cmake --preset dev-debug && cmake --build --preset dev-debug
rg "SpscRing|TransportEvent|drainScheduledEvents" out/build/dev-debug/compile_commands.json
# Expect: Matches in Engine.cpp (code included)
```

---

## Future Work (Post-Phase 1)

### W10.3: Wire to Mixer
- Hook `drainScheduledEvents()` → `mixer_.onScheduledEvent(ev, offset)`
- Track active clip state per-track
- Implement 2-5ms fade-in/fade-out on start/stop
- Gapless playback for back-to-back clips

### Phase 2+: Advanced Features
- **Live scrubbing:** MPMC queue or double-buffered queues
- **Event cancellation:** Track event IDs, allow targeted removal
- **Priority levels:** High-priority events (e.g., panic stop) bypass queue
- **Timeline view:** Read-only event timeline for UI visualization
- **Burst scheduling:** Batch API to push multiple events atomically

---

## References

- `zenith-core/include/lockfree/SpscRing.h` - Queue implementation
- `zenith-core/include/Engine.h` - Public API
- `zenith-core/src/Engine.cpp` - Scheduler implementation
- `docs/engine/C4_PHASE1_WIRING_PLAN.md` - Original Phase 1 plan
