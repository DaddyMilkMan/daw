# Transport Controller Migration Guide

## Overview

As of PR C (Transport Refactor), the `TransportController` is now a per-Engine instance with support for sample-accurate scheduled transport actions. This document describes how to migrate code that previously used global transport references.

## Key Changes

### 1. TransportController is Per-Engine

**Before:**
```cpp
// Old code may have used global or static transport access
GlobalTransport::play();  // NOT VALID ANYMORE
```

**After:**
```cpp
// Access transport through the Engine instance
Engine& engine = getEngine();
engine.getTransportController().play();
```

### 2. New Scheduled Action API

The `TransportController` now supports scheduling transport actions at specific wall clock times for sample-accurate synchronization:

```cpp
Engine& engine = getEngine();
TransportController& transport = engine.getTransportController();

// Get current wall clock time (ms since epoch)
auto now = std::chrono::system_clock::now();
auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
    now.time_since_epoch()).count();

// Schedule playback to start in 100ms at position 0
transport.playAt(nowMs + 100, 0.0);

// Schedule stop in 5 seconds
transport.stopAt(nowMs + 5000);

// Schedule seek to position 10.5 seconds in 1 second from now
transport.seekAt(nowMs + 1000, 10.5);
```

### 3. Sample-Accurate Timing

Scheduled actions are processed in the audio thread with sample-accurate timing:

- Actions are enqueued from any thread (message, network, MIDI)
- Actions are consumed in the audio callback at the precise sample offset
- Clock mapping automatically handles drift between wall time and sample time

## Migration Examples

### Example 1: Simple Transport Control

**Before:**
```cpp
void MyComponent::onPlayButtonClicked() {
    transport.play();  // Direct reference
}
```

**After:**
```cpp
void MyComponent::onPlayButtonClicked() {
    engine_.getTransportController().play();
}
```

### Example 2: Networked Sync

**Before:**
```cpp
void NetworkHandler::onSyncMessage(int64_t timestamp) {
    // Manual timing calculation
    waitUntil(timestamp);
    transport.play();
}
```

**After:**
```cpp
void NetworkHandler::onSyncMessage(int64_t timestamp) {
    // Sample-accurate scheduling
    engine_.getTransportController().playAt(timestamp, 0.0);
}
```

### Example 3: MIDI Clock Sync

**Before:**
```cpp
void MidiClockHandler::onStartMessage() {
    // Immediate start (not sample-accurate)
    transport.play();
}
```

**After:**
```cpp
void MidiClockHandler::onStartMessage() {
    // Calculate next bar/beat boundary
    int64_t nextBarTimeMs = calculateNextBarTime();
    
    // Schedule at bar boundary
    engine_.getTransportController().playAt(nextBarTimeMs, 0.0);
}
```

### Example 4: Command API Integration

The CommandAPI has been updated with new scheduling commands:

```cpp
// JSON command to schedule play
{
    "command": "playAt",
    "params": {
        "whenMs": 1673000000000,  // Unix timestamp in ms
        "positionSeconds": 5.0     // Optional: position to play from
    }
}

// JSON command to schedule stop
{
    "command": "stopAt",
    "params": {
        "whenMs": 1673000010000   // Unix timestamp in ms
    }
}

// JSON command to schedule seek
{
    "command": "seekAt",
    "params": {
        "whenMs": 1673000005000,   // Unix timestamp in ms
        "positionSeconds": 10.5
    }
}
```

## Thread Safety

### Message Thread
Immediate transport control methods should be called from the message thread:
- `play()`
- `stop()`
- `rewind()`
- `setPlayheadSamples()`

### Any Thread
Scheduled action methods are thread-safe and can be called from any thread:
- `playAt()`
- `stopAt()`
- `seekAt()`
- `enqueueScheduledAction()`

### Audio Thread
The audio thread processes scheduled actions automatically:
- `processScheduledActions()` - called internally by Engine

## Performance Considerations

### Queue Capacity
The scheduled action queue has a capacity of 256 actions. If the queue is full, enqueue operations will return `false`:

```cpp
bool success = transport.playAt(timestampMs, 0.0);
if (!success) {
    // Handle queue full condition
    DBG("Warning: Transport action queue is full");
}
```

### Clock Mapping
The transport controller maintains a mapping between wall clock time and sample time:
- Updated periodically in the audio thread (~once per second)
- Robust to system clock adjustments
- Uses linear interpolation for time conversions

### Real-Time Safety
Scheduled action processing is real-time safe:
- No allocations in audio thread
- Lock-free queue operations
- Atomic operations for thread safety

## API Reference

### TransportController Public Methods

#### Immediate Control (Message Thread)
```cpp
void play();
void stop();
void togglePlayback();
void rewind();
void setPlayheadSamples(juce::int64 position);
bool isPlaying() const;
```

#### Scheduled Actions (Any Thread)
```cpp
bool playAt(int64_t wallClockMsEpoch, double positionSeconds = -1.0);
bool stopAt(int64_t wallClockMsEpoch);
bool seekAt(int64_t wallClockMsEpoch, double positionSeconds);
bool enqueueScheduledAction(const ScheduledAction& action);
```

#### Configuration
```cpp
void setSampleRate(double sampleRate);
void setTempoMap(const TempoMap* tempoMap);
```

## Backward Compatibility

The existing immediate transport control API remains unchanged:
- `play()`, `stop()`, `rewind()` work as before
- All position getters are thread-safe (atomics)
- Loop control API is unchanged
- Tempo and time signature API is unchanged

## Testing

When testing code that uses scheduled actions:

```cpp
// Create a test harness
Engine engine;
engine.initialize();

TransportController& transport = engine.getTransportController();
transport.setSampleRate(44100.0);

// Schedule an action
auto now = std::chrono::system_clock::now();
auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
    now.time_since_epoch()).count();

transport.playAt(nowMs + 100, 0.0);

// Simulate audio callback
for (int i = 0; i < 100; ++i) {
    juce::int64 samplePos = i * 512;
    transport.processScheduledActions(samplePos, 512);
    
    // Check if action triggered
    if (transport.isPlaying()) {
        // Action was applied
        break;
    }
}
```

## Common Issues and Solutions

### Issue: Queue Full
**Symptom:** `playAt()` returns `false`

**Solution:** 
- Process pending actions before enqueueing more
- Increase spacing between scheduled events
- Check if old actions are being consumed properly

### Issue: Actions Not Triggering
**Symptom:** Scheduled actions never execute

**Solution:**
- Ensure `processScheduledActions()` is being called in audio callback
- Verify sample rate is set correctly
- Check that scheduled times are in the future

### Issue: Timing Drift
**Symptom:** Actions trigger slightly early/late

**Solution:**
- Clock mapping updates automatically handle drift
- For critical sync, consider pre-roll compensation
- Use shorter scheduling windows for better accuracy

## Future Enhancements

Planned improvements for future PRs:
- Buffer-split processing for sub-sample accurate action timing
- MIDI clock sync integration with scheduled actions
- Network sync protocol using scheduled actions
- Action cancellation/modification API

## Support

For questions or issues:
1. Check the unit tests in `TransportSchedulingTests.cpp`
2. Review the implementation in `TransportController.cpp`
3. See the audio thread safety policy in `docs/tech-briefs/06-audio-thread-safety-policy.md`

---

**Last Updated:** 2026-01-11  
**PR:** C - Transport Refactor  
**Version:** 1.0
