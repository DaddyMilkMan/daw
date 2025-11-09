/**
 * Audio Effects System for Zenith DAW
 * Implements professional audio effects using Web Audio API
 */

export interface EffectParameter {
  name: string;
  value: number;
  min: number;
  max: number;
  defaultValue: number;
  unit?: string;
}

export abstract class AudioEffect {
  abstract id: string;
  abstract name: string;
  abstract type: string;

  protected audioContext: AudioContext;
  protected inputNode: GainNode;
  protected outputNode: GainNode;
  protected enabled: boolean = true;
  protected wetDryMix: number = 1.0; // 0 = dry, 1 = wet

  constructor(audioContext: AudioContext) {
    this.audioContext = audioContext;
    this.inputNode = audioContext.createGain();
    this.outputNode = audioContext.createGain();
  }

  abstract getParameters(): Map<string, EffectParameter>;
  abstract setParameter(name: string, value: number): void;
  abstract getInputNode(): AudioNode;
  abstract getOutputNode(): AudioNode;

  /**
   * Connect this effect to another audio node
   */
  connect(destination: AudioNode): void {
    this.getOutputNode().connect(destination);
  }

  /**
   * Disconnect this effect
   */
  disconnect(): void {
    this.getOutputNode().disconnect();
  }

  /**
   * Enable/disable effect (bypass)
   */
  setEnabled(enabled: boolean): void {
    this.enabled = enabled;
    this.updateBypass();
  }

  /**
   * Set wet/dry mix (0-1)
   */
  setWetDryMix(mix: number): void {
    this.wetDryMix = Math.max(0, Math.min(1, mix));
    this.updateBypass();
  }

  protected abstract updateBypass(): void;

  /**
   * Cleanup and dispose
   */
  dispose(): void {
    this.inputNode.disconnect();
    this.outputNode.disconnect();
  }
}

/**
 * 3-Band Equalizer Effect
 */
export class ThreeBandEQ extends AudioEffect {
  id = 'three-band-eq';
  name = '3-Band EQ';
  type = 'eq';

  private lowFilter: BiquadFilterNode;
  private midFilter: BiquadFilterNode;
  private highFilter: BiquadFilterNode;

  private lowGain: number = 0;
  private midGain: number = 0;
  private highGain: number = 0;

  constructor(audioContext: AudioContext) {
    super(audioContext);

    // Low shelf filter
    this.lowFilter = audioContext.createBiquadFilter();
    this.lowFilter.type = 'lowshelf';
    this.lowFilter.frequency.value = 320;
    this.lowFilter.gain.value = 0;

    // Mid peaking filter
    this.midFilter = audioContext.createBiquadFilter();
    this.midFilter.type = 'peaking';
    this.midFilter.frequency.value = 1000;
    this.midFilter.Q.value = 0.5;
    this.midFilter.gain.value = 0;

    // High shelf filter
    this.highFilter = audioContext.createBiquadFilter();
    this.highFilter.type = 'highshelf';
    this.highFilter.frequency.value = 3200;
    this.highFilter.gain.value = 0;

    // Connect: input -> low -> mid -> high -> output
    this.inputNode.connect(this.lowFilter);
    this.lowFilter.connect(this.midFilter);
    this.midFilter.connect(this.highFilter);
    this.highFilter.connect(this.outputNode);
  }

  getParameters(): Map<string, EffectParameter> {
    return new Map([
      ['lowGain', { name: 'Low Gain', value: this.lowGain, min: -24, max: 24, defaultValue: 0, unit: 'dB' }],
      ['midGain', { name: 'Mid Gain', value: this.midGain, min: -24, max: 24, defaultValue: 0, unit: 'dB' }],
      ['highGain', { name: 'High Gain', value: this.highGain, min: -24, max: 24, defaultValue: 0, unit: 'dB' }],
      ['lowFreq', { name: 'Low Freq', value: this.lowFilter.frequency.value, min: 20, max: 500, defaultValue: 320, unit: 'Hz' }],
      ['midFreq', { name: 'Mid Freq', value: this.midFilter.frequency.value, min: 200, max: 5000, defaultValue: 1000, unit: 'Hz' }],
      ['highFreq', { name: 'High Freq', value: this.highFilter.frequency.value, min: 1000, max: 20000, defaultValue: 3200, unit: 'Hz' }],
    ]);
  }

  setParameter(name: string, value: number): void {
    switch (name) {
      case 'lowGain':
        this.lowGain = value;
        this.lowFilter.gain.setValueAtTime(value, this.audioContext.currentTime);
        break;
      case 'midGain':
        this.midGain = value;
        this.midFilter.gain.setValueAtTime(value, this.audioContext.currentTime);
        break;
      case 'highGain':
        this.highGain = value;
        this.highFilter.gain.setValueAtTime(value, this.audioContext.currentTime);
        break;
      case 'lowFreq':
        this.lowFilter.frequency.setValueAtTime(value, this.audioContext.currentTime);
        break;
      case 'midFreq':
        this.midFilter.frequency.setValueAtTime(value, this.audioContext.currentTime);
        break;
      case 'highFreq':
        this.highFilter.frequency.setValueAtTime(value, this.audioContext.currentTime);
        break;
    }
  }

  protected updateBypass(): void {
    if (!this.enabled) {
      this.lowFilter.gain.value = 0;
      this.midFilter.gain.value = 0;
      this.highFilter.gain.value = 0;
    } else {
      this.lowFilter.gain.value = this.lowGain;
      this.midFilter.gain.value = this.midGain;
      this.highFilter.gain.value = this.highGain;
    }
  }

  getInputNode(): AudioNode {
    return this.inputNode;
  }

  getOutputNode(): AudioNode {
    return this.outputNode;
  }
}

/**
 * Compressor/Limiter Effect
 */
export class Compressor extends AudioEffect {
  id = 'compressor';
  name = 'Compressor';
  type = 'dynamics';

  private compressor: DynamicsCompressorNode;

  constructor(audioContext: AudioContext) {
    super(audioContext);

    this.compressor = audioContext.createDynamicsCompressor();

    // Default compressor settings
    this.compressor.threshold.value = -24;
    this.compressor.knee.value = 30;
    this.compressor.ratio.value = 12;
    this.compressor.attack.value = 0.003; // 3ms
    this.compressor.release.value = 0.25; // 250ms

    // Connect: input -> compressor -> output
    this.inputNode.connect(this.compressor);
    this.compressor.connect(this.outputNode);
  }

  getParameters(): Map<string, EffectParameter> {
    return new Map([
      ['threshold', { name: 'Threshold', value: this.compressor.threshold.value, min: -100, max: 0, defaultValue: -24, unit: 'dB' }],
      ['knee', { name: 'Knee', value: this.compressor.knee.value, min: 0, max: 40, defaultValue: 30, unit: 'dB' }],
      ['ratio', { name: 'Ratio', value: this.compressor.ratio.value, min: 1, max: 20, defaultValue: 12 }],
      ['attack', { name: 'Attack', value: this.compressor.attack.value * 1000, min: 0, max: 100, defaultValue: 3, unit: 'ms' }],
      ['release', { name: 'Release', value: this.compressor.release.value * 1000, min: 0, max: 1000, defaultValue: 250, unit: 'ms' }],
    ]);
  }

  setParameter(name: string, value: number): void {
    const now = this.audioContext.currentTime;
    switch (name) {
      case 'threshold':
        this.compressor.threshold.setValueAtTime(value, now);
        break;
      case 'knee':
        this.compressor.knee.setValueAtTime(value, now);
        break;
      case 'ratio':
        this.compressor.ratio.setValueAtTime(value, now);
        break;
      case 'attack':
        this.compressor.attack.setValueAtTime(value / 1000, now); // Convert ms to seconds
        break;
      case 'release':
        this.compressor.release.setValueAtTime(value / 1000, now); // Convert ms to seconds
        break;
    }
  }

  /**
   * Configure as a limiter
   */
  setAsLimiter(): void {
    const now = this.audioContext.currentTime;
    this.compressor.threshold.setValueAtTime(0, now);
    this.compressor.knee.setValueAtTime(0, now);
    this.compressor.ratio.setValueAtTime(20, now);
    this.compressor.attack.setValueAtTime(0.005, now);
    this.compressor.release.setValueAtTime(0.050, now);
  }

  /**
   * Get current reduction in dB
   */
  getReduction(): number {
    return this.compressor.reduction;
  }

  protected updateBypass(): void {
    // Compressor doesn't have a simple bypass, we disconnect/reconnect
    if (!this.enabled) {
      this.inputNode.disconnect();
      this.compressor.disconnect();
      this.inputNode.connect(this.outputNode);
    } else {
      this.inputNode.disconnect();
      this.inputNode.connect(this.compressor);
      this.compressor.connect(this.outputNode);
    }
  }

  getInputNode(): AudioNode {
    return this.inputNode;
  }

  getOutputNode(): AudioNode {
    return this.outputNode;
  }
}

/**
 * Delay Effect
 */
export class Delay extends AudioEffect {
  id = 'delay';
  name = 'Delay';
  type = 'time';

  private delayNode: DelayNode;
  private feedbackNode: GainNode;
  private wetNode: GainNode;
  private dryNode: GainNode;

  private delayTime: number = 0.5;
  private feedback: number = 0.3;

  constructor(audioContext: AudioContext) {
    super(audioContext);

    this.delayNode = audioContext.createDelay(5.0); // Max 5 seconds
    this.feedbackNode = audioContext.createGain();
    this.wetNode = audioContext.createGain();
    this.dryNode = audioContext.createGain();

    this.delayNode.delayTime.value = this.delayTime;
    this.feedbackNode.gain.value = this.feedback;
    this.wetNode.gain.value = 0.5;
    this.dryNode.gain.value = 1.0;

    // Connect: input -> delay -> feedback -> delay (loop)
    //                  |-> wet -> output
    //          input -> dry -> output
    this.inputNode.connect(this.delayNode);
    this.delayNode.connect(this.feedbackNode);
    this.feedbackNode.connect(this.delayNode);

    this.delayNode.connect(this.wetNode);
    this.wetNode.connect(this.outputNode);

    this.inputNode.connect(this.dryNode);
    this.dryNode.connect(this.outputNode);
  }

  getParameters(): Map<string, EffectParameter> {
    return new Map([
      ['time', { name: 'Time', value: this.delayTime * 1000, min: 0, max: 5000, defaultValue: 500, unit: 'ms' }],
      ['feedback', { name: 'Feedback', value: this.feedback * 100, min: 0, max: 100, defaultValue: 30, unit: '%' }],
      ['wet', { name: 'Wet', value: this.wetNode.gain.value * 100, min: 0, max: 100, defaultValue: 50, unit: '%' }],
    ]);
  }

  setParameter(name: string, value: number): void {
    const now = this.audioContext.currentTime;
    switch (name) {
      case 'time':
        this.delayTime = value / 1000;
        this.delayNode.delayTime.setValueAtTime(this.delayTime, now);
        break;
      case 'feedback':
        this.feedback = value / 100;
        this.feedbackNode.gain.setValueAtTime(this.feedback, now);
        break;
      case 'wet':
        this.wetNode.gain.setValueAtTime(value / 100, now);
        break;
    }
  }

  protected updateBypass(): void {
    const now = this.audioContext.currentTime;
    if (!this.enabled) {
      this.wetNode.gain.setValueAtTime(0, now);
      this.dryNode.gain.setValueAtTime(1, now);
    } else {
      this.wetNode.gain.setValueAtTime(this.wetDryMix * 0.5, now);
      this.dryNode.gain.setValueAtTime(1, now);
    }
  }

  getInputNode(): AudioNode {
    return this.inputNode;
  }

  getOutputNode(): AudioNode {
    return this.outputNode;
  }
}

/**
 * Simple Gain/Volume Effect
 */
export class Gain extends AudioEffect {
  id = 'gain';
  name = 'Gain';
  type = 'utility';

  private gainNode: GainNode;
  private gainValue: number = 1.0;

  constructor(audioContext: AudioContext) {
    super(audioContext);

    this.gainNode = audioContext.createGain();
    this.gainNode.gain.value = this.gainValue;

    // Connect: input -> gain -> output
    this.inputNode.connect(this.gainNode);
    this.gainNode.connect(this.outputNode);
  }

  getParameters(): Map<string, EffectParameter> {
    return new Map([
      ['gain', { name: 'Gain', value: this.gainValueToDb(this.gainValue), min: -60, max: 12, defaultValue: 0, unit: 'dB' }],
    ]);
  }

  setParameter(name: string, value: number): void {
    if (name === 'gain') {
      this.gainValue = this.dbToGainValue(value);
      this.gainNode.gain.setValueAtTime(this.gainValue, this.audioContext.currentTime);
    }
  }

  private gainValueToDb(gain: number): number {
    return 20 * Math.log10(gain);
  }

  private dbToGainValue(db: number): number {
    return Math.pow(10, db / 20);
  }

  protected updateBypass(): void {
    if (!this.enabled) {
      this.gainNode.gain.value = 1.0;
    } else {
      this.gainNode.gain.value = this.gainValue;
    }
  }

  getInputNode(): AudioNode {
    return this.inputNode;
  }

  getOutputNode(): AudioNode {
    return this.outputNode;
  }
}

/**
 * Effects Chain Manager
 */
export class EffectsChain {
  private effects: AudioEffect[] = [];
  private inputNode: GainNode;
  private outputNode: GainNode;

  constructor(private audioContext: AudioContext) {
    this.inputNode = audioContext.createGain();
    this.outputNode = audioContext.createGain();

    // Initially connect input directly to output
    this.inputNode.connect(this.outputNode);
  }

  /**
   * Add effect to the chain
   */
  addEffect(effect: AudioEffect): void {
    this.effects.push(effect);
    this.reconnect();
  }

  /**
   * Remove effect from the chain
   */
  removeEffect(effectId: string): void {
    this.effects = this.effects.filter(e => e.id !== effectId);
    this.reconnect();
  }

  /**
   * Move effect to a different position in the chain
   */
  moveEffect(effectId: string, newIndex: number): void {
    const currentIndex = this.effects.findIndex(e => e.id === effectId);
    if (currentIndex === -1) return;

    const [effect] = this.effects.splice(currentIndex, 1);
    this.effects.splice(newIndex, 0, effect);
    this.reconnect();
  }

  /**
   * Reconnect all effects in order
   */
  private reconnect(): void {
    // Disconnect everything
    this.inputNode.disconnect();
    this.effects.forEach(effect => effect.disconnect());

    if (this.effects.length === 0) {
      // No effects, connect input directly to output
      this.inputNode.connect(this.outputNode);
    } else {
      // Connect effects in series
      this.inputNode.connect(this.effects[0].getInputNode());

      for (let i = 0; i < this.effects.length - 1; i++) {
        this.effects[i].getOutputNode().connect(this.effects[i + 1].getInputNode());
      }

      this.effects[this.effects.length - 1].getOutputNode().connect(this.outputNode);
    }
  }

  /**
   * Get all effects
   */
  getEffects(): AudioEffect[] {
    return [...this.effects];
  }

  /**
   * Get input node for connecting source
   */
  getInputNode(): AudioNode {
    return this.inputNode;
  }

  /**
   * Get output node for connecting to destination
   */
  getOutputNode(): AudioNode {
    return this.outputNode;
  }

  /**
   * Cleanup
   */
  dispose(): void {
    this.effects.forEach(effect => effect.dispose());
    this.inputNode.disconnect();
    this.outputNode.disconnect();
  }
}
