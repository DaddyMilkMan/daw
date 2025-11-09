/**
 * Audio Recorder Worklet
 * Low-latency audio recording processor
 * Runs in separate audio thread for optimal performance
 */

class AudioRecorderProcessor extends AudioWorkletProcessor {
  constructor() {
    super();
    this.isRecording = false;
    this.bufferSize = 128; // Process in chunks of 128 samples
    this.port.onmessage = this.handleMessage.bind(this);
  }

  handleMessage(event) {
    const { type } = event.data;

    switch (type) {
      case 'start':
        this.isRecording = true;
        this.port.postMessage({ type: 'started' });
        break;

      case 'stop':
        this.isRecording = false;
        this.port.postMessage({ type: 'stopped' });
        break;

      default:
        console.warn('Unknown message type:', type);
    }
  }

  process(inputs, outputs, parameters) {
    const input = inputs[0];

    if (!input || !input.length) {
      return true; // Keep processor alive
    }

    if (this.isRecording) {
      // Send audio data to main thread
      // Input is an array of channels, each channel is a Float32Array
      const channelData = [];

      for (let channel = 0; channel < input.length; channel++) {
        // Copy the channel data
        const data = new Float32Array(input[channel]);
        channelData.push(data);
      }

      // Post audio data to main thread
      this.port.postMessage({
        type: 'audioData',
        channelData,
        timestamp: currentTime,
      });
    }

    // Monitor mode: pass input to output
    for (let channel = 0; channel < Math.min(input.length, outputs[0].length); channel++) {
      outputs[0][channel].set(input[channel]);
    }

    return true; // Keep processor alive
  }
}

registerProcessor('audio-recorder-processor', AudioRecorderProcessor);
