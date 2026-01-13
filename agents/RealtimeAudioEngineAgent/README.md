# RealtimeAudioEngineAgent

## Purpose
Coordinates real-time audio engine operations, ensuring thread-safe communication between the audio processing thread and other system components. This agent handles audio graph setup, buffer management, and real-time performance monitoring.

## Triggers
- Audio device initialization or configuration changes
- Audio graph modifications (track/plugin additions/removals)
- Real-time performance issues (buffer underruns, xruns)
- Audio thread priority or scheduling adjustments needed

## Outputs
- Audio graph configuration commands
- Thread safety validation reports
- Real-time performance metrics
- Audio buffer allocation strategies

## Acceptance Criteria
- [ ] Compiles successfully in the Zenith DAW CMake build
- [ ] Follows Zenith DAW coding conventions (see docs/CODING_CONVENTIONS.md)
- [ ] Respects audio thread safety rules (see docs/THREADING_MODEL.md)
- [ ] Integrates with existing AudioRenderer and Engine classes
- [ ] Provides lock-free communication patterns for audio thread coordination
- [ ] Includes unit tests for non-RT functionality

## TODO Checklist

### Phase 1: Basic Structure
- [ ] Define agent interface with clear API methods
- [ ] Implement message queue for audio thread communication
- [ ] Add real-time safety assertions and validation

### Phase 2: Core Functionality
- [ ] Implement audio graph coordination logic
- [ ] Add buffer management strategies
- [ ] Integrate with TransportController for synchronized operations

### Phase 3: Monitoring & Diagnostics
- [ ] Add RT performance metrics collection (lock-free)
- [ ] Implement xrun detection and reporting
- [ ] Create diagnostic output for debugging audio issues

### Phase 4: Testing & Documentation
- [ ] Write unit tests for message queue and coordination logic
- [ ] Add integration tests with existing Engine classes
- [ ] Document API usage and threading contracts
- [ ] Add examples for common coordination patterns

## Dependencies
- JUCE framework (audio device management)
- AudioRenderer and Engine classes
- TransportController for synchronization
- Lock-free FIFO structures (juce::AbstractFifo)

## Related Documentation
- [Threading Model](../../docs/THREADING_MODEL.md)
- [Audio Thread Safety Policy](../../docs/tech-briefs/06-audio-thread-safety-policy.md)
- [RT Safety Quick Reference](../../docs/RT_SAFETY_QUICK_REF.md)
