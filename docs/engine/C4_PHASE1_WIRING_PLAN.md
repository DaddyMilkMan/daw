# Phase 1 Wiring Plan (Tracks/Clips → Audio Callback)

**Scope:** Connect `zenith::Track`/`Clip` to `Engine::processAudio()` with strict RT-safety.

## Targets
- Mixdown loop in `processAudio()` iterates over tracks (read-only)
- Per-track `prepareToPlay()` and shared `AudioFormatManager`
- No heap allocs, no locks on RT thread
- GUI metering via lock-free FIFO

## Steps

### 1. Preparation Layer
- Engine holds `AudioFormatManager` and pre-loads formats
- Call `track.prepareToPlay(sr, blockSize)` on device (re)start
- Store current sample rate and buffer size in atomics (already done)

### 2. Process Loop
**In `Engine::processAudio()`:**
```cpp
// Pseudo-code (Phase 1 implementation)
for (const auto& track : tracks_) {
    if (!track) continue;

    // Create temp buffer for this track
    juce::AudioBuffer<float> trackBuffer(numOutputChannels, numSamples);
    juce::AudioSourceChannelInfo trackInfo(&trackBuffer, 0, numSamples);

    // Get track's audio (calls Track::getNextAudioBlock)
    track->getNextAudioBlock(trackInfo);

    // Mix into output
    for (int ch = 0; ch < numOutputChannels; ++ch) {
        juce::FloatVectorOperations::add(
            outputChannelData[ch],
            trackBuffer.getReadPointer(ch),
            numSamples);
    }
}
```

### 3. Meters
- Write peaks to `juce::AbstractFifo` from RT thread
- Read on GUI timer (60 Hz)
- Per-track metering in Phase 1.1

### 4. Undo/Commands
**Non-RT operations (message thread only):**
- Create/delete track/clip
- File loads
- Fade adjustments
- Plugin loading (Phase 2)

### 5. ASIO/Exclusive
- Reuse device setup from Engine
- Re-init tracks on device change (call `prepareToPlay()` again)

### 6. Safety Rules
**Forbidden on RT thread:**
- `new`, `delete`, `malloc`, `free`
- `std::string`, `juce::String` (allocations)
- `std::vector::push_back` (reallocation)
- `std::mutex`, `juce::CriticalSection` (blocking)
- File I/O, logging, network

**Allowed on RT thread:**
- `std::atomic` load/store
- Pre-allocated buffers
- `juce::FloatVectorOperations` (SIMD)
- Lock-free data structures (`AbstractFifo`)

## Milestones
- **P1.0:** Audio playback of clips (no plugins)
- **P1.1:** Simple metering (peak levels)
- **P1.2:** Basic transport sync
- **P1.3:** Bar/beat mapping (later)

## Current Status (C4)
- ✅ Tracks can be created (`Engine::addTestTracks()`)
- ✅ Track container exists (`std::vector<std::unique_ptr<zenith::Track>>`)
- ⚠️ NOT wired to audio callback yet (intentional)
- ⚠️ `prepareToPlay()` not called (Phase 1 task)
- ⚠️ No mixdown loop (Phase 1 task)
