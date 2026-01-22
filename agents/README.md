# Zenith DAW Backend Coordinator Agents

This directory contains skeleton implementations for ten coordinator agents designed to drive realtime audio backend work.

## Overview

The agents are organized into three categories based on their implementation language and focus area:

### C++ Agents (Real-Time Audio Focus)
These agents handle performance-critical, real-time operations where lock-free programming and zero-allocation constraints are essential.

1. **RealtimeAudioEngineAgent** - Coordinates lock-free audio processing, plugin chains, and RT thread management
2. **ScheduledTransportAgent** - Manages timeline, tempo, and sample-accurate event scheduling
3. **TransportProtocolAgent** - Abstracts cross-platform audio I/O (ASIO, WASAPI, CoreAudio, ALSA, JACK)
4. **ClockSyncAgent** - Provides nanosecond-precision time synchronization across distributed systems
5. **ObservabilityAgent** - Lock-free metrics collection and monitoring for RT audio

### Python Agents (Backend Services)
These agents handle build automation, testing, collaboration, and non-real-time services.

6. **WebRTCGatewayAgent** - Real-time audio streaming and collaboration over WebRTC
7. **CIandBenchmarkAgent** - Automated build validation, testing, and performance benchmarking
8. **TestingAgent** - Comprehensive test orchestration including RT safety validation
9. **TriageBot** - Automated GitHub issue/PR triage and management
10. **LintingAgent** - Code style, naming conventions, and quality checks (Python implementation)

### Mixed Language Agents
These agents bridge C++ and Python for security and cross-language integration.

11. **SecurityAgent** - Security scanning and input validation (Python implementation + C++ bridge)

## Directory Structure

Each agent directory contains:
- `README.md` - Comprehensive documentation including:
  - Purpose and responsibilities
  - Triggers and inputs
  - Outputs and deliverables
  - Acceptance criteria
  - TODO checklist for next steps
- Source files - Minimal compilable (C++) or importable (Python) skeleton implementations

## Implementation Status

All agents are currently in **skeleton** phase:
- ✅ Directory structure created
- ✅ Documentation complete
- ✅ Minimal compilable/importable code
- ⏳ Full implementation (see individual agent TODOs)

## Getting Started

To work on an agent:
1. Review the agent's `README.md` to understand its purpose and architecture
2. Check the TODO checklist for next implementation steps
3. Follow the coding conventions in `docs/CODING_CONVENTIONS.md`
4. For C++ agents, ensure RT-safety constraints from `docs/THREADING_MODEL.md`
5. Add tests as you implement functionality

## Architecture Notes

### Real-Time Safety
All C++ agents that interact with the audio thread must follow strict RT-safety rules:
- No heap allocations in audio callback path
- No locks or blocking operations
- Use lock-free data structures for thread communication
- Use atomics for shared state
- Mark RT-safe functions with `noexcept`

### Thread Communication
- Use lock-free ring buffers for RT->UI communication
- Use lock-free command queues for UI->RT communication
- Atomic variables for simple state sharing

### Python Integration
Python agents can integrate with C++ via:
- Direct embedding (Python C API)
- Inter-process communication (sockets, pipes)
- Shared memory for bulk data
- C++ bridge classes (see SecurityAgent example)

## Contributing

When implementing agent functionality:
1. Maintain the minimal change philosophy
2. Add tests for new functionality
3. Update the agent's README.md with implementation notes
4. Check off completed TODO items
5. Run the project's linters and tests
6. Follow the security scanning process for new dependencies

## Questions?

See the main project documentation in `/docs` or reach out to maintainers for guidance on specific agents.
