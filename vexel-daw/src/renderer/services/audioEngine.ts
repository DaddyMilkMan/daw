/**
 * Audio Engine Service
 * Manages Web Audio API, recording, playback, and MIDI
 */

import { useAudioStore } from '../store/audioStore';
import { RecordingTrackState } from '../types/recording';

export class AudioEngine {
  private static instance: AudioEngine | null = null;
  private recorderWorklet: AudioWorkletNode | null = null;
  private animationFrameId: number | null = null;
  private started: boolean = false;

  private constructor() {}

  static getInstance(): AudioEngine {
    if (!AudioEngine.instance) {
      AudioEngine.instance = new AudioEngine();
    }
    return AudioEngine.instance;
  }

  /**
   * Initialize the audio engine
   */
  async initialize(): Promise<void> {
    const store = useAudioStore.getState();

    if (store.audioContext.initialized) {
      console.log('Audio engine already initialized');
      return;
    }

    try {
      await store.initializeAudioContext();
      await store.initializeMIDI();

      const context = store.audioContext.context!;

      // Load AudioWorklet module
      await context.audioWorklet.addModule('/audio-recorder-worklet.js');

      console.log('Audio engine initialized successfully');
    } catch (error) {
      console.error('Failed to initialize audio engine:', error);
      throw error;
    }
  }

  /**
   * Start the audio engine (begins transport and monitoring)
   */
  start(): void {
    if (this.started) return;

    this.started = true;
    this.startTransportLoop();
    this.startLevelMonitoring();

    console.log('Audio engine started');
  }

  /**
   * Stop the audio engine
   */
  stop(): void {
    if (!this.started) return;

    this.started = false;

    if (this.animationFrameId !== null) {
      cancelAnimationFrame(this.animationFrameId);
      this.animationFrameId = null;
    }

    console.log('Audio engine stopped');
  }

  /**
   * Set up audio input for recording
   */
  async setupAudioInput(deviceId: string): Promise<void> {
    const store = useAudioStore.getState();

    try {
      await store.selectInputDevice(deviceId);

      const { audioContext, audioInput } = store;

      if (!audioContext.context || !audioInput.source) {
        throw new Error('Audio context or input source not initialized');
      }

      // Create AudioWorklet for recording
      this.recorderWorklet = new AudioWorkletNode(
        audioContext.context,
        'audio-recorder-processor',
        {
          numberOfInputs: 1,
          numberOfOutputs: 1,
          channelCount: 2,
        }
      );

      // Handle messages from worklet
      this.recorderWorklet.port.onmessage = this.handleWorkletMessage.bind(this);

      // Connect input source to worklet
      audioInput.source.connect(this.recorderWorklet);

      // Connect worklet to destination if monitoring is enabled
      if (store.recording.inputMonitoring) {
        this.recorderWorklet.connect(audioContext.compressor!);
      }

      console.log('Audio input configured');
    } catch (error) {
      console.error('Failed to setup audio input:', error);
      throw error;
    }
  }

  /**
   * Start recording on armed tracks
   */
  async startRecording(): Promise<void> {
    const store = useAudioStore.getState();

    if (!store.audioContext.initialized) {
      await this.initialize();
    }

    // Ensure we have a recorder worklet
    if (!this.recorderWorklet) {
      const inputDevice = store.audioSettings.inputDevice;
      if (inputDevice) {
        await this.setupAudioInput(inputDevice);
      }
    }

    // Start recording in store (handles pre-count)
    await store.startRecording();

    // Send start message to worklet once pre-count is done
    const checkPreCount = setInterval(() => {
      const currentStore = useAudioStore.getState();
      if (currentStore.recording.isRecording && currentStore.recording.preCountRemaining === 0) {
        clearInterval(checkPreCount);
        this.recorderWorklet?.port.postMessage({ type: 'start' });
      }
    }, 100);
  }

  /**
   * Stop recording
   */
  async stopRecording(): Promise<void> {
    if (this.recorderWorklet) {
      this.recorderWorklet.port.postMessage({ type: 'stop' });
    }

    const store = useAudioStore.getState();
    await store.stopRecording();
  }

  /**
   * Toggle input monitoring
   */
  toggleInputMonitoring(enabled: boolean): void {
    const store = useAudioStore.getState();
    const { audioContext } = store;

    if (!this.recorderWorklet || !audioContext.compressor) {
      return;
    }

    if (enabled) {
      this.recorderWorklet.connect(audioContext.compressor);
    } else {
      this.recorderWorklet.disconnect(audioContext.compressor);
    }

    useAudioStore.setState((state) => ({
      recording: {
        ...state.recording,
        inputMonitoring: enabled,
      },
    }));
  }

  /**
   * Handle messages from AudioWorklet
   */
  private handleWorkletMessage(event: MessageEvent): void {
    const { type, channelData, timestamp } = event.data;

    switch (type) {
      case 'audioData':
        this.handleAudioData(channelData, timestamp);
        break;

      case 'started':
        console.log('Worklet recording started');
        break;

      case 'stopped':
        console.log('Worklet recording stopped');
        break;

      default:
        console.warn('Unknown worklet message:', type);
    }
  }

  /**
   * Handle incoming audio data from worklet
   */
  private handleAudioData(channelData: Float32Array[], timestamp: number): void {
    const store = useAudioStore.getState();

    if (!store.recording.isRecording) {
      return;
    }

    // Add audio data to all recording tracks
    store.recording.recordingTracks.forEach((trackState: RecordingTrackState, trackId: string) => {
      const track = store.tracks.find((t) => t.id === trackId);

      if (track && track.type === 'audio') {
        // Combine stereo channels into interleaved format
        const interleavedData = interleaveChannels(channelData);
        trackState.recordedData.push(interleavedData);

        // Calculate input level for meters
        const peak = calculatePeak(channelData[0]);
        const rms = calculateRMS(channelData[0]);

        store.updateInputLevel(trackId, {
          trackId,
          peak,
          rms,
          clipping: peak > 0.99,
        });
      }
    });
  }

  /**
   * Transport loop - updates currentBeat based on tempo
   */
  private startTransportLoop(): void {
    const loop = () => {
      const store = useAudioStore.getState();

      if (store.isPlaying && this.started) {
        const { tempo, timeSignature, currentBeat, looping, loopStart, loopEnd } = store;

        // Calculate beats per second
        const beatsPerSecond = tempo / 60;
        const deltaBeats = beatsPerSecond / 60; // Assuming 60 FPS

        let newBeat = currentBeat + deltaBeats;

        // Handle looping
        if (looping && newBeat >= loopEnd) {
          newBeat = loopStart;
        }

        store.setCurrentBeat(newBeat);
      }

      if (this.started) {
        this.animationFrameId = requestAnimationFrame(loop);
      }
    };

    loop();
  }

  /**
   * Level monitoring - updates input levels for armed tracks
   */
  private startLevelMonitoring(): void {
    const monitor = () => {
      const store = useAudioStore.getState();

      if (!this.started) return;

      const { audioInput, tracks, recording } = store;

      if (audioInput.analyser) {
        const dataArray = new Float32Array(audioInput.analyser.fftSize);
        audioInput.analyser.getFloatTimeDomainData(dataArray);

        const peak = calculatePeak(dataArray);
        const rms = calculateRMS(dataArray);

        // Update levels for all armed tracks
        recording.armedTracks.forEach((trackId) => {
          store.updateInputLevel(trackId, {
            trackId,
            peak,
            rms,
            clipping: peak > 0.99,
          });
        });
      }

      if (this.started) {
        setTimeout(monitor, 50); // Update every 50ms (20 FPS)
      }
    };

    monitor();
  }

  /**
   * Get available audio input devices
   */
  async getInputDevices(): Promise<MediaDeviceInfo[]> {
    const store = useAudioStore.getState();
    return await store.getInputDevices();
  }

  /**
   * Get available MIDI input devices
   */
  getMIDIInputDevices(): Map<string, MIDIInput> {
    const store = useAudioStore.getState();
    return store.midiInput.inputs;
  }

  /**
   * Select MIDI input device
   */
  selectMIDIInput(deviceId: string): void {
    const store = useAudioStore.getState();
    store.selectMIDIInput(deviceId);
  }
}

// Helper Functions

/**
 * Interleave stereo channels into a single array
 */
function interleaveChannels(channels: Float32Array[]): Float32Array {
  if (channels.length === 0) {
    return new Float32Array(0);
  }

  if (channels.length === 1) {
    return channels[0];
  }

  const length = channels[0].length;
  const result = new Float32Array(length * channels.length);

  for (let i = 0; i < length; i++) {
    for (let ch = 0; ch < channels.length; ch++) {
      result[i * channels.length + ch] = channels[ch][i];
    }
  }

  return result;
}

/**
 * Calculate peak level (0-1)
 */
function calculatePeak(data: Float32Array): number {
  let peak = 0;

  for (let i = 0; i < data.length; i++) {
    const abs = Math.abs(data[i]);
    if (abs > peak) {
      peak = abs;
    }
  }

  return peak;
}

/**
 * Calculate RMS (Root Mean Square) level (0-1)
 */
function calculateRMS(data: Float32Array): number {
  let sum = 0;

  for (let i = 0; i < data.length; i++) {
    sum += data[i] * data[i];
  }

  return Math.sqrt(sum / data.length);
}

// Export singleton instance
export const audioEngine = AudioEngine.getInstance();
