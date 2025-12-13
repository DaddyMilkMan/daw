# Zenith DAW Threading Model

## Overview

Zenith DAW uses a strict three-tier threading model to ensure real-time audio performance
while maintaining responsive UI and safe background operations. This document defines
the contract that ALL code must follow.

---

## Thread Categories

### 1. Audio Thread (Real-Time, Lock-Free)

**Identity**: The `audioDeviceIOCallbackWithContext()` callback and any code called from it.

**Rules**:
- ✅ **RT-Safe Only**: Never allocate, never block, never lock
- ✅ **Atomics for State**: Read transport state via `std::atomic` only
- ✅ **Pre-allocated Buffers**: Use `trackBuffers_`, `clipBuffer_`, etc.
- ✅ **Lock-free FIFOs**: Use `juce::AbstractFifo` for message passing
- ✅ **Track Snapshots**: Read from `activeSnapshot_` (RCU-style)

**Forbidden Actions**:
- ❌ `new`, `delete`, `malloc`, `free`
- ❌ `std::mutex`, `std::lock_guard`, `juce::CriticalSection::lock()`
- ❌ `juce::MessageManager::callAsync()`
- ❌ File I/O, network I/O
- ❌ `DBG()`, `juce::Logger`
- ❌ `jassert(juce::MessageManager::getInstance()->isThisTheMessageThread())`

**Key Classes (Audio Thread Safe)**:
- `AudioRenderer::renderAudioGraph()` - Main audio graph rendering
- `TransportController::advancePlayhead()` - Playhead advancement
- `RecordingManager::captureAudio()` - RT-safe audio capture
- `Track::processAudio()` - Per-track audio processing
- `MidiFifo` - Lock-free MIDI routing

---

### 2. Message Thread (JUCE Main Thread)

**Identity**: The thread where `juce::MessageManager::isThisTheMessageThread()` returns true.

**Rules**:
- ✅ **UI Operations**: All component painting, resizing, callbacks
- ✅ **ProjectState Mutations**: All ValueTree modifications
- ✅ **Engine Control**: `play()`, `stop()`, `record()`, track creation
- ✅ **Snapshot Publishing**: Update `activeSnapshot_` after track changes
- ✅ **Plugin GUI**: Open/close plugin editors

**Assertion Pattern**:
```cpp
void Engine::createTrack(...) {
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    // ... implementation
}
```

**Key Classes (Message Thread Only)**:
- `ProjectState` - All mutations
- `Engine::addTrack()`, `Engine::removeTrack()`
- `RecordingManager::startRecording()`, `stopRecording()`
- `TransportController::play()`, `stop()` (trigger callbacks)
- All UI Components

---

### 3. Background Threads

**Types**:
1. **Audio File Pool Thread** - Async audio file loading
2. **Plugin Scan Thread** - VST3/AU scanning  
3. **Recording Writer Thread** - Disk writing for recordings
4. **AI Agent Threads** - SampleHunterAgent, SessionDebuggerAgent

**Rules**:
- ✅ **Message Thread Callbacks**: Post results via `juce::MessageManager::callAsync()`
- ✅ **Atomic State Updates**: Use `std::atomic` for progress/status
- ✅ **Lock-free FIFO**: Communicate with audio thread via FIFO

**Example Pattern**:
```cpp
void BackgroundTask::run() {
    // Do heavy work...
    float result = computeExpensiveThing();
    
    // Post result to message thread
    juce::MessageManager::callAsync([this, result]() {
        onComplete(result);
    });
}
```

---

## Data Flow Contracts

### ProjectState → Engine (ValueTree to Audio)

**Direction**: Message Thread → Audio Thread

**Pattern**:
1. UI/Code modifies `ProjectState` ValueTree (message thread)
2. `ProjectEngineBridge` listener detects change (message thread)
3. Bridge batches updates, calls Engine setters (message thread)
4. Engine updates atomic state or rebuilds snapshot (message thread)
5. Audio thread reads atomics / snapshot on next buffer (audio thread)

```
[ProjectState]  --ValueTree::Listener-->  [ProjectEngineBridge]
                                               |
                                               v (message thread)
                                          [Engine setters]
                                               |
                                               v (atomic)
                                          [Audio Thread reads]
```

### Engine → ProjectState (Audio to ValueTree)

**Direction**: Audio Thread → Message Thread

**Pattern** (explicit commit, not automatic):
1. Engine atomics updated during audio processing
2. User triggers explicit commit (e.g., stop recording)
3. Message thread reads final atomic values
4. Message thread updates ProjectState

**Example**: Recording creates clips ONLY at `stopRecording()`, not during.

---

## Atomic vs. Message-Thread Patterns

### ✅ CORRECT: Pure Atomic (Audio Thread Readable)

```cpp
// In TransportController.h
std::atomic<bool> isPlaying_{false};
std::atomic<juce::int64> playheadSamples_{0};

// Read from audio thread - RT-safe
juce::int64 pos = transportController_->getPlayheadSamples();
```

### ✅ CORRECT: Pure Message Thread (UI-Triggered)

```cpp
void Engine::createTrack(const juce::String& name) {
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    
    auto track = std::make_shared<Track>(name);
    tracks_.push_back(track);
    updateTrackSnapshot();  // Publish to audio thread
}
```

### ❌ INCORRECT: Mixed Pattern (DO NOT USE)

```cpp
// BAD: Atomic operation with message-thread assertion
void Engine::setPlayhead(juce::int64 pos) {
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    playheadSamples_.store(pos);  // Why assert? Atomic is already safe!
}
```

### ✅ FIXED: Choose One Pattern

```cpp
// Option A: Remove assertion (atomic is inherently thread-safe)
void TransportController::setPlayheadSamples(juce::int64 pos) {
    playheadSamples_.store(pos);
    if (onSeek_) onSeek_();  // Callback may need message thread
}

// Option B: Keep assertion, document why (callback requirement)
void TransportController::setPlayheadSamples(juce::int64 pos) {
    // Message thread required because onSeek_ callback may touch UI
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    playheadSamples_.store(pos);
    if (onSeek_) onSeek_();
}
```

---

## Lock-Free Communication Patterns

### FIFO (Audio → Message Thread)

```cpp
// Producer (Audio Thread)
recordingManager_->captureMidi(msg, samplePos, trackIndex);

// Consumer (Message Thread, called on timer)
recordingManager_->drainMidiFifo();
```

### RCU Snapshots (Message → Audio Thread)

```cpp
// Producer (Message Thread)
void Engine::updateTrackSnapshot() {
    auto newSnapshot = std::make_shared<TrackSnapshot>(tracks_, auxBuses_);
    currentSnapshotHolder_ = newSnapshot;
    activeSnapshot_.store(newSnapshot.get());  // Publish atomically
}

// Consumer (Audio Thread)
TrackSnapshot* snapshot = activeSnapshot_.load();
for (Track* track : snapshot->tracks) {
    track->processAudio(...);
}
```

---

## Class Threading Annotations

Every class should document its threading requirements in the header:

```cpp
/**
 * @class AudioRenderer
 * 
 * Thread Safety:
 * - renderAudioGraph() is AUDIO THREAD ONLY
 * - All other methods are MESSAGE THREAD ONLY
 * - getMasterLevel() uses atomic (safe from any thread)
 */
class AudioRenderer { ... };
```

---

## Checklist for Code Review

### Audio Thread Code
- [ ] No `new`/`delete`/`malloc`/`free`
- [ ] No mutex locks
- [ ] No `jassert(isMessageThread)`
- [ ] Uses pre-allocated buffers
- [ ] Uses atomics for cross-thread state

### Message Thread Code
- [ ] Has `jassert(isMessageThread)` if required
- [ ] Publishes snapshots after modifying tracks
- [ ] Uses `callAsync` for UI updates from callbacks

### Background Thread Code
- [ ] Posts results via `callAsync`
- [ ] Uses atomic for progress reporting
- [ ] Never touches UI directly

---

## Migration Guide

### Removing Conflicting Patterns

**Before** (conflicting):
```cpp
void Engine::setLooping(bool loop) {
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    isLooping_.store(loop);  // Atomic but also asserts message thread
}
```

**After** (delegated to TransportController):
```cpp
void Engine::setLooping(bool loop) {
    transportController_->setLooping(loop);
}

// In TransportController (pure atomic, no assertion):
void setLooping(bool shouldLoop) {
    isLooping_.store(shouldLoop);
}
```

---

## Summary

| Thread | Purpose | Allocation | Locking | ValueTree |
|--------|---------|------------|---------|-----------|
| Audio | DSP, mixing | ❌ Never | ❌ Never | ❌ Never |
| Message | UI, state | ✅ Yes | ✅ Yes | ✅ Yes |
| Background | I/O, scanning | ✅ Yes | ✅ Yes | ❌ Post to message |

**Golden Rule**: If you're on the audio thread, pretend memory allocation and locking don't exist.
