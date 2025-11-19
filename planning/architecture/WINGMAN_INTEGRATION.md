# Wingman AI Integration Guide

This document describes the Wingman AI integration in Zenith DAW, including the MagentaService for AI music generation and the AI Bridge for communication with external AI services.

## Table of Contents

1. [Overview](#overview)
2. [MagentaService](#magentaservice)
3. [AI Bridge](#ai-bridge)
4. [Wingman Panel Integration](#wingman-panel-integration)
5. [Quick Actions](#quick-actions)
6. [Usage Examples](#usage-examples)
7. [Development Guide](#development-guide)

## Overview

The Wingman AI system consists of three main components:

### 1. MagentaService (`src/renderer/lib/MagentaService.ts`)
Local AI music generation service that provides:
- Chord progression generation
- Melody generation using RNN-style algorithms
- Drum pattern generation (multiple styles)
- Bass line generation

### 2. AI Bridge (`src/renderer/lib/AIBridge.ts`)
Communication layer for connecting to external AI services:
- WebSocket-based protocol (primary)
- UDP support (fallback, to be implemented in Electron main process)
- Request/response message matching
- Automatic reconnection
- Error handling and timeouts

### 3. Wingman Panel (`src/renderer/components/WingmanPanel.tsx`)
User interface for interacting with AI features:
- Chat-based interface
- Quick action buttons for instant AI generation
- Real-time generation feedback
- Integration with timeline/track system

## MagentaService

### Initialization

```typescript
import magentaService from '../lib/MagentaService';

await magentaService.initialize();
```

### API Reference

#### Generate Chord Progression

```typescript
const progression = await magentaService.generateChordProgression('C', {
  temperature: 1.0,
  steps: 4
});

console.log(progression);
// {
//   chords: ['C', 'Am', 'F', 'G'],
//   notes: Note[],
//   key: 'C',
//   scale: 'major'
// }
```

Supported keys: `C`, `Cm`, `Am`, `Dm`, `G`, `Em`, and more

#### Generate Melody

```typescript
const melody = await magentaService.generateMelody([], {
  temperature: 1.0,
  steps: 32,
  seed: []
});

console.log(melody);
// {
//   notes: Note[],
//   temperature: 1.0,
//   steps: 32
// }
```

Options:
- `temperature`: Controls randomness (0.0 - 2.0, default: 1.0)
- `steps`: Number of notes to generate (default: 32)
- `seed`: Optional starting notes

#### Generate Drum Pattern

```typescript
const drums = await magentaService.generateDrumPattern('trap', {
  style: 'trap'
});

console.log(drums);
// {
//   pattern: Note[],
//   style: 'trap',
//   bars: 4
// }
```

Supported styles:
- `trap`: Modern trap drums with 808s and hi-hat rolls
- `house`: Four-on-the-floor house pattern
- `techno`: (default) Generic electronic pattern

### Note Format

All generation methods return notes in this format:

```typescript
interface Note {
  pitch: number;      // MIDI note number (0-127)
  velocity: number;   // Note velocity (0-127)
  startTime: number;  // Start time in beats
  duration: number;   // Duration in beats
}
```

## AI Bridge

### Configuration

```typescript
import { AIBridge } from '../lib/AIBridge';

const bridge = new AIBridge({
  websocket: {
    enabled: true,
    host: 'localhost',
    port: 8765,
    secure: false,
    reconnectInterval: 3000,
    maxReconnectAttempts: 5
  },
  udp: {
    enabled: false,
    host: 'localhost',
    port: 8766,
    timeout: 5000
  },
  preferredTransport: 'websocket',
  messageTimeout: 10000,
  maxRetries: 3
});
```

### Connection Management

```typescript
// Connect to AI service
await bridge.connect();

// Check connection status
if (bridge.isConnected()) {
  console.log('Connected to AI Bridge');
}

// Get detailed status
const status = bridge.getStatus();
console.log(status);
// {
//   status: 'connected',
//   transport: 'websocket',
//   lastMessageTime: 1234567890,
//   reconnectAttempts: 0
// }

// Disconnect
bridge.disconnect();
```

### Sending Messages

```typescript
import { MessageType } from '../types/ai-bridge';

// Create and send request
const request = bridge.createRequest(MessageType.GENERATE_MELODY, {
  steps: 32,
  temperature: 1.0,
  key: 'C',
  scale: 'major'
});

const response = await bridge.sendMessage(request);
console.log('Generated notes:', response.payload.notes);
```

### Event Listeners

```typescript
// Listen for messages
const unsubscribe = bridge.onMessage((message) => {
  console.log('Received:', message);
});

// Listen for errors
bridge.onError((error) => {
  console.error('Bridge error:', error);
});

// Listen for status changes
bridge.onStatusChange((status) => {
  console.log('Status changed:', status.status);
});

// Cleanup
unsubscribe();
```

### Message Protocol

All messages follow this structure:

```typescript
interface BaseMessage {
  id: string;           // Unique message ID
  type: MessageType;    // Message type enum
  timestamp: number;    // Unix timestamp
}

interface RequestMessage extends BaseMessage {
  payload?: {
    // Request-specific data
  };
}

interface ResponseMessage extends BaseMessage {
  requestId: string;    // ID of the request this responds to
  payload: {
    // Response-specific data
  };
}
```

Supported message types:
- `GENERATE_MELODY`
- `GENERATE_CHORDS`
- `GENERATE_DRUMS`
- `GENERATE_BASS`
- `PLAY` / `STOP` / `PAUSE`
- `SET_TEMPO` / `SET_KEY`
- `GET_STATUS` / `GET_CAPABILITIES`

## Wingman Panel Integration

### Chat Interface

The Wingman panel provides a conversational interface for AI interactions:

```typescript
<WingmanPanel
  isOpen={isWingmanOpen}
  onClose={() => setIsWingmanOpen(false)}
/>
```

Users can type natural language prompts:
- "Create trap drums"
- "Generate chords in C minor"
- "Add a melody"
- "Make a bass line"

### Processing Flow

1. User sends message
2. `processAIRequest()` analyzes the prompt
3. Appropriate MagentaService method is called
4. Generated content is displayed in chat
5. Content is ready to be inserted into timeline

### Implementation

```typescript
const processAIRequest = async (input: string): Promise<string> => {
  const lowerInput = input.toLowerCase();

  await magentaService.initialize();

  if (lowerInput.includes('drum')) {
    const style = detectDrumStyle(lowerInput);
    const result = await magentaService.generateDrumPattern(style);
    return formatDrumResponse(result);
  }

  // ... other handlers
};
```

## Quick Actions

### UI Buttons

Quick action buttons provide instant access to common AI operations:

```typescript
<QuickActionButton
  icon={<Drum className="h-3 w-3" />}
  label="Generate Drums"
  onClick={() => handleQuickAction('drums')}
  variant="ai"
/>
```

Available quick actions:
- **Generate Drums**: Creates trap-style drum pattern
- **Generate Chords**: Creates I-vi-IV-V progression in C
- **Generate Melody**: Creates 32-step melodic sequence
- **Generate Bass**: Creates 16-step bass line

### Custom Implementation

```typescript
const handleQuickAction = async (action: 'drums' | 'chords' | 'melody' | 'bass') => {
  setIsProcessing(true);

  try {
    await magentaService.initialize();
    let result: string;

    switch (action) {
      case 'drums':
        const drums = await magentaService.generateDrumPattern('trap');
        result = `Created ${drums.style} drums: ${drums.pattern.length} hits`;
        break;
      // ... other cases
    }

    // Display result in chat
    addMessage('assistant', result);
  } finally {
    setIsProcessing(false);
  }
};
```

## Usage Examples

### Example 1: Generate and Insert Drums

```typescript
// In your component
const generateAndInsertDrums = async (trackId: string) => {
  // Initialize service
  await magentaService.initialize();

  // Generate pattern
  const drums = await magentaService.generateDrumPattern('trap');

  // Create clip from pattern
  const clip = {
    id: generateId(),
    name: 'AI Drums',
    trackId,
    notes: drums.pattern,
    color: getRandomColor(),
    length: drums.bars * 4,
  };

  // Insert into timeline
  addClipToTimeline(clip);
};
```

### Example 2: Connect to External AI Service

```typescript
import { aiBridge } from '../lib/AIBridge';
import { MessageType } from '../types/ai-bridge';

// Connect to external service
await aiBridge.connect();

// Request melody generation
const request = aiBridge.createRequest(MessageType.GENERATE_MELODY, {
  steps: 64,
  temperature: 1.2,
  key: 'Am',
  scale: 'minor'
});

const response = await aiBridge.sendMessage(request);

// Use generated notes
const melody = response.payload.notes;
insertNotesIntoTrack(melody, trackId);
```

### Example 3: Custom Chord Progression

```typescript
// Generate custom progression
const progression = await magentaService.generateChordProgression('Dm', {
  temperature: 0.8
});

console.log(`Progression: ${progression.chords.join(' - ')}`);
console.log(`Notes: ${progression.notes.length} total notes`);

// Use in your track
addChordTrack(progression.notes);
```

## Development Guide

### Adding New Generation Types

1. **Update MagentaService**

```typescript
// Add new method
async generateHarmony(seed: Note[], options?: GenerationOptions): Promise<HarmonyResult> {
  await this.ensureInitialized();

  // Your generation logic here
  const harmony = this.generateHarmonyVoices(seed);

  return {
    voices: harmony,
    style: options?.style || 'default'
  };
}
```

2. **Update AI Bridge Types**

```typescript
// In ai-bridge.ts
export enum MessageType {
  // ... existing types
  GENERATE_HARMONY = 'generate_harmony',
}

export interface GenerateHarmonyRequest extends BaseMessage {
  type: MessageType.GENERATE_HARMONY;
  payload: {
    seed?: Note[];
    voices?: number;
  };
}
```

3. **Update Wingman Panel**

```typescript
// Add to processAIRequest
if (lowerInput.includes('harmony')) {
  const result = await magentaService.generateHarmony();
  return formatHarmonyResponse(result);
}
```

4. **Add Quick Action Button**

```typescript
<QuickActionButton
  icon={<Music className="h-3 w-3" />}
  label="Generate Harmony"
  onClick={() => handleQuickAction('harmony')}
  variant="ai"
/>
```

### Testing

```typescript
// Test MagentaService
import magentaService from './MagentaService';

describe('MagentaService', () => {
  test('generates chord progression', async () => {
    await magentaService.initialize();
    const result = await magentaService.generateChordProgression('C');

    expect(result.chords).toHaveLength(4);
    expect(result.key).toBe('C');
    expect(result.notes.length).toBeGreaterThan(0);
  });
});
```

### Debugging

Enable debug logging:

```typescript
// In MagentaService
console.log('🎵 Generating melody with params:', params);

// In AIBridge
console.log('📨 Received message:', message.type);
console.log('📤 Sending message:', request.type);
```

Monitor WebSocket connection:

```typescript
aiBridge.onStatusChange((status) => {
  console.log('Bridge status:', status);
});
```

## Server Setup

To use the AI Bridge with an external service:

1. Navigate to the server directory:
```bash
cd ai-bridge-server
```

2. Install dependencies:
```bash
npm install
```

3. Start the server:
```bash
npm start
```

4. Connect from the DAW:
```typescript
import { aiBridge } from '../lib/AIBridge';

await aiBridge.connect(); // Connects to ws://localhost:8765
```

See `ai-bridge-server/README.md` for detailed server documentation.

## Future Enhancements

- [ ] Integrate actual Magenta.js models
- [ ] Add real-time preview of generated content
- [ ] Implement UDP transport in Electron main process
- [ ] Add authentication to AI Bridge
- [ ] Support for multiple simultaneous AI services
- [ ] Advanced generation options (genre, mood, complexity)
- [ ] Integration with external APIs (OpenAI, etc.)
- [ ] Collaborative AI sessions (multiple users)
- [ ] AI-powered mixing and mastering
- [ ] Style transfer and remixing capabilities

## Troubleshooting

### MagentaService not initializing
- Check console for errors
- Ensure `initialize()` is called before generation methods
- Verify browser compatibility (modern browsers required)

### AI Bridge connection fails
- Verify server is running (`npm start` in ai-bridge-server)
- Check server address and port in config
- Ensure no firewall blocking WebSocket connections
- Try disabling browser extensions that might block WebSockets

### Generation is slow
- Consider running AI Bridge server on more powerful hardware
- Reduce `steps` parameter for faster generation
- Use lower `temperature` for more predictable results
- Implement caching for common patterns

## Resources

- [Magenta.js Documentation](https://magenta.tensorflow.org/js)
- [WebSocket API](https://developer.mozilla.org/en-US/docs/Web/API/WebSocket)
- [MIDI Note Numbers](https://www.inspiredacoustics.com/en/MIDI_note_numbers_and_center_frequencies)
- [Music Theory Basics](https://www.musictheory.net/)

## License

MIT License - See LICENSE file for details
