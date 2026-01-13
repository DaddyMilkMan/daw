# Agents Directory

This directory contains specialized agents for the Zenith DAW. Each agent is a modular component responsible for a specific aspect of the DAW's functionality.

## Overview

Agents are self-contained modules that handle specific tasks within the DAW. They are designed to be:
- **Modular**: Each agent has clear boundaries and responsibilities
- **Testable**: Comprehensive unit and integration tests
- **Documented**: Clear README with purpose, triggers, outputs, and acceptance criteria
- **Maintainable**: Follow project coding conventions and best practices

## Directory Structure

Each agent follows this structure:
```
agents/
├── README.md                    # This file
└── AgentName/
    ├── README.md                # Agent documentation
    ├── src/
    │   ├── CMakeLists.txt      # Build configuration
    │   └── agent_code.cpp      # Agent implementation
    └── tests/
        └── test_agent.cpp       # Unit tests
```

## Available Agents

### ClockSyncAgent
**Purpose**: Synchronizes timing and clock information across DAW components

**Status**: Scaffold (initial implementation in progress)

**Location**: `agents/ClockSyncAgent/`

See individual agent READMEs for detailed documentation.

## Adding a New Agent

1. Create agent directory: `agents/YourAgent/`
2. Add README.md with purpose, triggers, outputs, acceptance criteria, TODO
3. Create `src/` directory with CMakeLists.txt and implementation
4. Create `tests/` directory with test files
5. Update this README.md to list the new agent
6. Follow project coding conventions (see `/docs/CODING_CONVENTIONS.md`)

## Building Agents

Agents can be built individually or as part of the main DAW build:

```bash
# Build specific agent
cd agents/ClockSyncAgent/src
cmake -B build
cmake --build build

# Or as part of main project (when integrated)
cd /home/runner/work/daw/daw
cmake -B build
cmake --build build
```

## Testing

Each agent includes its own test suite:

```bash
# Run agent tests
cd agents/ClockSyncAgent/tests
./test_clock_sync
```

## Guidelines

- **Thread Safety**: Agents that interact with the audio thread must follow real-time safety guidelines (see `/docs/THREADING_MODEL.md`)
- **Code Quality**: All code must pass project linters and follow conventions
- **Documentation**: Keep READMEs and inline documentation up to date
- **Testing**: Maintain high test coverage for all agent functionality
