# ScheduledTransportAgent

## Purpose
Manages scheduled transport operations including tempo automation, time signature changes, loop regions, and synchronization with external timecode sources. Coordinates playback timing and ensures sample-accurate event scheduling.

## Triggers
- Transport control commands (play, stop, record)
- Tempo automation changes
- Time signature modifications
- Loop region updates
- External timecode synchronization events
- Scheduled event insertions

## Outputs
- Sample-accurate playhead position
- Current tempo and time signature
- Loop region boundaries
- Scheduled event queue (MIDI, automation)
- Transport state notifications
- Timecode synchronization data

## Acceptance Criteria
- [ ] Sample-accurate playhead advancement
- [ ] Lock-free transport state access from audio thread
- [ ] Tempo map interpolation for smooth changes
- [ ] Loop region wraparound handling
- [ ] External timecode synchronization support
- [ ] Scheduled event priority queue (RT-safe)
- [ ] Compliant with `docs/THREADING_MODEL.md`
- [ ] Integration with TransportController

## TODO: Next Steps
- [ ] Implement tempo automation curve evaluation
- [ ] Add time signature change handling
- [ ] Create loop region management
- [ ] Implement event scheduler with priority queue
- [ ] Add external timecode synchronization
- [ ] Create unit tests for tempo interpolation
- [ ] Add integration tests with audio engine
- [ ] Document scheduling algorithms
