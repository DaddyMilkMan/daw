# RealtimeAudioEngineAgent

## Purpose

Coordinates and optimizes the realtime audio processing engine, ensuring thread-safe, lock-free audio rendering with minimal latency. This agent monitors audio thread performance, manages buffer allocation strategies, and prevents audio dropouts.

## Triggers

- Audio buffer underruns or xruns detected
- Audio thread taking >80% of available time budget
- Sample rate or buffer size changes requested
- New audio processing chains being added
- Performance metrics exceeding thresholds

## Outputs

- Audio engine configuration recommendations
- Real-time performance reports
- Buffer sizing adjustments
- Audio thread priority tuning
- Lock-free queue optimizations
- Diagnostic traces for audio glitches

## Acceptance Criteria

- [ ] Agent can monitor audio callback timing
- [ ] Agent detects and reports buffer underruns
- [ ] Agent provides actionable recommendations for performance improvements
- [ ] Agent respects RT-safety rules (no allocations, locks, or blocking on audio thread)
- [ ] Agent integrates with existing observability infrastructure

## TODO Checklist

- [ ] Implement audio callback timing measurements
- [ ] Add buffer underrun detection logic
- [ ] Create performance threshold configuration
- [ ] Implement recommendation engine for audio settings
- [ ] Add integration with TransportController and AudioRenderer
- [ ] Write unit tests for performance analysis
- [ ] Add real-time safety validation
- [ ] Document threading model and data flow
- [ ] Integrate with ObservabilityAgent for metrics export
- [ ] Add comprehensive logging (message thread only)
