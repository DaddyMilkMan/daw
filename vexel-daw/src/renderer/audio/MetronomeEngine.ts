/**
 * Metronome Engine
 * Professional metronome with precise Web Audio scheduling
 * Implements look-ahead scheduling pattern for rock-solid timing
 */

import { MetronomeSettings, MetronomeState, MIDIClockSettings } from '../types/metronome';
import { MetronomeSounds } from './MetronomeSounds';
import { MIDIClock } from './MIDIClock';

const SCHEDULE_AHEAD_TIME = 0.1; // How far ahead to schedule (100ms)
const SCHEDULER_INTERVAL = 25; // How often to run scheduler (25ms)

export type MetronomeCallback = (state: MetronomeState) => void;

export class MetronomeEngine {
  private audioContext: AudioContext;
  private sounds: MetronomeSounds;
  private midiClock: MIDIClock;

  private settings: MetronomeSettings;
  private midiSettings: MIDIClockSettings;

  private state: MetronomeState;
  private schedulerWorker: number | null = null;

  private tempo: number = 120;
  private timeSignature: { numerator: number; denominator: number } = { numerator: 4, denominator: 4 };

  private isRecording: boolean = false;
  private transportPlaying: boolean = false;

  private callbacks: MetronomeCallback[] = [];

  constructor(
    audioContext: AudioContext,
    settings: MetronomeSettings,
    midiSettings: MIDIClockSettings
  ) {
    this.audioContext = audioContext;
    this.settings = settings;
    this.midiSettings = midiSettings;

    this.sounds = new MetronomeSounds(audioContext);
    this.midiClock = new MIDIClock(midiSettings);

    this.state = {
      isPlaying: false,
      isInPreCount: false,
      preCountBeat: 0,
      currentBeat: 0,
      nextClickTime: 0,
    };
  }

  /**
   * Start the metronome
   */
  start(tempo: number, timeSignature: { numerator: number; denominator: number }, isRecording: boolean = false): void {
    this.tempo = tempo;
    this.timeSignature = timeSignature;
    this.isRecording = isRecording;
    this.transportPlaying = true;

    // Check if metronome should play
    if (!this.shouldPlay()) {
      return;
    }

    // Check if we should do pre-count
    const shouldPreCount = this.settings.preCount.enabled &&
      (!this.settings.preCount.onlyWhenRecording || isRecording);

    if (shouldPreCount) {
      this.startPreCount();
    } else {
      this.startNormal();
    }
  }

  /**
   * Start pre-count
   */
  private startPreCount(): void {
    this.state.isPlaying = true;
    this.state.isInPreCount = true;
    this.state.preCountBeat = 0;
    this.state.currentBeat = 0;
    this.state.nextClickTime = this.audioContext.currentTime;

    console.log(`🎵 Starting metronome pre-count: ${this.settings.preCount.bars} bars`);

    this.notifyCallbacks();
    this.startScheduler();

    // MIDI Clock - Send Start
    if (this.midiSettings.enabled && this.midiSettings.sendStart) {
      this.midiClock.sendStart();
    }
  }

  /**
   * Start normal playback (no pre-count)
   */
  private startNormal(): void {
    this.state.isPlaying = true;
    this.state.isInPreCount = false;
    this.state.currentBeat = 0;
    this.state.nextClickTime = this.audioContext.currentTime;

    console.log('🎵 Starting metronome');

    this.notifyCallbacks();
    this.startScheduler();

    // MIDI Clock - Send Start
    if (this.midiSettings.enabled && this.midiSettings.sendStart) {
      this.midiClock.sendStart();
    }
  }

  /**
   * Stop the metronome
   */
  stop(): void {
    this.transportPlaying = false;

    if (!this.state.isPlaying) {
      return;
    }

    this.state.isPlaying = false;
    this.state.isInPreCount = false;
    this.state.currentBeat = 0;
    this.state.preCountBeat = 0;

    console.log('⏹️ Stopping metronome');

    this.stopScheduler();
    this.notifyCallbacks();

    // MIDI Clock - Send Stop
    if (this.midiSettings.enabled && this.midiSettings.sendStop) {
      this.midiClock.sendStop();
    }
  }

  /**
   * Check if metronome should play based on settings
   */
  private shouldPlay(): boolean {
    if (!this.settings.enabled) {
      return false;
    }

    if (this.isRecording) {
      return this.settings.clickDuringRecording;
    } else {
      return this.settings.clickDuringPlayback;
    }
  }

  /**
   * Start the scheduler
   */
  private startScheduler(): void {
    if (this.schedulerWorker !== null) {
      return;
    }

    this.schedulerWorker = window.setInterval(() => {
      this.schedule();
    }, SCHEDULER_INTERVAL);
  }

  /**
   * Stop the scheduler
   */
  private stopScheduler(): void {
    if (this.schedulerWorker !== null) {
      clearInterval(this.schedulerWorker);
      this.schedulerWorker = null;
    }
  }

  /**
   * Schedule metronome clicks using look-ahead
   */
  private schedule(): void {
    const currentTime = this.audioContext.currentTime;
    const lookaheadTime = currentTime + SCHEDULE_AHEAD_TIME;

    while (this.state.nextClickTime < lookaheadTime) {
      this.scheduleClick(this.state.nextClickTime);
      this.advanceBeat();
    }
  }

  /**
   * Schedule a single click
   */
  private scheduleClick(time: number): void {
    const isAccent = this.isAccentedBeat();

    // Play sound if appropriate
    if (this.state.isInPreCount && this.settings.clickDuringCountIn) {
      this.sounds.playClick(time, isAccent, this.settings);
    } else if (!this.state.isInPreCount) {
      this.sounds.playClick(time, isAccent, this.settings);
    }

    // MIDI Clock - Send clock pulses (24 PPQN)
    if (this.midiSettings.enabled) {
      this.midiClock.sendClockPulse(time, this.tempo);
    }
  }

  /**
   * Check if current beat should be accented
   */
  private isAccentedBeat(): boolean {
    if (!this.settings.accentDownbeat) {
      return false;
    }

    const beat = this.state.isInPreCount ? this.state.preCountBeat : this.state.currentBeat;

    // Downbeat (beat 0) is always accented
    if (beat === 0) {
      return true;
    }

    // Custom grouping for odd meters
    if (this.settings.accentGrouping.length > 0) {
      let groupStart = 0;
      for (const groupSize of this.settings.accentGrouping) {
        if (beat === groupStart) {
          return true;
        }
        groupStart += groupSize;
      }
    }

    return false;
  }

  /**
   * Advance to next beat
   */
  private advanceBeat(): void {
    const beatsPerBar = this.timeSignature.numerator;
    const secondsPerBeat = 60.0 / this.tempo;

    // Advance time
    this.state.nextClickTime += secondsPerBeat;

    if (this.state.isInPreCount) {
      this.state.preCountBeat++;

      const totalPreCountBeats = this.settings.preCount.bars * beatsPerBar;

      if (this.state.preCountBeat >= totalPreCountBeats) {
        // Pre-count finished, switch to normal playback
        this.state.isInPreCount = false;
        this.state.preCountBeat = 0;
        this.state.currentBeat = 0;
        console.log('✅ Pre-count finished, starting normal playback');
        this.notifyCallbacks();
      } else {
        this.notifyCallbacks();
      }
    } else {
      this.state.currentBeat++;

      if (this.state.currentBeat >= beatsPerBar) {
        this.state.currentBeat = 0;
      }

      this.notifyCallbacks();
    }
  }

  /**
   * Update settings
   */
  updateSettings(settings: Partial<MetronomeSettings>): void {
    this.settings = { ...this.settings, ...settings };
    this.sounds.setVolume(this.settings.volume);
  }

  /**
   * Update MIDI settings
   */
  updateMIDISettings(settings: Partial<MIDIClockSettings>): void {
    this.midiSettings = { ...this.midiSettings, ...settings };
    this.midiClock.updateSettings(this.midiSettings);
  }

  /**
   * Update tempo
   */
  setTempo(tempo: number): void {
    this.tempo = tempo;
    // MIDI clock will automatically adjust based on tempo in sendClockPulse
  }

  /**
   * Update time signature
   */
  setTimeSignature(timeSignature: { numerator: number; denominator: number }): void {
    this.timeSignature = timeSignature;
  }

  /**
   * Load custom sound
   */
  async loadCustomSound(audioData: ArrayBuffer): Promise<void> {
    await this.sounds.loadCustomSound(audioData);
  }

  /**
   * Register callback for state changes
   */
  onStateChange(callback: MetronomeCallback): () => void {
    this.callbacks.push(callback);
    return () => {
      const index = this.callbacks.indexOf(callback);
      if (index > -1) {
        this.callbacks.splice(index, 1);
      }
    };
  }

  /**
   * Notify all callbacks of state change
   */
  private notifyCallbacks(): void {
    this.callbacks.forEach(callback => callback({ ...this.state }));
  }

  /**
   * Get current state
   */
  getState(): MetronomeState {
    return { ...this.state };
  }

  /**
   * Clean up resources
   */
  destroy(): void {
    this.stop();
    this.sounds.destroy();
    this.midiClock.destroy();
    this.callbacks = [];
  }
}
