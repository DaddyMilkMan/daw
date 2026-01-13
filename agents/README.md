# Agents

This directory contains scaffolds and documentation for various agents used in the Zenith DAW project.

## Overview

Agents are specialized components that perform specific tasks within the DAW. Each agent is self-contained with its own source code, tests, and documentation.

## Available Agents

### TransportProtocolAgent
**Location:** `agents/TransportProtocolAgent/`  
**Purpose:** Handles transport protocol communication and synchronization  
**Status:** Scaffold/Initial Implementation  

See individual agent README files for detailed documentation.

## Agent Structure

Each agent follows this standard structure:

```
agents/
└── [AgentName]/
    ├── README.md           # Agent documentation
    ├── src/
    │   ├── CMakeLists.txt  # Build configuration
    │   └── *.cpp/*.h       # Source files
    └── tests/
        └── test_*.cpp      # Test files
```

## Building Agents

Agents can be built individually using CMake:

```bash
cd agents/[AgentName]
cmake -S src -B build
cmake --build build
```

## Testing Agents

Run agent tests:

```bash
cd agents/[AgentName]
./build/test_[agent_name]
```

## Adding New Agents

1. Create a new directory under `agents/`
2. Follow the standard agent structure
3. Include comprehensive README.md with purpose, triggers, outputs, and acceptance criteria
4. Add entry to this file
