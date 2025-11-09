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
    engine.createTrack('Drums', 'midi', '#ef4444');
    engine.createTrack('Bass', 'midi', '#3b82f6');
    engine.createTrack('Synth', 'instrument', '#8b5cf6');
    engine.createTrack('Vocals', 'audio', '#10b981');

    set({
      engine,
      tracks: engine.getTracks(),
      transport: engine.getTransport(),
    });

    console.log('🎵 Audio store initialized with engine');

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
