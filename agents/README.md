# Zenith DAW Agents

This directory contains specialized agent modules that provide intelligent automation, monitoring, and assistance capabilities for the Zenith DAW.

## Available Agents

### ObservabilityAgent
**Location:** `agents/ObservabilityAgent/`  
**Status:** Scaffold/In Development  
**Purpose:** Real-time monitoring, telemetry, and diagnostics for the DAW system.

Provides performance metrics, health monitoring, audio thread safety tracking, and system diagnostics to enable proactive monitoring and debugging.

**Key Features:**
- Performance metrics collection (CPU, memory, I/O)
- Audio thread safety violation detection
- Plugin performance statistics
- System health status reporting
- Real-time telemetry data streams
- Diagnostic logging and tracing

**Documentation:** See [ObservabilityAgent/README.md](ObservabilityAgent/README.md)

---

## Agent Development Guidelines

When creating new agents:

1. **Follow Project Standards:** Adhere to C++20 standards, JUCE conventions, and the coding guidelines in `docs/CODING_CONVENTIONS.md`

2. **Thread Safety:** All agents must be thread-safe, especially when interacting with the audio thread. See `docs/THREADING_MODEL.md` and `docs/tech-briefs/06-audio-thread-safety-policy.md`

3. **Minimal Audio Thread Impact:** Agents should have minimal (<0.1% CPU) overhead on the audio processing thread

4. **Documentation:** Each agent must include:
   - README.md with purpose, triggers, outputs, and acceptance criteria
   - API documentation for public interfaces
   - Usage examples

5. **Testing:** Comprehensive unit tests and integration tests are required

6. **Structure:** Standard agent structure:
   ```
   agents/
   └── AgentName/
       ├── README.md
       ├── src/
       │   ├── CMakeLists.txt
       │   └── agent_implementation.cpp
       └── tests/
           └── test_agent.cpp
   ```

## Integration with Main Application

Agents in this directory are designed to be modular and can be integrated into:
- Desktop application (`apps/desktop/`)
- Backend services (`backend/`)
- Plugin implementations

See the main CMakeLists.txt for integration instructions.
