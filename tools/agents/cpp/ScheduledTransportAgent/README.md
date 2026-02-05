# ScheduledTransportAgent

## Purpose

The ScheduledTransportAgent manages timeline-based playback control, tempo/time-signature changes, loop regions, and sample-accurate event scheduling. It coordinates transport state (play/stop/record) across the audio engine and ensures all scheduled events fire at precisely the correct sample position.

## Triggers

- Transport control commands (play, stop, record, rewind)
- Tempo or time signature changes from UI
- Loop region modifications
- MIDI clock sync events
- Scheduled automation events reaching their trigger time
- External sync sources (MTC, MIDI clock)

## Inputs

- Transport control commands
- Tempo map and time signature changes
- Loop region boundaries
- Sample position from audio callback
- External sync signals
- Event scheduling requests with sample-accurate timing

## Outputs

- Current playback position (samples, beats, bars)
- Transport state changes
- Scheduled event triggers at exact sample positions
- Tempo/time-signature for current position
- Loop points and punch in/out boundaries
- Sync signals for external devices

## Acceptance Criteria

- [ ] Sample-accurate event scheduling (no jitter)
- [ ] Zero-latency response to transport commands
- [ ] Smooth tempo changes without audio glitches
- [ ] Correct loop boundary handling at all buffer sizes
- [ ] Support for complex tempo maps
- [ ] External sync with < 1ms latency
- [ ] Thread-safe state queries from UI

## TODO: Next Steps

- [ ] Implement lock-free timeline representation
- [ ] Add sample-accurate event scheduling queue
- [ ] Create tempo map with interpolation support
- [ ] Implement loop region handling with crossfades
- [ ] Add MIDI clock and MTC sync
- [ ] Create transport state machine with atomic transitions
- [ ] Add unit tests for edge cases (loop boundaries, tempo changes)
- [ ] Implement punch in/out recording
- [ ] Add support for pre-roll and count-in
- [ ] Document scheduling guarantees and precision
