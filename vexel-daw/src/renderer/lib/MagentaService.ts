/**
 * MagentaService - AI Music Generation Service
 *
 * Provides methods for generating musical content using AI models:
 * - Chord progressions
 * - Melodies (MelodyRNN, MusicVAE)
 * - Drum patterns (DrumsRNN, GrooVAE)
 *
 * Integrates with @magenta/music for TensorFlow.js-based generation.
 * Dynamically loads the library to avoid hard dependency.
 * Falls back to procedural generation if library not available.
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

// Type definitions for Magenta.js
type MagentaMusic = any;
type MusicVAE = any;
type MelodyRNN = any;
type DrumsRNN = any;

class MagentaService {
  private initialized = false;
  private magentaMusic: MagentaMusic | null = null;
  private musicVAE: MusicVAE | null = null;
  private melodyRNN: MelodyRNN | null = null;
  private drumsRNN: DrumsRNN | null = null;
  private useMagentaModels = false;

  /**
   * Lazy-load @magenta/music library
   */
  private async loadMagentaMusic(): Promise<boolean> {
    if (this.magentaMusic) return true;

    try {
      // Dynamic import to avoid hard dependency
      this.magentaMusic = await import('@magenta/music');
      console.log('✅ Magenta.js library loaded');
      return true;
    } catch (error) {
      console.warn('@magenta/music library not installed. Run: npm install @magenta/music');
      console.warn('Falling back to procedural generation');
      return false;
    }
  }

  /**
   * Initialize the Magenta service with AI models
   */
  async initialize(): Promise<void> {
    if (this.initialized) return;

    console.log('🎵 Initializing MagentaService...');

    const magentaAvailable = await this.loadMagentaMusic();

    if (magentaAvailable) {
      try {
        // Initialize MusicVAE for chord/melody generation
        this.musicVAE = new this.magentaMusic.MusicVAE(
          'https://storage.googleapis.com/magentadata/js/checkpoints/music_vae/mel_2bar_small'
        );
        await this.musicVAE.initialize();
        console.log('✅ MusicVAE model loaded');

        // Initialize MelodyRNN for melody continuation
        this.melodyRNN = new this.magentaMusic.MelodyRNN(
          'https://storage.googleapis.com/magentadata/js/checkpoints/music_rnn/basic_rnn'
        );
        await this.melodyRNN.initialize();
        console.log('✅ MelodyRNN model loaded');

        // Initialize DrumsRNN for drum pattern generation
        this.drumsRNN = new this.magentaMusic.DrumsRNN(
          'https://storage.googleapis.com/magentadata/js/checkpoints/music_rnn/drum_kit_rnn'
        );
        await this.drumsRNN.initialize();
        console.log('✅ DrumsRNN model loaded');

        this.useMagentaModels = true;
      } catch (error) {
        console.warn('Failed to load Magenta models:', error);
        console.warn('Falling back to procedural generation');
        this.useMagentaModels = false;
      }
    }

    this.initialized = true;
    console.log(`✅ MagentaService initialized (mode: ${this.useMagentaModels ? 'AI' : 'Procedural'})`);
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
   * Generate a melody using RNN (with Magenta.js or procedural fallback)
   * @param seed - Seed notes to start the melody
   * @param options - Generation options
   */
  async generateMelody(
    seed: Note[] = [],
    options: GenerationOptions = {}
  ): Promise<MelodySequence> {
    await this.ensureInitialized();

    const temperature = options.temperature ?? 1.0;
    const steps = options.steps ?? 32;

    if (this.useMagentaModels && this.melodyRNN) {
      console.log('🎼 Generating melody with MelodyRNN...');

      try {
        // Convert seed notes to NoteSequence format
        const seedSequence = seed.length > 0 ? {
          notes: seed.map(note => ({
            pitch: note.pitch,
            velocity: note.velocity,
            startTime: note.startTime,
            endTime: note.startTime + note.duration,
          })),
          totalTime: seed[seed.length - 1]?.startTime + seed[seed.length - 1]?.duration || 0,
        } : undefined;

        // Generate using MelodyRNN
        const generated = await this.melodyRNN.continueSequence(
          seedSequence || this.magentaMusic.sequences.createQuantizedNoteSequence(4, 120),
          steps,
          temperature
        );

        // Convert NoteSequence back to our Note format
        const notes: Note[] = generated.notes.map((note: any) => ({
          pitch: note.pitch,
          velocity: note.velocity || 80,
          startTime: note.startTime,
          duration: note.endTime - note.startTime,
        }));

        return { notes, temperature, steps };
      } catch (error) {
        console.warn('MelodyRNN generation failed, falling back to procedural:', error);
      }
    }

    // Procedural fallback
    console.log('🎼 Generating melody procedurally...');
    const notes: Note[] = [];
    const scale = [60, 62, 64, 65, 67, 69, 71, 72]; // C major scale

    let currentTime = 0;

    for (let i = 0; i < steps; i++) {
      const noteIndex = Math.floor(Math.random() * scale.length);
      const pitch = scale[noteIndex];
      const duration = Math.random() > 0.7 ? 0.5 : 0.25;

      notes.push({
        pitch,
        velocity: 80 + Math.floor(Math.random() * 20),
        startTime: currentTime,
        duration,
      });

      currentTime += duration;
    }

    return { notes, temperature, steps };
  }

  /**
   * Generate drum patterns (with Magenta.js DrumsRNN or procedural fallback)
   * @param style - Drum pattern style (e.g., 'trap', 'house', 'techno')
   * @param options - Generation options
   */
  async generateDrumPattern(
    style: string = 'trap',
    options: GenerationOptions = {}
  ): Promise<DrumPattern> {
    await this.ensureInitialized();

    const bars = 4;

    if (this.useMagentaModels && this.drumsRNN) {
      console.log(`🥁 Generating ${style} drum pattern with DrumsRNN...`);

      try {
        const temperature = options.temperature ?? 1.0;
        const steps = options.steps ?? 32;

        // Generate drum pattern using DrumsRNN
        const generated = await this.drumsRNN.continueSequence(
          this.magentaMusic.sequences.createQuantizedNoteSequence(4, 120),
          steps,
          temperature
        );

        // Convert to our Note format
        const pattern: Note[] = generated.notes.map((note: any) => ({
          pitch: note.pitch,
          velocity: note.velocity || 80,
          startTime: note.startTime,
          duration: note.endTime - note.startTime,
        }));

        return { pattern, style, bars };
      } catch (error) {
        console.warn('DrumsRNN generation failed, falling back to procedural:', error);
      }
    }

    // Procedural fallback
    console.log(`🥁 Generating ${style} drum pattern procedurally...`);

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
