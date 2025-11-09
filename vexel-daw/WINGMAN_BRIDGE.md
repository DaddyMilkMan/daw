# Wingman AI Bridge Documentation

## Overview

The Wingman AI Bridge is a dual-protocol communication system that enables real-time communication between Vexel DAW and an external AI core. It supports both **WebSocket** (for bidirectional communication) and **UDP** (for low-latency command streaming) protocols.

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│                      Vexel DAW                          │
│                                                          │
│  ┌──────────────────────────────────────────────────┐  │
│  │           React Frontend (Renderer)               │  │
│  │                                                    │  │
│  │  ├─ useWingmanBridge() hook                      │  │
│  │  ├─ useWingmanTransportSync() hook               │  │
│  │  ├─ useWingmanClipInsertion() hook               │  │
│  │  └─ WingmanConnectionStatus component            │  │
│  └──────────────────┬───────────────────────────────┘  │
│                     │ IPC                               │
│  ┌──────────────────▼───────────────────────────────┐  │
│  │      Electron Main Process (Node.js)              │  │
│  │                                                    │  │
│  │  └─ WingmanBridgeService                         │  │
│  │     ├─ WebSocket Client (ws library)             │  │
│  │     └─ UDP Socket (dgram)                        │  │
│  └──────────────────┬───────────────────────────────┘  │
└────────────────────┼────────────────────────────────────┘
                     │
       ┌─────────────┴──────────────┐
       │                            │
       ▼                            ▼
  WebSocket                       UDP
  (Port 8765)                 (Port 8766)
       │                            │
       └─────────────┬──────────────┘
                     │
                     ▼
           ┌─────────────────┐
           │  External AI     │
           │  Core/Server     │
           └─────────────────┘
```

## Features

### Dual Protocol Support
- **WebSocket**: Reliable, bidirectional communication with automatic reconnection
- **UDP**: Low-latency, fire-and-forget messages for real-time commands
- Both protocols can run simultaneously for maximum reliability

### Command Categories
1. **Transport Commands**: Play, pause, stop, tempo, metronome
2. **Track Commands**: Create, delete, rename, volume, pan, mute, solo
3. **Clip Commands**: Insert, delete, launch (Ableton-style)
4. **Generation Commands**: MIDI, audio, drums, bass, melody generation
5. **Query Commands**: Get state, tracks, clips, tempo

### Real-time Synchronization
- Automatic transport state sync (playback, tempo)
- Bidirectional event notifications
- AI-generated content insertion into timeline

## Installation

### 1. Install Dependencies

```bash
npm install
```

The required dependencies are:
- `ws@^8.18.0` - WebSocket client
- `uuid@^11.0.3` - Unique ID generation

### 2. Start the DAW

```bash
npm run dev
```

## Usage

### Connecting to Wingman AI

#### Method 1: UI Connection (Recommended)

1. Open Vexel DAW
2. Look for the **Wingman AI** status indicator in the Transport Bar (top right)
3. Click on the status indicator to open the connection panel
4. Configure connection settings:
   - **WebSocket URL**: `ws://localhost:8765` (default)
   - **UDP Host**: `localhost` (default)
   - **UDP Port**: `8766` (default)
5. Click **Connect**

#### Method 2: Programmatic Connection

```typescript
import { useWingmanBridge } from './hooks/useWingmanBridge';

function MyComponent() {
  const { connect, isConnected, error } = useWingmanBridge();

  const handleConnect = async () => {
    await connect({
      websocket: {
        enabled: true,
        url: 'ws://localhost:8765',
        reconnect: true,
        reconnectInterval: 5000,
        heartbeatInterval: 30000,
      },
      udp: {
        enabled: true,
        host: 'localhost',
        port: 8766,
        receivePort: 8767,
      },
      messageTimeout: 30000,
      maxRetries: 3,
    });
  };

  return (
    <div>
      {isConnected ? 'Connected' : 'Disconnected'}
      <button onClick={handleConnect}>Connect</button>
    </div>
  );
}
```

## Message Protocol

### Message Structure

All messages use JSON format with the following base structure:

```typescript
interface WingmanMessage {
  id: string;              // Unique message ID (UUID)
  timestamp: number;       // Unix timestamp
  type: 'command' | 'response' | 'event' | 'error';
}
```

### Command Messages (DAW → AI)

```json
{
  "id": "550e8400-e29b-41d4-a716-446655440000",
  "timestamp": 1672531200000,
  "type": "command",
  "command": "SET_TEMPO",
  "payload": {
    "tempo": 128
  }
}
```

### Response Messages (AI → DAW)

```json
{
  "id": "660e8400-e29b-41d4-a716-446655440001",
  "timestamp": 1672531200100,
  "type": "response",
  "requestId": "550e8400-e29b-41d4-a716-446655440000",
  "success": true,
  "data": {
    "tempo": 128
  }
}
```

### Event Messages (Bidirectional)

```json
{
  "id": "770e8400-e29b-41d4-a716-446655440002",
  "timestamp": 1672531200200,
  "type": "event",
  "event": "PLAYBACK_STARTED",
  "data": {
    "position": 0
  }
}
```

## Command Reference

### Transport Commands

#### TRANSPORT_PLAY
Start playback.

```json
{
  "command": "TRANSPORT_PLAY",
  "payload": {
    "position": 0  // Optional: start position in beats
  }
}
```

#### TRANSPORT_PAUSE
Pause playback.

```json
{
  "command": "TRANSPORT_PAUSE",
  "payload": {}
}
```

#### TRANSPORT_STOP
Stop playback and reset position.

```json
{
  "command": "TRANSPORT_STOP",
  "payload": {}
}
```

#### SET_TEMPO
Set the project tempo.

```json
{
  "command": "SET_TEMPO",
  "payload": {
    "tempo": 120  // BPM (20-999)
  }
}
```

### Track Commands

#### CREATE_TRACK
Create a new track.

```json
{
  "command": "CREATE_TRACK",
  "payload": {
    "name": "Bass",
    "type": "midi",  // "midi" | "audio" | "instrument"
    "index": 0       // Optional: position in track list
  }
}
```

**Response:**
```json
{
  "success": true,
  "data": {
    "id": "1672531200000",
    "name": "Bass",
    "type": "midi",
    "volume": 0.8,
    "pan": 0,
    "muted": false,
    "solo": false
  }
}
```

#### SET_TRACK_VOLUME
Set track volume.

```json
{
  "command": "SET_TRACK_VOLUME",
  "payload": {
    "trackId": "1672531200000",
    "volume": 0.8  // 0.0 to 1.0
  }
}
```

### Clip Commands

#### INSERT_CLIP
Insert an AI-generated clip into the timeline.

```json
{
  "command": "INSERT_CLIP",
  "payload": {
    "trackId": "1672531200000",
    "sceneIndex": 0,  // For session view
    "clip": {
      "id": "clip-001",
      "name": "AI Bass Line",
      "type": "midi",
      "length": 4,  // In beats
      "color": "#3b82f6",
      "notes": [
        {
          "pitch": 48,      // C3
          "velocity": 100,
          "startTime": 0,
          "duration": 0.5
        },
        {
          "pitch": 52,      // E3
          "velocity": 95,
          "startTime": 1,
          "duration": 0.5
        }
      ]
    }
  }
}
```

#### LAUNCH_CLIP
Launch a clip (Ableton-style).

```json
{
  "command": "LAUNCH_CLIP",
  "payload": {
    "clipId": "clip-001",
    "mode": "trigger"  // "trigger" | "gate" | "toggle"
  }
}
```

### Generation Commands

#### GENERATE_MIDI
Request AI to generate a MIDI clip.

```json
{
  "command": "GENERATE_MIDI",
  "payload": {
    "prompt": "Generate a dark techno bassline",
    "trackId": "1672531200000",
    "duration": 8,       // In beats
    "key": "Am",
    "scale": "minor",
    "style": "techno"
  }
}
```

**Event Response:**
```json
{
  "type": "event",
  "event": "GENERATION_COMPLETED",
  "data": {
    "trackId": "1672531200000",
    "sceneIndex": 0,
    "clip": {
      "name": "AI Techno Bass",
      "type": "midi",
      "length": 8,
      "notes": [/* ... */]
    }
  }
}
```

## Events

### Transport Events
- `PLAYBACK_STARTED` - Playback started
- `PLAYBACK_STOPPED` - Playback stopped
- `TEMPO_CHANGED` - Tempo changed

### Track Events
- `TRACK_CREATED` - New track created
- `TRACK_DELETED` - Track deleted
- `TRACK_UPDATED` - Track properties updated

### Clip Events
- `CLIP_INSERTED` - Clip inserted into timeline
- `CLIP_DELETED` - Clip deleted
- `CLIP_LAUNCHED` - Clip launched
- `CLIP_STOPPED` - Clip stopped

### Generation Events
- `GENERATION_STARTED` - AI generation started
- `GENERATION_PROGRESS` - Generation progress update
- `GENERATION_COMPLETED` - Generation completed
- `GENERATION_FAILED` - Generation failed

## React Hooks

### useWingmanBridge()

Main hook for managing Wingman AI connection.

```typescript
const {
  connectionState,  // Current connection state
  isConnected,      // Boolean: connected?
  isConnecting,     // Boolean: connecting?
  error,            // Error message if any
  connect,          // Function: connect to AI
  disconnect,       // Function: disconnect from AI
  sendCommand,      // Function: send command
  sendEvent,        // Function: send event
  lastEvent,        // Last received event
  lastError,        // Last received error
} = useWingmanBridge();
```

### useWingmanTransportSync()

Auto-sync transport state with Wingman AI.

```typescript
useWingmanTransportSync(isPlaying, tempo);
```

### useWingmanClipInsertion()

Handle AI-generated clip insertions.

```typescript
useWingmanClipInsertion((clipData) => {
  console.log('Clip inserted:', clipData);
  // Handle clip insertion in your UI
});
```

## Example: External AI Server

Here's a simple example of an external AI server using Python and `websockets`:

```python
import asyncio
import json
import websockets
from datetime import datetime

async def handle_message(websocket, path):
    async for message in websocket:
        data = json.loads(message)
        print(f"Received: {data['type']} - {data.get('command', data.get('event'))}")

        if data['type'] == 'command':
            # Handle command
            if data['command'] == 'GENERATE_MIDI':
                # Generate MIDI (mock example)
                response = {
                    'id': str(uuid.uuid4()),
                    'timestamp': int(datetime.now().timestamp() * 1000),
                    'type': 'response',
                    'requestId': data['id'],
                    'success': True,
                    'data': {}
                }
                await websocket.send(json.dumps(response))

                # Send generation completed event
                event = {
                    'id': str(uuid.uuid4()),
                    'timestamp': int(datetime.now().timestamp() * 1000),
                    'type': 'event',
                    'event': 'GENERATION_COMPLETED',
                    'data': {
                        'trackId': data['payload']['trackId'],
                        'clip': {
                            'name': 'AI Generated',
                            'type': 'midi',
                            'length': 4,
                            'notes': [
                                {'pitch': 60, 'velocity': 100, 'startTime': 0, 'duration': 0.5}
                            ]
                        }
                    }
                }
                await websocket.send(json.dumps(event))

# Start WebSocket server
start_server = websockets.serve(handle_message, "localhost", 8765)
asyncio.get_event_loop().run_until_complete(start_server)
asyncio.get_event_loop().run_forever()
```

Run with:
```bash
pip install websockets
python ai_server.py
```

## Troubleshooting

### Connection Failed

**Error**: "Failed to connect to Wingman AI"

**Solution**:
1. Ensure the external AI server is running
2. Check the WebSocket URL and UDP host/port
3. Verify firewall settings allow connections on ports 8765-8767

### No Response from AI

**Error**: "Command timeout"

**Solution**:
1. Check that the AI server is responding to commands
2. Increase `messageTimeout` in config
3. Check server logs for errors

### Clips Not Inserting

**Issue**: AI generates clips but they don't appear in timeline

**Solution**:
1. Check browser console for errors
2. Ensure `useWingmanClipInsertion` hook is properly connected
3. Verify clip data format matches the expected structure

## Configuration

### Default Configuration

```typescript
{
  websocket: {
    enabled: true,
    url: 'ws://localhost:8765',
    reconnect: true,
    reconnectInterval: 5000,      // 5 seconds
    heartbeatInterval: 30000,     // 30 seconds
  },
  udp: {
    enabled: true,
    host: 'localhost',
    port: 8766,
    receivePort: 8767,
  },
  messageTimeout: 30000,          // 30 seconds
  maxRetries: 3,
}
```

### Custom Configuration

You can customize the configuration when connecting:

```typescript
await wingman.connect({
  websocket: {
    url: 'ws://192.168.1.100:9000',  // Remote server
    reconnectInterval: 10000,         // 10 seconds
  },
  udp: {
    enabled: false,  // Disable UDP
  },
});
```

## Security Considerations

1. **Local Network Only**: By default, the bridge is configured for `localhost`. Exposing it to the internet requires proper authentication and encryption.

2. **No Authentication**: The current implementation does not include authentication. Add authentication if deploying in a multi-user environment.

3. **Message Validation**: Always validate incoming messages before processing to prevent injection attacks.

## Future Enhancements

- [ ] Authentication and authorization
- [ ] TLS/SSL support for WebSocket
- [ ] Message queuing for offline mode
- [ ] Compression for large messages
- [ ] Rate limiting
- [ ] Connection pooling
- [ ] Metrics and telemetry

## License

MIT License - See LICENSE file for details

## Support

For issues or questions:
- GitHub: [DaddyMilkMan/vexel-daw](https://github.com/DaddyMilkMan/daw)
- Discord: [Join our community](#)

---

**Built with ❤️ for AI-native music production**
