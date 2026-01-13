# Zenith DAW Backend Agents

This directory contains coordinator agents for managing various aspects of the DAW's real-time audio backend and automation workflows.

## Overview

These agents are designed to coordinate complex operations across the DAW's architecture, ensuring thread safety, real-time performance, and proper system integration.

## Agent Categories

### Real-Time Audio Agents (C++)

These agents handle time-critical audio operations and must adhere to strict real-time safety constraints:

1. **RealtimeAudioEngineAgent** - Coordinates real-time audio engine operations
   - Manages audio graph setup and coordination
   - Provides lock-free communication with audio thread
   - Monitors RT performance and xrun detection

2. **ScheduledTransportAgent** - Handles transport scheduling with sample accuracy
   - Manages play/stop/record scheduling
   - Coordinates loop points and tempo changes
   - Provides sample-accurate event processing

3. **TransportProtocolAgent** - Protocol encoding/decoding for network sync
   - Supports MIDI clock, MTC, and custom sync protocols
   - Handles protocol conversion and timing coordination
   - Network transport layer integration

4. **ClockSyncAgent** - Distributed clock synchronization
   - Manages clock drift compensation
   - Provides microsecond-level time alignment
   - Handles network jitter and packet loss

5. **ObservabilityAgent** - Monitoring and diagnostics without RT impact
   - Lock-free metrics collection
   - Zero-copy event logging
   - Performance tracking and diagnostics

### Automation & Tooling Agents (Python)

These agents automate development workflows and provide operational support:

6. **WebRTCGatewayAgent** - WebRTC gateway coordination
   - Manages peer connections for collaboration
   - Handles signaling and ICE negotiation
   - Coordinates audio stream routing

7. **CIandBenchmarkAgent** - CI pipeline and performance benchmarking
   - Automates build validation
   - Executes performance benchmarks
   - Detects performance regressions

8. **TestingAgent** - Comprehensive test orchestration
   - Coordinates C++ and Python test execution
   - Validates real-time safety constraints
   - Generates test coverage reports

9. **SecurityAgent** - Security scanning and vulnerability detection
   - Scans dependencies for known CVEs
   - Detects hardcoded secrets
   - Validates secure coding practices

10. **TriageBot** - Automated issue triage and management
    - Automatically labels and categorizes issues
    - Routes to appropriate maintainers
    - Detects duplicates and generates responses

## Agent Structure

Each agent directory contains:

- **README.md** - Purpose, triggers, outputs, acceptance criteria, and TODO checklist
- **Source files** - Implementation (C++: .h/.cpp, Python: .py)
- Language-appropriate scaffolding that compiles or imports successfully

## Thread Safety

C++ agents follow the Zenith DAW threading model (see [docs/THREADING_MODEL.md](../docs/THREADING_MODEL.md)):

- **Audio Thread**: Lock-free, no allocations, real-time safe
- **Message Thread**: UI operations, state mutations
- **Background Threads**: Async I/O, processing, with message thread callbacks

All C++ agents include thread safety annotations in their headers.

## Development Guidelines

### For C++ Agents

1. Follow [CODING_CONVENTIONS.md](../docs/CODING_CONVENTIONS.md)
2. Adhere to [THREADING_MODEL.md](../docs/THREADING_MODEL.md)
3. Review [RT_SAFETY_QUICK_REF.md](../docs/RT_SAFETY_QUICK_REF.md)
4. Use JUCE framework patterns (atomics, AbstractFifo, etc.)
5. Add `jassert` for thread safety validation

### For Python Agents

1. Use type hints throughout
2. Follow PEP 8 style guidelines
3. Include comprehensive docstrings
4. Use async/await for I/O operations
5. Write unit tests for core logic

## Integration

These agents are designed to integrate with:

- Existing Engine and AudioRenderer classes (C++)
- Backend Python services (WebRTC, networking)
- GitHub Actions workflows (CI/CD)
- Build and test infrastructure

## Next Steps

Each agent's README.md contains a detailed TODO checklist outlining:

1. Basic structure implementation
2. Core functionality development
3. Advanced features and optimizations
4. Testing and documentation

## Contributing

When extending or implementing these agents:

1. Read the agent's README.md for specific requirements
2. Follow the existing code patterns in the DAW
3. Ensure thread safety for C++ agents
4. Add comprehensive tests
5. Update documentation

## References

- [Architecture Documentation](../docs/ARCHITECTURE.md)
- [Developer Guide](../docs/DEVELOPER.md)
- [Threading Model](../docs/THREADING_MODEL.md)
- [RT Safety Policy](../docs/tech-briefs/06-audio-thread-safety-policy.md)
