/**
 * Metronome Service using Web Audio API
 * Provides precise timing for metronome clicks with high/low beep sounds
 */

export class MetronomeService {
  private audioContext: AudioContext | null = null;
  private nextNoteTime: number = 0;
  private scheduleAheadTime: number = 0.1; // Schedule notes 100ms ahead
  private scheduleInterval: number = 25; // Check every 25ms
  private timerID: number | null = null;
  private currentBeat: number = 0;
  private isRunning: boolean = false;

  // Metronome settings
  private tempo: number = 120;
  private timeSignature: { numerator: number; denominator: number } = { numerator: 4, denominator: 4 };
  private volume: number = 0.5; // 0.0 to 1.0
  private enabled: boolean = false;

  // Pre-count settings
  private preCountBars: number = 0;
  private currentPreCountBeat: number = 0;
  private isInPreCount: boolean = false;
  private onPreCountComplete?: () => void;

  constructor() {
    // Audio context will be created on first use to avoid autoplay policy issues
  }

  /**
   * Initialize the audio context (must be called after user interaction)
   */
  private initAudioContext() {
    if (!this.audioContext) {
      this.audioContext = new AudioContext();
    }

    // Resume context if suspended (for browsers with autoplay restrictions)
    if (this.audioContext.state === 'suspended') {
      this.audioContext.resume();
    }
  }

  /**
   * Play a metronome click sound
   * @param isAccent - true for beat 1 (high pitch), false for other beats (low pitch)
   * @param time - scheduled time to play the sound
   */
  private playClick(isAccent: boolean, time: number) {
    if (!this.audioContext || !this.enabled) return;

    const osc = this.audioContext.createOscillator();
    const gainNode = this.audioContext.createGain();

    // High beep for accent (beat 1), low beep for other beats
    osc.frequency.value = isAccent ? 1000 : 800;

    // Short click sound
    gainNode.gain.value = this.volume;
    gainNode.gain.exponentialRampToValueAtTime(0.01, time + 0.05);

    osc.connect(gainNode);
    gainNode.connect(this.audioContext.destination);

    osc.start(time);
    osc.stop(time + 0.05);
  }

  /**
   * Schedule metronome clicks ahead of time
   */
  private scheduler() {
    if (!this.audioContext) return;

    // Schedule all notes that need to play before the next callback
    while (this.nextNoteTime < this.audioContext.currentTime + this.scheduleAheadTime) {
      const secondsPerBeat = 60.0 / this.tempo;

      if (this.isInPreCount) {
        // During pre-count
        const isAccent = this.currentPreCountBeat % this.timeSignature.numerator === 0;
        this.playClick(isAccent, this.nextNoteTime);

        this.currentPreCountBeat++;
        const totalPreCountBeats = this.preCountBars * this.timeSignature.numerator;

        if (this.currentPreCountBeat >= totalPreCountBeats) {
          // Pre-count complete
          this.isInPreCount = false;
          this.currentBeat = 0;
          if (this.onPreCountComplete) {
            setTimeout(() => this.onPreCountComplete?.(), 0);
          }
        }
      } else {
        // Normal metronome
        const isAccent = this.currentBeat % this.timeSignature.numerator === 0;
        this.playClick(isAccent, this.nextNoteTime);
        this.currentBeat++;
      }

      this.nextNoteTime += secondsPerBeat;
    }

    // Continue scheduling
    if (this.isRunning) {
      this.timerID = window.setTimeout(() => this.scheduler(), this.scheduleInterval);
    }
  }

  /**
   * Start the metronome
   */
  start() {
    if (this.isRunning) return;

    this.initAudioContext();

    if (!this.audioContext) return;

    this.isRunning = true;
    this.currentBeat = 0;
    this.nextNoteTime = this.audioContext.currentTime;
    this.scheduler();
  }

  /**
   * Start metronome with pre-count
   * @param bars - number of bars to count before starting
   * @param onComplete - callback when pre-count is complete
   */
  startWithPreCount(bars: number, onComplete?: () => void) {
    if (this.isRunning) return;

    this.initAudioContext();

    if (!this.audioContext) return;

    this.isRunning = true;
    this.isInPreCount = true;
    this.preCountBars = bars;
    this.currentPreCountBeat = 0;
    this.currentBeat = 0;
    this.onPreCountComplete = onComplete;
    this.nextNoteTime = this.audioContext.currentTime;
    this.scheduler();
  }

  /**
   * Stop the metronome
   */
  stop() {
    this.isRunning = false;
    this.isInPreCount = false;
    this.currentBeat = 0;
    this.currentPreCountBeat = 0;

    if (this.timerID !== null) {
      clearTimeout(this.timerID);
      this.timerID = null;
    }
  }

  /**
   * Set tempo in BPM
   */
  setTempo(bpm: number) {
    this.tempo = Math.max(20, Math.min(999, bpm));
  }

  /**
   * Set time signature
   */
  setTimeSignature(numerator: number, denominator: number) {
    this.timeSignature = { numerator, denominator };
  }

  /**
   * Set volume (0.0 to 1.0)
   */
  setVolume(volume: number) {
    this.volume = Math.max(0, Math.min(1, volume));
  }

  /**
   * Enable or disable metronome
   */
  setEnabled(enabled: boolean) {
    this.enabled = enabled;
    if (!enabled && this.isRunning) {
      // If metronome is disabled while running, we keep the scheduler
      // running but stop playing sounds
    }
  }

  /**
   * Get current settings
   */
  getSettings() {
    return {
      tempo: this.tempo,
      timeSignature: this.timeSignature,
      volume: this.volume,
      enabled: this.enabled,
      isRunning: this.isRunning,
      isInPreCount: this.isInPreCount,
    };
  }

  /**
   * Cleanup and release resources
   */
  dispose() {
    this.stop();
    if (this.audioContext) {
      this.audioContext.close();
      this.audioContext = null;
    }
  }
}

// Create a singleton instance
export const metronome = new MetronomeService();
