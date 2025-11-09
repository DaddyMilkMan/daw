import { BasePlugin } from '../PluginHost';
import { PluginParameter } from '../../types/plugin';

export class ReverbEffect extends BasePlugin {
  private convolver!: ConvolverNode;
  private dryGain!: GainNode;
  private wetGain!: GainNode;
  private impulseBuffer: AudioBuffer | null = null;

  constructor(audioContext: AudioContext) {
    super(audioContext, 'reverb', 'Reverb');
  }

  protected initializeNodes(): void {
    // Create convolver and gain nodes
    this.convolver = this.audioContext.createConvolver();
    this.dryGain = this.audioContext.createGain();
    this.wetGain = this.audioContext.createGain();

    // Set default dry/wet mix (50/50)
    this.dryGain.gain.value = 0.5;
    this.wetGain.gain.value = 0.5;

    // Create initial impulse response
    this.generateImpulseResponse(2.0);

    // Connect: input splits to dry and wet paths
    this.inputNode.connect(this.dryGain);
    this.inputNode.connect(this.convolver);
    this.convolver.connect(this.wetGain);

    // Both paths merge to output
    this.dryGain.connect(this.outputNode);
    this.wetGain.connect(this.outputNode);

    // Initialize parameters
    this.parameters.set('mix', {
      id: 'mix',
      name: 'Mix',
      value: 50,
      min: 0,
      max: 100,
      default: 50,
      unit: '%',
      step: 1,
    });

    this.parameters.set('decay', {
      id: 'decay',
      name: 'Decay Time',
      value: 2.0,
      min: 0.1,
      max: 10.0,
      default: 2.0,
      unit: 's',
      step: 0.1,
    });

    this.parameters.set('preDelay', {
      id: 'preDelay',
      name: 'Pre-Delay',
      value: 0,
      min: 0,
      max: 0.1,
      default: 0,
      unit: 's',
      step: 0.001,
    });
  }

  protected updateParameter(id: string, value: number): void {
    switch (id) {
      case 'mix':
        // Convert percentage to 0-1 range and apply equal-power crossfade
        const mixNormalized = value / 100;
        const wetAmount = Math.sin(mixNormalized * Math.PI / 2);
        const dryAmount = Math.cos(mixNormalized * Math.PI / 2);

        this.wetGain.gain.setValueAtTime(wetAmount, this.audioContext.currentTime);
        this.dryGain.gain.setValueAtTime(dryAmount, this.audioContext.currentTime);
        break;

      case 'decay':
        // Regenerate impulse response with new decay time
        this.generateImpulseResponse(value);
        break;

      case 'preDelay':
        // Pre-delay would require rebuilding the impulse response
        this.generateImpulseResponse(
          this.parameters.get('decay')?.value || 2.0,
          value
        );
        break;
    }
  }

  private generateImpulseResponse(decayTime: number, preDelay: number = 0): void {
    const sampleRate = this.audioContext.sampleRate;
    const length = sampleRate * decayTime;
    const preDelaySamples = Math.floor(sampleRate * preDelay);
    const totalLength = length + preDelaySamples;

    // Create stereo buffer
    const impulse = this.audioContext.createBuffer(2, totalLength, sampleRate);
    const leftChannel = impulse.getChannelData(0);
    const rightChannel = impulse.getChannelData(1);

    // Generate impulse response using a simple algorithmic approach
    // This creates a realistic-sounding reverb tail
    for (let i = 0; i < length; i++) {
      const sampleIndex = i + preDelaySamples;

      // Exponential decay envelope
      const decay = Math.exp(-(i / length) * (10 / decayTime));

      // Random noise for diffusion
      const noise = (Math.random() * 2 - 1) * decay;

      // Add some early reflections (first 10% of the impulse)
      let earlyReflections = 0;
      if (i < length * 0.1) {
        const reflectionPattern = Math.sin(i * 0.01) * Math.cos(i * 0.03);
        earlyReflections = reflectionPattern * decay * 0.5;
      }

      // Combine noise and early reflections
      leftChannel[sampleIndex] = noise + earlyReflections;

      // Slight difference for stereo width
      const rightNoise = (Math.random() * 2 - 1) * decay;
      rightChannel[sampleIndex] = rightNoise + earlyReflections * 0.8;
    }

    // Apply the impulse response to the convolver
    this.convolver.buffer = impulse;
    this.impulseBuffer = impulse;
  }

  dispose(): void {
    this.convolver.disconnect();
    this.dryGain.disconnect();
    this.wetGain.disconnect();
    super.dispose();
  }
}
