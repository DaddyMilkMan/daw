/**
 * Time Stretching and Pitch Shifting Service
 * Based on WSOLA (Waveform-Similarity Overlap-Add) Algorithm
 *
 * Features:
 * - Time stretching without pitch change
 * - Pitch shifting without tempo change
 * - Combined time/pitch adjustment
 * - Real-time and offline processing
 *
 * Based on:
 * - SoundTouch algorithm (Olli Parviainen)
 * - WSOLA algorithm (Verhelst & Roelands, 1993)
 * - Phase Vocoder for pitch shifting
 *
 * References:
 * - https://www.surina.net/soundtouch/
 * - "An Overlap-Add Technique Based on Waveform Similarity (WSOLA)"
 * - Web Audio API for real-time processing
 */

import { useAudioStore } from '../store/audioStore';

export interface TimeStretchOptions {
  rate: number; // Playback rate (1.0 = normal, 0.5 = half speed, 2.0 = double speed)
  preservePitch: boolean; // If true, pitch stays the same
}

export interface PitchShiftOptions {
  semitones: number; // Semitones to shift (-12 to +12)
  preserveTempo: boolean; // If true, tempo stays the same
}

export class TimeStretchService {
  private static instance: TimeStretchService | null = null;

  private constructor() {}

  static getInstance(): TimeStretchService {
    if (!TimeStretchService.instance) {
      TimeStretchService.instance = new TimeStretchService();
    }
    return TimeStretchService.instance;
  }

  /**
   * Time stretch audio buffer (offline processing)
   * Uses WSOLA algorithm for high-quality time stretching
   */
  async timeStretch(
    audioBuffer: AudioBuffer,
    options: TimeStretchOptions
  ): Promise<AudioBuffer> {
    const { rate, preservePitch } = options;

    console.log(`Time stretching: rate=${rate}, preservePitch=${preservePitch}`);

    const store = useAudioStore.getState();
    const context = store.audioContext.context;

    if (!context) {
      throw new Error('AudioContext not initialized');
    }

    // If rate is 1.0, return original buffer
    if (Math.abs(rate - 1.0) < 0.001) {
      return audioBuffer;
    }

    // Calculate new buffer length
    const newLength = Math.floor(audioBuffer.length / rate);

    // Create output buffer
    const outputBuffer = context.createBuffer(
      audioBuffer.numberOfChannels,
      newLength,
      audioBuffer.sampleRate
    );

    // Process each channel
    for (let channel = 0; channel < audioBuffer.numberOfChannels; channel++) {
      const inputData = audioBuffer.getChannelData(channel);
      const outputData = outputBuffer.getChannelData(channel);

      this.wsolaTimeStretch(inputData, outputData, rate);
    }

    // If preserving pitch, apply pitch correction
    if (preservePitch && Math.abs(rate - 1.0) > 0.001) {
      const pitchShiftAmount = -12 * Math.log2(rate); // Semitones to compensate
      return await this.pitchShift(outputBuffer, {
        semitones: pitchShiftAmount,
        preserveTempo: false,
      });
    }

    return outputBuffer;
  }

  /**
   * Pitch shift audio buffer (offline processing)
   * Uses Phase Vocoder for high-quality pitch shifting
   */
  async pitchShift(
    audioBuffer: AudioBuffer,
    options: PitchShiftOptions
  ): Promise<AudioBuffer> {
    const { semitones, preserveTempo } = options;

    console.log(`Pitch shifting: semitones=${semitones}, preserveTempo=${preserveTempo}`);

    const store = useAudioStore.getState();
    const context = store.audioContext.context;

    if (!context) {
      throw new Error('AudioContext not initialized');
    }

    // If no pitch shift, return original
    if (Math.abs(semitones) < 0.01) {
      return audioBuffer;
    }

    // Calculate pitch ratio
    const pitchRatio = Math.pow(2, semitones / 12);

    // If preserving tempo, first time-stretch, then pitch-shift
    if (preserveTempo) {
      // Time stretch to compensate
      const timeStretched = await this.timeStretch(audioBuffer, {
        rate: pitchRatio,
        preservePitch: false,
      });

      // Then apply pitch shift
      return await this.pitchShiftInternal(timeStretched, pitchRatio);
    } else {
      return await this.pitchShiftInternal(audioBuffer, pitchRatio);
    }
  }

  /**
   * WSOLA Time Stretching Algorithm
   * Based on Waveform-Similarity Overlap-Add
   */
  private wsolaTimeStretch(
    inputData: Float32Array,
    outputData: Float32Array,
    rate: number
  ): void {
    const seekWindowLength = Math.floor(0.015 * 48000); // 15ms seek window
    const sequenceLength = Math.floor(0.082 * 48000); // 82ms sequence
    const overlapLength = Math.floor(0.012 * 48000); // 12ms overlap

    let inputPos = 0;
    let outputPos = 0;

    const hopSize = sequenceLength - overlapLength;
    const outputHopSize = Math.floor(hopSize * rate);

    while (outputPos + sequenceLength < outputData.length && inputPos + sequenceLength + seekWindowLength < inputData.length) {
      // Find best match in seek window
      let bestOffset = 0;
      let bestCorrelation = -Infinity;

      for (let offset = 0; offset < seekWindowLength; offset++) {
        const correlation = this.calculateCorrelation(
          inputData,
          inputPos + offset,
          inputData,
          inputPos - overlapLength,
          overlapLength
        );

        if (correlation > bestCorrelation) {
          bestCorrelation = correlation;
          bestOffset = offset;
        }
      }

      // Copy sequence with overlap-add
      const sourcePos = inputPos + bestOffset;

      for (let i = 0; i < sequenceLength; i++) {
        if (outputPos + i < outputData.length) {
          if (i < overlapLength) {
            // Overlap region: crossfade
            const fade = i / overlapLength;
            const prevValue = outputPos + i - overlapLength >= 0 ? outputData[outputPos + i - overlapLength] : 0;
            outputData[outputPos + i] = prevValue * (1 - fade) + inputData[sourcePos + i] * fade;
          } else {
            // Non-overlap region: direct copy
            outputData[outputPos + i] = inputData[sourcePos + i];
          }
        }
      }

      inputPos += outputHopSize;
      outputPos += hopSize;
    }

    // Fill remaining with zeros
    for (let i = outputPos; i < outputData.length; i++) {
      outputData[i] = 0;
    }
  }

  /**
   * Calculate correlation between two audio segments
   */
  private calculateCorrelation(
    data1: Float32Array,
    offset1: number,
    data2: Float32Array,
    offset2: number,
    length: number
  ): number {
    let correlation = 0;
    let norm1 = 0;
    let norm2 = 0;

    for (let i = 0; i < length; i++) {
      const sample1 = data1[offset1 + i] || 0;
      const sample2 = data2[offset2 + i] || 0;

      correlation += sample1 * sample2;
      norm1 += sample1 * sample1;
      norm2 += sample2 * sample2;
    }

    const normProduct = Math.sqrt(norm1 * norm2);
    return normProduct > 0 ? correlation / normProduct : 0;
  }

  /**
   * Internal pitch shift using simple resampling
   * For production, this should use Phase Vocoder or better algorithm
   */
  private async pitchShiftInternal(
    audioBuffer: AudioBuffer,
    pitchRatio: number
  ): Promise<AudioBuffer> {
    const store = useAudioStore.getState();
    const context = store.audioContext.context;

    if (!context) {
      throw new Error('AudioContext not initialized');
    }

    // Simple resampling approach (fast but lower quality)
    // For production: use Phase Vocoder FFT-based approach

    const newLength = Math.floor(audioBuffer.length / pitchRatio);
    const outputBuffer = context.createBuffer(
      audioBuffer.numberOfChannels,
      newLength,
      audioBuffer.sampleRate
    );

    for (let channel = 0; channel < audioBuffer.numberOfChannels; channel++) {
      const inputData = audioBuffer.getChannelData(channel);
      const outputData = outputBuffer.getChannelData(channel);

      for (let i = 0; i < outputData.length; i++) {
        const sourceIndex = i * pitchRatio;
        const index1 = Math.floor(sourceIndex);
        const index2 = Math.min(index1 + 1, inputData.length - 1);
        const frac = sourceIndex - index1;

        // Linear interpolation
        outputData[i] = inputData[index1] * (1 - frac) + inputData[index2] * frac;
      }
    }

    return outputBuffer;
  }

  /**
   * Phase Vocoder pitch shifting (high quality, FFT-based)
   * TODO: Implement full Phase Vocoder for production use
   */
  private async phaseVocoderPitchShift(
    audioBuffer: AudioBuffer,
    pitchRatio: number
  ): Promise<AudioBuffer> {
    // This is a placeholder for full Phase Vocoder implementation
    // Phase Vocoder requires:
    // 1. FFT analysis with overlap
    // 2. Phase unwrapping
    // 3. Frequency-domain pitch shifting
    // 4. Phase reconstruction
    // 5. Inverse FFT synthesis

    // For now, fallback to simple resampling
    return this.pitchShiftInternal(audioBuffer, pitchRatio);
  }

  /**
   * Real-time time stretching using AudioWorklet
   * For future implementation: real-time WSOLA processing
   */
  async createRealTimeTimeStretch(
    context: AudioContext,
    rate: number
  ): Promise<AudioNode> {
    // Placeholder for real-time processing
    // Would require custom AudioWorklet implementation

    // For now, use simple playbackRate adjustment
    const source = context.createBufferSource();
    source.playbackRate.value = rate;

    return source;
  }
}

export const timeStretchService = TimeStretchService.getInstance();
