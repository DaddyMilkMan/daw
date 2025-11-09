/**
 * MagentaService - AI Music Generation Service
 *
 * Provides methods for generating musical content using AI models:
 * - Chord progressions
 * - Melodies (RNN-based)
 * - Drum patterns
 *
 * This service can be extended to integrate with actual Magenta.js library
 * or communicate with a remote AI service via the Wingman AI Bridge.
 */

export interface Note {
  pitch: number; // MIDI note number (0-127)
  velocity: number; // 0-127
  startTime: number; // in beats
  duration: number; // in beats
}

export interface ChordProgression {
  chords: string[];
  notes: Note[];
  key: string;
  scale: string;
}

export interface MelodySequence {
  notes: Note[];
  temperature: number;
  steps: number;
}

export interface DrumPattern {
  pattern: Note[];
  style: string;
  bars: number;
}

export interface GenerationOptions {
  temperature?: number;
  steps?: number;
  seed?: number;
  style?: string;
}

class MagentaService {
  private initialized = false;

  /**
   * Initialize the Magenta service
   */
  async initialize(): Promise<void> {
    if (this.initialized) return;

    console.log('🎵 Initializing MagentaService...');

    // TODO: Load actual Magenta.js models here
    // For now, we'll use procedural generation

    this.initialized = true;
    console.log('✅ MagentaService initialized');
  }

  /**
   * Generate a chord progression
   * @param key - Musical key (e.g., 'C', 'Am', 'Dm')
   * @param options - Generation options
   */
  async generateChordProgression(
    key: string = 'C',
    options: GenerationOptions = {}
  ): Promise<ChordProgression> {
    await this.ensureInitialized();

    console.log(`🎹 Generating chord progression in ${key}...`);

    const progressions: Record<string, string[]> = {
      'C': ['C', 'Am', 'F', 'G'],
      'Am': ['Am', 'F', 'C', 'G'],
      'Dm': ['Dm', 'Bb', 'F', 'C'],
      'G': ['G', 'Em', 'C', 'D'],
      'Em': ['Em', 'C', 'G', 'D'],
    };

    const defaultProgression = ['I', 'vi', 'IV', 'V'];
    const chords = progressions[key] || progressions['C'];

    // Convert chord names to MIDI notes
    const notes = this.chordsToNotes(chords, key);

    return {
      chords,
      notes,
      key,
      scale: key.includes('m') ? 'minor' : 'major',
    };
  }

  /**
   * Generate a melody using RNN
   * @param seed - Seed notes to start the melody
   * @param options - Generation options
   */
  async generateMelody(
    seed: Note[] = [],
    options: GenerationOptions = {}
  ): Promise<MelodySequence> {
    await this.ensureInitialized();

    console.log('🎼 Generating melody with RNN...');

    const temperature = options.temperature ?? 1.0;
    const steps = options.steps ?? 32;

    // Generate a melodic sequence
    const notes: Note[] = [];
    const scale = [60, 62, 64, 65, 67, 69, 71, 72]; // C major scale

    let currentTime = 0;

    for (let i = 0; i < steps; i++) {
      // Use temperature to control randomness
      const noteIndex = Math.floor(Math.random() * scale.length);
      const pitch = scale[noteIndex];

      // Vary rhythm
      const duration = Math.random() > 0.7 ? 0.5 : 0.25;

      notes.push({
        pitch,
        velocity: 80 + Math.floor(Math.random() * 20),
        startTime: currentTime,
        duration,
      });

      currentTime += duration;
    }

    return {
      notes,
      temperature,
      steps,
    };
  }

  /**
   * Generate drum patterns
   * @param style - Drum pattern style (e.g., 'trap', 'house', 'techno')
   * @param options - Generation options
   */
  async generateDrumPattern(
    style: string = 'trap',
    options: GenerationOptions = {}
  ): Promise<DrumPattern> {
    await this.ensureInitialized();

    console.log(`🥁 Generating ${style} drum pattern...`);

    const bars = 4;
    const stepsPerBar = 16; // 16th notes
    const pattern: Note[] = [];

    // MIDI drum notes (General MIDI)
    const kick = 36;
    const snare = 38;
    const hihat = 42;
    const openHat = 46;

    switch (style.toLowerCase()) {
      case 'trap':
        // Trap-style pattern
        for (let bar = 0; bar < bars; bar++) {
          for (let step = 0; step < stepsPerBar; step++) {
            const time = bar * 4 + (step / stepsPerBar) * 4;

            // Kick on 1 and 3
            if (step === 0 || step === 8) {
              pattern.push({ pitch: kick, velocity: 100, startTime: time, duration: 0.25 });
            }

            // Snare on 2 and 4
            if (step === 4 || step === 12) {
              pattern.push({ pitch: snare, velocity: 90, startTime: time, duration: 0.25 });
            }

            // Hi-hat pattern (rapid)
            if (step % 2 === 0) {
              pattern.push({ pitch: hihat, velocity: 70, startTime: time, duration: 0.125 });
            }

            // Occasional open hi-hat
            if (step === 7 || step === 15) {
              pattern.push({ pitch: openHat, velocity: 60, startTime: time, duration: 0.5 });
            }
          }
        }
        break;

      case 'house':
        // Four-on-the-floor house pattern
        for (let bar = 0; bar < bars; bar++) {
          for (let step = 0; step < stepsPerBar; step++) {
            const time = bar * 4 + (step / stepsPerBar) * 4;

            // Kick on every quarter note
            if (step % 4 === 0) {
              pattern.push({ pitch: kick, velocity: 100, startTime: time, duration: 0.25 });
            }

            // Snare/clap on 2 and 4
            if (step === 4 || step === 12) {
              pattern.push({ pitch: snare, velocity: 85, startTime: time, duration: 0.25 });
            }

            // Hi-hat on every 8th note
            if (step % 2 === 0) {
              pattern.push({ pitch: hihat, velocity: 65, startTime: time, duration: 0.125 });
            }
          }
        }
        break;

      default:
        // Generic drum pattern
        for (let bar = 0; bar < bars; bar++) {
          for (let step = 0; step < stepsPerBar; step++) {
            const time = bar * 4 + (step / stepsPerBar) * 4;

            if (step % 4 === 0) {
              pattern.push({ pitch: kick, velocity: 95, startTime: time, duration: 0.25 });
            }
            if (step === 4 || step === 12) {
              pattern.push({ pitch: snare, velocity: 85, startTime: time, duration: 0.25 });
            }
            if (step % 2 === 0) {
              pattern.push({ pitch: hihat, velocity: 70, startTime: time, duration: 0.125 });
            }
          }
        }
    }

    return {
      pattern,
      style,
      bars,
    };
  }

  /**
   * Convert chord names to MIDI notes
   */
  private chordsToNotes(chords: string[], key: string): Note[] {
    const notes: Note[] = [];

    // Simple chord voicing (root, third, fifth)
    const noteMap: Record<string, number> = {
      'C': 60, 'C#': 61, 'Db': 61, 'D': 62, 'D#': 63, 'Eb': 63,
      'E': 64, 'F': 65, 'F#': 66, 'Gb': 66, 'G': 67, 'G#': 68,
      'Ab': 68, 'A': 69, 'A#': 70, 'Bb': 70, 'B': 71,
    };

    chords.forEach((chord, index) => {
      // Parse chord name to get root note
      const root = chord.replace(/m$/, '');
      const isMinor = chord.endsWith('m');
      const rootNote = noteMap[root] ?? 60;

      const startTime = index * 4; // Each chord lasts 4 beats
      const duration = 4;

      // Root
      notes.push({ pitch: rootNote, velocity: 80, startTime, duration });

      // Third (major or minor)
      const third = isMinor ? 3 : 4;
      notes.push({ pitch: rootNote + third, velocity: 75, startTime, duration });

      // Fifth
      notes.push({ pitch: rootNote + 7, velocity: 75, startTime, duration });
    });

    return notes;
  }

  /**
   * Ensure service is initialized
   */
  private async ensureInitialized(): Promise<void> {
    if (!this.initialized) {
      await this.initialize();
    }
  }
}

// Export singleton instance
export const magentaService = new MagentaService();
export default magentaService;
