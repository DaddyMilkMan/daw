# InstrumentAIAdapter - Example Workflow

This document demonstrates the complete JSON RPC sequences for the InstrumentAIAdapter.

## Setup

1. Start Zenith DAW (with CommandAPI on `http://localhost:8080/command`)
2. Start AI Bridge Server:

```bash
cd /home/user/daw/ai-bridge-server
node server.js
```

3. Connect via WebSocket on `ws://localhost:8765`

---

## Example 1: Suggest Preset for Synthwave Lead

### Client → Server (WebSocket Message)

```json
{
  "id": "msg-001",
  "type": "suggest_preset",
  "payload": {
    "genre": "synthwave",
    "mood": "nostalgic",
    "role": "lead",
    "instrumentId": "zenith_poly_synth",
    "trackId": "track_0"
  }
}
```

### Internal Flow (Server-side)

#### 1. Adapter → CommandAPI: Query Presets

```json
{
  "command": "list_presets",
  "params": {
    "instrumentId": "zenith_poly_synth"
  }
}
```

#### 2. CommandAPI → Adapter: Preset List

```json
{
  "status": "ok",
  "data": {
    "presets": [
      {
        "id": "bright_pluck_1234567891",
        "name": "Bright Pluck",
        "category": "Factory",
        "tags": ["pluck", "bright", "lead"]
      },
      {
        "id": "retro_lead_1234567892",
        "name": "Retro Lead",
        "category": "Factory",
        "tags": ["lead", "retro", "warm"]
      },
      {
        "id": "dark_pad_1234567893",
        "name": "Dark Pad",
        "category": "Factory",
        "tags": ["pad", "dark", "ambient"]
      }
    ]
  }
}
```

#### 3. Adapter → LLM: Reasoning Request

**System Prompt:**
```
You are an expert sound designer for the Zenith Poly Synth. Select the most appropriate preset based on musical context.

Available presets:
- bright_pluck_1234567891: "Bright Pluck" [pluck, bright, lead]
- retro_lead_1234567892: "Retro Lead" [lead, retro, warm]
- dark_pad_1234567893: "Dark Pad" [pad, dark, ambient]

Respond with JSON: { "presetId": "...", "reasoning": "..." }
```

**User Prompt:**
```
Select a preset for:
- Genre: synthwave
- Mood: nostalgic
- Role: lead

Which preset best matches these requirements?
```

#### 4. LLM → Adapter: Selection

```json
{
  "presetId": "bright_pluck_1234567891",
  "reasoning": "Bright Pluck is perfect for synthwave leads with its bright, punchy character. The pluck envelope gives it a nostalgic 80s vibe that fits the genre perfectly."
}
```

### Server → Client (WebSocket Response)

```json
{
  "type": "preset_suggestion",
  "requestId": "msg-001",
  "timestamp": 1699123456789,
  "payload": {
    "presetId": "bright_pluck_1234567891",
    "presetName": "Bright Pluck",
    "reasoning": "Bright Pluck is perfect for synthwave leads with its bright, punchy character. The pluck envelope gives it a nostalgic 80s vibe that fits the genre perfectly.",
    "genre": "synthwave",
    "mood": "nostalgic",
    "role": "lead"
  }
}
```

### Client → CommandAPI: Load Preset (Optional)

The client can then load the suggested preset:

```json
{
  "command": "load_preset",
  "params": {
    "trackId": "track_0",
    "instrumentId": "zenith_poly_synth",
    "presetId": "bright_pluck_1234567891"
  }
}
```

---

## Example 2: Tweak Preset with Natural Language

### Client → Server (WebSocket Message)

```json
{
  "id": "msg-002",
  "type": "tweak_preset",
  "payload": {
    "trackId": "track_0",
    "instructions": "make it darker and more detuned, add a touch of reverb"
  }
}
```

### Internal Flow (Server-side)

#### 1. Adapter → CommandAPI: Get Current Parameters

```json
{
  "command": "get_instrument_parameters",
  "params": {
    "trackId": "track_0"
  }
}
```

#### 2. CommandAPI → Adapter: Current State

```json
{
  "status": "ok",
  "data": {
    "parameters": [
      {
        "id": "filter_cutoff",
        "name": "Filter Cutoff",
        "min": 0.0,
        "max": 1.0,
        "default": 0.8,
        "value": 0.75
      },
      {
        "id": "filter_resonance",
        "name": "Filter Resonance",
        "min": 0.0,
        "max": 1.0,
        "default": 0.5,
        "value": 0.3
      },
      {
        "id": "unison_detune",
        "name": "Unison Detune",
        "min": 0.0,
        "max": 1.0,
        "default": 0.0,
        "value": 0.1
      },
      {
        "id": "reverb_send",
        "name": "Reverb Send",
        "min": 0.0,
        "max": 1.0,
        "default": 0.0,
        "value": 0.0
      }
    ]
  }
}
```

#### 3. Adapter → LLM: Parameter Tweak Request

**System Prompt:**
```
You are an expert sound designer. Translate natural language instructions into parameter adjustments.

Current parameters:
- filter_cutoff: 0.75 (brightness)
- filter_resonance: 0.3 (character)
- unison_detune: 0.1 (width)
- reverb_send: 0.0 (space)

Parameter knowledge:
- filter_cutoff: Controls brightness (lower = darker, higher = brighter)
- filter_resonance: Adds character
- unison_detune: Detuning for width
- reverb_send: Reverb amount

Respond with JSON: { "parameters": {...}, "reasoning": "..." }
```

**User Prompt:**
```
Current track: track_0

User instructions: "make it darker and more detuned, add a touch of reverb"

What parameter changes should be made?
```

#### 4. LLM → Adapter: Parameter Changes

```json
{
  "parameters": {
    "filter_cutoff": 0.35,
    "unison_detune": 0.45,
    "reverb_send": 0.25
  },
  "reasoning": "Lowered filter cutoff from 0.75 to 0.35 for darkness, increased detune from 0.1 to 0.45 for width, added reverb send at 0.25 for subtle space."
}
```

#### 5. Adapter: Apply Safety Clamping

All values are within safe ranges:
- `filter_cutoff: 0.35` ✓ (< maxFilterCutoff 0.95)
- `unison_detune: 0.45` ✓ (< 1.0)
- `reverb_send: 0.25` ✓ (< maxGain 0.9)

### Server → Client (WebSocket Response)

```json
{
  "type": "preset_tweaks",
  "requestId": "msg-002",
  "timestamp": 1699123457890,
  "payload": {
    "parameters": {
      "filter_cutoff": 0.35,
      "unison_detune": 0.45,
      "reverb_send": 0.25
    },
    "reasoning": "Lowered filter cutoff from 0.75 to 0.35 for darkness, increased detune from 0.1 to 0.45 for width, added reverb send at 0.25 for subtle space.",
    "commands": [
      {
        "command": "set_instrument_parameters",
        "params": {
          "trackId": "track_0",
          "params": {
            "filter_cutoff": 0.35,
            "unison_detune": 0.45,
            "reverb_send": 0.25
          }
        }
      }
    ],
    "instructions": "make it darker and more detuned, add a touch of reverb"
  }
}
```

### Client → CommandAPI: Apply Tweaks

```json
{
  "command": "set_instrument_parameters",
  "params": {
    "trackId": "track_0",
    "params": {
      "filter_cutoff": 0.35,
      "unison_detune": 0.45,
      "reverb_send": 0.25
    }
  }
}
```

### CommandAPI → Client: Success

```json
{
  "status": "ok",
  "data": {
    "success": true,
    "updated": ["filter_cutoff", "unison_detune", "reverb_send"]
  }
}
```

---

## Example 3: Complex Transformation

### Scenario: Transform Pad into Techno Bass

**Client → Server:**
```json
{
  "id": "msg-003",
  "type": "tweak_preset",
  "payload": {
    "trackId": "track_1",
    "instructions": "transform this into an aggressive techno bass - punchy, tight, and powerful"
  }
}
```

**LLM Analysis:** The LLM interprets this as requiring:
- Low filter cutoff (bass frequencies)
- High resonance (aggressive character)
- Instant attack (punchy)
- Short release (tight)

**Server → Client Response:**
```json
{
  "type": "preset_tweaks",
  "requestId": "msg-003",
  "timestamp": 1699123458901,
  "payload": {
    "parameters": {
      "filter_cutoff": 0.25,
      "filter_resonance": 0.75,
      "attack": 0.001,
      "release": 0.12,
      "saturation": 0.6
    },
    "reasoning": "Set filter cutoff to 0.25 for bass focus, increased resonance to 0.75 for aggressive character, minimized attack for punchiness, shortened release to 0.12 for tightness, added saturation for power.",
    "commands": [...]
  }
}
```

---

## Testing with curl

You can test the adapter integration using curl (for debugging):

### Test Suggest Preset

```bash
# This requires the WebSocket connection to be open
# Use a WebSocket client like wscat instead:

npm install -g wscat
wscat -c ws://localhost:8765

# Then send:
{
  "id": "test-001",
  "type": "suggest_preset",
  "payload": {
    "genre": "ambient",
    "mood": "dark",
    "role": "pad",
    "instrumentId": "zenith_poly_synth"
  }
}
```

### Test Tweak Preset

```bash
# Via wscat:
{
  "id": "test-002",
  "type": "tweak_preset",
  "payload": {
    "trackId": "track_0",
    "instructions": "make it brighter with more movement"
  }
}
```

---

## Complete End-to-End Workflow

### 1. Start Services

```bash
# Terminal 1: Start Zenith DAW
cd /home/user/daw
./build/zenith-core

# Terminal 2: Start AI Bridge Server
cd /home/user/daw/ai-bridge-server
node server.js
```

### 2. Connect Client

```javascript
const ws = new WebSocket('ws://localhost:8765');

ws.on('open', () => {
  console.log('Connected to AI Bridge Server');

  // Suggest preset
  ws.send(JSON.stringify({
    id: 'req-1',
    type: 'suggest_preset',
    payload: {
      genre: 'synthwave',
      mood: 'nostalgic',
      role: 'lead',
      instrumentId: 'zenith_poly_synth',
      trackId: 'track_0'
    }
  }));
});

ws.on('message', (data) => {
  const response = JSON.parse(data);
  console.log('Received:', response);

  if (response.type === 'preset_suggestion') {
    console.log(`Suggested: ${response.payload.presetName}`);
    console.log(`Reasoning: ${response.payload.reasoning}`);

    // Load the preset
    fetch('http://localhost:8080/command', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        command: 'load_preset',
        params: {
          trackId: 'track_0',
          instrumentId: 'zenith_poly_synth',
          presetId: response.payload.presetId
        }
      })
    });
  }
});
```

---

## Performance Metrics

### Typical Latencies

| Operation | Time | Notes |
|-----------|------|-------|
| WebSocket roundtrip | ~2ms | Local network |
| CommandAPI query | ~10ms | list_presets, get_parameters |
| LLM call (mock) | ~50ms | Mock implementation |
| LLM call (OpenAI GPT-4) | ~1000-2000ms | Network + inference |
| LLM call (Anthropic Claude) | ~800-1500ms | Network + inference |
| Parameter validation | <1ms | Local computation |
| Safety clamping | <1ms | Local computation |
| CommandAPI execute | ~10ms | set_instrument_parameters |

### Total End-to-End Time

- **Suggest Preset**: ~1-2 seconds (with real LLM)
- **Tweak Preset**: ~1-2 seconds (with real LLM)
- **Mock LLM**: ~100ms total

---

## Environment Variables

Configure the server behavior with environment variables:

```bash
# CommandAPI endpoint
export COMMAND_API_URL=http://localhost:8080/command

# LLM provider (mock, openai, anthropic)
export LLM_PROVIDER=openai

# OpenAI configuration
export OPENAI_API_KEY=sk-...

# Anthropic configuration
export ANTHROPIC_API_KEY=sk-ant-...

# Start server
node server.js
```

---

## Next Steps

1. **Deploy Real LLM**: Replace `mockLLMCall()` with actual API calls
2. **Add Caching**: Cache preset lists and LLM responses
3. **Add UI**: Build frontend controls in WingmanPanel
4. **Extend Coverage**: Support more instruments and parameters
5. **Add Presets**: Create more factory presets with rich metadata
