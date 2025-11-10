// Web Audio API Engine for Zenith DAW
// Handles audio playback, mixing, and processing

import { AudioClip, FadeCurve, CrossfadeCurve } from '../types/clip';

export class AudioEngine {
  private static instance: AudioEngine | null = null;
  private audioContext: AudioContext | null = null;
  private masterGain: GainNode | null = null;
  private isInitialized = false;
  private activeSourceNodes: Map<string, AudioBufferSourceNode> = new Map();
  private scheduledNodes: Map<string, AudioBufferSourceNode> = new Map();

  private constructor() {}

  static getInstance(): AudioEngine {
    if (!AudioEngine.instance) {
      AudioEngine.instance = new AudioEngine();
    }
    return AudioEngine.instance;
  }

  // Initialize Audio Context (must be called after user interaction)
  async initialize(): Promise<void> {
    if (this.isInitialized) return;

    try {
      // Create AudioContext with optimal settings
      this.audioContext = new (window.AudioContext || (window as any).webkitAudioContext)({
        latencyHint: 'playback',
        sampleRate: 44100,
      });

      // Create master gain node
      this.masterGain = this.audioContext.createGain();
      this.masterGain.connect(this.audioContext.destination);
      this.masterGain.gain.value = 0.8; // Prevent clipping

      // Resume context if suspended (mobile browsers)
      if (this.audioContext.state === 'suspended') {
        await this.audioContext.resume();
      }

      this.isInitialized = true;
      console.log('🎵 AudioEngine initialized:', {
        sampleRate: this.audioContext.sampleRate,
        state: this.audioContext.state,
        baseLatency: this.audioContext.baseLatency,
      });
    } catch (error) {
      console.error('Failed to initialize AudioEngine:', error);
      throw error;
    }
  }

  getContext(): AudioContext | null {
    return this.audioContext;
  }

  isReady(): boolean {
    return this.isInitialized && this.audioContext !== null && this.audioContext.state === 'running';
  }

  // Load audio file from path or blob
  async loadAudioFile(file: File): Promise<AudioBuffer> {
    if (!this.audioContext) {
      throw new Error('AudioContext not initialized');
    }

    try {
      const arrayBuffer = await file.arrayBuffer();
      const audioBuffer = await this.audioContext.decodeAudioData(arrayBuffer);
      return audioBuffer;
    } catch (error) {
      console.error('Failed to load audio file:', error);
      throw error;
    }
  }

  // Load audio from base64 string (for project loading)
  async loadAudioFromBase64(base64: string): Promise<AudioBuffer> {
    if (!this.audioContext) {
      throw new Error('AudioContext not initialized');
    }

    try {
      // Remove data URL prefix if present
      const base64Data = base64.includes(',') ? base64.split(',')[1] : base64;

      // Decode base64 to binary
      const binaryString = atob(base64Data);
      const len = binaryString.length;
      const bytes = new Uint8Array(len);
      for (let i = 0; i < len; i++) {
        bytes[i] = binaryString.charCodeAt(i);
      }

      const audioBuffer = await this.audioContext.decodeAudioData(bytes.buffer);
      return audioBuffer;
    } catch (error) {
      console.error('Failed to load audio from base64:', error);
      throw error;
    }
  }

  // Convert AudioBuffer to base64 for serialization
  async audioBufferToBase64(buffer: AudioBuffer): Promise<string> {
    // Convert AudioBuffer to WAV format
    const wav = this.audioBufferToWav(buffer);

    // Convert to base64
    let binary = '';
    const bytes = new Uint8Array(wav);
    const len = bytes.byteLength;
    for (let i = 0; i < len; i++) {
      binary += String.fromCharCode(bytes[i]);
    }

    return `data:audio/wav;base64,${btoa(binary)}`;
  }

  // Convert AudioBuffer to WAV format (for serialization)
  private audioBufferToWav(buffer: AudioBuffer): ArrayBuffer {
    const numChannels = buffer.numberOfChannels;
    const sampleRate = buffer.sampleRate;
    const format = 1; // PCM
    const bitDepth = 16;

    const bytesPerSample = bitDepth / 8;
    const blockAlign = numChannels * bytesPerSample;

    const data = new Float32Array(buffer.length * numChannels);
    for (let channel = 0; channel < numChannels; channel++) {
      const channelData = buffer.getChannelData(channel);
      for (let i = 0; i < buffer.length; i++) {
        data[i * numChannels + channel] = channelData[i];
      }
    }

    const dataLength = data.length * bytesPerSample;
    const bufferLength = 44 + dataLength;
    const arrayBuffer = new ArrayBuffer(bufferLength);
    const view = new DataView(arrayBuffer);

    // Write WAV header
    const writeString = (offset: number, string: string) => {
      for (let i = 0; i < string.length; i++) {
        view.setUint8(offset + i, string.charCodeAt(i));
      }
    };

    writeString(0, 'RIFF');
    view.setUint32(4, 36 + dataLength, true);
    writeString(8, 'WAVE');
    writeString(12, 'fmt ');
    view.setUint32(16, 16, true); // fmt chunk size
    view.setUint16(20, format, true);
    view.setUint16(22, numChannels, true);
    view.setUint32(24, sampleRate, true);
    view.setUint32(28, sampleRate * blockAlign, true);
    view.setUint16(32, blockAlign, true);
    view.setUint16(34, bitDepth, true);
    writeString(36, 'data');
    view.setUint32(40, dataLength, true);

    // Write audio data
    let offset = 44;
    for (let i = 0; i < data.length; i++) {
      const sample = Math.max(-1, Math.min(1, data[i]));
      view.setInt16(offset, sample < 0 ? sample * 0x8000 : sample * 0x7fff, true);
      offset += 2;
    }

    return arrayBuffer;
  }

  // Play a single clip (for preview)
  async playClip(clip: AudioClip, startTime?: number): Promise<void> {
    if (!this.audioContext || !this.masterGain || !clip.audioFile?.buffer) {
      throw new Error('AudioContext or clip buffer not available');
    }

    // Stop existing playback for this clip
    this.stopClip(clip.id);

    const source = this.audioContext.createBufferSource();
    source.buffer = clip.audioFile.buffer;
    source.playbackRate.value = clip.playbackRate;

    // Create gain node for this clip
    const gainNode = this.audioContext.createGain();
    gainNode.gain.value = clip.gain;

    // Create panner node for stereo positioning
    const panNode = this.audioContext.createStereoPanner();
    panNode.pan.value = clip.pan;

    // Apply fade in/out
    const now = this.audioContext.currentTime;
    const actualStartTime = startTime || now;

    if (clip.fadeIn > 0) {
      gainNode.gain.setValueAtTime(0, actualStartTime);
      this.applyFadeCurve(gainNode.gain, 0, clip.gain, actualStartTime, clip.fadeIn, clip.fadeInCurve);
    }

    const clipDuration = this.getClipDuration(clip);
    if (clip.fadeOut > 0) {
      const fadeOutStart = actualStartTime + clipDuration - clip.fadeOut;
      this.applyFadeCurve(gainNode.gain, clip.gain, 0, fadeOutStart, clip.fadeOut, clip.fadeOutCurve);
    }

    // Connect nodes
    source.connect(gainNode);
    gainNode.connect(panNode);
    panNode.connect(this.masterGain);

    // Store reference
    this.activeSourceNodes.set(clip.id, source);

    // Start playback with trimming
    const offset = clip.trimStart;
    const duration = clipDuration;
    source.start(actualStartTime, offset, duration);

    // Auto-cleanup when finished
    source.onended = () => {
      this.activeSourceNodes.delete(clip.id);
      gainNode.disconnect();
      panNode.disconnect();
    };
  }

  // Stop clip playback
  stopClip(clipId: string): void {
    const source = this.activeSourceNodes.get(clipId);
    if (source) {
      try {
        source.stop();
        source.disconnect();
      } catch (e) {
        // Already stopped
      }
      this.activeSourceNodes.delete(clipId);
    }
  }

  // Stop all playback
  stopAll(): void {
    this.activeSourceNodes.forEach((source) => {
      try {
        source.stop();
        source.disconnect();
      } catch (e) {
        // Already stopped
      }
    });
    this.activeSourceNodes.clear();
    this.scheduledNodes.clear();
  }

  // Apply fade curve to AudioParam
  private applyFadeCurve(
    param: AudioParam,
    startValue: number,
    endValue: number,
    startTime: number,
    duration: number,
    curve: FadeCurve
  ): void {
    param.cancelScheduledValues(startTime);
    param.setValueAtTime(startValue, startTime);

    switch (curve) {
      case 'linear':
        param.linearRampToValueAtTime(endValue, startTime + duration);
        break;
      case 'exponential':
        // Exponential ramp can't go to/from 0, so use a small value
        const safeEndValue = endValue === 0 ? 0.0001 : endValue;
        param.exponentialRampToValueAtTime(safeEndValue, startTime + duration);
        break;
      case 'logarithmic':
        // Simulate logarithmic with exponential (inverse)
        param.exponentialRampToValueAtTime(endValue === 0 ? 0.0001 : endValue, startTime + duration);
        break;
      case 'sCurve':
        // S-curve using setValueCurveAtTime
        const curveLength = 256;
        const curveArray = new Float32Array(curveLength);
        for (let i = 0; i < curveLength; i++) {
          const t = i / (curveLength - 1);
          const sCurveValue = this.sCurveInterpolate(t);
          curveArray[i] = startValue + (endValue - startValue) * sCurveValue;
        }
        param.setValueCurveAtTime(curveArray, startTime, duration);
        break;
    }
  }

  // S-curve interpolation (smooth ease in/out)
  private sCurveInterpolate(t: number): number {
    return t * t * (3 - 2 * t);
  }

  // Apply crossfade curve
  applyCrossfadeCurve(curve: CrossfadeCurve, position: number): { out: number; in: number } {
    // position: 0 = fully out, 1 = fully in
    switch (curve) {
      case 'linear':
        return { out: 1 - position, in: position };
      case 'equalPower':
        // Equal power crossfade: sqrt(cos/sin) curves
        const angle = position * Math.PI * 0.5;
        return {
          out: Math.cos(angle),
          in: Math.sin(angle),
        };
      case 'logarithmic':
        return {
          out: Math.pow(1 - position, 2),
          in: Math.pow(position, 2),
        };
    }
  }

  // Get actual playback duration of clip considering trim and stretch
  getClipDuration(clip: AudioClip): number {
    if (!clip.audioFile) return 0;
    const fileDuration = clip.audioFile.duration;
    const trimmedDuration = fileDuration - clip.trimStart - clip.trimEnd;
    return trimmedDuration / clip.playbackRate;
  }

  // Set master volume
  setMasterVolume(volume: number): void {
    if (this.masterGain) {
      this.masterGain.gain.setValueAtTime(volume, this.audioContext!.currentTime);
    }
  }

  // Clean up resources
  dispose(): void {
    this.stopAll();
    if (this.audioContext) {
      this.audioContext.close();
      this.audioContext = null;
    }
    this.masterGain = null;
    this.isInitialized = false;
  }
}

// Export singleton instance
export const audioEngine = AudioEngine.getInstance();
