/**
 * MagentaService - AI Music Generation Service
 *
 * Provides methods for generating musical content using AI models:
 * - Chord progressions
 * - Melodies (RNN-based)
 * - Drum patterns
 *
 * Powered by Google Magenta.js and TensorFlow.js
 */

import * as mm from '@magenta/music';

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
  private melodyRNN: mm.MusicRNN | null = null;
  private musicVAE: mm.MusicVAE | null = null;
  private drumsRNN: mm.MusicRNN | null = null;

  // Pre-trained model URLs hosted by Google
  private readonly MELODY_RNN_URL = 'https://storage.googleapis.com/magentadata/js/checkpoints/music_rnn/basic_rnn';
  private readonly DRUMS_RNN_URL = 'https://storage.googleapis.com/magentadata/js/checkpoints/music_rnn/drum_kit_rnn';
  private readonly MUSIC_VAE_MEL_URL = 'https://storage.googleapis.com/magentadata/js/checkpoints/music_vae/mel_2bar_small';

  /**
   * Initialize the Magenta service with pre-trained models
   */
  async initialize(): Promise<void> {
    if (this.initialized) return;

    console.log('🎵 Initializing MagentaService with AI models...');

    try {
      // Initialize MusicRNN for melody generation
      console.log('📥 Loading MelodyRNN model...');
      this.melodyRNN = new mm.MusicRNN(this.MELODY_RNN_URL);
      await this.melodyRNN.initialize();
      console.log('✅ MelodyRNN loaded');

      // Initialize MusicVAE for variation and interpolation
      console.log('📥 Loading MusicVAE model...');
      this.musicVAE = new mm.MusicVAE(this.MUSIC_VAE_MEL_URL);
      await this.musicVAE.initialize();
      console.log('✅ MusicVAE loaded');

      // Initialize DrumsRNN for drum patterns
      console.log('📥 Loading DrumsRNN model...');
      this.drumsRNN = new mm.MusicRNN(this.DRUMS_RNN_URL);
      await this.drumsRNN.initialize();
      console.log('✅ DrumsRNN loaded');

      this.initialized = true;
      console.log('✅ MagentaService initialized with AI models');
    } catch (error) {
      console.error('❌ Failed to initialize Magenta models:', error);
      console.log('⚠️ Falling back to procedural generation');
      this.initialized = true; // Mark as initialized to allow fallback
    }
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
   * Generate a melody using AI (MusicRNN)
   * @param seed - Seed notes to start the melody
   * @param options - Generation options
   */
  async generateMelody(
    seed: Note[] = [],
    options: GenerationOptions = {}
  ): Promise<MelodySequence> {
    await this.ensureInitialized();

    console.log('🎼 Generating melody with AI...');

    const temperature = options.temperature ?? 1.0;
    const steps = options.steps ?? 32;

    // Try AI generation first
    if (this.melodyRNN || this.musicVAE) {
      try {
        let noteSequence: mm.INoteSequence;

        if (seed.length > 0 && this.melodyRNN) {
          // Continue from seed using MusicRNN
          const seedSequence = this.notesToNoteSequence(seed);
          noteSequence = await this.melodyRNN.continueSequence(seedSequence, steps, temperature);
          console.log('✅ Melody generated with MusicRNN (continuation)');
        } else if (this.musicVAE) {
          // Sample new melody using MusicVAE
          const samples = await this.musicVAE.sample(1, temperature);
          noteSequence = samples[0];
          console.log('✅ Melody generated with MusicVAE (sampling)');
        } else if (this.melodyRNN) {
          // Generate from empty sequence
          const emptySequence: mm.INoteSequence = {
            ticksPerQuarter: 220,
            totalTime: 0,
            timeSignatures: [{ time: 0, numerator: 4, denominator: 4 }],
            tempos: [{ time: 0, qpm: 120 }],
            notes: [],
          };
          noteSequence = await this.melodyRNN.continueSequence(emptySequence, steps, temperature);
          console.log('✅ Melody generated with MusicRNN');
        } else {
          throw new Error('No models available');
        }

        // Convert NoteSequence to our Note format
        const notes = this.noteSequenceToNotes(noteSequence);

        return {
          notes,
          temperature,
          steps: notes.length,
        };
      } catch (error) {
        console.error('❌ AI generation failed:', error);
        console.log('⚠️ Falling back to procedural generation');
      }
    }

    // Fallback to procedural generation
    return this.generateProceduralMelody(steps, temperature);
  }

  /**
   * Fallback procedural melody generation
   */
  private generateProceduralMelody(steps: number, temperature: number): MelodySequence {
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

    return {
      notes,
      temperature,
      steps,
    };
  }

  /**
   * Generate drum patterns using AI (DrumsRNN)
   * @param style - Drum pattern style (e.g., 'trap', 'house', 'techno')
   * @param options - Generation options
   */
  async generateDrumPattern(
    style: string = 'trap',
    options: GenerationOptions = {}
  ): Promise<DrumPattern> {
    await this.ensureInitialized();

    console.log(`🥁 Generating ${style} drum pattern with AI...`);

    const bars = 4;
    const temperature = options.temperature ?? 1.0;
    const steps = options.steps ?? 32;

    // Try AI generation with DrumsRNN
    if (this.drumsRNN) {
      try {
        const emptySequence: mm.INoteSequence = {
          ticksPerQuarter: 220,
          totalTime: 0,
          timeSignatures: [{ time: 0, numerator: 4, denominator: 4 }],
          tempos: [{ time: 0, qpm: 120 }],
          notes: [],
        };

        const noteSequence = await this.drumsRNN.continueSequence(emptySequence, steps, temperature);
        const pattern = this.noteSequenceToNotes(noteSequence);

        console.log('✅ Drum pattern generated with DrumsRNN');

        return {
          pattern,
          style: 'ai-generated',
          bars,
        };
      } catch (error) {
        console.error('❌ AI drum generation failed:', error);
        console.log('⚠️ Falling back to procedural generation');
      }
    }

    // Fallback to procedural generation
    return this.generateProceduralDrums(style, bars);
  }

  /**
   * Fallback procedural drum generation
   */
  private generateProceduralDrums(style: string, bars: number): DrumPattern {
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
   * Convert our Note format to Magenta NoteSequence
   */
  private notesToNoteSequence(notes: Note[]): mm.INoteSequence {
    const qpm = 120; // Quarter notes per minute (tempo)
    const ticksPerQuarter = 220;

    const noteSequence: mm.INoteSequence = {
      ticksPerQuarter,
      totalTime: 0,
      timeSignatures: [{ time: 0, numerator: 4, denominator: 4 }],
      tempos: [{ time: 0, qpm }],
      notes: [],
    };

    notes.forEach((note) => {
      const startTime = (note.startTime / 4) * (60 / qpm) * 4; // Convert beats to seconds
      const endTime = startTime + (note.duration / 4) * (60 / qpm) * 4;

      noteSequence.notes!.push({
        pitch: note.pitch,
        velocity: note.velocity,
        startTime,
        endTime,
        instrument: 0,
        program: 0,
      });

      noteSequence.totalTime = Math.max(noteSequence.totalTime, endTime);
    });

    return noteSequence;
  }

  /**
   * Convert Magenta NoteSequence to our Note format
   */
  private noteSequenceToNotes(noteSequence: mm.INoteSequence): Note[] {
    if (!noteSequence.notes || noteSequence.notes.length === 0) {
      return [];
    }

    const qpm = noteSequence.tempos?.[0]?.qpm ?? 120;
    const notes: Note[] = [];

    noteSequence.notes.forEach((note) => {
      // Convert seconds to beats (assuming 4/4 time)
      const startTimeBeats = (note.startTime * qpm) / 60 * (4 / 4);
      const endTimeBeats = (note.endTime * qpm) / 60 * (4 / 4);
      const durationBeats = endTimeBeats - startTimeBeats;

      notes.push({
        pitch: note.pitch,
        velocity: note.velocity ?? 80,
        startTime: startTimeBeats,
        duration: durationBeats,
      });
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
