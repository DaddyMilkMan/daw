# Agents

This directory contains specialized agents for the Zenith DAW project. Each agent is a modular component that handles specific functionality within the system.

## Available Agents

### WebRTCGatewayAgent
**Location:** `agents/WebRTCGatewayAgent/`

Manages WebRTC signaling and gateway functionality for real-time audio collaboration. Handles peer connections, ICE candidate exchange, and media stream routing between collaborating clients.

**Key Features:**
- WebRTC signaling protocol handling
- ICE candidate gathering and exchange
- Media stream routing
- Connection state management
- Performance monitoring

**Documentation:** See [WebRTCGatewayAgent/README.md](WebRTCGatewayAgent/README.md)

## Agent Structure

Each agent follows a standard structure:
```
agents/
└── AgentName/
    ├── README.md                    # Agent documentation
    ├── AgentName.py                 # Agent implementation
    └── tests/
        └── test_AgentName.py        # Agent tests
```

## Running Tests

To run tests for a specific agent:
```bash
cd agents/AgentName/tests
python -m pytest test_AgentName.py
```

Or run tests directly:
```bash
python agents/AgentName/tests/test_AgentName.py
```
