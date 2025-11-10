/**
 * Playback Engine
 * Sample-accurate audio and MIDI playback with lookahead scheduling
 *
 * Based on Web Audio API best practices:
 * - Lookahead scheduling (25ms check interval, 0.1s lookahead)
 * - Sample-accurate timing using AudioContext.currentTime
 * - Separate scheduler and audio threads for perfect timing
 */

import { useAudioStore } from '../store/audioStore';
import { AudioClip, MIDIClip, MIDINote } from '../types/recording';

interface ScheduledEvent {
  id: string;
  type: 'audio' | 'midi' | 'metronome';
  time: number; // AudioContext time
  beat: number; // Beat position
  data: any;
}

export class PlaybackEngine {
  private static instance: PlaybackEngine | null = null;

  // Scheduling constants (based on Web Audio best practices)
  private readonly LOOKAHEAD_TIME = 0.1; // Schedule 100ms ahead
  private readonly SCHEDULE_INTERVAL = 25; // Check every 25ms

  // State
  private schedulerInterval: number | null = null;
  private nextNoteTime = 0;
  private currentBeatInScheduler = 0;
  private scheduledEvents: ScheduledEvent[] = [];
  private activeAudioSources: Map<string, AudioBufferSourceNode> = new Map();
  private activeMIDINotes: Map<string, { oscillator: OscillatorNode; gain: GainNode }> = new Map();

  // Metronome
  private metronomeEnabled = false;
  private metronomeTick: AudioBuffer | null = null;

  private constructor() {}

  static getInstance(): PlaybackEngine {
    if (!PlaybackEngine.instance) {
      PlaybackEngine.instance = new PlaybackEngine();
    }
    return PlaybackEngine.instance;
  }

  /**
   * Start playback scheduler
   */
  start(): void {
    const store = useAudioStore.getState();
    const context = store.audioContext.context;

    if (!context) {
      console.error('AudioContext not initialized');
      return;
    }

    // Initialize scheduler timing
    this.nextNoteTime = context.currentTime;
    this.currentBeatInScheduler = store.currentBeat;

    // Start lookahead scheduler
    this.schedulerInterval = window.setInterval(() => {
      this.scheduler();
    }, this.SCHEDULE_INTERVAL);

    console.log('Playback started');
  }

  /**
   * Stop playback
   */
  stop(): void {
    if (this.schedulerInterval !== null) {
      clearInterval(this.schedulerInterval);
      this.schedulerInterval = null;
    }

    // Stop all active audio sources
    this.activeAudioSources.forEach((source) => {
      try {
        source.stop();
      } catch (e) {
        // Already stopped
      }
    });
    this.activeAudioSources.clear();

    // Stop all active MIDI notes
    this.activeMIDINotes.forEach(({ oscillator }) => {
      try {
        oscillator.stop();
      } catch (e) {
        // Already stopped
      }
    });
    this.activeMIDINotes.clear();

    // Clear scheduled events
    this.scheduledEvents = [];

    console.log('Playback stopped');
  }

  /**
   * Lookahead scheduler (called every 25ms)
   * Schedules events that fall within the lookahead window
   */
  private scheduler(): void {
    const store = useAudioStore.getState();
    const context = store.audioContext.context;

    if (!context || !store.isPlaying) {
      return;
    }

    // Schedule all events within the lookahead window
    while (this.nextNoteTime < context.currentTime + this.LOOKAHEAD_TIME) {
      this.scheduleEventsAtBeat(this.currentBeatInScheduler, this.nextNoteTime);

      // Advance time and beat
      this.advanceScheduler(store.tempo);
    }

    // Update current beat in store (for UI)
    const actualBeat = this.getCurrentBeat(context.currentTime, store.tempo);
    if (Math.abs(actualBeat - store.currentBeat) > 0.01) {
      store.setCurrentBeat(actualBeat);
    }
  }

  /**
   * Schedule all events that occur at a specific beat
   */
  private scheduleEventsAtBeat(beat: number, time: number): void {
    const store = useAudioStore.getState();

    // Schedule metronome
    if (this.metronomeEnabled) {
      this.scheduleMetronomeClick(beat, time);
    }

    // Schedule audio clips
    this.scheduleAudioClips(beat, time);

    // Schedule MIDI clips
    this.scheduleMIDIClips(beat, time);
  }

  /**
   * Schedule metronome click
   */
  private scheduleMetronomeClick(beat: number, time: number): void {
    const store = useAudioStore.getState();
    const context = store.audioContext.context;

    if (!context) return;

    const beatInBar = beat % store.timeSignature.numerator;
    const isDownbeat = Math.abs(beatInBar) < 0.01;

    // Create oscillator for click (simple beep)
    const osc = context.createOscillator();
    const gain = context.createGain();

    osc.frequency.value = isDownbeat ? 1000 : 800; // Higher pitch on downbeat
    osc.connect(gain);
    gain.connect(context.destination);

    // Envelope
    gain.gain.setValueAtTime(0.3, time);
    gain.gain.exponentialRampToValueAtTime(0.01, time + 0.05);

    osc.start(time);
    osc.stop(time + 0.05);
  }

  /**
   * Schedule audio clips
   */
  private scheduleAudioClips(beat: number, time: number): void {
    const store = useAudioStore.getState();
    const context = store.audioContext.context;

    if (!context) return;

    store.audioClips.forEach((clip) => {
      // Check if clip should start at this beat
      if (Math.abs(clip.start - beat) < 0.01) {
        this.playAudioClip(clip, time);
      }
    });
  }

  /**
   * Play audio clip at specified time
   */
  private playAudioClip(clip: AudioClip, startTime: number): void {
    const store = useAudioStore.getState();
    const context = store.audioContext.context;

    if (!context || !clip.audioBuffer) {
      return;
    }

    const track = store.tracks.find((t) => t.id === clip.trackId);
    if (!track || track.muted) {
      return;
    }

    // Check solo mode
    const hasSolo = store.tracks.some((t) => t.solo);
    if (hasSolo && !track.solo) {
      return;
    }

    // Create audio source
    const source = context.createBufferSource();
    source.buffer = clip.audioBuffer;

    // Create gain node for clip
    const clipGain = context.createGain();
    clipGain.gain.value = clip.gain;

    // Create gain node for track
    const trackGain = context.createGain();
    trackGain.gain.value = track.volume;

    // Create pan node
    const panner = context.createStereoPanner();
    panner.pan.value = track.pan;

    // Connect: source -> clipGain -> trackGain -> panner -> destination
    source.connect(clipGain);
    clipGain.connect(trackGain);
    trackGain.connect(panner);
    panner.connect(context.destination);

    // Apply fade in
    if (clip.fadeIn > 0) {
      const fadeInDuration = this.beatsToSeconds(clip.fadeIn, store.tempo);
      clipGain.gain.setValueAtTime(0, startTime);
      clipGain.gain.linearRampToValueAtTime(clip.gain, startTime + fadeInDuration);
    }

    // Apply fade out
    if (clip.fadeOut > 0) {
      const clipDuration = clip.audioBuffer.duration;
      const fadeOutStart = startTime + clipDuration - this.beatsToSeconds(clip.fadeOut, store.tempo);
      clipGain.gain.setValueAtTime(clip.gain, fadeOutStart);
      clipGain.gain.linearRampToValueAtTime(0, startTime + clipDuration);
    }

    // Start playback
    source.start(startTime, clip.offset);

    // Store reference
    this.activeAudioSources.set(clip.id, source);

    // Clean up when finished
    source.onended = () => {
      this.activeAudioSources.delete(clip.id);
    };
  }

  /**
   * Schedule MIDI clips
   */
  private scheduleMIDIClips(beat: number, time: number): void {
    const store = useAudioStore.getState();

    store.midiClips.forEach((clip) => {
      // Check if any notes in this clip should trigger at this beat
      clip.notes.forEach((note) => {
        const noteBeat = clip.start + note.start;

        // Note On
        if (Math.abs(noteBeat - beat) < 0.01) {
          this.playMIDINote(clip.trackId, note, time);
        }

        // Note Off
        const noteOffBeat = noteBeat + note.length;
        if (Math.abs(noteOffBeat - beat) < 0.01) {
          this.stopMIDINote(note.id);
        }
      });
    });
  }

  /**
   * Play MIDI note
   */
  private playMIDINote(trackId: string, note: MIDINote, startTime: number): void {
    const store = useAudioStore.getState();
    const context = store.audioContext.context;

    if (!context) return;

    const track = store.tracks.find((t) => t.id === trackId);
    if (!track || track.muted || note.muted) {
      return;
    }

    // Check solo mode
    const hasSolo = store.tracks.some((t) => t.solo);
    if (hasSolo && !track.solo) {
      return;
    }

    // Convert MIDI note to frequency
    const frequency = this.midiNoteToFrequency(note.pitch);

    // Create oscillator
    const osc = context.createOscillator();
    osc.type = 'triangle'; // Simple synth sound
    osc.frequency.value = frequency;

    // Create gain envelope (ADSR-lite)
    const gain = context.createGain();
    const velocity = note.velocity / 127;
    const maxGain = velocity * track.volume * 0.3; // Scale down volume

    // Attack (10ms)
    gain.gain.setValueAtTime(0, startTime);
    gain.gain.linearRampToValueAtTime(maxGain, startTime + 0.01);

    // Sustain (hold at max)
    const duration = this.beatsToSeconds(note.length, store.tempo);
    gain.gain.setValueAtTime(maxGain, startTime + 0.01);

    // Release (50ms)
    gain.gain.setValueAtTime(maxGain, startTime + duration - 0.05);
    gain.gain.exponentialRampToValueAtTime(0.001, startTime + duration);

    // Apply track pan
    const panner = context.createStereoPanner();
    panner.pan.value = track.pan;

    // Connect: osc -> gain -> panner -> destination
    osc.connect(gain);
    gain.connect(panner);
    panner.connect(context.destination);

    // Start and stop
    osc.start(startTime);
    osc.stop(startTime + duration);

    // Store reference
    this.activeMIDINotes.set(note.id, { oscillator: osc, gain });

    // Clean up when finished
    osc.onended = () => {
      this.activeMIDINotes.delete(note.id);
    };
  }

  /**
   * Stop MIDI note early (for note-off events)
   */
  private stopMIDINote(noteId: string): void {
    const noteData = this.activeMIDINotes.get(noteId);
    if (noteData) {
      const context = useAudioStore.getState().audioContext.context;
      if (context) {
        // Quick release
        noteData.gain.gain.cancelScheduledValues(context.currentTime);
        noteData.gain.gain.setValueAtTime(noteData.gain.gain.value, context.currentTime);
        noteData.gain.gain.exponentialRampToValueAtTime(0.001, context.currentTime + 0.05);

        try {
          noteData.oscillator.stop(context.currentTime + 0.05);
        } catch (e) {
          // Already stopped
        }
      }
    }
  }

  /**
   * Convert MIDI note number to frequency (Hz)
   * Formula: f = 440 * 2^((n-69)/12)
   * Where 69 = A4 = 440Hz
   */
  private midiNoteToFrequency(note: number): number {
    return Math.pow(2, (note - 69) / 12) * 440.0;
  }

  /**
   * Advance scheduler to next beat
   */
  private advanceScheduler(tempo: number): void {
    const secondsPerBeat = 60.0 / tempo;
    const store = useAudioStore.getState();

    this.nextNoteTime += secondsPerBeat;
    this.currentBeatInScheduler += 1;

    // Handle looping
    if (store.looping) {
      const loopLength = store.loopEnd - store.loopStart;
      if (this.currentBeatInScheduler >= store.loopEnd) {
        this.currentBeatInScheduler = store.loopStart;
      }
    }
  }

  /**
   * Calculate current beat from audio context time
   */
  private getCurrentBeat(currentTime: number, tempo: number): number {
    const store = useAudioStore.getState();
    const secondsPerBeat = 60.0 / tempo;
    const elapsedBeats = (currentTime - (this.nextNoteTime - this.beatsToSeconds(1, tempo))) / secondsPerBeat;

    let beat = this.currentBeatInScheduler + elapsedBeats;

    // Handle looping
    if (store.looping) {
      const loopLength = store.loopEnd - store.loopStart;
      if (beat >= store.loopEnd) {
        beat = store.loopStart + ((beat - store.loopStart) % loopLength);
      }
    }

    return beat;
  }

  /**
   * Convert beats to seconds
   */
  private beatsToSeconds(beats: number, tempo: number): number {
    return (beats * 60.0) / tempo;
  }

  /**
   * Enable/disable metronome
   */
  setMetronomeEnabled(enabled: boolean): void {
    this.metronomeEnabled = enabled;
  }

  /**
   * Seek to specific beat position
   */
  seekToBeat(beat: number): void {
    const store = useAudioStore.getState();
    const context = store.audioContext.context;

    if (!context) return;

    // Stop all currently playing audio
    this.activeAudioSources.forEach((source) => {
      try {
        source.stop();
      } catch (e) {}
    });
    this.activeAudioSources.clear();

    this.activeMIDINotes.forEach(({ oscillator }) => {
      try {
        oscillator.stop();
      } catch (e) {}
    });
    this.activeMIDINotes.clear();

    // Reset scheduler
    this.currentBeatInScheduler = beat;
    this.nextNoteTime = context.currentTime;

    // Update store
    store.setCurrentBeat(beat);
  }

  /**
   * Trigger a MIDI clip immediately (for Session View)
   * Returns a clip ID that can be used to stop the clip
   */
  triggerMIDIClip(
    trackId: string,
    notes: MIDINote[],
    clipLength: number,
    loopEnabled: boolean = true,
    clipId?: string
  ): string {
    const store = useAudioStore.getState();
    const context = store.audioContext.context;

    if (!context || notes.length === 0) {
      return clipId || `clip-${Date.now()}`;
    }

    const generatedClipId = clipId || `session-clip-${trackId}-${Date.now()}`;
    const startTime = context.currentTime;

    // Schedule all notes in the clip
    const scheduleNotes = () => {
      const currentTime = context.currentTime;
      notes.forEach((note) => {
        const noteStartTime = currentTime + this.beatsToSeconds(note.start, store.tempo);
        this.playMIDINote(trackId, note, noteStartTime);
      });
    };

    // Play notes immediately
    scheduleNotes();

    // If looping is enabled, schedule the clip to loop
    if (loopEnabled) {
      const clipDuration = this.beatsToSeconds(clipLength, store.tempo);
      const loopClip = () => {
        const isStillActive = this.activeSessionClips.has(generatedClipId);
        if (isStillActive) {
          scheduleNotes();
          setTimeout(loopClip, clipDuration * 1000);
        }
      };

      // Schedule the first loop
      setTimeout(loopClip, clipDuration * 1000);
      this.activeSessionClips.set(generatedClipId, { trackId, startTime, loopEnabled });
    }

    return generatedClipId;
  }

  /**
   * Trigger an audio clip immediately (for Session View)
   */
  triggerAudioClip(
    trackId: string,
    audioBuffer: AudioBuffer,
    clipLength: number,
    loopEnabled: boolean = true,
    clipId?: string
  ): string {
    const store = useAudioStore.getState();
    const context = store.audioContext.context;

    if (!context) {
      return clipId || `clip-${Date.now()}`;
    }

    const generatedClipId = clipId || `session-clip-${trackId}-${Date.now()}`;
    const startTime = context.currentTime;

    const playAudio = () => {
      const track = store.tracks.find((t) => t.id === trackId);
      if (!track || track.muted) {
        return null;
      }

      // Check solo mode
      const hasSolo = store.tracks.some((t) => t.solo);
      if (hasSolo && !track.solo) {
        return null;
      }

      // Create audio source
      const source = context.createBufferSource();
      source.buffer = audioBuffer;
      source.loop = false; // We handle looping manually

      // Create gain node for track
      const trackGain = context.createGain();
      trackGain.gain.value = track.volume;

      // Create pan node
      const panner = context.createStereoPanner();
      panner.pan.value = track.pan;

      // Connect: source -> trackGain -> panner -> destination
      source.connect(trackGain);
      trackGain.connect(panner);
      panner.connect(context.destination);

      // Start playback
      source.start(context.currentTime);

      return source;
    };

    // Play audio immediately
    const source = playAudio();

    if (loopEnabled && source) {
      const clipDuration = this.beatsToSeconds(clipLength, store.tempo);
      const loopAudio = () => {
        const isStillActive = this.activeSessionClips.has(generatedClipId);
        if (isStillActive) {
          playAudio();
          setTimeout(loopAudio, clipDuration * 1000);
        }
      };

      // Schedule the loop
      setTimeout(loopAudio, clipDuration * 1000);
      this.activeSessionClips.set(generatedClipId, { trackId, startTime, loopEnabled });
    }

    return generatedClipId;
  }

  /**
   * Stop a session clip
   */
  stopSessionClip(clipId: string): void {
    this.activeSessionClips.delete(clipId);
    // Note: Individual notes will stop naturally based on their durations
    // Audio sources will also complete naturally
  }

  /**
   * Get all active session clips
   */
  getActiveSessionClips(): string[] {
    return Array.from(this.activeSessionClips.keys());
  }

  // Track active session clips
  private activeSessionClips: Map<string, { trackId: string; startTime: number; loopEnabled: boolean }> = new Map();
}

export const playbackEngine = PlaybackEngine.getInstance();
