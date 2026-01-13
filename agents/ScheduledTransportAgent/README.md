# ScheduledTransportAgent

## Purpose
Coordinates transport scheduling operations including playback control, loop points, tempo changes, and timeline synchronization. Ensures thread-safe scheduling of transport state changes.

## Triggers
- User transport control actions (play, stop, record)
- Loop point or tempo changes
- Timeline marker navigation requests
- External sync signals (MIDI clock, MTC, etc.)

## Outputs
- Scheduled transport state transitions
- Timeline position updates
- Tempo change scheduling
- Loop boundary coordination

## Acceptance Criteria
- [ ] Compiles successfully in the Zenith DAW CMake build
- [ ] Follows Zenith DAW coding conventions
- [ ] Respects audio thread safety rules for transport control
- [ ] Integrates with existing TransportController
- [ ] Provides sample-accurate scheduling
- [ ] Includes unit tests for scheduling logic

## TODO Checklist

### Phase 1: Basic Structure
- [ ] Define agent interface for transport scheduling
- [ ] Implement event queue for scheduled transport changes
- [ ] Add sample-accurate timestamp handling

### Phase 2: Core Functionality
- [ ] Implement play/stop/record scheduling
- [ ] Add loop point coordination
- [ ] Implement tempo change scheduling with interpolation

### Phase 3: External Sync
- [ ] Add MIDI clock synchronization support
- [ ] Implement MTC (MIDI Time Code) handling
- [ ] Add link/sync protocol integration

### Phase 4: Testing & Documentation
- [ ] Write unit tests for scheduling accuracy
- [ ] Add integration tests with TransportController
- [ ] Document scheduling API and timing guarantees
- [ ] Add examples for common scheduling patterns

## Dependencies
- TransportController (existing)
- Atomic time/position tracking
- Lock-free event queue

## Related Documentation
- [Threading Model](../../docs/THREADING_MODEL.md)
- [RT Safety Quick Reference](../../docs/RT_SAFETY_QUICK_REF.md)
