/**
 * Web Audio API Engine for Zenith DAW
 *
 * Architecture based on Web Audio API best practices:
 * - Modular routing graph with audio nodes
 * - Global transport for synchronization
 * - Sample-accurate scheduling
 * - Effects chain support
 */

export interface AudioClip {
  id: string;
  trackId: string;
  buffer: AudioBuffer | null;
  startTime: number; // Position in timeline (seconds)
  duration: number;
  offset: number; // Offset into the buffer
  gain: number;
  muted: boolean;
  name: string;
  color: string;
}

export interface AudioTrackNode {
  id: string;
  name: string;
  gainNode: GainNode;
  panNode: StereoPannerNode;
  analyserNode: AnalyserNode;
  effectsChain: AudioNode[];
  muted: boolean;
  solo: boolean;
  volume: number;
  pan: number;
  clips: AudioClip[];
}

export interface TransportPosition {
  seconds: number;
  bars: number;
  beats: number;
}

export class AudioEngine {
  private audioContext: AudioContext | null = null;
  private masterGainNode: GainNode | null = null;
  private masterAnalyserNode: AnalyserNode | null = null;

  private tracks: Map<string, AudioTrackNode> = new Map();
  private playingSources: Map<string, AudioBufferSourceNode> = new Map();

  // Transport state
  private isPlaying: boolean = false;
  private isPaused: boolean = false;
  private currentPosition: number = 0; // Current playback position in seconds
  private startTime: number = 0; // AudioContext time when playback started
  private pauseTime: number = 0; // Position when paused

  // Tempo and time signature
  private tempo: number = 120;
  private timeSignature = { numerator: 4, denominator: 4 };

  // Lookahead scheduling
  private scheduleAheadTime = 0.1; // Schedule 100ms ahead
  private scheduleInterval: number | null = null;

  // Callbacks
  private onPositionUpdate?: (position: TransportPosition) => void;
  private onPlayStateChange?: (isPlaying: boolean) => void;

  constructor() {
    // Audio context will be created on first user interaction
  }

  /**
   * Initialize the audio context (must be called after user gesture)
   */
  async initialize(): Promise<void> {
    if (!this.audioContext) {
      this.audioContext = new AudioContext({
        latencyHint: 'interactive',
        sampleRate: 48000,
      });

      // Create master chain
      this.masterGainNode = this.audioContext.createGain();
      this.masterAnalyserNode = this.audioContext.createAnalyser();

      this.masterAnalyserNode.fftSize = 2048;

      // Connect master chain: master gain -> analyser -> destination
      this.masterGainNode.connect(this.masterAnalyserNode);
      this.masterAnalyserNode.connect(this.audioContext.destination);
    }

    // Resume context if suspended (browser autoplay policy)
    if (this.audioContext.state === 'suspended') {
      await this.audioContext.resume();
    }
  }

  /**
   * Create a new audio track
   */
  createTrack(id: string, name: string): AudioTrackNode {
    if (!this.audioContext || !this.masterGainNode) {
      throw new Error('Audio context not initialized');
    }

    const gainNode = this.audioContext.createGain();
    const panNode = this.audioContext.createStereoPanner();
    const analyserNode = this.audioContext.createAnalyser();

    analyserNode.fftSize = 2048;

    // Connect track chain: gain -> pan -> analyser -> master
    gainNode.connect(panNode);
    panNode.connect(analyserNode);
    analyserNode.connect(this.masterGainNode);

    const track: AudioTrackNode = {
      id,
      name,
      gainNode,
      panNode,
      analyserNode,
      effectsChain: [],
      muted: false,
      solo: false,
      volume: 0.8,
      pan: 0,
      clips: [],
    };

    this.tracks.set(id, track);
    return track;
  }

  /**
   * Remove a track
   */
  removeTrack(trackId: string): void {
    const track = this.tracks.get(trackId);
    if (track) {
      // Disconnect all nodes
      track.gainNode.disconnect();
      track.panNode.disconnect();
      track.analyserNode.disconnect();

      this.tracks.delete(trackId);
    }
  }

  /**
   * Load audio file and decode it
   */
  async loadAudioFile(file: File): Promise<AudioBuffer> {
    if (!this.audioContext) {
      throw new Error('Audio context not initialized');
    }

    const arrayBuffer = await file.arrayBuffer();
    const audioBuffer = await this.audioContext.decodeAudioData(arrayBuffer);
    return audioBuffer;
  }

  /**
   * Add audio clip to a track
   */
  addClip(trackId: string, clip: AudioClip): void {
    const track = this.tracks.get(trackId);
    if (track) {
      track.clips.push(clip);
    }
  }

  /**
   * Remove clip from track
   */
  removeClip(trackId: string, clipId: string): void {
    const track = this.tracks.get(trackId);
    if (track) {
      track.clips = track.clips.filter(c => c.id !== clipId);
    }
  }

  /**
   * Set track volume
   */
  setTrackVolume(trackId: string, volume: number): void {
    const track = this.tracks.get(trackId);
    if (track) {
      track.volume = volume;
      track.gainNode.gain.setValueAtTime(volume, this.audioContext?.currentTime || 0);
    }
  }

  /**
   * Set track pan
   */
  setTrackPan(trackId: string, pan: number): void {
    const track = this.tracks.get(trackId);
    if (track) {
      track.pan = pan;
      track.panNode.pan.setValueAtTime(pan, this.audioContext?.currentTime || 0);
    }
  }

  /**
   * Mute/unmute track
   */
  setTrackMuted(trackId: string, muted: boolean): void {
    const track = this.tracks.get(trackId);
    if (track) {
      track.muted = muted;
      track.gainNode.gain.setValueAtTime(muted ? 0 : track.volume, this.audioContext?.currentTime || 0);
    }
  }

  /**
   * Solo/unsolo track
   */
  setTrackSolo(trackId: string, solo: boolean): void {
    const track = this.tracks.get(trackId);
    if (track) {
      track.solo = solo;
      this.updateSoloState();
    }
  }

  /**
   * Update solo state across all tracks
   */
  private updateSoloState(): void {
    const hasSolo = Array.from(this.tracks.values()).some(t => t.solo);

    this.tracks.forEach(track => {
      if (hasSolo) {
        // If any track is soloed, mute all non-solo tracks
        const shouldMute = !track.solo;
        track.gainNode.gain.setValueAtTime(
          shouldMute ? 0 : track.volume,
          this.audioContext?.currentTime || 0
        );
      } else {
        // No solo, restore original mute state
        track.gainNode.gain.setValueAtTime(
          track.muted ? 0 : track.volume,
          this.audioContext?.currentTime || 0
        );
      }
    });
  }

  /**
   * Schedule and play clips for all tracks
   */
  private scheduleClips(): void {
    if (!this.audioContext) return;

    const currentTime = this.audioContext.currentTime;
    const scheduleTo = this.currentPosition + this.scheduleAheadTime;

    this.tracks.forEach(track => {
      track.clips.forEach(clip => {
        if (!clip.buffer || clip.muted) return;

        const clipEndTime = clip.startTime + clip.duration;

        // Check if clip should be playing in the schedule window
        if (clip.startTime <= scheduleTo && clipEndTime >= this.currentPosition) {
          const sourceKey = `${track.id}-${clip.id}`;

          // Don't reschedule if already playing
          if (this.playingSources.has(sourceKey)) return;

          // Create source node
          const source = this.audioContext!.createBufferSource();
          source.buffer = clip.buffer;

          // Create clip gain node for clip-level control
          const clipGain = this.audioContext!.createGain();
          clipGain.gain.value = clip.gain;

          // Connect: source -> clip gain -> track gain
          source.connect(clipGain);
          clipGain.connect(track.gainNode);

          // Calculate when to start playback
          const startOffset = Math.max(0, this.currentPosition - clip.startTime);
          const whenToStart = currentTime + (clip.startTime - this.currentPosition);
          const duration = clip.duration - startOffset;

          // Schedule playback
          source.start(Math.max(currentTime, whenToStart), clip.offset + startOffset, duration);

          this.playingSources.set(sourceKey, source);

          // Clean up when finished
          source.onended = () => {
            this.playingSources.delete(sourceKey);
          };
        }
      });
    });
  }

  /**
   * Start playback
   */
  play(): void {
    if (!this.audioContext) {
      throw new Error('Audio context not initialized');
    }

    if (this.isPlaying) return;

    this.isPlaying = true;
    this.isPaused = false;
    this.startTime = this.audioContext.currentTime - this.currentPosition;

    // Start scheduling
    this.scheduleInterval = window.setInterval(() => {
      if (this.isPlaying && this.audioContext) {
        this.currentPosition = this.audioContext.currentTime - this.startTime;
        this.scheduleClips();
        this.updatePosition();
      }
    }, 25); // Check every 25ms

    this.onPlayStateChange?.(true);
  }

  /**
   * Pause playback
   */
  pause(): void {
    if (!this.isPlaying) return;

    this.isPlaying = false;
    this.isPaused = true;
    this.pauseTime = this.currentPosition;

    // Stop all playing sources
    this.stopAllSources();

    if (this.scheduleInterval !== null) {
      clearInterval(this.scheduleInterval);
      this.scheduleInterval = null;
    }

    this.onPlayStateChange?.(false);
  }

  /**
   * Stop playback and return to start
   */
  stop(): void {
    this.pause();
    this.seek(0);
    this.isPaused = false;
  }

  /**
   * Seek to position (in seconds)
   */
  seek(seconds: number): void {
    this.currentPosition = seconds;
    this.pauseTime = seconds;

    // Stop all currently playing sources
    this.stopAllSources();

    // If playing, update start time to maintain continuous playback
    if (this.isPlaying && this.audioContext) {
      this.startTime = this.audioContext.currentTime - seconds;
    }

    this.updatePosition();
  }

  /**
   * Stop all currently playing audio sources
   */
  private stopAllSources(): void {
    this.playingSources.forEach(source => {
      try {
        source.stop();
      } catch (e) {
        // Source may have already stopped
      }
    });
    this.playingSources.clear();
  }

  /**
   * Update position callback
   */
  private updatePosition(): void {
    const beatsPerSecond = this.tempo / 60;
    const totalBeats = this.currentPosition * beatsPerSecond;
    const bars = Math.floor(totalBeats / this.timeSignature.numerator);
    const beats = totalBeats % this.timeSignature.numerator;

    this.onPositionUpdate?.({
      seconds: this.currentPosition,
      bars,
      beats,
    });
  }

  /**
   * Set tempo
   */
  setTempo(bpm: number): void {
    this.tempo = bpm;
  }

  /**
   * Set time signature
   */
  setTimeSignature(numerator: number, denominator: number): void {
    this.timeSignature = { numerator, denominator };
  }

  /**
   * Set master volume
   */
  setMasterVolume(volume: number): void {
    if (this.masterGainNode && this.audioContext) {
      this.masterGainNode.gain.setValueAtTime(volume, this.audioContext.currentTime);
    }
  }

  /**
   * Get master analyser data for visualization
   */
  getMasterAnalyserData(): { timeDomain: Uint8Array; frequency: Uint8Array } {
    if (!this.masterAnalyserNode) {
      return {
        timeDomain: new Uint8Array(0),
        frequency: new Uint8Array(0),
      };
    }

    const bufferLength = this.masterAnalyserNode.frequencyBinCount;
    const timeDomain = new Uint8Array(bufferLength);
    const frequency = new Uint8Array(bufferLength);

    this.masterAnalyserNode.getByteTimeDomainData(timeDomain);
    this.masterAnalyserNode.getByteFrequencyData(frequency);

    return { timeDomain, frequency };
  }

  /**
   * Get track analyser data for visualization
   */
  getTrackAnalyserData(trackId: string): { timeDomain: Uint8Array; frequency: Uint8Array } | null {
    const track = this.tracks.get(trackId);
    if (!track) return null;

    const bufferLength = track.analyserNode.frequencyBinCount;
    const timeDomain = new Uint8Array(bufferLength);
    const frequency = new Uint8Array(bufferLength);

    track.analyserNode.getByteTimeDomainData(timeDomain);
    track.analyserNode.getByteFrequencyData(frequency);

    return { timeDomain, frequency };
  }

  /**
   * Set position update callback
   */
  setOnPositionUpdate(callback: (position: TransportPosition) => void): void {
    this.onPositionUpdate = callback;
  }

  /**
   * Set play state change callback
   */
  setOnPlayStateChange(callback: (isPlaying: boolean) => void): void {
    this.onPlayStateChange = callback;
  }

  /**
   * Get current state
   */
  getState() {
    return {
      isPlaying: this.isPlaying,
      isPaused: this.isPaused,
      currentPosition: this.currentPosition,
      tempo: this.tempo,
      timeSignature: this.timeSignature,
      tracks: Array.from(this.tracks.values()).map(track => ({
        id: track.id,
        name: track.name,
        volume: track.volume,
        pan: track.pan,
        muted: track.muted,
        solo: track.solo,
        clipCount: track.clips.length,
      })),
    };
  }

  /**
   * Cleanup and dispose
   */
  async dispose(): Promise<void> {
    this.stop();

    this.tracks.forEach(track => {
      track.gainNode.disconnect();
      track.panNode.disconnect();
      track.analyserNode.disconnect();
    });

    this.tracks.clear();

    if (this.masterGainNode) {
      this.masterGainNode.disconnect();
    }
    if (this.masterAnalyserNode) {
      this.masterAnalyserNode.disconnect();
    }

    if (this.audioContext) {
      await this.audioContext.close();
      this.audioContext = null;
    }
  }
}

// Create singleton instance
export const audioEngine = new AudioEngine();
