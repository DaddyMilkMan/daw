import { BasePlugin } from '../PluginHost';
import { PluginParameter } from '../../types/plugin';

export class EqualizerEffect extends BasePlugin {
  private lowBand!: BiquadFilterNode;
  private midBand!: BiquadFilterNode;
  private highBand!: BiquadFilterNode;

  constructor(audioContext: AudioContext) {
    super(audioContext, 'equalizer', 'Equalizer');
  }

  protected initializeNodes(): void {
    // Create three biquad filters for low, mid, and high bands
    this.lowBand = this.audioContext.createBiquadFilter();
    this.lowBand.type = 'lowshelf';
    this.lowBand.frequency.value = 250;
    this.lowBand.gain.value = 0;

    this.midBand = this.audioContext.createBiquadFilter();
    this.midBand.type = 'peaking';
    this.midBand.frequency.value = 1000;
    this.midBand.Q.value = 1;
    this.midBand.gain.value = 0;

    this.highBand = this.audioContext.createBiquadFilter();
    this.highBand.type = 'highshelf';
    this.highBand.frequency.value = 4000;
    this.highBand.gain.value = 0;

    // Connect the chain: input -> low -> mid -> high -> output
    this.inputNode.connect(this.lowBand);
    this.lowBand.connect(this.midBand);
    this.midBand.connect(this.highBand);
    this.highBand.connect(this.outputNode);

    // Initialize parameters
    this.parameters.set('lowGain', {
      id: 'lowGain',
      name: 'Low Gain',
      value: 0,
      min: -12,
      max: 12,
      default: 0,
      unit: 'dB',
      step: 0.1,
    });

    this.parameters.set('lowFreq', {
      id: 'lowFreq',
      name: 'Low Frequency',
      value: 250,
      min: 20,
      max: 500,
      default: 250,
      unit: 'Hz',
      step: 1,
    });

    this.parameters.set('midGain', {
      id: 'midGain',
      name: 'Mid Gain',
      value: 0,
      min: -12,
      max: 12,
      default: 0,
      unit: 'dB',
      step: 0.1,
    });

    this.parameters.set('midFreq', {
      id: 'midFreq',
      name: 'Mid Frequency',
      value: 1000,
      min: 200,
      max: 5000,
      default: 1000,
      unit: 'Hz',
      step: 1,
    });

    this.parameters.set('midQ', {
      id: 'midQ',
      name: 'Mid Q',
      value: 1,
      min: 0.1,
      max: 10,
      default: 1,
      unit: '',
      step: 0.1,
    });

    this.parameters.set('highGain', {
      id: 'highGain',
      name: 'High Gain',
      value: 0,
      min: -12,
      max: 12,
      default: 0,
      unit: 'dB',
      step: 0.1,
    });

    this.parameters.set('highFreq', {
      id: 'highFreq',
      name: 'High Frequency',
      value: 4000,
      min: 2000,
      max: 20000,
      default: 4000,
      unit: 'Hz',
      step: 1,
    });
  }

  protected updateParameter(id: string, value: number): void {
    switch (id) {
      case 'lowGain':
        this.lowBand.gain.setValueAtTime(value, this.audioContext.currentTime);
        break;
      case 'lowFreq':
        this.lowBand.frequency.setValueAtTime(value, this.audioContext.currentTime);
        break;
      case 'midGain':
        this.midBand.gain.setValueAtTime(value, this.audioContext.currentTime);
        break;
      case 'midFreq':
        this.midBand.frequency.setValueAtTime(value, this.audioContext.currentTime);
        break;
      case 'midQ':
        this.midBand.Q.setValueAtTime(value, this.audioContext.currentTime);
        break;
      case 'highGain':
        this.highBand.gain.setValueAtTime(value, this.audioContext.currentTime);
        break;
      case 'highFreq':
        this.highBand.frequency.setValueAtTime(value, this.audioContext.currentTime);
        break;
    }
  }

  // Get frequency response for visualization
  getFrequencyResponse(frequencies: Float32Array): Float32Array {
    const magResponse = new Float32Array(frequencies.length);
    const phaseResponse = new Float32Array(frequencies.length);

    // Get response for each band
    const lowMag = new Float32Array(frequencies.length);
    const lowPhase = new Float32Array(frequencies.length);
    const midMag = new Float32Array(frequencies.length);
    const midPhase = new Float32Array(frequencies.length);
    const highMag = new Float32Array(frequencies.length);
    const highPhase = new Float32Array(frequencies.length);

    this.lowBand.getFrequencyResponse(frequencies, lowMag, lowPhase);
    this.midBand.getFrequencyResponse(frequencies, midMag, midPhase);
    this.highBand.getFrequencyResponse(frequencies, highMag, highPhase);

    // Combine responses (multiply magnitudes)
    for (let i = 0; i < frequencies.length; i++) {
      magResponse[i] = lowMag[i] * midMag[i] * highMag[i];
    }

    return magResponse;
  }

  dispose(): void {
    this.lowBand.disconnect();
    this.midBand.disconnect();
    this.highBand.disconnect();
    super.dispose();
  }
}
