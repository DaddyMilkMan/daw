# RealtimeAudioEngineAgent

## Purpose
Coordinates real-time audio engine operations, ensuring thread-safe communication between the audio callback and other system components. Manages audio graph routing, buffer management, and performance monitoring for the DAW's audio engine.

## Triggers
- Audio device callback invocations
- Audio graph topology changes
- Buffer size or sample rate changes
- Performance monitoring requests
- Audio thread state queries

## Outputs
- Real-time audio processing coordination
- Performance metrics (CPU usage, buffer underruns, latency)
- Audio graph state snapshots (RCU-style)
- Thread-safe state updates for UI/message thread
- Audio callback statistics

## Acceptance Criteria
- [ ] No memory allocation in audio callback path
- [ ] Lock-free communication with message thread
- [ ] Pre-allocated buffer pools managed correctly
- [ ] Atomic operations for cross-thread state
- [ ] Performance metrics collected without RT violations
- [ ] Graceful handling of buffer underruns
- [ ] Compliant with `docs/THREADING_MODEL.md`
- [ ] Follows `docs/tech-briefs/06-audio-thread-safety-policy.md`

## TODO: Next Steps
- [ ] Implement audio graph snapshot management
- [ ] Add lock-free FIFO for command passing
- [ ] Implement buffer pool allocation strategy
- [ ] Add CPU usage monitoring (non-RT safe)
- [ ] Create unit tests for thread safety
- [ ] Add integration with TransportController
- [ ] Document API and threading contracts
- [ ] Add performance benchmarks
