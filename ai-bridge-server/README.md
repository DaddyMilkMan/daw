# Zenith AI Bridge Server

Example WebSocket server implementation for communication between zenith DAW and external AI services.

## Overview

The AI Bridge enables real-time communication between the zenith DAW and external AI services. It supports:

- **AI Music Generation**: Melody, chords, drums, and bass generation
- **Intelligent Instrument Control** (NEW): Natural language preset suggestions and parameter tweaking via InstrumentAIAdapter
- **Remote Control**: Play, stop, pause, tempo, and key control
- **Status Queries**: Get current playback state and server capabilities
- **Bidirectional Communication**: WebSocket-based protocol with request/response matching

## Installation

```bash
npm install
```

## Usage

Start the server:

```bash
npm start
```

Or with auto-reload during development:

```bash
npm run dev
```

The server will listen on `ws://localhost:8765` by default.

## Protocol

### Message Format

All messages follow this JSON structure:

```json
{
  "id": "unique-message-id",
  "type": "message_type",
  "timestamp": 1234567890,
  "payload": {}
}
```

### Request Types

#### Generate Melody
```json
{
  "id": "msg-123",
  "type": "generate_melody",
  "timestamp": 1234567890,
  "payload": {
    "seed": [],
    "temperature": 1.0,
    "steps": 32,
    "key": "C",
    "scale": "major"
  }
}
```

#### Generate Chords
```json
{
  "id": "msg-124",
  "type": "generate_chords",
  "timestamp": 1234567890,
  "payload": {
    "key": "C",
    "progression": "I-V-vi-IV",
    "bars": 4
  }
}
```

#### Generate Drums
```json
{
  "id": "msg-125",
  "type": "generate_drums",
  "timestamp": 1234567890,
  "payload": {
    "style": "trap",
    "bars": 4,
    "complexity": 0.7
  }
}
```

#### Control Commands
```json
{
  "id": "msg-126",
  "type": "play",
  "timestamp": 1234567890,
  "payload": {
    "position": 0
  }
}
```

Supported control types: `play`, `stop`, `pause`, `set_tempo`, `set_key`

#### Query Commands
```json
{
  "id": "msg-127",
  "type": "get_status",
  "timestamp": 1234567890
}
```

Supported query types: `get_status`, `get_capabilities`

#### Suggest Preset (InstrumentAIAdapter)
```json
{
  "id": "msg-128",
  "type": "suggest_preset",
  "timestamp": 1234567890,
  "payload": {
    "genre": "synthwave",
    "mood": "nostalgic",
    "role": "lead",
    "instrumentId": "zenith_poly_synth",
    "trackId": "track_0"
  }
}
```

#### Tweak Preset (InstrumentAIAdapter)
```json
{
  "id": "msg-129",
  "type": "tweak_preset",
  "timestamp": 1234567890,
  "payload": {
    "trackId": "track_0",
    "instructions": "make it darker and more detuned, add a touch of reverb"
  }
}
```

### Response Types

#### Generation Complete
```json
{
  "type": "generation_complete",
  "requestId": "msg-123",
  "timestamp": 1234567890,
  "payload": {
    "notes": [
      {
        "pitch": 60,
        "velocity": 80,
        "startTime": 0,
        "duration": 0.5
      }
    ],
    "metadata": {
      "key": "C",
      "scale": "major",
      "steps": 32
    }
  }
}
```

#### Success
```json
{
  "type": "success",
  "requestId": "msg-126",
  "timestamp": 1234567890,
  "payload": {
    "action": "play",
    "state": true
  }
}
```

#### Error
```json
{
  "type": "error",
  "requestId": "msg-123",
  "timestamp": 1234567890,
  "payload": {
    "code": "GENERATION_FAILED",
    "message": "Failed to generate melody",
    "details": {}
  }
}
```

#### Preset Suggestion
```json
{
  "type": "preset_suggestion",
  "requestId": "msg-128",
  "timestamp": 1234567890,
  "payload": {
    "presetId": "bright_pluck_1234567891",
    "presetName": "Bright Pluck",
    "reasoning": "Perfect for synthwave leads with bright, punchy character",
    "genre": "synthwave",
    "mood": "nostalgic",
    "role": "lead"
  }
}
```

#### Preset Tweaks
```json
{
  "type": "preset_tweaks",
  "requestId": "msg-129",
  "timestamp": 1234567890,
  "payload": {
    "parameters": {
      "filter_cutoff": 0.35,
      "unison_detune": 0.45,
      "reverb_send": 0.25
    },
    "reasoning": "Lowered cutoff for darkness, increased detune for width, added reverb",
    "commands": [
      {
        "command": "set_instrument_parameters",
        "params": {
          "trackId": "track_0",
          "params": { "filter_cutoff": 0.35, "unison_detune": 0.45, "reverb_send": 0.25 }
        }
      }
    ]
  }
}
```

## InstrumentAIAdapter

The **InstrumentAIAdapter** provides intelligent instrument control using natural language and high-level musical intent. It translates user requests into CommandAPI calls with LLM reasoning.

### Features

- **Preset Suggestions**: Get preset recommendations based on genre, mood, and role
- **Natural Language Tweaking**: Describe desired sound changes in plain English
- **Safety First**: Automatic parameter clamping to prevent extreme settings
- **LLM Agnostic**: Works with OpenAI, Anthropic, or custom LLM providers
- **Stateless Design**: Relies on CommandAPI as source of truth

### Documentation

- **[InstrumentAIAdapter.js](./InstrumentAIAdapter.js)** - Core adapter implementation
- **[INSTRUMENT_AI_ADAPTER_GUIDE.md](./INSTRUMENT_AI_ADAPTER_GUIDE.md)** - Comprehensive integration guide
- **[EXAMPLE_WORKFLOW.md](./EXAMPLE_WORKFLOW.md)** - Complete JSON RPC sequences and examples

### Quick Example

```javascript
const adapter = new InstrumentAIAdapter({
  sendCommand: async (cmd) => {
    // Send to CommandAPI
  },
  callLLM: async (prompt) => {
    // Call your LLM provider
  }
});

// Suggest a preset
const suggestion = await adapter.suggestPreset({
  genre: 'synthwave',
  mood: 'nostalgic',
  role: 'lead',
  instrumentId: 'zenith_poly_synth'
});

// Tweak parameters
const tweaks = await adapter.tweakPreset({
  trackId: 'track_0',
  instructions: 'make it darker and more detuned'
});
```

### Environment Variables

```bash
# CommandAPI URL
export COMMAND_API_URL=http://localhost:8080/command

# LLM provider (mock, openai, anthropic)
export LLM_PROVIDER=mock

# API keys (if using real LLM)
export OPENAI_API_KEY=sk-...
export ANTHROPIC_API_KEY=sk-ant-...
```

## Extending the Server

### Adding New AI Models

1. Define a new message type in `MessageType`
2. Add a handler function (e.g., `handleGenerateXYZ`)
3. Register the handler in `handleMessage` switch statement

Example:

```javascript
async function handleGenerateHarmony(requestId, payload) {
  // Your AI model logic here
  const notes = await yourAIModel.generate(payload);

  return {
    type: MessageType.GENERATION_COMPLETE,
    requestId,
    timestamp: Date.now(),
    payload: { notes, metadata: {} }
  };
}
```

### Integrating Real AI Models

Replace the mock implementations with real AI services:

```javascript
// Example with Magenta.js
const magenta = require('@magenta/music');

async function handleGenerateMelody(requestId, payload) {
  const model = new magenta.MusicRNN('...');
  const sequence = await model.continueSequence(
    payload.seed,
    payload.steps,
    payload.temperature
  );

  // Convert to our note format
  const notes = convertMagentaToNotes(sequence);

  return {
    type: MessageType.GENERATION_COMPLETE,
    requestId,
    timestamp: Date.now(),
    payload: { notes, metadata: {} }
  };
}
```

## UDP Support

To add UDP support (for low-latency communication):

1. Install dgram (built-in): No installation needed
2. Create UDP socket alongside WebSocket server
3. Implement message parsing for UDP packets
4. Update client to support UDP fallback

```javascript
const dgram = require('dgram');
const udpServer = dgram.createSocket('udp4');

udpServer.on('message', (msg, rinfo) => {
  const message = JSON.parse(msg.toString());
  // Handle message and send response
  const response = await handleMessage(message);
  udpServer.send(
    JSON.stringify(response),
    rinfo.port,
    rinfo.address
  );
});

udpServer.bind(8766);
```

## Architecture

```
┌──────────────┐         WebSocket/UDP        ┌──────────────┐
│              │◄──────────────────────────────►│              │
│  Zenith DAW   │                               │  AI Bridge   │
│  (Client)    │  Request/Response Protocol    │   Server     │
│              │◄──────────────────────────────►│              │
└──────────────┘                               └──────┬───────┘
                                                      │
                                                      ▼
                                               ┌──────────────┐
                                               │ AI Models    │
                                               │ - Magenta.js │
                                               │ - Custom ML  │
                                               │ - External   │
                                               └──────────────┘
```

## Testing

Test the server using a WebSocket client:

```javascript
const WebSocket = require('ws');

const ws = new WebSocket('ws://localhost:8765');

ws.on('open', () => {
  // Send test request
  ws.send(JSON.stringify({
    id: 'test-123',
    type: 'generate_melody',
    timestamp: Date.now(),
    payload: { steps: 16 }
  }));
});

ws.on('message', (data) => {
  console.log('Response:', JSON.parse(data));
});
```

## Configuration

Modify server configuration at the top of `server.js`:

```javascript
const PORT = 8765;
const HOST = 'localhost';
```

For production, consider:
- Using environment variables
- Adding authentication
- Implementing rate limiting
- Adding logging middleware
- Using WSS (secure WebSocket)

## License

MIT
