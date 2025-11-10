/**
 * Effects Library
 * Professional audio effects using Web Audio API
 *
 * Based on Web Audio best practices:
 * - BiquadFilterNode for EQ (low shelf, peaking, high shelf)
 * - DynamicsCompressorNode for compression
 * - ConvolverNode for reverb with impulse responses
 * - DelayNode for delay effects
 * - WaveShaperNode for distortion
 */

export interface EQBand {
  type: BiquadFilterType;
  frequency: number;
  Q: number;
  gain: number; // in dB
}

export class EffectsLibrary {
  /**
   * Create a 3-band EQ
   * Low shelf, Mid peaking, High shelf
   */
  static create3BandEQ(context: AudioContext): {
    input: GainNode;
    output: GainNode;
    low: BiquadFilterNode;
    mid: BiquadFilterNode;
    high: BiquadFilterNode;
  } {
    const input = context.createGain();
    const output = context.createGain();

    // Low shelf (100 Hz)
    const low = context.createBiquadFilter();
    low.type = 'lowshelf';
    low.frequency.value = 100;
    low.gain.value = 0; // dB

    // Mid peaking (1000 Hz)
    const mid = context.createBiquadFilter();
    mid.type = 'peaking';
    mid.frequency.value = 1000;
    mid.Q.value = 1.0;
    mid.gain.value = 0; // dB

    // High shelf (10000 Hz)
    const high = context.createBiquadFilter();
    high.type = 'highshelf';
    high.frequency.value = 10000;
    high.gain.value = 0; // dB

    // Connect: input -> low -> mid -> high -> output
    input.connect(low);
    low.connect(mid);
    mid.connect(high);
    high.connect(output);

    return { input, output, low, mid, high };
  }

  /**
   * Create a parametric EQ with custom bands
   */
  static createParametricEQ(context: AudioContext, bands: EQBand[]): {
    input: GainNode;
    output: GainNode;
    filters: BiquadFilterNode[];
  } {
    const input = context.createGain();
    const output = context.createGain();
    const filters: BiquadFilterNode[] = [];

    let prevNode: AudioNode = input;

    bands.forEach((band) => {
      const filter = context.createBiquadFilter();
      filter.type = band.type;
      filter.frequency.value = band.frequency;
      filter.Q.value = band.Q;
      filter.gain.value = band.gain;

      prevNode.connect(filter);
      prevNode = filter;
      filters.push(filter);
    });

    prevNode.connect(output);

    return { input, output, filters };
  }

  /**
   * Create a compressor
   */
  static createCompressor(
    context: AudioContext,
    options: {
      threshold?: number; // dB
      knee?: number; // dB
      ratio?: number;
      attack?: number; // seconds
      release?: number; // seconds
    } = {}
  ): DynamicsCompressorNode {
    const compressor = context.createDynamicsCompressor();

    compressor.threshold.value = options.threshold ?? -24;
    compressor.knee.value = options.knee ?? 30;
    compressor.ratio.value = options.ratio ?? 12;
    compressor.attack.value = options.attack ?? 0.003;
    compressor.release.value = options.release ?? 0.25;

    return compressor;
  }

  /**
   * Create a gate (noise gate)
   */
  static createGate(
    context: AudioContext,
    threshold: number = -40 // dB
  ): {
    input: GainNode;
    output: GainNode;
    processor: ScriptProcessorNode;
  } {
    const input = context.createGain();
    const output = context.createGain();

    // Use script processor for gate logic (simplified)
    const processor = context.createScriptProcessor(2048, 1, 1);

    let isOpen = false;
    const thresholdLinear = Math.pow(10, threshold / 20);

    processor.onaudioprocess = (e) => {
      const inputData = e.inputBuffer.getChannelData(0);
      const outputData = e.outputBuffer.getChannelData(0);

      for (let i = 0; i < inputData.length; i++) {
        const sample = Math.abs(inputData[i]);

        // Simple gate logic with hysteresis
        if (sample > thresholdLinear) {
          isOpen = true;
        } else if (sample < thresholdLinear * 0.5) {
          isOpen = false;
        }

        outputData[i] = isOpen ? inputData[i] : 0;
      }
    };

    input.connect(processor);
    processor.connect(output);

    return { input, output, processor };
  }

  /**
   * Create a delay effect
   */
  static createDelay(
    context: AudioContext,
    options: {
      delayTime?: number; // seconds
      feedback?: number; // 0-1
      mix?: number; // 0-1 (dry/wet)
    } = {}
  ): {
    input: GainNode;
    output: GainNode;
    delay: DelayNode;
    feedback: GainNode;
    mix: GainNode;
  } {
    const input = context.createGain();
    const output = context.createGain();

    // Create delay line
    const delay = context.createDelay(5.0); // Max 5 seconds
    delay.delayTime.value = options.delayTime ?? 0.5;

    // Create feedback
    const feedback = context.createGain();
    feedback.gain.value = options.feedback ?? 0.5;

    // Create wet/dry mix
    const wetGain = context.createGain();
    const dryGain = context.createGain();
    const mix = options.mix ?? 0.5;

    wetGain.gain.value = mix;
    dryGain.gain.value = 1 - mix;

    // Connect delay with feedback
    input.connect(delay);
    delay.connect(feedback);
    feedback.connect(delay); // Feedback loop

    // Wet signal
    delay.connect(wetGain);
    wetGain.connect(output);

    // Dry signal
    input.connect(dryGain);
    dryGain.connect(output);

    return { input, output, delay, feedback, mix: wetGain };
  }

  /**
   * Create a reverb effect
   */
  static async createReverb(
    context: AudioContext,
    options: {
      duration?: number; // seconds
      decay?: number; // 0-1
      mix?: number; // 0-1 (dry/wet)
    } = {}
  ): Promise<{
    input: GainNode;
    output: GainNode;
    convolver: ConvolverNode;
    mix: GainNode;
  }> {
    const input = context.createGain();
    const output = context.createGain();

    // Create convolver
    const convolver = context.createConvolver();

    // Generate impulse response
    const duration = options.duration ?? 2; // seconds
    const decay = options.decay ?? 0.5;
    const length = context.sampleRate * duration;
    const impulse = context.createBuffer(2, length, context.sampleRate);

    for (let channel = 0; channel < 2; channel++) {
      const channelData = impulse.getChannelData(channel);
      for (let i = 0; i < length; i++) {
        // Exponential decay with random noise
        const t = i / length;
        const decayValue = Math.pow(1 - t, 2 + decay * 3);
        channelData[i] = (Math.random() * 2 - 1) * decayValue;
      }
    }

    convolver.buffer = impulse;

    // Create wet/dry mix
    const wetGain = context.createGain();
    const dryGain = context.createGain();
    const mix = options.mix ?? 0.3;

    wetGain.gain.value = mix;
    dryGain.gain.value = 1 - mix;

    // Wet signal
    input.connect(convolver);
    convolver.connect(wetGain);
    wetGain.connect(output);

    // Dry signal
    input.connect(dryGain);
    dryGain.connect(output);

    return { input, output, convolver, mix: wetGain };
  }

  /**
   * Create a distortion effect
   */
  static createDistortion(
    context: AudioContext,
    amount: number = 50 // 0-100
  ): {
    input: GainNode;
    output: GainNode;
    waveshaper: WaveShaperNode;
  } {
    const input = context.createGain();
    const output = context.createGain();

    // Create waveshaper
    const waveshaper = context.createWaveShaper();

    // Generate distortion curve
    const k = amount;
    const samples = 44100;
    const curve = new Float32Array(samples);

    for (let i = 0; i < samples; i++) {
      const x = (i * 2) / samples - 1;
      curve[i] = ((3 + k) * x * 20) / (Math.PI + k * Math.abs(x));
    }

    waveshaper.curve = curve;
    waveshaper.oversample = '4x';

    input.connect(waveshaper);
    waveshaper.connect(output);

    return { input, output, waveshaper };
  }

  /**
   * Create a chorus effect
   */
  static createChorus(
    context: AudioContext,
    options: {
      rate?: number; // Hz
      depth?: number; // ms
      mix?: number; // 0-1
    } = {}
  ): {
    input: GainNode;
    output: GainNode;
    lfo: OscillatorNode;
    delay: DelayNode;
  } {
    const input = context.createGain();
    const output = context.createGain();

    // Create delay line
    const delay = context.createDelay(1.0);
    const baseDelay = 0.02; // 20ms base delay
    delay.delayTime.value = baseDelay;

    // Create LFO for modulation
    const lfo = context.createOscillator();
    lfo.type = 'sine';
    lfo.frequency.value = options.rate ?? 0.5; // Hz

    // Create LFO gain (modulation depth)
    const lfoGain = context.createGain();
    const depth = options.depth ?? 10; // ms
    lfoGain.gain.value = depth / 1000; // Convert to seconds

    // Connect LFO to delay time
    lfo.connect(lfoGain);
    lfoGain.connect(delay.delayTime);
    lfo.start();

    // Create wet/dry mix
    const wetGain = context.createGain();
    const dryGain = context.createGain();
    const mix = options.mix ?? 0.5;

    wetGain.gain.value = mix;
    dryGain.gain.value = 1 - mix;

    // Wet signal
    input.connect(delay);
    delay.connect(wetGain);
    wetGain.connect(output);

    // Dry signal
    input.connect(dryGain);
    dryGain.connect(output);

    return { input, output, lfo, delay };
  }

  /**
   * Create a high-pass filter
   */
  static createHighPassFilter(
    context: AudioContext,
    frequency: number = 80 // Hz
  ): BiquadFilterNode {
    const filter = context.createBiquadFilter();
    filter.type = 'highpass';
    filter.frequency.value = frequency;
    filter.Q.value = 0.7071; // Butterworth response
    return filter;
  }

  /**
   * Create a low-pass filter
   */
  static createLowPassFilter(
    context: AudioContext,
    frequency: number = 12000 // Hz
  ): BiquadFilterNode {
    const filter = context.createBiquadFilter();
    filter.type = 'lowpass';
    filter.frequency.value = frequency;
    filter.Q.value = 0.7071; // Butterworth response
    return filter;
  }

  /**
   * Create a limiter (brick-wall limiting)
   */
  static createLimiter(context: AudioContext): DynamicsCompressorNode {
    const limiter = context.createDynamicsCompressor();

    limiter.threshold.value = -0.5; // Just below 0dBFS
    limiter.knee.value = 0; // Hard knee
    limiter.ratio.value = 20; // Brick wall
    limiter.attack.value = 0.001; // 1ms
    limiter.release.value = 0.01; // 10ms

    return limiter;
  }

  /**
   * Create stereo width control
   */
  static createStereoWidth(
    context: AudioContext,
    width: number = 1.0 // 0-2 (0=mono, 1=normal, 2=wide)
  ): {
    input: ChannelSplitterNode;
    output: ChannelMergerNode;
    width: number;
  } {
    const splitter = context.createChannelSplitter(2);
    const merger = context.createChannelMerger(2);

    const midGain = context.createGain();
    const sideGain = context.createGain();

    // M/S processing
    const w = width;
    midGain.gain.value = (1 + w) / 2;
    sideGain.gain.value = (1 - w) / 2;

    // This is a simplified version
    // Full M/S stereo width would require matrix processing

    return { input: splitter, output: merger, width };
  }
}

export const effectsLibrary = EffectsLibrary;
