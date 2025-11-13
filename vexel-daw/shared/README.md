# Zenith DAW Protocol & Schemas

This directory contains the canonical protocol definition and JSON schemas for Zenith DAW.

## Directory Structure

```
shared/
├── schemas/              # JSON Schema definitions
│   ├── commands/         # Command schemas
│   │   ├── transport.json
│   │   ├── track.json
│   │   ├── clip.json
│   │   ├── plugin.json
│   │   └── project.json
│   └── events/          # Event schemas
│       ├── transport.json
│       ├── track.json
│       ├── clip.json
│       ├── plugin.json
│       ├── meters.json
│       └── project.json
├── types/               # Generated TypeScript types
│   └── protocol.ts      # AUTO-GENERATED - do not edit
├── protocol.md          # Protocol specification
└── README.md           # This file
```

## Overview

The Zenith DAW protocol follows a **command-event** architecture:

- **Commands**: Actions sent to the DAW engine (e.g., `transport_play`, `create_track`)
- **Events**: Notifications broadcast when state changes (e.g., `track_added`, `transport_state`)

## JSON Schemas

All protocol messages are defined using [JSON Schema (Draft-07)](https://json-schema.org/draft-07/schema).

### Schema Features

- **Strict typing**: All properties have explicit types and constraints
- **Validation**: Range checks (e.g., tempo: 1-999 BPM, volume: -120 to +12 dB)
- **UUID format**: All IDs follow UUID v4 pattern validation
- **Enums**: Predefined values for types, modes, etc.
- **Documentation**: `description` fields for all properties

### Schema Organization

**Commands** (`shared/schemas/commands/`)
- `transport.json` - Playback control, tempo, time signature
- `track.json` - Track creation, deletion, property updates
- `clip.json` - MIDI/audio clip management
- `plugin.json` - Plugin insertion, parameter changes
- `project.json` - Project save/load operations

**Events** (`shared/schemas/events/`)
- `transport.json` - Transport state, playhead position
- `track.json` - Track added/updated/removed
- `clip.json` - Clip added/updated/removed
- `plugin.json` - Plugin inserted, parameters changed
- `meters.json` - Audio level metering
- `project.json` - Project state snapshots

## TypeScript Types

TypeScript types are automatically generated from JSON schemas.

### Generating Types

```bash
npm run build:schemas
```

This command:
1. Scans all JSON schemas in `shared/schemas/**/*.json`
2. Compiles them to TypeScript using `json-schema-to-typescript`
3. Outputs a single file: `shared/types/protocol.ts`

**⚠️ Important**: Never edit `protocol.ts` manually - it will be overwritten!

### Using Types

```typescript
import {
  Command,
  Event,
  TransportPlayCommand,
  TrackAddedEvent,
  CommandRequest,
  CommandResponse,
} from '../shared/types/protocol';

// Type-safe command
const cmd: TransportPlayCommand = {
  command: 'transport_play',
  params: {},
};

// Type-safe event
const event: TrackAddedEvent = {
  type: 'track_added',
  timestamp: new Date().toISOString(),
  data: {
    track: { /* ... */ },
  },
};

// Command request with ID
const request: CommandRequest<TransportPlayCommand> = {
  id: crypto.randomUUID(),
  command: 'transport_play',
  params: {},
};
```

### Type Guards

The protocol provides type guard functions:

```typescript
import { isCommand, isEvent, isCommandRequest, isCommandResponse } from '../shared/types/protocol';

if (isCommand(msg)) {
  // msg is Command type
  console.log('Command:', msg.command);
}

if (isEvent(msg)) {
  // msg is Event type
  console.log('Event:', msg.type);
}
```

## Units and Conventions

### Time
- **Beats**: Musical time (e.g., clip positions, markers)
- **Samples**: Absolute time at sample rate
- **Seconds**: Real time
- **ISO 8601**: Timestamps (e.g., `2025-11-10T12:00:00Z`)

### Audio
- **Volume/Gain**: Decibels (dB), range: -∞ to +12 dB
- **Pan**: -1.0 (left) to +1.0 (right), 0.0 = center
- **Tempo**: BPM, range: 1-999
- **MIDI Notes**: 0-127 (C-1 to G9)
- **MIDI Velocity**: 1-127

### IDs
- **UUID v4 Format**: `xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx`
- **Pattern**: Lowercase hex characters with hyphens
- **Generation**: `crypto.randomUUID()` (browser/Node.js)

## Command Structure

All commands follow this structure:

```json
{
  "command": "command_name",
  "params": {
    "requiredParam": "value",
    "optionalParam": "value"
  }
}
```

### Command Request/Response

```json
// Request
{
  "id": "550e8400-e29b-41d4-a716-446655440000",
  "command": "set_tempo",
  "params": { "tempo": 140 }
}

// Success Response
{
  "id": "550e8400-e29b-41d4-a716-446655440000",
  "success": true,
  "result": { "tempo": 140 }
}

// Error Response
{
  "id": "550e8400-e29b-41d4-a716-446655440000",
  "success": false,
  "error": {
    "code": "INVALID_PARAM",
    "message": "Tempo must be between 1 and 999 BPM",
    "details": { "param": "tempo", "value": 1500 }
  }
}
```

## Event Structure

All events follow this structure:

```json
{
  "type": "event_type",
  "timestamp": "2025-11-10T12:00:00.123Z",
  "data": {
    // Event-specific payload
  }
}
```

### Event Examples

```json
// Transport state changed
{
  "type": "transport_state",
  "timestamp": "2025-11-10T12:00:00Z",
  "data": {
    "isPlaying": true,
    "isRecording": false,
    "position": 64.5,
    "tempo": 120,
    "timeSignature": { "numerator": 4, "denominator": 4 }
  }
}

// Track added
{
  "type": "track_added",
  "timestamp": "2025-11-10T12:00:01Z",
  "data": {
    "track": {
      "id": "uuid-here",
      "name": "Vocals",
      "type": "audio",
      // ... full track object
    }
  }
}
```

## Error Codes

| Code | Description |
|------|-------------|
| `INVALID_PARAM` | Parameter validation failed |
| `NOT_FOUND` | Entity not found |
| `PERMISSION_DENIED` | Operation not allowed |
| `RESOURCE_BUSY` | Resource locked |
| `AUDIO_ERROR` | Audio engine failure |
| `IO_ERROR` | File system error |
| `PLUGIN_ERROR` | Plugin error |
| `UNKNOWN_ERROR` | Unexpected error |

## Validation

To validate messages against schemas:

```bash
# Using ajv-cli (install globally or as dev dependency)
npx ajv-cli validate -s shared/schemas/commands/transport.json -d message.json
```

## Development Workflow

1. **Modify schemas**: Edit JSON files in `shared/schemas/`
2. **Regenerate types**: Run `npm run build:schemas`
3. **Update code**: Use new types in your TypeScript code
4. **Test**: Ensure messages validate against schemas

## Documentation

- **Core Model**: See `docs/core-model.md` for data model specification
- **Protocol Spec**: See `shared/protocol.md` for detailed command/event docs
- **API Examples**: See integration examples in respective feature docs

## Version History

### Version 1.0.0 (2025-11-10)
- Initial protocol and schema definition
- Command schemas: transport, track, clip, plugin, project
- Event schemas: transport, track, clip, plugin, meters, project
- TypeScript type generation pipeline
- Complete documentation

---

**Generated Types Location**: `shared/types/protocol.ts`
**Regenerate Command**: `npm run build:schemas`
**Schema Version**: 1.0.0
