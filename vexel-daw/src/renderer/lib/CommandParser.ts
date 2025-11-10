/**
 * CommandParser - Natural Language Command Parser for Vexel DAW
 *
 * Converts natural language queries into structured DAW commands.
 * Supports commands like:
 * - "add a new track"
 * - "set tempo to 120 BPM"
 * - "generate a 4-bar chord progression in A minor"
 * - "insert a drum loop"
 * - "play", "stop", "record"
 */

// ============================================================================
// Types & Interfaces
// ============================================================================

export type CommandType =
  | 'transport'       // Play, pause, stop, record
  | 'tempo'          // Set tempo
  | 'track'          // Add, remove, rename tracks
  | 'generate'       // Generate MIDI, chords, melodies
  | 'insert'         // Insert loops, samples
  | 'mix'            // Mixing operations
  | 'help'           // Help and info
  | 'unknown';       // Unrecognized command

export type TransportAction = 'play' | 'pause' | 'stop' | 'record';
export type TrackType = 'midi' | 'audio' | 'instrument';
export type GenerationType = 'drums' | 'bass' | 'chords' | 'melody' | 'progression';

export interface ParsedCommand {
  type: CommandType;
  action?: string;
  parameters: Record<string, any>;
  confidence: number; // 0-1, how confident we are in the parse
  originalInput: string;
}

export interface CommandResponse {
  success: boolean;
  message: string;
  parsedCommand?: ParsedCommand;
  dawActions?: DAWAction[];
  suggestions?: string[];
  actions?: Array<{ label: string; onClick: () => void }>;
}

export interface DAWAction {
  type: string;
  [key: string]: any;
}

// ============================================================================
// Command Patterns
// ============================================================================

interface CommandPattern {
  regex: RegExp;
  type: CommandType;
  action?: string;
  extractor: (match: RegExpMatchArray, input: string) => Record<string, any>;
  confidence?: number;
}

// Musical key detection
const MUSICAL_KEYS = [
  'C', 'C#', 'Db', 'D', 'D#', 'Eb', 'E', 'F', 'F#', 'Gb', 'G', 'G#', 'Ab', 'A', 'A#', 'Bb', 'B'
];

const KEY_MODIFIERS = ['major', 'minor', 'maj', 'min', 'm'];

// Command patterns ordered by specificity (most specific first)
const COMMAND_PATTERNS: CommandPattern[] = [
  // ========== Transport Commands ==========
  {
    regex: /\b(play|start|resume)\b/i,
    type: 'transport',
    action: 'play',
    extractor: () => ({}),
    confidence: 0.95,
  },
  {
    regex: /\b(pause)\b/i,
    type: 'transport',
    action: 'pause',
    extractor: () => ({}),
    confidence: 0.95,
  },
  {
    regex: /\b(stop|halt)\b/i,
    type: 'transport',
    action: 'stop',
    extractor: () => ({}),
    confidence: 0.95,
  },
  {
    regex: /\b(record|rec|arm)\b/i,
    type: 'transport',
    action: 'record',
    extractor: () => ({}),
    confidence: 0.9,
  },

  // ========== Tempo Commands ==========
  {
    regex: /(?:set|change|adjust|make)?\s*(?:the)?\s*tempo\s*(?:to|at|=)?\s*(\d+)\s*(?:bpm)?/i,
    type: 'tempo',
    action: 'set',
    extractor: (match) => ({
      tempo: parseInt(match[1]),
    }),
    confidence: 0.95,
  },
  {
    regex: /(\d+)\s*bpm/i,
    type: 'tempo',
    action: 'set',
    extractor: (match) => ({
      tempo: parseInt(match[1]),
    }),
    confidence: 0.85,
  },

  // ========== Track Commands ==========
  {
    regex: /(?:add|create|new|insert)\s+(?:a|an)?\s*(?:new)?\s*(midi|audio|instrument)?\s*track(?:\s+(?:called|named)\s+["']?([^"']+)["']?)?/i,
    type: 'track',
    action: 'create',
    extractor: (match) => ({
      trackType: (match[1]?.toLowerCase() as TrackType) || 'midi',
      name: match[2] || undefined,
    }),
    confidence: 0.9,
  },
  {
    regex: /(?:delete|remove|clear)\s+track\s*(\d+|all)?/i,
    type: 'track',
    action: 'delete',
    extractor: (match) => ({
      trackId: match[1] === 'all' ? 'all' : match[1] ? parseInt(match[1]) : undefined,
    }),
    confidence: 0.9,
  },

  // ========== Generate Commands - Chords ==========
  {
    regex: /(?:generate|create|make|add)\s+(?:a|an)?\s*(?:(\d+)[\s-]*bar)?\s*chord\s*(?:progression|sequence|pattern)?\s*(?:in|for|using)?\s*([A-G][#b]?)\s*(major|minor|maj|min|m)?/i,
    type: 'generate',
    action: 'chords',
    extractor: (match) => ({
      bars: match[1] ? parseInt(match[1]) : 4,
      key: match[2],
      mode: match[3]?.toLowerCase().startsWith('maj') ? 'major' : 'minor',
    }),
    confidence: 0.95,
  },
  {
    regex: /(?:suggest|give|show)\s+(?:me|some)?\s*chords?\s*(?:in|for)?\s*([A-G][#b]?)\s*(major|minor|maj|min|m)?/i,
    type: 'generate',
    action: 'chords',
    extractor: (match) => ({
      key: match[1],
      mode: match[2]?.toLowerCase().startsWith('maj') ? 'major' : 'minor',
      suggest: true,
    }),
    confidence: 0.9,
  },

  // ========== Generate Commands - Drums ==========
  {
    regex: /(?:generate|create|make|add)\s+(?:a|an)?\s*(?:(\d+)[\s-]*bar)?\s*(trap|house|techno|hip[\s-]?hop|rock|jazz)?\s*(?:drum|beat|drums|rhythm)\s*(?:pattern|loop|sequence)?/i,
    type: 'generate',
    action: 'drums',
    extractor: (match) => ({
      bars: match[1] ? parseInt(match[1]) : 4,
      style: match[2]?.toLowerCase() || 'trap',
    }),
    confidence: 0.9,
  },

  // ========== Generate Commands - Bass ==========
  {
    regex: /(?:generate|create|make|add)\s+(?:a|an)?\s*(?:(\d+)[\s-]*bar)?\s*bass\s*(?:line|pattern|sequence)?\s*(?:in|for)?\s*([A-G][#b]?)\s*(major|minor|maj|min|m)?/i,
    type: 'generate',
    action: 'bass',
    extractor: (match) => ({
      bars: match[1] ? parseInt(match[1]) : 4,
      key: match[2],
      mode: match[3]?.toLowerCase().startsWith('maj') ? 'major' : 'minor',
    }),
    confidence: 0.9,
  },

  // ========== Generate Commands - Melody ==========
  {
    regex: /(?:generate|create|make|add)\s+(?:a|an)?\s*(?:(\d+)[\s-]*bar)?\s*melody\s*(?:in|for)?\s*([A-G][#b]?)\s*(major|minor|maj|min|m)?/i,
    type: 'generate',
    action: 'melody',
    extractor: (match) => ({
      bars: match[1] ? parseInt(match[1]) : 4,
      key: match[2],
      mode: match[3]?.toLowerCase().startsWith('maj') ? 'major' : 'minor',
    }),
    confidence: 0.9,
  },

  // ========== Insert Commands ==========
  {
    regex: /(?:insert|add|load)\s+(?:a|an)?\s*(drum|bass|synth|vocal|fx)?\s*(?:loop|sample)/i,
    type: 'insert',
    action: 'loop',
    extractor: (match) => ({
      category: match[1]?.toLowerCase() || 'drum',
    }),
    confidence: 0.85,
  },

  // ========== Mix Commands ==========
  {
    regex: /(?:auto[\s-]?mix|mix|balance)\s*(?:this|the)?\s*(?:track|project)?/i,
    type: 'mix',
    action: 'auto',
    extractor: () => ({}),
    confidence: 0.85,
  },

  // ========== Help Commands ==========
  {
    regex: /\b(help|what can you do|commands|capabilities)\b/i,
    type: 'help',
    extractor: () => ({}),
    confidence: 1.0,
  },
];

// ============================================================================
// Main Parser Class
// ============================================================================

export class CommandParser {
  /**
   * Parse a natural language input into a structured command
   */
  static parse(input: string): ParsedCommand {
    const trimmedInput = input.trim();

    // Try each pattern in order
    for (const pattern of COMMAND_PATTERNS) {
      const match = trimmedInput.match(pattern.regex);
      if (match) {
        const parameters = pattern.extractor(match, trimmedInput);
        return {
          type: pattern.type,
          action: pattern.action,
          parameters,
          confidence: pattern.confidence || 0.8,
          originalInput: trimmedInput,
        };
      }
    }

    // No pattern matched
    return {
      type: 'unknown',
      parameters: {},
      confidence: 0,
      originalInput: trimmedInput,
    };
  }

  /**
   * Execute a parsed command and return a response
   */
  static execute(parsedCommand: ParsedCommand): CommandResponse {
    switch (parsedCommand.type) {
      case 'transport':
        return this.handleTransport(parsedCommand);

      case 'tempo':
        return this.handleTempo(parsedCommand);

      case 'track':
        return this.handleTrack(parsedCommand);

      case 'generate':
        return this.handleGenerate(parsedCommand);

      case 'insert':
        return this.handleInsert(parsedCommand);

      case 'mix':
        return this.handleMix(parsedCommand);

      case 'help':
        return this.handleHelp(parsedCommand);

      case 'unknown':
        return this.handleUnknown(parsedCommand);

      default:
        return {
          success: false,
          message: "I didn't understand that command.",
          suggestions: this.getSuggestions(parsedCommand.originalInput),
        };
    }
  }

  // ========== Command Handlers ==========

  private static handleTransport(cmd: ParsedCommand): CommandResponse {
    const actionMap: Record<string, string> = {
      play: '▶️ Starting playback',
      pause: '⏸️ Paused',
      stop: '⏹️ Stopped',
      record: '⏺️ Recording armed (UI only)',
    };

    const message = actionMap[cmd.action || ''] || 'Transport action executed';

    return {
      success: true,
      message,
      parsedCommand: cmd,
      dawActions: [{ type: `transport${cmd.action?.charAt(0).toUpperCase()}${cmd.action?.slice(1)}` }],
    };
  }

  private static handleTempo(cmd: ParsedCommand): CommandResponse {
    const { tempo } = cmd.parameters;

    // Validate tempo range
    if (tempo < 20 || tempo > 999) {
      return {
        success: false,
        message: `Tempo must be between 20 and 999 BPM. You entered ${tempo} BPM.`,
        parsedCommand: cmd,
      };
    }

    return {
      success: true,
      message: `✓ Tempo set to ${tempo} BPM`,
      parsedCommand: cmd,
      dawActions: [{ type: 'setTempo', value: tempo }],
    };
  }

  private static handleTrack(cmd: ParsedCommand): CommandResponse {
    const { action } = cmd;

    if (action === 'create') {
      const { trackType, name } = cmd.parameters;
      const trackName = name || `New ${trackType?.toUpperCase() || 'MIDI'} Track`;
      const type = trackType || 'midi';

      return {
        success: true,
        message: `✓ Created ${type.toUpperCase()} track: "${trackName}"`,
        parsedCommand: cmd,
        dawActions: [{ type: 'createTrack', name: trackName, trackType: type }],
        actions: [
          { label: 'Open Piano Roll', onClick: () => console.log('Open piano roll') },
        ],
      };
    }

    if (action === 'delete') {
      const { trackId } = cmd.parameters;
      return {
        success: true,
        message: trackId === 'all'
          ? '✓ All tracks deleted'
          : `✓ Track ${trackId || 'selected'} deleted`,
        parsedCommand: cmd,
        dawActions: [{ type: 'deleteTrack', trackId }],
      };
    }

    return {
      success: false,
      message: 'Unknown track action',
      parsedCommand: cmd,
    };
  }

  private static handleGenerate(cmd: ParsedCommand): CommandResponse {
    const { action, parameters } = cmd;

    if (action === 'chords') {
      const { bars, key, mode, suggest } = parameters;

      if (suggest) {
        // Just suggest chords, don't generate
        const chordProgressions = this.getChordProgressions(key, mode);
        return {
          success: true,
          message: `Chord progressions in ${key} ${mode}:\n\n${chordProgressions}\n\nWould you like me to generate one?`,
          parsedCommand: cmd,
          actions: [
            { label: 'Generate Progression', onClick: () => console.log('Generate') },
            { label: 'Try Different Key', onClick: () => console.log('Different key') },
          ],
        };
      }

      const trackName = `Chords - ${key} ${mode}`;
      return {
        success: true,
        message: `✓ Generated ${bars}-bar chord progression in ${key} ${mode}\n✓ Created track: "${trackName}"\n\nThe progression is ready to edit!`,
        parsedCommand: cmd,
        dawActions: [
          { type: 'createTrack', name: trackName, trackType: 'midi' },
          { type: 'generateChords', bars, key, mode },
        ],
        actions: [
          { label: 'View in Piano Roll', onClick: () => console.log('Open piano roll') },
          { label: 'Adjust Voicing', onClick: () => console.log('Adjust voicing') },
        ],
      };
    }

    if (action === 'drums') {
      const { bars, style } = parameters;
      const trackName = `${style.charAt(0).toUpperCase()}${style.slice(1)} Drums`;

      return {
        success: true,
        message: `✓ Generated ${bars}-bar ${style} drum pattern\n✓ Created track: "${trackName}"\n✓ Pattern includes kick, snare, hi-hats, and percussion\n\nReady to edit!`,
        parsedCommand: cmd,
        dawActions: [
          { type: 'createTrack', name: trackName, trackType: 'midi' },
          { type: 'generateDrums', bars, style },
        ],
        actions: [
          { label: 'View Pattern', onClick: () => console.log('View pattern') },
          { label: 'Randomize', onClick: () => console.log('Randomize') },
        ],
      };
    }

    if (action === 'bass') {
      const { bars, key, mode } = parameters;
      const trackName = `Bass - ${key} ${mode}`;

      return {
        success: true,
        message: `✓ Generated ${bars}-bar bass line in ${key} ${mode}\n✓ Created track: "${trackName}"\n✓ Root notes with octave variations\n\nPlaced on new track!`,
        parsedCommand: cmd,
        dawActions: [
          { type: 'createTrack', name: trackName, trackType: 'midi' },
          { type: 'generateBass', bars, key, mode },
        ],
        actions: [
          { label: 'Edit Bass Line', onClick: () => console.log('Edit') },
        ],
      };
    }

    if (action === 'melody') {
      const { bars, key, mode } = parameters;
      const trackName = `Melody - ${key} ${mode}`;

      return {
        success: true,
        message: `✓ Generated ${bars}-bar melody in ${key} ${mode}\n✓ Created track: "${trackName}"\n✓ Uses ${mode === 'major' ? 'major scale' : 'natural minor scale'}\n\nReady to edit!`,
        parsedCommand: cmd,
        dawActions: [
          { type: 'createTrack', name: trackName, trackType: 'midi' },
          { type: 'generateMelody', bars, key, mode },
        ],
        actions: [
          { label: 'View in Piano Roll', onClick: () => console.log('Open piano roll') },
        ],
      };
    }

    return {
      success: false,
      message: 'Unknown generation type',
      parsedCommand: cmd,
    };
  }

  private static handleInsert(cmd: ParsedCommand): CommandResponse {
    const { category } = cmd.parameters;

    return {
      success: true,
      message: `✓ Browser opened to ${category} loops\n\nBrowse and drag loops into your arrangement!`,
      parsedCommand: cmd,
      dawActions: [{ type: 'openBrowser', category }],
      actions: [
        { label: 'Browse Loops', onClick: () => console.log('Browse') },
      ],
    };
  }

  private static handleMix(cmd: ParsedCommand): CommandResponse {
    return {
      success: true,
      message: `Analyzing your mix...\n\n✓ Balanced levels for clarity\n✓ Applied EQ (cut mud @ 250Hz, boost presence @ 3kHz)\n✓ Compression for consistency\n✓ Stereo panning for width\n✓ Headroom at -6dB\n\nYour mix is ready! Check the mixer for details.`,
      parsedCommand: cmd,
      dawActions: [{ type: 'autoMix' }],
      actions: [
        { label: 'View Mixer', onClick: () => console.log('View mixer') },
        { label: 'Undo Mix', onClick: () => console.log('Undo') },
      ],
    };
  }

  private static handleHelp(cmd: ParsedCommand): CommandResponse {
    const helpMessage = `I can help you with:

🎵 **Transport Controls**
• "play", "pause", "stop", "record"

⚡ **Tempo**
• "set tempo to 120 BPM"
• "change tempo to 140"

🎹 **Tracks**
• "add a new MIDI track"
• "create an audio track called Vocals"
• "delete track 2"

🎼 **Generate MIDI**
• "generate a 4-bar chord progression in A minor"
• "create an 8-bar drum pattern"
• "make a bass line in C major"
• "generate a melody in E minor"

🥁 **Insert Loops**
• "insert a drum loop"
• "add a bass sample"

🎚️ **Mixing**
• "auto-mix this track"
• "balance the mix"

Try any command or ask me a question!`;

    return {
      success: true,
      message: helpMessage,
      parsedCommand: cmd,
    };
  }

  private static handleUnknown(cmd: ParsedCommand): CommandResponse {
    const suggestions = this.getSuggestions(cmd.originalInput);

    return {
      success: false,
      message: `I didn't quite understand that. Did you mean:\n\n${suggestions.join('\n')}`,
      parsedCommand: cmd,
      suggestions,
      actions: [
        { label: 'Show Help', onClick: () => console.log('help') },
      ],
    };
  }

  // ========== Helper Methods ==========

  private static getSuggestions(input: string): string[] {
    const lowerInput = input.toLowerCase();
    const suggestions: string[] = [];

    // Context-aware suggestions
    if (lowerInput.includes('track')) {
      suggestions.push('• "add a new MIDI track"');
      suggestions.push('• "create an audio track"');
    }

    if (lowerInput.includes('tempo') || lowerInput.includes('bpm') || /\d+/.test(lowerInput)) {
      suggestions.push('• "set tempo to 120 BPM"');
    }

    if (lowerInput.includes('chord') || lowerInput.includes('progression')) {
      suggestions.push('• "generate a 4-bar chord progression in A minor"');
    }

    if (lowerInput.includes('drum') || lowerInput.includes('beat')) {
      suggestions.push('• "create a trap beat"');
      suggestions.push('• "generate an 8-bar drum pattern"');
    }

    if (lowerInput.includes('bass')) {
      suggestions.push('• "add a bass line in C minor"');
    }

    // Default suggestions if none found
    if (suggestions.length === 0) {
      suggestions.push('• "add a new track"');
      suggestions.push('• "set tempo to 120"');
      suggestions.push('• "generate chords in A minor"');
      suggestions.push('• Type "help" for all commands');
    }

    return suggestions;
  }

  private static getChordProgressions(key: string, mode: string): string {
    if (mode === 'minor') {
      return `**Popular progressions:**

1️⃣ ${key}m - ${this.shiftKey(key, 3)} - ${this.shiftKey(key, 8)} - ${this.shiftKey(key, 10)}
   (i - III - VI - VII) - Dark, emotional

2️⃣ ${key}m - ${this.shiftKey(key, 10)} - ${this.shiftKey(key, 3)} - ${this.shiftKey(key, 8)}
   (i - VII - III - VI) - Classic, powerful

3️⃣ ${key}m - ${this.shiftKey(key, 8)} - ${this.shiftKey(key, 3)} - ${this.shiftKey(key, 10)}
   (i - VI - III - VII) - Lo-fi, chill`;
    } else {
      return `**Popular progressions:**

1️⃣ ${key} - ${this.shiftKey(key, 9)} - ${this.shiftKey(key, 5)} - ${this.shiftKey(key, 7)}
   (I - vi - IV - V) - Classic pop

2️⃣ ${key} - ${this.shiftKey(key, 5)} - ${this.shiftKey(key, 9)} - ${this.shiftKey(key, 7)}
   (I - IV - vi - V) - Uplifting

3️⃣ ${key} - ${this.shiftKey(key, 7)} - ${this.shiftKey(key, 9)} - ${this.shiftKey(key, 5)}
   (I - V - vi - IV) - Most popular!`;
    }
  }

  private static shiftKey(key: string, semitones: number): string {
    const keys = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];
    const flatKeys = ['C', 'Db', 'D', 'Eb', 'E', 'F', 'Gb', 'G', 'Ab', 'A', 'Bb', 'B'];

    // Normalize key
    let normalizedKey = key.replace('b', 'b');
    let useFlats = key.includes('b');

    // Find index
    let index = useFlats
      ? flatKeys.indexOf(normalizedKey.replace('b', 'b'))
      : keys.indexOf(normalizedKey.replace('#', '#'));

    if (index === -1) {
      // Try the other notation
      index = useFlats
        ? keys.indexOf(normalizedKey)
        : flatKeys.indexOf(normalizedKey);
      useFlats = !useFlats;
    }

    if (index === -1) return key; // Fallback

    // Shift
    const newIndex = (index + semitones) % 12;
    return useFlats ? flatKeys[newIndex] : keys[newIndex];
  }

  /**
   * Convenience method to parse and execute in one call
   */
  static parseAndExecute(input: string): CommandResponse {
    const parsed = this.parse(input);
    return this.execute(parsed);
  }
}

export default CommandParser;
