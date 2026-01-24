# ObservabilityAgent

## Purpose

The ObservabilityAgent provides comprehensive monitoring, metrics collection, distributed tracing, and logging for the DAW's real-time audio system. It enables production debugging, performance analysis, and system health monitoring without impacting audio thread performance through lock-free data collection and asynchronous aggregation.

## Triggers

- Periodic metrics collection intervals
- Performance threshold violations (CPU, memory, latency)
- Audio dropouts or buffer underruns
- Plugin failures or timeouts
- System resource warnings
- Crash or error events
- User-initiated diagnostic capture

## Inputs

- Real-time performance metrics from audio thread
- CPU and memory usage statistics
- Audio buffer statistics (underruns, latency)
- Plugin processing times
- MIDI event timing data
- System resource availability
- Error and warning logs

## Outputs

- Time-series metrics (Prometheus format)
- Distributed traces (OpenTelemetry)
- Structured logs (JSON)
- Performance dashboards
- Alert notifications
- Diagnostic reports
- System health scores

## Acceptance Criteria

- [x] Zero allocations in RT thread metric collection
- [x] Lock-free metric submission from audio thread
- [ ] < 1% CPU overhead for full observability
- [ ] Support for distributed tracing across network sessions
- [ ] Real-time performance metric streaming
- [ ] Automatic anomaly detection
- [ ] Exporters for standard observability platforms

## TODO: Next Steps

- [x] Implement lock-free ring buffer for RT thread metrics
- [x] Add Prometheus metrics exporter
- [ ] Create OpenTelemetry trace integration
- [x] Implement structured logging with log levels
- [ ] Add performance profiler with flame graphs
- [ ] Create real-time metrics dashboard
- [ ] Implement anomaly detection for audio dropouts
- [ ] Add crash dump generation and symbolication
- [ ] Create diagnostic snapshot capture
- [ ] Document observability best practices

## Implementation Notes

### Thread Safety Model

The ObservabilityAgent uses a strict single-producer, single-consumer (SPSC) model:

- **Producer (Audio/RT Thread)**: Calls `recordCounter()`, `recordGauge()`, `startTimer()`, and `endTimer()` to submit metrics
  - These methods are RT-safe: no allocations, no locks, only atomic operations
  - Uses `juce::AbstractFifo` for lock-free coordination
  - Assumes SINGLE producer thread (one audio callback thread)

- **Consumer (Export Thread)**: Calls `processRingBuffer()` to drain and aggregate metrics
  - Processes events from the ring buffer into aggregated metrics map
  - Protected by `metricsMutex_` for safe access to aggregated data
  - Runs asynchronously on background thread

**IMPORTANT**: The current implementation assumes a single audio thread producer. If metrics are submitted from multiple threads concurrently, additional synchronization would be required.
