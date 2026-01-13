# RealtimeAudioEngineAgent

## Purpose

The **RealtimeAudioEngineAgent** is a specialized coordinator agent responsible for optimizing and managing the realtime audio processing backend. It ensures audio thread safety, low-latency performance, and adherence to the DAW's strict RT-safe threading model.

## Triggers

- Audio dropout or glitch detection
- High CPU usage in audio thread
- Request for audio engine optimization
- Plugin performance issues
- Buffer underrun events
- Real-time safety violations detected

## Outputs

- Audio engine performance analysis
- RT-safety violation reports
- Optimization recommendations
- Code changes to improve audio thread performance
- Configuration tuning suggestions
- Debugging information for audio glitches

## Acceptance Criteria

- [ ] Monitors audio thread performance metrics
- [ ] Detects non-RT-safe operations (allocations, locks, I/O)
- [ ] Suggests lock-free alternatives
- [ ] Validates audio callback implementations
- [ ] Ensures proper use of atomics and lock-free structures
- [ ] Integrates with threading model documentation (`docs/THREADING_MODEL.md`)
- [ ] Provides actionable optimization steps

## TODO: Next Steps

- [ ] Define agent interface and API
- [ ] Implement performance monitoring hooks
- [ ] Create RT-safety validation tooling
- [ ] Integrate with existing `AudioRenderer` and `Engine` classes
- [ ] Add test suite for audio thread safety checks
- [ ] Document usage and integration patterns
- [ ] Connect to observability/metrics system
