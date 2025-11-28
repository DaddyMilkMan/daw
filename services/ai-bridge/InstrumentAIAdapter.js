/**
 * InstrumentAIAdapter - AI-powered instrument control for Zenith DAW
 *
 * This adapter sits on the host/companion side and provides natural language
 * control over instruments via CommandAPI. It translates high-level intents
 * (genre, mood, role) into concrete preset selections and parameter tweaks.
 *
 * Design principles:
 * - Stateless: relies on CommandAPI as source of truth
 * - Safe: clamps all parameter values, avoids extreme settings
 * - LLM-agnostic: works with OpenAI, Anthropic, or any JSON-compatible LLM
 */

class InstrumentAIAdapter {
  /**
   * @param {Object} options Configuration options
   * @param {Function} options.sendCommand - Function to send CommandAPI requests
   * @param {Function} options.callLLM - Function to call LLM with prompt
   * @param {Object} options.safety - Safety limits for parameters
   */
  constructor(options = {}) {
    this.sendCommand = options.sendCommand || this._defaultSendCommand;
    this.callLLM = options.callLLM || this._defaultCallLLM;

    // Safety limits to prevent extreme/dangerous settings
    this.safety = {
      maxFilterCutoff: 0.95,      // Prevent ear-piercing highs
      maxResonance: 0.85,          // Prevent runaway resonance
      maxGain: 0.9,                // Prevent extreme volume
      minAttack: 0.001,            // Prevent clicks (unless intentional)
      maxRelease: 5.0,             // Prevent infinite notes
      ...options.safety
    };

    // Instrument knowledge base for LLM context
    this.instrumentKnowledge = {
      zenith_poly_synth: {
        id: 'zenith_poly_synth',
        name: 'Zenith Poly Synth',
        description: 'Versatile polyphonic synthesizer with dual oscillators, analog-style filter, and envelope modulation',
        categories: ['Synth', 'Lead', 'Pad', 'Bass'],
        keyParameters: {
          filter_cutoff: 'Controls brightness (0=dark, 1=bright)',
          filter_resonance: 'Adds character and emphasis at cutoff frequency',
          attack: 'Time to reach full volume (0=instant, 1=slow)',
          decay: 'Time to fall to sustain level',
          sustain: 'Held volume level (0=silent, 1=full)',
          release: 'Time to fade after note off',
          unison_detune: 'Detuning for chorus/supersaw effect (0=tight, 1=wide)',
          osc_mix: 'Balance between oscillators (0=osc1, 1=osc2)',
          lfo_rate: 'Modulation speed',
          lfo_depth: 'Modulation intensity'
        },
        commonTags: ['pad', 'lead', 'bass', 'pluck', 'warm', 'bright', 'dark', 'aggressive', 'smooth']
      },
      zenith_sampler: {
        id: 'zenith_sampler',
        name: 'Zenith Sampler',
        description: 'Sample playback engine with pitch, filter, and envelope control',
        categories: ['Sampler', 'Drums', 'Percussion', 'FX'],
        keyParameters: {
          filter_cutoff: 'Sample brightness (0=muffled, 1=full spectrum)',
          filter_resonance: 'Tonal emphasis',
          attack: 'Fade-in time',
          release: 'Fade-out time',
          pitch: 'Pitch shift in semitones',
          sample_start: 'Playback start position (0=beginning)',
          loop_enabled: 'Enable sample looping'
        },
        commonTags: ['808', 'kick', 'snare', 'hihat', 'percussion', 'one-shot', 'loop', 'texture']
      }
    };
  }

  /**
   * Suggest a preset based on high-level musical intent
   *
   * @param {Object} request
   * @param {string} request.genre - Musical genre (e.g., "synthwave", "techno", "ambient")
   * @param {string} request.mood - Emotional character (e.g., "dark", "uplifting", "aggressive")
   * @param {string} request.role - Instrument role (e.g., "lead", "pad", "bass", "percussion")
   * @param {string} request.trackId - Target track ID (optional, for context)
   * @param {string} request.instrumentId - Target instrument (optional, defaults to zenith_poly_synth)
   * @returns {Promise<Object>} { presetId, presetName, reasoning }
   */
  async suggestPreset({ genre, mood, role, trackId, instrumentId = 'zenith_poly_synth' }) {
    console.log(`[InstrumentAIAdapter] Suggesting preset: genre=${genre}, mood=${mood}, role=${role}`);

    // Step 1: Get available presets from CommandAPI
    const presetsResponse = await this.sendCommand({
      command: 'list_presets',
      params: { instrumentId }
    });

    if (presetsResponse.status !== 'ok' || !presetsResponse.data.presets.length) {
      throw new Error(`No presets found for instrument: ${instrumentId}`);
    }

    const presets = presetsResponse.data.presets;
    console.log(`[InstrumentAIAdapter] Found ${presets.length} presets`);

    // Step 2: Build LLM prompt with instrument knowledge and preset options
    const instrument = this.instrumentKnowledge[instrumentId] || { name: instrumentId, description: 'Unknown instrument' };

    const prompt = this._buildPresetSelectionPrompt({
      instrument,
      presets,
      genre,
      mood,
      role
    });

    // Step 3: Call LLM to select best preset
    const llmResponse = await this.callLLM(prompt);

    // Step 4: Parse LLM response and validate
    const selection = this._parsePresetSelection(llmResponse, presets);

    console.log(`[InstrumentAIAdapter] Selected preset: ${selection.presetName} (${selection.presetId})`);
    console.log(`[InstrumentAIAdapter] Reasoning: ${selection.reasoning}`);

    return selection;
  }

  /**
   * Tweak preset parameters using natural language instructions
   *
   * @param {Object} request
   * @param {string} request.trackId - Target track ID
   * @param {string} request.instructions - Natural language tweaking instructions
   * @returns {Promise<Object>} { parameters, reasoning, commands }
   */
  async tweakPreset({ trackId, instructions }) {
    console.log(`[InstrumentAIAdapter] Tweaking preset on track ${trackId}: "${instructions}"`);

    // Step 1: Get current instrument parameters
    const paramsResponse = await this.sendCommand({
      command: 'get_instrument_parameters',
      params: { trackId }
    });

    if (paramsResponse.status !== 'ok') {
      throw new Error(`Failed to get parameters for track ${trackId}: ${paramsResponse.error}`);
    }

    const currentParams = paramsResponse.data.parameters;
    console.log(`[InstrumentAIAdapter] Current parameters:`, currentParams.length);

    // Step 2: Build LLM prompt with current state and instructions
    const prompt = this._buildParameterTweakPrompt({
      currentParams,
      instructions,
      trackId
    });

    // Step 3: Call LLM to generate parameter changes
    const llmResponse = await this.callLLM(prompt);

    // Step 4: Parse and validate parameter changes
    const tweaks = this._parseParameterTweaks(llmResponse, currentParams);

    // Step 5: Apply safety clamping
    const safeParams = this._applySafety(tweaks.parameters);

    console.log(`[InstrumentAIAdapter] Parameter changes:`, safeParams);
    console.log(`[InstrumentAIAdapter] Reasoning: ${tweaks.reasoning}`);

    // Step 6: Generate CommandAPI command
    const command = {
      command: 'set_instrument_parameters',
      params: {
        trackId,
        params: safeParams
      }
    };

    return {
      parameters: safeParams,
      reasoning: tweaks.reasoning,
      commands: [command]
    };
  }

  /**
   * Execute a preset suggestion (loads the preset on the track)
   */
  async executeSuggestPreset({ presetId, instrumentId, trackId }) {
    const command = {
      command: 'load_preset',
      params: {
        trackId,
        presetId,
        instrumentId
      }
    };

    return await this.sendCommand(command);
  }

  /**
   * Execute parameter tweaks (applies the parameter changes)
   */
  async executeTweakPreset({ trackId, parameters }) {
    const command = {
      command: 'set_instrument_parameters',
      params: {
        trackId,
        params: parameters
      }
    };

    return await this.sendCommand(command);
  }

  // ============================================================================
  // PRIVATE HELPER METHODS
  // ============================================================================

  _buildPresetSelectionPrompt({ instrument, presets, genre, mood, role }) {
    return {
      systemPrompt: `You are an expert sound designer for the ${instrument.name}. Your task is to select the most appropriate preset based on musical context.

Instrument: ${instrument.name}
Description: ${instrument.description}

Available presets (${presets.length} total):
${presets.map(p => `- ${p.id}: "${p.name}" [${p.tags.join(', ')}]`).join('\n')}

Instructions:
1. Analyze the user's requirements (genre, mood, role)
2. Match preset tags and names to the requirements
3. Select the single best preset
4. Explain your reasoning

Respond with valid JSON only:
{
  "presetId": "exact_preset_id_from_list",
  "reasoning": "brief explanation of why this preset fits"
}`,
      userPrompt: `Select a preset for:
- Genre: ${genre}
- Mood: ${mood}
- Role: ${role}

Which preset best matches these requirements?`
    };
  }

  _buildParameterTweakPrompt({ currentParams, instructions, trackId }) {
    const paramInfo = currentParams.map(p =>
      `- ${p.id}: ${p.name} = ${p.value.toFixed(3)} [range: ${p.min}-${p.max}, default: ${p.default}]`
    ).join('\n');

    return {
      systemPrompt: `You are an expert sound designer. Your task is to translate natural language instructions into precise parameter adjustments.

Current instrument parameters:
${paramInfo}

Parameter knowledge:
- filter_cutoff: Controls brightness (lower = darker/warmer, higher = brighter/harsher)
- filter_resonance: Adds emphasis/character at cutoff frequency (higher = more pronounced)
- attack: Note onset time (lower = instant/punchy, higher = slow fade-in)
- decay: Time to fall to sustain level after attack
- sustain: Held note volume level
- release: Note fade-out time after key release
- unison_detune: Detuning amount for chorus/width (higher = wider/more detuned)
- osc_mix: Oscillator balance

Instructions:
1. Parse the natural language instructions carefully
2. Determine which parameters need adjustment
3. Calculate appropriate new values (normalized 0-1)
4. Consider parameter interactions (e.g., bright sound = higher cutoff + moderate resonance)
5. Make musical, not extreme changes

Respond with valid JSON only:
{
  "parameters": {
    "param_id": 0.75,
    "another_param": 0.3
  },
  "reasoning": "brief explanation of changes"
}`,
      userPrompt: `Current track: ${trackId}

User instructions: "${instructions}"

What parameter changes should be made?`
    };
  }

  _parsePresetSelection(llmResponse, presets) {
    try {
      const parsed = typeof llmResponse === 'string' ? JSON.parse(llmResponse) : llmResponse;

      // Validate preset ID exists
      const preset = presets.find(p => p.id === parsed.presetId);
      if (!preset) {
        throw new Error(`LLM selected invalid preset ID: ${parsed.presetId}`);
      }

      return {
        presetId: parsed.presetId,
        presetName: preset.name,
        reasoning: parsed.reasoning || 'No reasoning provided'
      };
    } catch (error) {
      console.error('[InstrumentAIAdapter] Failed to parse preset selection:', error);
      // Fallback: select first preset
      return {
        presetId: presets[0].id,
        presetName: presets[0].name,
        reasoning: 'Fallback selection due to parsing error'
      };
    }
  }

  _parseParameterTweaks(llmResponse, currentParams) {
    try {
      const parsed = typeof llmResponse === 'string' ? JSON.parse(llmResponse) : llmResponse;

      // Validate all parameter IDs exist
      const validParamIds = new Set(currentParams.map(p => p.id));
      const parameters = {};

      for (const [paramId, value] of Object.entries(parsed.parameters || {})) {
        if (validParamIds.has(paramId)) {
          parameters[paramId] = parseFloat(value);
        } else {
          console.warn(`[InstrumentAIAdapter] Ignoring invalid parameter: ${paramId}`);
        }
      }

      return {
        parameters,
        reasoning: parsed.reasoning || 'No reasoning provided'
      };
    } catch (error) {
      console.error('[InstrumentAIAdapter] Failed to parse parameter tweaks:', error);
      return {
        parameters: {},
        reasoning: 'Failed to parse LLM response'
      };
    }
  }

  _applySafety(parameters) {
    const safe = {};

    for (const [paramId, value] of Object.entries(parameters)) {
      let safeValue = value;

      // Apply parameter-specific safety limits
      switch (paramId) {
        case 'filter_cutoff':
          safeValue = Math.min(value, this.safety.maxFilterCutoff);
          break;
        case 'filter_resonance':
          safeValue = Math.min(value, this.safety.maxResonance);
          break;
        case 'gain':
        case 'volume':
          safeValue = Math.min(value, this.safety.maxGain);
          break;
        case 'attack':
          safeValue = Math.max(value, this.safety.minAttack);
          break;
        case 'release':
          safeValue = Math.min(value, this.safety.maxRelease);
          break;
      }

      // Always clamp to [0, 1] for normalized parameters
      safeValue = Math.max(0, Math.min(1, safeValue));

      safe[paramId] = safeValue;

      if (safeValue !== value) {
        console.warn(`[InstrumentAIAdapter] Safety clamping ${paramId}: ${value} -> ${safeValue}`);
      }
    }

    return safe;
  }

  // Default implementations (to be overridden by constructor options)

  async _defaultSendCommand(command) {
    throw new Error('sendCommand function not provided to InstrumentAIAdapter');
  }

  async _defaultCallLLM(prompt) {
    throw new Error('callLLM function not provided to InstrumentAIAdapter');
  }
}

// ============================================================================
// EXAMPLE USAGE & INTEGRATION PATTERNS
// ============================================================================

/**
 * Example: Integration with HTTP-based CommandAPI
 */
async function exampleHTTPIntegration() {
  const adapter = new InstrumentAIAdapter({
    // Send commands to local DAW via HTTP
    sendCommand: async (command) => {
      const response = await fetch('http://localhost:8080/command', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(command)
      });
      return await response.json();
    },

    // Call LLM (example: OpenAI GPT-4)
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
          response_format: { type: 'json_object' }
        })
      });

      const data = await response.json();
      return data.choices[0].message.content;
    }
  });

  // Example 1: Suggest preset
  const suggestion = await adapter.suggestPreset({
    genre: 'synthwave',
    mood: 'nostalgic',
    role: 'lead',
    instrumentId: 'zenith_poly_synth',
    trackId: 'track_0'
  });
  console.log('Suggested preset:', suggestion);

  // Example 2: Tweak preset
  const tweaks = await adapter.tweakPreset({
    trackId: 'track_0',
    instructions: 'make it darker and more detuned, add a touch of reverb'
  });
  console.log('Parameter tweaks:', tweaks);
}

/**
 * Example: Integration with Anthropic Claude
 */
async function exampleAnthropicIntegration() {
  const adapter = new InstrumentAIAdapter({
    sendCommand: async (command) => {
      // Same as above
    },

    // Call Anthropic Claude API
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
      return data.content[0].text;
    }
  });

  return adapter;
}

module.exports = {
  InstrumentAIAdapter,
  exampleHTTPIntegration,
  exampleAnthropicIntegration
};
