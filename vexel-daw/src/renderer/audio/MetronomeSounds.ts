/**
 * Metronome Sound Synthesizer
 * Professional-grade metronome sounds using Web Audio API
 * Supports wood, beep, click, and custom samples
 */

import { MetronomeSoundType, MetronomeSettings } from '../types/metronome';

export class MetronomeSounds {
  private audioContext: AudioContext;
  private customSoundBuffer: AudioBuffer | null = null;
  private masterGain: GainNode;

  constructor(audioContext: AudioContext) {
    this.audioContext = audioContext;
    this.masterGain = audioContext.createGain();
    this.masterGain.connect(audioContext.destination);
  }

  /**
   * Play a metronome click
   * @param time - AudioContext time to play the sound
   * @param isAccent - Whether this is an accented beat (downbeat)
   * @param settings - Metronome settings
   */
  playClick(time: number, isAccent: boolean, settings: MetronomeSettings): void {
    const volume = settings.volume;

    switch (settings.soundType) {
      case 'wood':
        this.playWoodSound(time, isAccent, settings, volume);
        break;
      case 'beep':
        this.playBeepSound(time, isAccent, settings, volume);
        break;
      case 'click':
        this.playClickSound(time, isAccent, settings, volume);
        break;
      case 'custom':
        if (this.customSoundBuffer) {
          this.playCustomSound(time, isAccent, volume);
        } else {
          // Fallback to wood if custom sound not loaded
          this.playWoodSound(time, isAccent, settings, volume);
        }
        break;
    }
  }

  /**
   * Woodblock sound - warm, organic click
   * Uses filtered noise and resonant filter ping
   */
  private playWoodSound(
    time: number,
    isAccent: boolean,
    settings: MetronomeSettings,
    volume: number
  ): void {
    const { highFreq, lowFreq } = settings.sounds.wood;
    const freq = isAccent ? highFreq : lowFreq;

    // Create filtered noise burst for woody character
    const noise = this.audioContext.createBufferSource();
    const noiseBuffer = this.createNoiseBuffer(0.01, 0.3); // Short burst
    noise.buffer = noiseBuffer;

    // Bandpass filter for woodblock character
    const filter = this.audioContext.createBiquadFilter();
    filter.type = 'bandpass';
    filter.frequency.value = freq;
    filter.Q.value = 20; // High resonance for "ping"

    // Envelope
    const envelope = this.audioContext.createGain();
    envelope.gain.setValueAtTime(0, time);
    envelope.gain.linearRampToValueAtTime(volume * (isAccent ? 1.0 : 0.7), time + 0.001);
    envelope.gain.exponentialRampToValueAtTime(0.001, time + 0.05);

    // Add a bit of tone for the "knock"
    const osc = this.audioContext.createOscillator();
    osc.type = 'sine';
    osc.frequency.value = freq * 0.8;

    const oscEnv = this.audioContext.createGain();
    oscEnv.gain.setValueAtTime(0, time);
    oscEnv.gain.linearRampToValueAtTime(volume * 0.3 * (isAccent ? 1.0 : 0.6), time + 0.001);
    oscEnv.gain.exponentialRampToValueAtTime(0.001, time + 0.03);

    // Connect noise path
    noise.connect(filter);
    filter.connect(envelope);
    envelope.connect(this.masterGain);

    // Connect tone path
    osc.connect(oscEnv);
    oscEnv.connect(this.masterGain);

    // Start and stop
    noise.start(time);
    noise.stop(time + 0.05);
    osc.start(time);
    osc.stop(time + 0.03);
  }

  /**
   * Beep sound - clean, tonal click
   * Classic digital metronome sound
   */
  private playBeepSound(
    time: number,
    isAccent: boolean,
    settings: MetronomeSettings,
    volume: number
  ): void {
    const { highFreq, lowFreq } = settings.sounds.beep;
    const freq = isAccent ? highFreq : lowFreq;

    const osc = this.audioContext.createOscillator();
    osc.type = 'sine';
    osc.frequency.value = freq;

    const envelope = this.audioContext.createGain();
    envelope.gain.setValueAtTime(0, time);
    envelope.gain.linearRampToValueAtTime(volume * (isAccent ? 1.0 : 0.6), time + 0.002);
    envelope.gain.exponentialRampToValueAtTime(0.001, time + 0.04);

    osc.connect(envelope);
    envelope.connect(this.masterGain);

    osc.start(time);
    osc.stop(time + 0.04);
  }

  /**
   * Click sound - sharp, percussive
   * Short burst with high-frequency content
   */
  private playClickSound(
    time: number,
    isAccent: boolean,
    settings: MetronomeSettings,
    volume: number
  ): void {
    const { highFreq, lowFreq } = settings.sounds.click;
    const freq = isAccent ? highFreq : lowFreq;

    // Very short noise burst
    const noise = this.audioContext.createBufferSource();
    const noiseBuffer = this.createNoiseBuffer(0.003, 1.0); // Very short
    noise.buffer = noiseBuffer;

    // High-pass filter for click character
    const filter = this.audioContext.createBiquadFilter();
    filter.type = 'highpass';
    filter.frequency.value = freq;
    filter.Q.value = 1;

    // Very short envelope
    const envelope = this.audioContext.createGain();
    envelope.gain.setValueAtTime(0, time);
    envelope.gain.linearRampToValueAtTime(volume * (isAccent ? 1.0 : 0.5), time + 0.0005);
    envelope.gain.exponentialRampToValueAtTime(0.001, time + 0.01);

    noise.connect(filter);
    filter.connect(envelope);
    envelope.connect(this.masterGain);

    noise.start(time);
    noise.stop(time + 0.01);
  }

  /**
   * Play custom user-uploaded sound
   */
  private playCustomSound(time: number, isAccent: boolean, volume: number): void {
    if (!this.customSoundBuffer) return;

    const source = this.audioContext.createBufferSource();
    source.buffer = this.customSoundBuffer;

    const gain = this.audioContext.createGain();
    gain.gain.value = volume * (isAccent ? 1.0 : 0.7);

    // Optional pitch shift for accent
    if (isAccent) {
      source.playbackRate.value = 1.1; // Slightly higher pitch for accent
    }

    source.connect(gain);
    gain.connect(this.masterGain);

    source.start(time);
  }

  /**
   * Load a custom metronome sound from a file
   */
  async loadCustomSound(audioData: ArrayBuffer): Promise<void> {
    try {
      this.customSoundBuffer = await this.audioContext.decodeAudioData(audioData);
      console.log('✅ Custom metronome sound loaded');
    } catch (error) {
      console.error('❌ Failed to load custom metronome sound:', error);
      throw new Error('Failed to load custom metronome sound');
    }
  }

  /**
   * Create a noise buffer for percussive sounds
   */
  private createNoiseBuffer(duration: number, density: number = 1.0): AudioBuffer {
    const sampleRate = this.audioContext.sampleRate;
    const bufferSize = sampleRate * duration;
    const buffer = this.audioContext.createBuffer(1, bufferSize, sampleRate);
    const data = buffer.getChannelData(0);

    for (let i = 0; i < bufferSize; i++) {
      data[i] = (Math.random() * 2 - 1) * density;
    }

    return buffer;
  }

  /**
   * Set master volume
   */
  setVolume(volume: number): void {
    this.masterGain.gain.value = volume;
  }

  /**
   * Clean up resources
   */
  destroy(): void {
    this.masterGain.disconnect();
    this.customSoundBuffer = null;
  }
}
