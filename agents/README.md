# Backend Coordinator Agents - Skeleton Implementations

This directory contains skeleton implementations for ten coordinator agents designed to drive realtime audio backend work for the Zenith DAW.

## Overview

The agents are split into two categories:

### C++ Agents (Realtime-Safe)
These agents handle performance-critical, realtime operations and follow strict RT-safety rules:

1. **RealtimeAudioEngineAgent** - Audio performance monitoring and optimization
2. **ScheduledTransportAgent** - Sample-accurate transport scheduling
3. **TransportProtocolAgent** - MIDI sync protocol management
4. **ClockSyncAgent** - External clock synchronization
5. **ObservabilityAgent** - Metrics, tracing, and telemetry

### Python Agents (Orchestration & Analysis)
These agents handle non-realtime orchestration, analysis, and automation:

6. **WebRTCGatewayAgent** - Real-time collaboration via WebRTC
7. **CIandBenchmarkAgent** - Automated testing and benchmarking
8. **TestingAgent** - Test generation and coverage analysis
9. **SecurityAgent** - Security scanning and vulnerability detection
10. **TriageBot** - Issue and PR triage automation

## Directory Structure

```
agents/
├── README.md (this file)
├── RealtimeAudioEngineAgent/
│   ├── README.md
│   ├── RealtimeAudioEngineAgent.h
│   └── RealtimeAudioEngineAgent.cpp
├── ScheduledTransportAgent/
│   ├── README.md
│   ├── ScheduledTransportAgent.h
│   └── ScheduledTransportAgent.cpp
├── TransportProtocolAgent/
│   ├── README.md
│   ├── TransportProtocolAgent.h
│   └── TransportProtocolAgent.cpp
├── ClockSyncAgent/
│   ├── README.md
│   ├── ClockSyncAgent.h
│   └── ClockSyncAgent.cpp
├── ObservabilityAgent/
│   ├── README.md
│   ├── ObservabilityAgent.h
│   └── ObservabilityAgent.cpp
├── WebRTCGatewayAgent/
│   ├── README.md
│   └── webrtc_gateway_agent.py
├── CIandBenchmarkAgent/
│   ├── README.md
│   └── ci_benchmark_agent.py
├── TestingAgent/
│   ├── README.md
│   └── testing_agent.py
├── SecurityAgent/
│   ├── README.md
│   └── security_agent.py
└── TriageBot/
    ├── README.md
    └── triage_bot.py
```

## Key Features

### Threading Safety (C++ Agents)
All C++ agents follow the Zenith DAW threading model:
- **Audio Thread**: Lock-free, RT-safe operations using atomics
- **Message Thread**: UI updates, state changes, allocations
- Clear documentation of which methods can be called from which threads

### Async Patterns (Python Agents)
Python agents use modern async/await patterns for:
- Non-blocking I/O operations
- Concurrent task execution
- Clean resource management

### Documentation
Each agent includes:
- **Purpose**: What the agent does
- **Triggers**: When the agent activates
- **Outputs**: What the agent produces
- **Acceptance Criteria**: Definition of done
- **TODO Checklist**: Next steps for implementation

## Getting Started

### C++ Agents

To integrate a C++ agent into the build:

1. Add the agent directory to your CMakeLists.txt
2. Include the header in your engine code
3. Instantiate and initialize the agent on the message thread
4. Call RT-safe methods from the audio thread as documented

Example:
```cpp
#include "agents/RealtimeAudioEngineAgent/RealtimeAudioEngineAgent.h"

// Message thread
auto agent = std::make_unique<RealtimeAudioEngineAgent>();
agent->initialize(44100.0, 512);

// Audio thread (in callback)
agent->collectMetrics(callbackDurationUs, underrunOccurred);

// Message thread (report)
std::string report = agent->analyzePerformance();
```

### Python Agents

To use a Python agent:

1. Import the agent module
2. Create an instance with optional config
3. Initialize and use async methods

Example:
```python
from agents.SecurityAgent.security_agent import SecurityAgent

async def main():
    agent = SecurityAgent()
    await agent.initialize()
    
    vulnerabilities = await agent.scan_file("source.cpp")
    report = agent.generate_report()
    print(report)

asyncio.run(main())
```

## Development Guidelines

### Adding New Features

1. Review the agent's README.md and TODO checklist
2. Implement one feature at a time
3. Follow the threading model strictly (C++)
4. Add tests for new functionality
5. Update the README.md as features are completed

### Testing

- **C++ agents**: Use Google Test or JUCE UnitTest
- **Python agents**: Use pytest with async support
- Test both functionality and thread safety

### Code Review

Before submitting changes:
1. Verify all threading annotations are accurate
2. Ensure RT-safe code has no allocations/locks
3. Run static analysis tools
4. Update documentation

## Integration Points

### With Existing DAW Components

- **RealtimeAudioEngineAgent** → AudioRenderer, TransportController
- **ScheduledTransportAgent** → TransportController, Engine
- **TransportProtocolAgent** → MIDI subsystem, TransportController
- **ClockSyncAgent** → Audio device, external sync sources
- **ObservabilityAgent** → All components (telemetry collection)

### Between Agents

- **ObservabilityAgent** collects metrics from all other agents
- **RealtimeAudioEngineAgent** may trigger **ScheduledTransportAgent**
- **ClockSyncAgent** provides timing to **TransportProtocolAgent**

## Performance Considerations

### C++ Agents
- Audio thread methods must complete within microseconds
- Use pre-allocated buffers (no dynamic allocation)
- Prefer lock-free atomics over mutexes
- Profile audio thread code regularly

### Python Agents
- Use async I/O to avoid blocking
- Batch operations where possible
- Cache expensive computations
- Monitor memory usage for long-running agents

## Future Work

See individual agent README.md files for detailed TODO lists. High-priority items:

1. Integration with existing DAW infrastructure
2. Comprehensive test coverage
3. Production-ready lock-free data structures
4. Metrics export to monitoring systems
5. Documentation of agent interactions

## Contributing

When contributing to these agents:

1. Follow the coding conventions in `docs/CODING_CONVENTIONS.md`
2. Respect the threading model in `docs/THREADING_MODEL.md`
3. Add tests for all new functionality
4. Update documentation as you go
5. Run code review and security checks before submitting

## Questions?

For questions about:
- **Threading model**: See `docs/THREADING_MODEL.md`
- **Coding conventions**: See `docs/CODING_CONVENTIONS.md`
- **RT safety**: See `docs/RT_SAFETY.md`
- **Specific agents**: See the agent's README.md file

## License

These agents are part of the Zenith DAW project and follow the same license terms.
