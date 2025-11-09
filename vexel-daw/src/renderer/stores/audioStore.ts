/**
 * audioStore.ts
 * Zustand store for managing audio engine state
 */

import { create } from 'zustand';
import { AudioEngine, AudioTrack, TransportState, MIDINote } from '../audio/AudioEngine';

interface AudioStore {
  // Audio engine instance
  engine: AudioEngine | null;

  // State
  tracks: AudioTrack[];
  transport: TransportState;
  masterLevel: { left: number; right: number };

  // Initialization
  initEngine: () => void;

  // Transport controls
  play: () => void;
  pause: () => void;
  stop: () => void;
  setTempo: (bpm: number) => void;
  setTimeSignature: (numerator: number, denominator: number) => void;
  toggleLoop: () => void;
  setLoopPoints: (start: number, end: number) => void;

  // Track management
  createTrack: (name: string, type: AudioTrack['type'], color?: string) => string;
  deleteTrack: (trackId: string) => void;
  setTrackVolume: (trackId: string, volume: number) => void;
  setTrackPan: (trackId: string, pan: number) => void;
  setTrackMuted: (trackId: string, muted: boolean) => void;
  setTrackSoloed: (trackId: string, soloed: boolean) => void;
  setTrackArmed: (trackId: string, armed: boolean) => void;
  renameTrack: (trackId: string, name: string) => void;
  setTrackColor: (trackId: string, color: string) => void;

  // Audio/MIDI management
  loadAudioFile: (trackId: string, file: File) => Promise<void>;
  addMIDINotes: (trackId: string, notes: MIDINote[]) => void;
  clearMIDINotes: (trackId: string) => void;

  // Metering
  updateMeters: () => void;

  // Cleanup
  dispose: () => void;
}

export const useAudioStore = create<AudioStore>((set, get) => ({
  // Initial state
  engine: null,
  tracks: [],
  transport: {
    isPlaying: false,
    currentTime: 0,
    currentBar: 0,
    currentBeat: 0,
    tempo: 120,
    timeSignature: { numerator: 4, denominator: 4 },
    loopEnabled: false,
    loopStart: 0,
    loopEnd: 16,
  },
  masterLevel: { left: 0, right: 0 },

  /**
   * Initialize audio engine
   */
  initEngine: () => {
    const engine = new AudioEngine();

    // Subscribe to transport updates
    engine.onTransportChange((transport) => {
      set({ transport });
    });

    // Subscribe to track updates
    engine.onTracksChange((tracks) => {
      set({ tracks });
    });

    // Initialize with default tracks for testing
    const drumsId = engine.createTrack('Drums', 'midi', '#ef4444');
    const bassId = engine.createTrack('Bass', 'midi', '#3b82f6');
    const synthId = engine.createTrack('Synth', 'instrument', '#8b5cf6');
    engine.createTrack('Vocals', 'audio', '#10b981');

    // Add demo MIDI notes for testing
    // Drums - Simple kick and snare pattern
    engine.addMIDINotes(drumsId, [
      // Kick on beats 1 and 3 (every 4 beats)
      { pitch: 36, startTime: 0, duration: 0.25, velocity: 100 },
      { pitch: 36, startTime: 2, duration: 0.25, velocity: 100 },
      { pitch: 36, startTime: 4, duration: 0.25, velocity: 100 },
      { pitch: 36, startTime: 6, duration: 0.25, velocity: 100 },
      // Snare on beats 2 and 4
      { pitch: 38, startTime: 1, duration: 0.25, velocity: 90 },
      { pitch: 38, startTime: 3, duration: 0.25, velocity: 90 },
      { pitch: 38, startTime: 5, duration: 0.25, velocity: 90 },
      { pitch: 38, startTime: 7, duration: 0.25, velocity: 90 },
      // Hi-hat on every half beat
      { pitch: 42, startTime: 0, duration: 0.125, velocity: 70 },
      { pitch: 42, startTime: 0.5, duration: 0.125, velocity: 60 },
      { pitch: 42, startTime: 1, duration: 0.125, velocity: 70 },
      { pitch: 42, startTime: 1.5, duration: 0.125, velocity: 60 },
      { pitch: 42, startTime: 2, duration: 0.125, velocity: 70 },
      { pitch: 42, startTime: 2.5, duration: 0.125, velocity: 60 },
      { pitch: 42, startTime: 3, duration: 0.125, velocity: 70 },
      { pitch: 42, startTime: 3.5, duration: 0.125, velocity: 60 },
      { pitch: 42, startTime: 4, duration: 0.125, velocity: 70 },
      { pitch: 42, startTime: 4.5, duration: 0.125, velocity: 60 },
      { pitch: 42, startTime: 5, duration: 0.125, velocity: 70 },
      { pitch: 42, startTime: 5.5, duration: 0.125, velocity: 60 },
      { pitch: 42, startTime: 6, duration: 0.125, velocity: 70 },
      { pitch: 42, startTime: 6.5, duration: 0.125, velocity: 60 },
      { pitch: 42, startTime: 7, duration: 0.125, velocity: 70 },
      { pitch: 42, startTime: 7.5, duration: 0.125, velocity: 60 },
    ]);

    // Bass - Simple bass line (C minor scale)
    engine.addMIDINotes(bassId, [
      { pitch: 36, startTime: 0, duration: 1, velocity: 85 }, // C
      { pitch: 39, startTime: 1, duration: 1, velocity: 80 }, // Eb
      { pitch: 43, startTime: 2, duration: 1, velocity: 85 }, // G
      { pitch: 39, startTime: 3, duration: 1, velocity: 80 }, // Eb
      { pitch: 36, startTime: 4, duration: 1, velocity: 85 }, // C
      { pitch: 39, startTime: 5, duration: 1, velocity: 80 }, // Eb
      { pitch: 43, startTime: 6, duration: 1, velocity: 85 }, // G
      { pitch: 41, startTime: 7, duration: 1, velocity: 80 }, // F
    ]);

    // Synth - Simple chord progression (C minor)
    engine.addMIDINotes(synthId, [
      // Cm chord (bar 1)
      { pitch: 48, startTime: 0, duration: 2, velocity: 70 }, // C
      { pitch: 51, startTime: 0, duration: 2, velocity: 65 }, // Eb
      { pitch: 55, startTime: 0, duration: 2, velocity: 70 }, // G
      // Fm chord (bar 2)
      { pitch: 53, startTime: 2, duration: 2, velocity: 70 }, // F
      { pitch: 56, startTime: 2, duration: 2, velocity: 65 }, // Ab
      { pitch: 60, startTime: 2, duration: 2, velocity: 70 }, // C
      // G chord (bar 3)
      { pitch: 55, startTime: 4, duration: 2, velocity: 70 }, // G
      { pitch: 59, startTime: 4, duration: 2, velocity: 65 }, // B
      { pitch: 62, startTime: 4, duration: 2, velocity: 70 }, // D
      // Ab chord (bar 4)
      { pitch: 56, startTime: 6, duration: 2, velocity: 70 }, // Ab
      { pitch: 60, startTime: 6, duration: 2, velocity: 65 }, // C
      { pitch: 63, startTime: 6, duration: 2, velocity: 70 }, // Eb
    ]);

    set({
      engine,
      tracks: engine.getTracks(),
      transport: engine.getTransport(),
    });

    console.log('🎵 Audio store initialized with engine and demo MIDI notes');

    // Start metering loop
    get().startMeteringLoop();
  },

  /**
   * Play transport
   */
  play: async () => {
    const { engine } = get();
    if (!engine) return;

    await engine.start(); // Resume audio context if suspended
    engine.play();
  },

  /**
   * Pause transport
   */
  pause: () => {
    const { engine } = get();
    if (!engine) return;

    engine.pause();
  },

  /**
   * Stop transport
   */
  stop: () => {
    const { engine } = get();
    if (!engine) return;

    engine.stop();
  },

  /**
   * Set tempo
   */
  setTempo: (bpm: number) => {
    const { engine } = get();
    if (!engine) return;

    engine.setTempo(bpm);
  },

  /**
   * Set time signature
   */
  setTimeSignature: (numerator: number, denominator: number) => {
    const { engine } = get();
    if (!engine) return;

    engine.setTimeSignature(numerator, denominator);
  },

  /**
   * Toggle loop
   */
  toggleLoop: () => {
    const { engine } = get();
    if (!engine) return;

    engine.toggleLoop();
  },

  /**
   * Set loop points
   */
  setLoopPoints: (start: number, end: number) => {
    const { engine } = get();
    if (!engine) return;

    engine.setLoopPoints(start, end);
  },

  /**
   * Create track
   */
  createTrack: (name: string, type: AudioTrack['type'], color?: string): string => {
    const { engine } = get();
    if (!engine) return '';

    return engine.createTrack(name, type, color);
  },

  /**
   * Delete track
   */
  deleteTrack: (trackId: string) => {
    const { engine } = get();
    if (!engine) return;

    engine.deleteTrack(trackId);
  },

  /**
   * Set track volume
   */
  setTrackVolume: (trackId: string, volume: number) => {
    const { engine } = get();
    if (!engine) return;

    engine.setTrackVolume(trackId, volume);
  },

  /**
   * Set track pan
   */
  setTrackPan: (trackId: string, pan: number) => {
    const { engine } = get();
    if (!engine) return;

    engine.setTrackPan(trackId, pan);
  },

  /**
   * Set track muted
   */
  setTrackMuted: (trackId: string, muted: boolean) => {
    const { engine } = get();
    if (!engine) return;

    engine.setTrackMuted(trackId, muted);
  },

  /**
   * Set track soloed
   */
  setTrackSoloed: (trackId: string, soloed: boolean) => {
    const { engine } = get();
    if (!engine) return;

    engine.setTrackSoloed(trackId, soloed);
  },

  /**
   * Set track armed for recording
   */
  setTrackArmed: (trackId: string, armed: boolean) => {
    const { engine } = get();
    if (!engine) return;

    engine.setTrackArmed(trackId, armed);
  },

  /**
   * Rename track
   */
  renameTrack: (trackId: string, name: string) => {
    const { tracks } = get();
    const track = tracks.find(t => t.id === trackId);
    if (!track) return;

    track.name = name;
    set({ tracks: [...tracks] });
  },

  /**
   * Set track color
   */
  setTrackColor: (trackId: string, color: string) => {
    const { tracks } = get();
    const track = tracks.find(t => t.id === trackId);
    if (!track) return;

    track.color = color;
    set({ tracks: [...tracks] });
  },

  /**
   * Load audio file
   */
  loadAudioFile: async (trackId: string, file: File) => {
    const { engine } = get();
    if (!engine) return;

    await engine.loadAudioFile(trackId, file);
  },

  /**
   * Add MIDI notes
   */
  addMIDINotes: (trackId: string, notes: MIDINote[]) => {
    const { engine } = get();
    if (!engine) return;

    engine.addMIDINotes(trackId, notes);
  },

  /**
   * Clear MIDI notes
   */
  clearMIDINotes: (trackId: string) => {
    const { engine } = get();
    if (!engine) return;

    engine.clearMIDINotes(trackId);
  },

  /**
   * Update audio meters (called on animation frame)
   */
  updateMeters: () => {
    const { engine } = get();
    if (!engine) return;

    const masterLevel = engine.getMasterLevel();
    set({ masterLevel });
  },

  /**
   * Start metering loop
   */
  startMeteringLoop: () => {
    const update = () => {
      get().updateMeters();
      requestAnimationFrame(update);
    };
    requestAnimationFrame(update);
  },

  /**
   * Cleanup
   */
  dispose: () => {
    const { engine } = get();
    if (!engine) return;

    engine.dispose();
    set({ engine: null, tracks: [], masterLevel: { left: 0, right: 0 } });
  },
}));
