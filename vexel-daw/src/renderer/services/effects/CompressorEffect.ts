import { BasePlugin } from '../PluginHost';
import { PluginParameter } from '../../types/plugin';

export class CompressorEffect extends BasePlugin {
  private compressor!: DynamicsCompressorNode;

  constructor(audioContext: AudioContext) {
    super(audioContext, 'compressor', 'Compressor');
  }

  protected initializeNodes(): void {
    // Create dynamics compressor node
    this.compressor = this.audioContext.createDynamicsCompressor();

    // Set default values
    this.compressor.threshold.value = -24;
    this.compressor.knee.value = 30;
    this.compressor.ratio.value = 12;
    this.compressor.attack.value = 0.003;
    this.compressor.release.value = 0.25;

    // Connect: input -> compressor -> output
    this.inputNode.connect(this.compressor);
    this.compressor.connect(this.outputNode);

    // Initialize parameters
    this.parameters.set('threshold', {
      id: 'threshold',
      name: 'Threshold',
      value: -24,
      min: -100,
      max: 0,
      default: -24,
      unit: 'dB',
      step: 0.1,
    });

    this.parameters.set('knee', {
      id: 'knee',
      name: 'Knee',
      value: 30,
      min: 0,
      max: 40,
      default: 30,
      unit: 'dB',
      step: 0.1,
    });

    this.parameters.set('ratio', {
      id: 'ratio',
      name: 'Ratio',
      value: 12,
      min: 1,
      max: 20,
      default: 12,
      unit: ':1',
      step: 0.1,
    });

    this.parameters.set('attack', {
      id: 'attack',
      name: 'Attack',
      value: 0.003,
      min: 0,
      max: 1,
      default: 0.003,
      unit: 's',
      step: 0.001,
    });

    this.parameters.set('release', {
      id: 'release',
      name: 'Release',
      value: 0.25,
      min: 0,
      max: 1,
      default: 0.25,
      unit: 's',
      step: 0.001,
    });
  }

  protected updateParameter(id: string, value: number): void {
    switch (id) {
      case 'threshold':
        this.compressor.threshold.setValueAtTime(value, this.audioContext.currentTime);
        break;
      case 'knee':
        this.compressor.knee.setValueAtTime(value, this.audioContext.currentTime);
        break;
      case 'ratio':
        this.compressor.ratio.setValueAtTime(value, this.audioContext.currentTime);
        break;
      case 'attack':
        this.compressor.attack.setValueAtTime(value, this.audioContext.currentTime);
        break;
      case 'release':
        this.compressor.release.setValueAtTime(value, this.audioContext.currentTime);
        break;
    }
  }

  // Get current reduction for metering
  getReduction(): number {
    return this.compressor.reduction;
  }

  dispose(): void {
    this.compressor.disconnect();
    super.dispose();
  }
}
