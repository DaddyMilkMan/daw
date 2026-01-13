# Zenith DAW Agents

This directory contains specialized agents that provide autonomous functionality for the Zenith DAW. Each agent is a self-contained module with specific responsibilities and well-defined interfaces.

## Agent Directory Structure

Each agent follows a standard directory structure:
```
agents/
└── AgentName/
    ├── README.md           # Agent documentation
    ├── src/                # Source code
    │   ├── CMakeLists.txt  # Build configuration
    │   └── *.cpp/*.h       # Implementation files
    └── tests/              # Test files
        └── test_*.cpp      # Unit tests
```

## Available Agents

### ScheduledTransportAgent
**Location:** `agents/ScheduledTransportAgent/`  
**Purpose:** Manages scheduled transport operations and time-based playback control  
**Status:** Scaffold implementation

Handles scheduled starts, stops, and seeks within the DAW timeline. Provides thread-safe transport state management that integrates with the audio engine without blocking real-time audio processing.

**Key Features:**
- Scheduled playback operations with millisecond precision
- Thread-safe transport state changes
- MIDI clock synchronization support
- Timeline marker detection
- Audio-thread safe event queue

## Adding New Agents

When creating a new agent:

1. Create a directory under `agents/` with the agent name
2. Add a `README.md` describing:
   - Purpose and responsibilities
   - Triggers (what activates the agent)
   - Outputs (what the agent produces)
   - Acceptance criteria
   - TODO list
3. Create `src/` directory with:
   - `CMakeLists.txt` for build configuration
   - Implementation files (`.cpp` and `.h`)
4. Create `tests/` directory with unit tests
5. Update this README.md to list the new agent
6. Follow the DAW's threading model and RT-safety policies

## Design Principles

All agents should adhere to these principles:

- **Thread Safety:** All agents must be thread-safe and RT-safe where applicable
- **Modularity:** Agents should be loosely coupled and independently testable
- **Documentation:** Clear API documentation and usage examples
- **Testing:** Comprehensive unit and integration tests
- **Performance:** Profile RT-critical code paths for deterministic behavior

## Building Agents

Agents are built as part of the main Zenith DAW build process. To build:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

To build and run tests:

```bash
cd build
ctest --output-on-failure
```

## Documentation

For more information on DAW architecture and threading model, see:
- `docs/THREADING_MODEL.md`
- `docs/tech-briefs/06-audio-thread-safety-policy.md`
- `docs/CODING_CONVENTIONS.md`
