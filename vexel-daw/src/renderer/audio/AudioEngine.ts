/**
 * AudioEngine.ts
 * Core Web Audio API integration for Vexel DAW
 * Handles real-time audio processing, routing, and playback
 */

import { MIDISequencer } from './MIDISequencer';

export interface AudioTrack {
  id: string;
  name: string;
  type: 'audio' | 'midi' | 'instrument';
  volume: number; // 0.0 to 1.0
  pan: number; // -1.0 (left) to 1.0 (right)
  muted: boolean;
  soloed: boolean;
  armed: boolean;
  color: string;

  // Web Audio nodes
  gainNode?: GainNode;
  panNode?: StereoPannerNode;
  analyserNode?: AnalyserNode;

  // Audio buffer for playback
  buffer?: AudioBuffer;

  // MIDI data
  notes?: MIDINote[];

  // MIDI sequencer (for MIDI/instrument tracks)
  sequencer?: MIDISequencer;

  // Effects chain
  effects: AudioEffect[];
}

export interface MIDINote {
  pitch: number; // 0-127 MIDI note number
  startTime: number; // In beats
  duration: number; // In beats
  velocity: number; // 0-127
}

export interface AudioEffect {
  id: string;
  type: 'eq' | 'compressor' | 'reverb' | 'delay' | 'distortion' | 'custom';
  enabled: boolean;
  parameters: Record<string, number>;
  node?: AudioNode;
}

export interface TransportState {
  isPlaying: boolean;
  currentTime: number; // In seconds
  currentBar: number;
  currentBeat: number;
  tempo: number; // BPM
  timeSignature: { numerator: number; denominator: number };
  loopEnabled: boolean;
  loopStart: number; // In beats
  loopEnd: number; // In beats
}

export class AudioEngine {
  private context: AudioContext;
  private masterGain: GainNode;
  private masterAnalyser: AnalyserNode;
  private tracks: Map<string, AudioTrack>;
  private transport: TransportState;
  private startTime: number = 0;
  private animationFrameId: number | null = null;

  // Callbacks
  private onTransportUpdate?: (transport: TransportState) => void;
  private onTracksUpdate?: (tracks: AudioTrack[]) => void;

  constructor() {
    // Initialize Web Audio API context
    this.context = new AudioContext();

    // Create master gain and analyser
    this.masterGain = this.context.createGain();
    this.masterGain.gain.value = 0.8; // Prevent clipping

    this.masterAnalyser = this.context.createAnalyser();
    this.masterAnalyser.fftSize = 2048;

    // Connect master chain: masterGain -> masterAnalyser -> destination
    this.masterGain.connect(this.masterAnalyser);
    this.masterAnalyser.connect(this.context.destination);

    // Initialize tracks
    this.tracks = new Map();

    // Initialize transport
    this.transport = {
      isPlaying: false,
      currentTime: 0,
      currentBar: 0,
      currentBeat: 0,
      tempo: 120,
      timeSignature: { numerator: 4, denominator: 4 },
      loopEnabled: false,
      loopStart: 0,
      loopEnd: 16,
    };

    console.log('🎵 AudioEngine initialized', {
      sampleRate: this.context.sampleRate,
      state: this.context.state
    });
  }

  /**
   * Start the audio context (required after user interaction)
   */
  async start() {
    if (this.context.state === 'suspended') {
      await this.context.resume();
      console.log('▶️ Audio context resumed');
    }
  }

  /**
   * Create a new track
   */
  createTrack(name: string, type: AudioTrack['type'], color: string = '#3b82f6'): string {
    const id = `track-${Date.now()}-${Math.random().toString(36).substr(2, 9)}`;

    // Create Web Audio nodes
    const gainNode = this.context.createGain();
    const panNode = this.context.createStereoPanner();
    const analyserNode = this.context.createAnalyser();

    // Configure analyser
    analyserNode.fftSize = 1024;

    // Connect nodes: source -> gain -> pan -> analyser -> master
    gainNode.connect(panNode);
    panNode.connect(analyserNode);
    analyserNode.connect(this.masterGain);

    // Create MIDI sequencer for MIDI/instrument tracks
    let sequencer: MIDISequencer | undefined;
    if (type === 'midi' || type === 'instrument') {
      sequencer = new MIDISequencer(this.context, gainNode);
      sequencer.setTempo(this.transport.tempo);
    }

    const track: AudioTrack = {
      id,
      name,
      type,
      volume: 0.8,
      pan: 0,
      muted: false,
      soloed: false,
      armed: false,
      color,
      gainNode,
      panNode,
      analyserNode,
      effects: [],
      notes: type === 'midi' || type === 'instrument' ? [] : undefined,
      sequencer,
    };

    this.tracks.set(id, track);
    this.notifyTracksUpdate();

    console.log(`➕ Created ${type} track:`, name);
    return id;
  }

  /**
   * Delete a track
   */
  deleteTrack(trackId: string) {
    const track = this.tracks.get(trackId);
    if (!track) return;

    // Disconnect and cleanup nodes (Web Audio API throws if no connections exist)
    try { track.gainNode?.disconnect(); } catch (e) { /* ignore */ }
    try { track.panNode?.disconnect(); } catch (e) { /* ignore */ }
    try { track.analyserNode?.disconnect(); } catch (e) { /* ignore */ }

    // Cleanup sequencer
    if (track.sequencer) {
      track.sequencer.dispose();
    }

    this.tracks.delete(trackId);
    this.notifyTracksUpdate();

    console.log('➖ Deleted track:', track.name);
  }

  /**
   * Set track volume (0.0 to 1.0)
   */
  setTrackVolume(trackId: string, volume: number) {
    const track = this.tracks.get(trackId);
    if (!track || !track.gainNode) return;

    track.volume = Math.max(0, Math.min(1, volume));
    track.gainNode.gain.setValueAtTime(track.volume, this.context.currentTime);
    this.notifyTracksUpdate();
  }

  /**
   * Set track pan (-1.0 to 1.0)
   */
  setTrackPan(trackId: string, pan: number) {
    const track = this.tracks.get(trackId);
    if (!track || !track.panNode) return;

    track.pan = Math.max(-1, Math.min(1, pan));
    track.panNode.pan.setValueAtTime(track.pan, this.context.currentTime);
    this.notifyTracksUpdate();
  }

  /**
   * Mute/unmute track
   */
  setTrackMuted(trackId: string, muted: boolean) {
    const track = this.tracks.get(trackId);
    if (!track || !track.gainNode) return;

    track.muted = muted;

    // Smoothly fade to prevent clicks
    const targetVolume = muted ? 0 : track.volume;
    track.gainNode.gain.setTargetAtTime(
      targetVolume,
      this.context.currentTime,
      0.01 // Time constant for smooth fade
    );

    this.notifyTracksUpdate();
  }

  /**
   * Solo/unsolo track
   */
  setTrackSoloed(trackId: string, soloed: boolean) {
    const track = this.tracks.get(trackId);
    if (!track) return;

    track.soloed = soloed;

    // If any track is soloed, mute all non-soloed tracks
    const hasSoloedTracks = Array.from(this.tracks.values()).some(t => t.soloed);

    this.tracks.forEach((t) => {
      if (t.gainNode) {
        const shouldMute = hasSoloedTracks && !t.soloed;
        const targetVolume = shouldMute ? 0 : t.volume;
        t.gainNode.gain.setTargetAtTime(targetVolume, this.context.currentTime, 0.01);
      }
    });

    this.notifyTracksUpdate();
  }

  /**
   * Set track arm for recording
   */
  setTrackArmed(trackId: string, armed: boolean) {
    const track = this.tracks.get(trackId);
    if (!track) return;

    track.armed = armed;
    this.notifyTracksUpdate();
  }

  /**
   * Load audio buffer into track
   */
  async loadAudioFile(trackId: string, file: File) {
    const track = this.tracks.get(trackId);
    if (!track || track.type !== 'audio') return;

    try {
      const arrayBuffer = await file.arrayBuffer();
      const audioBuffer = await this.context.decodeAudioData(arrayBuffer);

      track.buffer = audioBuffer;
      this.notifyTracksUpdate();

      console.log(`📁 Loaded audio file into ${track.name}:`, {
        duration: audioBuffer.duration,
        sampleRate: audioBuffer.sampleRate,
        channels: audioBuffer.numberOfChannels
      });
    } catch (error) {
      console.error('❌ Failed to load audio file:', error);
    }
  }

  /**
   * Add MIDI notes to track
   */
  addMIDINotes(trackId: string, notes: MIDINote[]) {
    const track = this.tracks.get(trackId);
    if (!track || (track.type !== 'midi' && track.type !== 'instrument')) return;

    if (!track.notes) track.notes = [];
    track.notes.push(...notes);
    track.notes.sort((a, b) => a.startTime - b.startTime);

    // Update sequencer
    if (track.sequencer) {
      track.sequencer.loadNotes(track.notes);
    }

    this.notifyTracksUpdate();
    console.log(`🎹 Added ${notes.length} MIDI notes to ${track.name}`);
  }

  /**
   * Clear MIDI notes from track
   */
  clearMIDINotes(trackId: string) {
    const track = this.tracks.get(trackId);
    if (!track || (track.type !== 'midi' && track.type !== 'instrument')) return;

    track.notes = [];

    if (track.sequencer) {
      track.sequencer.clearNotes();
    }

    this.notifyTracksUpdate();
    console.log(`🎹 Cleared MIDI notes from ${track.name}`);
  }

  /**
   * Play transport
   */
  play() {
    if (this.transport.isPlaying) return;

    this.transport.isPlaying = true;
    this.startTime = this.context.currentTime - this.transport.currentTime;

    // Start all MIDI sequencers
    const beatsPerSecond = this.transport.tempo / 60;
    const currentBeat = this.transport.currentTime * beatsPerSecond;

    this.tracks.forEach(track => {
      if (track.sequencer) {
        track.sequencer.start(currentBeat);
      }
    });

    // Start animation loop for transport updates
    this.startTransportLoop();

    console.log('▶️ Transport playing');
    this.notifyTransportUpdate();
  }

  /**
   * Pause transport
   */
  pause() {
    if (!this.transport.isPlaying) return;

    this.transport.isPlaying = false;

    // Stop all MIDI sequencers
    this.tracks.forEach(track => {
      if (track.sequencer) {
        track.sequencer.stop();
      }
    });

    // Stop animation loop
    if (this.animationFrameId !== null) {
      cancelAnimationFrame(this.animationFrameId);
      this.animationFrameId = null;
    }

    console.log('⏸️ Transport paused');
    this.notifyTransportUpdate();
  }

  /**
   * Stop transport and return to start
   */
  stop() {
    this.pause();
    this.transport.currentTime = 0;
    this.transport.currentBar = 0;
    this.transport.currentBeat = 0;

    console.log('⏹️ Transport stopped');
    this.notifyTransportUpdate();
  }

  /**
   * Set tempo in BPM
   */
  setTempo(bpm: number) {
    this.transport.tempo = Math.max(20, Math.min(999, bpm));

    // Update all sequencers with new tempo
    this.tracks.forEach(track => {
      if (track.sequencer) {
        track.sequencer.setTempo(this.transport.tempo);
      }
    });

    console.log(`🎵 Tempo set to ${this.transport.tempo} BPM`);
    this.notifyTransportUpdate();
  }

  /**
   * Set time signature
   */
  setTimeSignature(numerator: number, denominator: number) {
    this.transport.timeSignature = { numerator, denominator };
    console.log(`🎼 Time signature set to ${numerator}/${denominator}`);
    this.notifyTransportUpdate();
  }

  /**
   * Toggle loop
   */
  toggleLoop() {
    this.transport.loopEnabled = !this.transport.loopEnabled;
    console.log(`🔁 Loop ${this.transport.loopEnabled ? 'enabled' : 'disabled'}`);
    this.notifyTransportUpdate();
  }

  /**
   * Set loop points (in beats)
   */
  setLoopPoints(start: number, end: number) {
    this.transport.loopStart = start;
    this.transport.loopEnd = end;
    console.log(`🔁 Loop points set: ${start} - ${end} beats`);
    this.notifyTransportUpdate();
  }

  /**
   * Get master level (for metering)
   */
  getMasterLevel(): { left: number; right: number } {
    const dataArray = new Uint8Array(this.masterAnalyser.frequencyBinCount);
    this.masterAnalyser.getByteTimeDomainData(dataArray);

    // Calculate RMS level
    let sum = 0;
    for (let i = 0; i < dataArray.length; i++) {
      const normalized = (dataArray[i] - 128) / 128;
      sum += normalized * normalized;
    }
    const rms = Math.sqrt(sum / dataArray.length);

    return { left: rms, right: rms }; // Simplified - same for both channels
  }

  /**
   * Get track level (for metering)
   */
  getTrackLevel(trackId: string): { left: number; right: number } {
    const track = this.tracks.get(trackId);
    if (!track || !track.analyserNode) return { left: 0, right: 0 };

    const dataArray = new Uint8Array(track.analyserNode.frequencyBinCount);
    track.analyserNode.getByteTimeDomainData(dataArray);

    let sum = 0;
    for (let i = 0; i < dataArray.length; i++) {
      const normalized = (dataArray[i] - 128) / 128;
      sum += normalized * normalized;
    }
    const rms = Math.sqrt(sum / dataArray.length);

    return { left: rms, right: rms };
  }

  /**
   * Get all tracks
   */
  getTracks(): AudioTrack[] {
    return Array.from(this.tracks.values());
  }

  /**
   * Get transport state
   */
  getTransport(): TransportState {
    return { ...this.transport };
  }

  /**
   * Register callback for transport updates
   */
  onTransportChange(callback: (transport: TransportState) => void) {
    this.onTransportUpdate = callback;
  }

  /**
   * Register callback for track updates
   */
  onTracksChange(callback: (tracks: AudioTrack[]) => void) {
    this.onTracksUpdate = callback;
  }

  /**
   * Animation loop for transport updates
   */
  private startTransportLoop() {
    const update = () => {
      if (!this.transport.isPlaying) return;

      // Calculate current time
      this.transport.currentTime = this.context.currentTime - this.startTime;

      // Convert to beats
      const beatsPerSecond = this.transport.tempo / 60;
      const totalBeats = this.transport.currentTime * beatsPerSecond;

      // Calculate bars and beats
      const beatsPerBar = this.transport.timeSignature.numerator;
      this.transport.currentBar = Math.floor(totalBeats / beatsPerBar);
      this.transport.currentBeat = Math.floor(totalBeats % beatsPerBar);

      // Handle looping
      if (this.transport.loopEnabled) {
        if (totalBeats >= this.transport.loopEnd) {
          this.transport.currentTime = (this.transport.loopStart / beatsPerSecond);
          this.startTime = this.context.currentTime - this.transport.currentTime;
        }
      }

      this.notifyTransportUpdate();
      this.animationFrameId = requestAnimationFrame(update);
    };

    this.animationFrameId = requestAnimationFrame(update);
  }

  /**
   * Notify listeners of transport update
   */
  private notifyTransportUpdate() {
    if (this.onTransportUpdate) {
      this.onTransportUpdate({ ...this.transport });
    }
  }

  /**
   * Notify listeners of tracks update
   */
  private notifyTracksUpdate() {
    if (this.onTracksUpdate) {
      this.onTracksUpdate(this.getTracks());
    }
  }

  /**
   * Cleanup
   */
  dispose() {
    this.stop();

    // Disconnect all tracks (Web Audio API throws if no connections exist)
    this.tracks.forEach(track => {
      try { track.gainNode?.disconnect(); } catch (e) { /* ignore */ }
      try { track.panNode?.disconnect(); } catch (e) { /* ignore */ }
      try { track.analyserNode?.disconnect(); } catch (e) { /* ignore */ }

      // Dispose sequencers
      if (track.sequencer) {
        track.sequencer.dispose();
      }
    });

    // Disconnect master (Web Audio API throws if no connections exist)
    try { this.masterGain.disconnect(); } catch (e) { /* ignore */ }
    try { this.masterAnalyser.disconnect(); } catch (e) { /* ignore */ }

    // Close context
    this.context.close();

    console.log('🛑 AudioEngine disposed');
  }
}
