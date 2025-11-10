/**
 * Mixing Engine
 * Real-time audio mixing with gain nodes, routing, sends/returns, and effects
 *
 * Based on Web Audio API mixing principles:
 * - GainNodes for volume control
 * - Multiple inputs act as unity-gain summing junctions
 * - Send/return buses using parallel routing
 * - Master bus with compression and limiting
 */

import { useAudioStore } from '../store/audioStore';
import { ExtendedTrack } from '../types/recording';

interface TrackChannel {
  trackId: string;
  inputGain: GainNode;
  volumeGain: GainNode;
  panner: StereoPannerNode;
  mute: GainNode;
  sendGains: Map<string, GainNode>;
  insertNodes: AudioNode[];
}

interface SendBus {
  id: string;
  name: string;
  input: GainNode;
  output: GainNode;
  effect: AudioNode | null;
}

export class MixingEngine {
  private static instance: MixingEngine | null = null;

  // Track channels
  private trackChannels: Map<string, TrackChannel> = new Map();

  // Buses
  private masterBus: {
    input: GainNode;
    compressor: DynamicsCompressorNode;
    limiter: DynamicsCompressorNode;
    output: GainNode;
  } | null = null;

  private sendBuses: Map<string, SendBus> = new Map();

  private constructor() {}

  static getInstance(): MixingEngine {
    if (!MixingEngine.instance) {
      MixingEngine.instance = new MixingEngine();
    }
    return MixingEngine.instance;
  }

  /**
   * Initialize mixing engine
   */
  async initialize(): Promise<void> {
    const store = useAudioStore.getState();
    const context = store.audioContext.context;

    if (!context) {
      throw new Error('AudioContext not initialized');
    }

    // Create master bus
    const masterInput = context.createGain();
    const compressor = context.createDynamicsCompressor();
    const limiter = context.createDynamicsCompressor();
    const masterOutput = context.createGain();

    // Configure master compressor (gentle compression)
    compressor.threshold.value = -24;
    compressor.knee.value = 30;
    compressor.ratio.value = 4;
    compressor.attack.value = 0.003;
    compressor.release.value = 0.25;

    // Configure limiter (brick wall)
    limiter.threshold.value = -0.5;
    limiter.knee.value = 0;
    limiter.ratio.value = 20;
    limiter.attack.value = 0.001;
    limiter.release.value = 0.01;

    // Set master volume
    masterOutput.gain.value = store.masterVolume;

    // Connect master chain
    masterInput.connect(compressor);
    compressor.connect(limiter);
    limiter.connect(masterOutput);
    masterOutput.connect(context.destination);

    this.masterBus = {
      input: masterInput,
      compressor,
      limiter,
      output: masterOutput,
    };

    // Create default send buses
    await this.createSendBus('reverb', 'Reverb');
    await this.createSendBus('delay', 'Delay');

    console.log('Mixing engine initialized');
  }

  /**
   * Create a track channel
   */
  createTrackChannel(track: ExtendedTrack): void {
    const store = useAudioStore.getState();
    const context = store.audioContext.context;

    if (!context || !this.masterBus) {
      console.error('Mixing engine not initialized');
      return;
    }

    // Check if already exists
    if (this.trackChannels.has(track.id)) {
      return;
    }

    // Create nodes
    const inputGain = context.createGain();
    const volumeGain = context.createGain();
    const panner = context.createStereoPanner();
    const mute = context.createGain();

    // Set initial values
    inputGain.gain.value = track.inputGain;
    volumeGain.gain.value = track.volume;
    panner.pan.value = track.pan;
    mute.gain.value = track.muted ? 0 : 1;

    // Create send gains
    const sendGains = new Map<string, GainNode>();
    this.sendBuses.forEach((bus, busId) => {
      const sendGain = context.createGain();
      sendGain.gain.value = 0; // Default: no send
      sendGains.set(busId, sendGain);

      // Connect to send bus
      mute.connect(sendGain);
      sendGain.connect(bus.input);
    });

    // Connect main signal path
    // inputGain -> volumeGain -> panner -> mute -> master
    inputGain.connect(volumeGain);
    volumeGain.connect(panner);
    panner.connect(mute);
    mute.connect(this.masterBus.input);

    // Store channel
    this.trackChannels.set(track.id, {
      trackId: track.id,
      inputGain,
      volumeGain,
      panner,
      mute,
      sendGains,
      insertNodes: [],
    });

    console.log(`Created track channel: ${track.name}`);
  }

  /**
   * Remove a track channel
   */
  removeTrackChannel(trackId: string): void {
    const channel = this.trackChannels.get(trackId);
    if (!channel) return;

    // Disconnect all nodes
    channel.inputGain.disconnect();
    channel.volumeGain.disconnect();
    channel.panner.disconnect();
    channel.mute.disconnect();
    channel.sendGains.forEach((gain) => gain.disconnect());

    // Remove from map
    this.trackChannels.delete(trackId);
  }

  /**
   * Get track channel input (for connecting audio sources)
   */
  getTrackInput(trackId: string): GainNode | null {
    const channel = this.trackChannels.get(trackId);
    return channel ? channel.inputGain : null;
  }

  /**
   * Update track volume
   */
  setTrackVolume(trackId: string, volume: number): void {
    const channel = this.trackChannels.get(trackId);
    if (channel) {
      const context = useAudioStore.getState().audioContext.context;
      if (context) {
        channel.volumeGain.gain.setTargetAtTime(volume, context.currentTime, 0.01);
      }
    }
  }

  /**
   * Update track pan
   */
  setTrackPan(trackId: string, pan: number): void {
    const channel = this.trackChannels.get(trackId);
    if (channel) {
      const context = useAudioStore.getState().audioContext.context;
      if (context) {
        channel.panner.pan.setTargetAtTime(pan, context.currentTime, 0.01);
      }
    }
  }

  /**
   * Update track mute
   */
  setTrackMute(trackId: string, muted: boolean): void {
    const channel = this.trackChannels.get(trackId);
    if (channel) {
      const context = useAudioStore.getState().audioContext.context;
      if (context) {
        channel.mute.gain.setTargetAtTime(muted ? 0 : 1, context.currentTime, 0.01);
      }
    }
  }

  /**
   * Update track solo
   */
  setTrackSolo(trackId: string, solo: boolean): void {
    const store = useAudioStore.getState();

    // Get all tracks
    const tracks = store.tracks;

    // Check if any tracks are soloed
    const hasSolo = tracks.some((t) => t.id === trackId ? solo : t.solo);

    // Mute/unmute tracks based on solo state
    tracks.forEach((track) => {
      const channel = this.trackChannels.get(track.id);
      if (channel && useAudioStore.getState().audioContext.context) {
        const context = useAudioStore.getState().audioContext.context!;

        if (hasSolo) {
          // If solo is active, mute all non-soloed tracks
          const isSoloed = track.id === trackId ? solo : track.solo;
          channel.mute.gain.setTargetAtTime(isSoloed ? 1 : 0, context.currentTime, 0.01);
        } else {
          // If no solo, respect mute state
          channel.mute.gain.setTargetAtTime(track.muted ? 0 : 1, context.currentTime, 0.01);
        }
      }
    });
  }

  /**
   * Set send amount for a track
   */
  setTrackSend(trackId: string, sendBusId: string, amount: number): void {
    const channel = this.trackChannels.get(trackId);
    if (channel) {
      const sendGain = channel.sendGains.get(sendBusId);
      if (sendGain) {
        const context = useAudioStore.getState().audioContext.context;
        if (context) {
          sendGain.gain.setTargetAtTime(amount, context.currentTime, 0.01);
        }
      }
    }
  }

  /**
   * Set master volume
   */
  setMasterVolume(volume: number): void {
    if (this.masterBus) {
      const context = useAudioStore.getState().audioContext.context;
      if (context) {
        this.masterBus.output.gain.setTargetAtTime(volume, context.currentTime, 0.01);
      }
    }
  }

  /**
   * Create a send bus
   */
  private async createSendBus(id: string, name: string): Promise<void> {
    const context = useAudioStore.getState().audioContext.context;

    if (!context || !this.masterBus) return;

    const input = context.createGain();
    const output = context.createGain();

    let effect: AudioNode | null = null;

    // Create default effects for known buses
    if (id === 'reverb') {
      effect = await this.createReverbEffect();
    } else if (id === 'delay') {
      effect = context.createDelay(2.0); // 2 second max delay
      (effect as DelayNode).delayTime.value = 0.375; // Dotted eighth at 120 BPM
    }

    // Connect bus
    if (effect) {
      input.connect(effect);
      effect.connect(output);
    } else {
      input.connect(output);
    }
    output.connect(this.masterBus.input);

    // Store bus
    this.sendBuses.set(id, {
      id,
      name,
      input,
      output,
      effect,
    });
  }

  /**
   * Create reverb effect using ConvolverNode
   */
  private async createReverbEffect(): Promise<ConvolverNode> {
    const context = useAudioStore.getState().audioContext.context!;

    // Create convolver
    const convolver = context.createConvolver();

    // Generate impulse response (simple algorithmic reverb)
    // In production, you'd load actual impulse response files
    const length = context.sampleRate * 2; // 2 seconds
    const impulse = context.createBuffer(2, length, context.sampleRate);

    for (let channel = 0; channel < 2; channel++) {
      const channelData = impulse.getChannelData(channel);
      for (let i = 0; i < length; i++) {
        // Exponential decay with random noise
        const decay = Math.pow(1 - i / length, 2);
        channelData[i] = (Math.random() * 2 - 1) * decay;
      }
    }

    convolver.buffer = impulse;

    return convolver;
  }

  /**
   * Insert effect on track
   */
  insertEffect(trackId: string, effect: AudioNode, position: number): void {
    const channel = this.trackChannels.get(trackId);
    if (!channel) return;

    const context = useAudioStore.getState().audioContext.context;
    if (!context) return;

    // Disconnect current chain
    channel.inputGain.disconnect();
    if (channel.insertNodes.length > 0) {
      channel.insertNodes.forEach((node) => node.disconnect());
    }

    // Insert effect at position
    channel.insertNodes.splice(position, 0, effect);

    // Rebuild chain: inputGain -> inserts -> volumeGain
    let prevNode: AudioNode = channel.inputGain;

    channel.insertNodes.forEach((node) => {
      prevNode.connect(node);
      prevNode = node;
    });

    prevNode.connect(channel.volumeGain);
  }

  /**
   * Remove effect from track
   */
  removeEffect(trackId: string, position: number): void {
    const channel = this.trackChannels.get(trackId);
    if (!channel) return;

    const context = useAudioStore.getState().audioContext.context;
    if (!context) return;

    // Disconnect current chain
    channel.inputGain.disconnect();
    if (channel.insertNodes.length > 0) {
      channel.insertNodes.forEach((node) => node.disconnect());
    }

    // Remove effect
    channel.insertNodes.splice(position, 1);

    // Rebuild chain
    let prevNode: AudioNode = channel.inputGain;

    channel.insertNodes.forEach((node) => {
      prevNode.connect(node);
      prevNode = node;
    });

    prevNode.connect(channel.volumeGain);
  }

  /**
   * Get master bus compressor for metering
   */
  getMasterCompressor(): DynamicsCompressorNode | null {
    return this.masterBus?.compressor || null;
  }
}

export const mixingEngine = MixingEngine.getInstance();
