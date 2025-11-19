# Instrument AI Adapter - Integration Guide

The **InstrumentAIAdapter** is a host-side AI service that translates natural language instructions into CommandAPI calls for intelligent instrument control in Zenith DAW.

## Architecture

```
┌─────────────────────────────────────────┐
│   User Input (Natural Language)         │
│   "Make it darker and more detuned"     │
└────────────────┬────────────────────────┘
                 │
                 v
┌─────────────────────────────────────────┐
│   InstrumentAIAdapter (TypeScript)      │
│   - Parse intent                        │
│   - Query current state via CommandAPI  │
│   - Call LLM for reasoning              │
│   - Generate parameter changes          │
│   - Apply safety limits                 │
└────────────────┬────────────────────────┘
                 │
                 v
┌─────────────────────────────────────────┐
│   CommandAPI (Zenith DAW)               │
│   - get_instrument_parameters           │
│   - set_instrument_parameters           │
│   - list_presets                        │
│   - load_preset                         │
└─────────────────────────────────────────┘
```

## Key Features

✅ **Stateless Design**: No state stored in adapter - CommandAPI is source of truth
✅ **Safety First**: Automatic parameter clamping to prevent extreme/dangerous settings
✅ **LLM Agnostic**: Works with OpenAI, Anthropic Claude, or any JSON-compatible LLM
✅ **Musical Intelligence**: Context-aware parameter suggestions based on genre/mood/role
✅ **Batch Operations**: Groups parameter changes into single undo transactions

---

## Core Functions

### 1. `suggestPreset({ genre, mood, role, instrumentId, trackId })`

**Purpose**: Suggests an appropriate preset based on high-level musical intent.

**How it works**:
1. Queries available presets via `list_presets` CommandAPI
2. Sends preset list + intent to LLM for reasoning
3. Returns selected preset with explanation
4. Can be executed via `load_preset` CommandAPI

**Example Usage**:

```javascript
const suggestion = await adapter.suggestPreset({
  genre: 'synthwave',
  mood: 'nostalgic',
  role: 'lead',
  instrumentId: 'zenith_poly_synth',
  trackId: 'track_0'
});

// Result:
// {
//   presetId: 'bright_pluck_1234567891',
//   presetName: 'Bright Pluck',
//   reasoning: 'This preset has bright, punchy characteristics perfect for synthwave leads with a nostalgic vibe'
// }
```

---

### 2. `tweakPreset({ trackId, instructions })`

**Purpose**: Translates natural language parameter tweaking instructions into CommandAPI calls.

**How it works**:
1. Queries current parameters via `get_instrument_parameters`
2. Sends current state + instructions to LLM
3. LLM generates parameter change suggestions
4. Applies safety clamping to prevent extreme values
5. Returns CommandAPI commands ready to execute

**Example Usage**:

```javascript
const tweaks = await adapter.tweakPreset({
  trackId: 'track_0',
  instructions: 'make it darker and more detuned, add a touch of reverb'
});

// Result:
// {
//   parameters: {
//     filter_cutoff: 0.35,      // Darker (was 0.8)
//     unison_detune: 0.45,      // More detuned (was 0.1)
//     reverb_send: 0.25         // Touch of reverb (was 0.0)
//   },
//   reasoning: 'Lowered filter cutoff for darkness, increased detune for width, added reverb',
//   commands: [
//     {
//       command: 'set_instrument_parameters',
//       params: {
//         trackId: 'track_0',
//         params: { filter_cutoff: 0.35, unison_detune: 0.45, reverb_send: 0.25 }
//       }
//     }
//   ]
// }
```

---

## Complete Workflow Examples

### Example 1: Synthwave Lead Generation

**User Request**: *"I need a synthwave lead sound - something bright and nostalgic"*

#### Step 1: Suggest Preset

**AI Adapter → CommandAPI**:
```json
{
  "command": "list_presets",
  "params": {
    "instrumentId": "zenith_poly_synth"
  }
}
```

**CommandAPI Response**:
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
        "id": "basic_pad_1234567890",
        "name": "Basic Pad",
        "category": "Factory",
        "tags": ["pad", "warm", "lush"]
      }
    ]
  }
}
```

**AI Adapter → LLM**:
```
System: You are an expert sound designer. Select the best preset for synthwave lead, bright, nostalgic.

Available presets:
- bright_pluck_1234567891: "Bright Pluck" [pluck, bright, lead]
- retro_lead_1234567892: "Retro Lead" [lead, retro, warm]
- basic_pad_1234567890: "Basic Pad" [pad, warm, lush]

Respond with JSON: { "presetId": "...", "reasoning": "..." }
```

**LLM Response**:
```json
{
  "presetId": "bright_pluck_1234567891",
  "reasoning": "Bright Pluck has the perfect combination of brightness and lead characteristics for synthwave. The pluck envelope gives it a punchy, nostalgic quality."
}
```

**AI Adapter → CommandAPI (Load Preset)**:
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

### Example 2: Natural Language Tweaking

**User Request**: *"Make this pad darker, add more reverb, and give it some movement"*

#### Step 1: Get Current State

**AI Adapter → CommandAPI**:
```json
{
  "command": "get_instrument_parameters",
  "params": {
    "trackId": "track_1"
  }
}
```

**CommandAPI Response**:
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
        "id": "reverb_send",
        "name": "Reverb Send",
        "min": 0.0,
        "max": 1.0,
        "default": 0.0,
        "value": 0.15
      },
      {
        "id": "lfo_rate",
        "name": "LFO Rate",
        "min": 0.0,
        "max": 1.0,
        "default": 0.5,
        "value": 0.4
      },
      {
        "id": "lfo_depth",
        "name": "LFO Depth",
        "min": 0.0,
        "max": 1.0,
        "default": 0.0,
        "value": 0.0
      }
    ]
  }
}
```

#### Step 2: Generate Parameter Changes

**AI Adapter → LLM**:
```
System: You are an expert sound designer. Translate instructions into parameter changes.

Current parameters:
- filter_cutoff: 0.75 (brightness)
- filter_resonance: 0.3 (character)
- reverb_send: 0.15 (reverb amount)
- lfo_rate: 0.4 (modulation speed)
- lfo_depth: 0.0 (modulation intensity)

User instructions: "Make this pad darker, add more reverb, and give it some movement"

Respond with JSON: { "parameters": {...}, "reasoning": "..." }
```

**LLM Response**:
```json
{
  "parameters": {
    "filter_cutoff": 0.45,
    "reverb_send": 0.35,
    "lfo_depth": 0.25
  },
  "reasoning": "Lowered filter cutoff from 0.75 to 0.45 for darkness, increased reverb send from 0.15 to 0.35 for more space, enabled LFO depth at 0.25 for subtle movement (using existing LFO rate of 0.4)"
}
```

#### Step 3: Apply Safety & Execute

**AI Adapter (Internal Safety Check)**:
```javascript
// All values within safe ranges:
// filter_cutoff: 0.45 < maxFilterCutoff (0.95) ✓
// reverb_send: 0.35 < maxGain (0.9) ✓
// lfo_depth: 0.25 < 1.0 ✓
```

**AI Adapter → CommandAPI**:
```json
{
  "command": "set_instrument_parameters",
  "params": {
    "trackId": "track_1",
    "params": {
      "filter_cutoff": 0.45,
      "reverb_send": 0.35,
      "lfo_depth": 0.25
    }
  }
}
```

**CommandAPI Response**:
```json
{
  "status": "ok",
  "data": {
    "success": true,
    "updated": ["filter_cutoff", "reverb_send", "lfo_depth"]
  }
}
```

---

### Example 3: Complex Multi-Step Transformation

**User Request**: *"Transform this basic pad into an aggressive techno bass"*

This example shows multiple operations in sequence:

#### Step 1: Get Current Preset Info

**AI Adapter → CommandAPI**:
```json
{
  "command": "get_instrument_parameters",
  "params": { "trackId": "track_2" }
}
```

#### Step 2: AI Reasoning Phase

**LLM Analysis**:
```json
{
  "approach": "dramatic_transformation",
  "steps": [
    "Lower filter cutoff significantly (0.3) for bass frequency focus",
    "Increase resonance (0.7) for aggressive character",
    "Shorten attack (0.001) for punchy onset",
    "Reduce release (0.15) for tight, rhythmic sound",
    "Add distortion/saturation if available"
  ]
}
```

#### Step 3: Execute Transformation

**AI Adapter → CommandAPI**:
```json
{
  "command": "set_instrument_parameters",
  "params": {
    "trackId": "track_2",
    "params": {
      "filter_cutoff": 0.3,
      "filter_resonance": 0.7,
      "attack": 0.001,
      "release": 0.15,
      "saturation": 0.6
    }
  }
}
```

#### Step 4: Optional - Save as New Preset

**AI Adapter → CommandAPI**:
```json
{
  "command": "save_preset",
  "params": {
    "trackId": "track_2",
    "name": "Aggressive Techno Bass",
    "category": "User",
    "tags": ["bass", "techno", "aggressive", "punchy"]
  }
}
```

**CommandAPI Response**:
```json
{
  "status": "ok",
  "data": {
    "success": true,
    "presetId": "aggressive_techno_bass_1699123456"
  }
}
```

---

## Safety Mechanisms

The adapter includes multiple safety layers to prevent dangerous parameter values:

### Default Safety Limits

```javascript
{
  maxFilterCutoff: 0.95,      // Prevent ear-piercing highs
  maxResonance: 0.85,          // Prevent runaway resonance
  maxGain: 0.9,                // Prevent extreme volume
  minAttack: 0.001,            // Prevent clicks (unless intentional)
  maxRelease: 5.0              // Prevent infinite notes
}
```

### Clamping Examples

| Parameter | LLM Suggestion | After Safety | Reason |
|-----------|---------------|--------------|--------|
| filter_cutoff | 1.0 | 0.95 | Prevent ear damage |
| filter_resonance | 0.99 | 0.85 | Prevent oscillation |
| gain | 1.5 | 0.9 | Prevent clipping |
| attack | 0.0 | 0.001 | Prevent clicks |
| release | 10.0 | 5.0 | Prevent stuck notes |

### Custom Safety Configuration

```javascript
const adapter = new InstrumentAIAdapter({
  sendCommand: myCommandFunction,
  callLLM: myLLMFunction,
  safety: {
    maxFilterCutoff: 0.9,   // More conservative
    maxResonance: 0.75,     // Even safer
    maxGain: 0.85           // Prevent any clipping risk
  }
});
```

---

## Integration Patterns

### Pattern 1: Express.js HTTP Server

```javascript
const express = require('express');
const { InstrumentAIAdapter } = require('./InstrumentAIAdapter');

const app = express();
app.use(express.json());

// Initialize adapter
const adapter = new InstrumentAIAdapter({
  sendCommand: async (cmd) => {
    // Forward to Zenith DAW CommandAPI
    const response = await fetch('http://localhost:8080/command', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(cmd)
    });
    return await response.json();
  },
  callLLM: async (prompt) => {
    // Call OpenAI GPT-4
    const response = await fetch('https://api.openai.com/v1/chat/completions', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
        'Authorization': `Bearer ${process.env.OPENAI_API_KEY}`
      },
      body: JSON.stringify({
        model: 'gpt-4',
        messages: [
          { role: 'system', content: prompt.systemPrompt },
          { role: 'user', content: prompt.userPrompt }
        ],
        response_format: { type: 'json_object' }
      })
    });
    const data = await response.json();
    return data.choices[0].message.content;
  }
});

// Endpoint: Suggest preset
app.post('/api/suggest-preset', async (req, res) => {
  try {
    const { genre, mood, role, instrumentId, trackId } = req.body;
    const suggestion = await adapter.suggestPreset({
      genre, mood, role, instrumentId, trackId
    });
    res.json({ success: true, suggestion });
  } catch (error) {
    res.status(500).json({ success: false, error: error.message });
  }
});

// Endpoint: Tweak preset
app.post('/api/tweak-preset', async (req, res) => {
  try {
    const { trackId, instructions } = req.body;
    const tweaks = await adapter.tweakPreset({ trackId, instructions });
    res.json({ success: true, tweaks });
  } catch (error) {
    res.status(500).json({ success: false, error: error.message });
  }
});

app.listen(8765, () => {
  console.log('Instrument AI Adapter running on port 8765');
});
```

### Pattern 2: WebSocket Real-time Integration

```javascript
const WebSocket = require('ws');
const { InstrumentAIAdapter } = require('./InstrumentAIAdapter');

const wss = new WebSocket.Server({ port: 8765 });

wss.on('connection', (ws) => {
  console.log('Client connected');

  ws.on('message', async (message) => {
    const request = JSON.parse(message);

    if (request.type === 'suggest_preset') {
      const suggestion = await adapter.suggestPreset(request.params);
      ws.send(JSON.stringify({
        type: 'preset_suggestion',
        requestId: request.id,
        data: suggestion
      }));
    }

    if (request.type === 'tweak_preset') {
      const tweaks = await adapter.tweakPreset(request.params);
      ws.send(JSON.stringify({
        type: 'preset_tweaks',
        requestId: request.id,
        data: tweaks
      }));
    }
  });
});
```

---

## LLM Provider Examples

### OpenAI GPT-4

```javascript
callLLM: async (prompt) => {
  const response = await fetch('https://api.openai.com/v1/chat/completions', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
      'Authorization': `Bearer ${process.env.OPENAI_API_KEY}`
    },
    body: JSON.stringify({
      model: 'gpt-4',
      messages: [
        { role: 'system', content: prompt.systemPrompt },
        { role: 'user', content: prompt.userPrompt }
      ],
      response_format: { type: 'json_object' },
      temperature: 0.7
    })
  });
  const data = await response.json();
  return data.choices[0].message.content;
}
```

### Anthropic Claude

```javascript
callLLM: async (prompt) => {
  const response = await fetch('https://api.anthropic.com/v1/messages', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
      'x-api-key': process.env.ANTHROPIC_API_KEY,
      'anthropic-version': '2023-06-01'
    },
    body: JSON.stringify({
      model: 'claude-3-5-sonnet-20241022',
      max_tokens: 1024,
      system: prompt.systemPrompt,
      messages: [
        { role: 'user', content: prompt.userPrompt }
      ]
    })
  });
  const data = await response.json();

  // Extract JSON from Claude's response (may be wrapped in markdown)
  const text = data.content[0].text;
  const jsonMatch = text.match(/\{[\s\S]*\}/);
  return jsonMatch ? jsonMatch[0] : text;
}
```

### Local LLM (Ollama)

```javascript
callLLM: async (prompt) => {
  const response = await fetch('http://localhost:11434/api/generate', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({
      model: 'llama3',
      prompt: `${prompt.systemPrompt}\n\n${prompt.userPrompt}`,
      format: 'json',
      stream: false
    })
  });
  const data = await response.json();
  return data.response;
}
```

---

## Common Prompt Patterns

### 1. Genre-Based Preset Selection

```
User: "I need a synthwave lead"

Adapter translates to:
{
  genre: "synthwave",
  mood: "nostalgic",
  role: "lead"
}
```

### 2. Mood-Based Tweaking

```
User: "Make it more aggressive"

Adapter interprets as:
- Increase filter resonance
- Shorten attack/release
- Add distortion/saturation
- Increase unison detune
```

### 3. Timbral Adjustments

```
User: "Make it brighter but warmer"

Adapter interprets as:
- Increase filter cutoff (brightness)
- Decrease resonance (warmth)
- Add subtle chorus/unison (warmth)
```

### 4. Dynamic Character

```
User: "Give it more movement"

Adapter interprets as:
- Enable/increase LFO depth
- Add filter modulation
- Increase unison detune
```

---

## Testing & Validation

### Unit Test Example

```javascript
const { InstrumentAIAdapter } = require('./InstrumentAIAdapter');

describe('InstrumentAIAdapter', () => {
  let adapter;
  let mockCommandAPI;
  let mockLLM;

  beforeEach(() => {
    mockCommandAPI = jest.fn();
    mockLLM = jest.fn();

    adapter = new InstrumentAIAdapter({
      sendCommand: mockCommandAPI,
      callLLM: mockLLM
    });
  });

  test('suggestPreset queries presets and calls LLM', async () => {
    mockCommandAPI.mockResolvedValue({
      status: 'ok',
      data: {
        presets: [
          { id: 'preset1', name: 'Test', tags: ['lead'] }
        ]
      }
    });

    mockLLM.mockResolvedValue(JSON.stringify({
      presetId: 'preset1',
      reasoning: 'Perfect match'
    }));

    const result = await adapter.suggestPreset({
      genre: 'test',
      mood: 'test',
      role: 'lead'
    });

    expect(result.presetId).toBe('preset1');
    expect(mockCommandAPI).toHaveBeenCalledWith({
      command: 'list_presets',
      params: { instrumentId: 'zenith_poly_synth' }
    });
  });

  test('tweakPreset applies safety clamping', async () => {
    mockCommandAPI.mockResolvedValue({
      status: 'ok',
      data: {
        parameters: [
          { id: 'filter_cutoff', min: 0, max: 1, value: 0.5 }
        ]
      }
    });

    mockLLM.mockResolvedValue(JSON.stringify({
      parameters: { filter_cutoff: 1.5 },  // Exceeds max
      reasoning: 'Test'
    }));

    const result = await adapter.tweakPreset({
      trackId: 'track_0',
      instructions: 'test'
    });

    // Should be clamped to safety limit
    expect(result.parameters.filter_cutoff).toBeLessThanOrEqual(0.95);
  });
});
```

---

## Troubleshooting

### Issue: LLM Returns Invalid JSON

**Solution**: Add fallback parsing and validation

```javascript
_parsePresetSelection(llmResponse, presets) {
  try {
    // Try direct JSON parse
    const parsed = JSON.parse(llmResponse);
    return this._validatePresetSelection(parsed, presets);
  } catch (error) {
    // Try extracting JSON from markdown code blocks
    const jsonMatch = llmResponse.match(/```json\n([\s\S]*?)\n```/);
    if (jsonMatch) {
      const parsed = JSON.parse(jsonMatch[1]);
      return this._validatePresetSelection(parsed, presets);
    }

    // Fallback: select first preset
    console.error('Failed to parse LLM response, using fallback');
    return {
      presetId: presets[0].id,
      presetName: presets[0].name,
      reasoning: 'Fallback selection'
    };
  }
}
```

### Issue: CommandAPI Connection Fails

**Solution**: Add retry logic and error handling

```javascript
async sendCommand(command, retries = 3) {
  for (let i = 0; i < retries; i++) {
    try {
      const response = await fetch('http://localhost:8080/command', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(command),
        timeout: 5000
      });
      return await response.json();
    } catch (error) {
      console.error(`Command failed (attempt ${i + 1}/${retries}):`, error);
      if (i === retries - 1) throw error;
      await new Promise(r => setTimeout(r, 1000 * (i + 1)));
    }
  }
}
```

### Issue: LLM Suggests Unsafe Parameters

**Solution**: Safety layer catches this automatically

```javascript
// Safety is always applied in _applySafety()
// Logs warnings when values are clamped:
// [InstrumentAIAdapter] Safety clamping filter_cutoff: 1.0 -> 0.95
```

---

## Performance Considerations

### Latency Breakdown

| Operation | Typical Latency | Optimization |
|-----------|----------------|--------------|
| CommandAPI query | 5-20ms | Fast (local) |
| LLM call | 500-2000ms | Cache common patterns |
| Parameter validation | <1ms | Fast (local) |
| CommandAPI execution | 5-20ms | Batch multiple params |

### Optimization Strategies

1. **Preset Caching**: Cache preset lists to avoid repeated queries
2. **LLM Response Caching**: Cache LLM responses for similar prompts
3. **Batch Operations**: Group multiple parameter changes into single CommandAPI call
4. **Async Processing**: Return immediately with "processing" status, update when complete

---

## Next Steps

1. **Deploy**: Integrate adapter into ai-bridge-server
2. **Test**: Run with real CommandAPI and LLM provider
3. **Iterate**: Refine prompts based on actual usage patterns
4. **Extend**: Add support for more instruments (Zenith Sampler, future instruments)
5. **UI Integration**: Build frontend controls in WingmanPanel or separate UI

---

## Additional Resources

- [CommandAPI Reference](/home/user/daw/COMMAND_API_REFERENCE.md)
- [Wingman Integration Plan](/home/user/daw/planning/architecture/WINGMAN_INTEGRATION_PLAN.md)
- [Instrument Command API](/home/user/daw/docs/INSTRUMENT_COMMAND_API.md)
- [AI Preset Design Guide](/home/user/daw/docs/AI-PRESET-DESIGN-GUIDE.md)
