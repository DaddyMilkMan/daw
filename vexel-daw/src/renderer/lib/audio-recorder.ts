/**
 * Audio Recording Service using MediaRecorder and getUserMedia
 * Provides microphone/line input recording capabilities
 */

export interface RecordingConfig {
  sampleRate?: number;
  channels?: number;
  mimeType?: string;
}

export class AudioRecorder {
  private mediaRecorder: MediaRecorder | null = null;
  private audioStream: MediaStream | null = null;
  private recordedChunks: Blob[] = [];
  private isRecording: boolean = false;

  // Callbacks
  private onDataAvailable?: (blob: Blob) => void;
  private onRecordingComplete?: (blob: Blob) => void;
  private onError?: (error: Error) => void;

  /**
   * Request microphone access and initialize recorder
   */
  async initialize(config: RecordingConfig = {}): Promise<void> {
    try {
      // Request microphone access
      this.audioStream = await navigator.mediaDevices.getUserMedia({
        audio: {
          sampleRate: config.sampleRate || 48000,
          channelCount: config.channels || 2,
          echoCancellation: false,
          noiseSuppression: false,
          autoGainControl: false,
        },
      });

      // Determine supported MIME type
      const mimeType = config.mimeType || this.getSupportedMimeType();

      // Create MediaRecorder
      this.mediaRecorder = new MediaRecorder(this.audioStream, {
        mimeType,
      });

      // Set up event handlers
      this.mediaRecorder.ondataavailable = (event) => {
        if (event.data.size > 0) {
          this.recordedChunks.push(event.data);
          this.onDataAvailable?.(event.data);
        }
      };

      this.mediaRecorder.onstop = () => {
        const blob = new Blob(this.recordedChunks, {
          type: this.mediaRecorder?.mimeType || 'audio/webm',
        });
        this.onRecordingComplete?.(blob);
        this.recordedChunks = [];
      };

      this.mediaRecorder.onerror = (event: Event) => {
        const errorEvent = event as ErrorEvent;
        this.onError?.(new Error(errorEvent.message || 'Recording error'));
      };

    } catch (error) {
      throw new Error(`Failed to initialize recorder: ${error}`);
    }
  }

  /**
   * Get the best supported MIME type for recording
   */
  private getSupportedMimeType(): string {
    const types = [
      'audio/webm;codecs=opus',
      'audio/webm',
      'audio/ogg;codecs=opus',
      'audio/ogg',
      'audio/mp4',
    ];

    for (const type of types) {
      if (MediaRecorder.isTypeSupported(type)) {
        return type;
      }
    }

    return 'audio/webm'; // Fallback
  }

  /**
   * Start recording
   * @param timeslice - Optional timeslice in ms for chunked recording
   */
  startRecording(timeslice?: number): void {
    if (!this.mediaRecorder) {
      throw new Error('Recorder not initialized');
    }

    if (this.isRecording) {
      throw new Error('Already recording');
    }

    this.recordedChunks = [];
    this.isRecording = true;

    if (timeslice) {
      this.mediaRecorder.start(timeslice);
    } else {
      this.mediaRecorder.start();
    }
  }

  /**
   * Stop recording
   */
  stopRecording(): void {
    if (!this.mediaRecorder || !this.isRecording) {
      return;
    }

    this.isRecording = false;
    this.mediaRecorder.stop();
  }

  /**
   * Pause recording
   */
  pauseRecording(): void {
    if (this.mediaRecorder && this.isRecording && this.mediaRecorder.state === 'recording') {
      this.mediaRecorder.pause();
    }
  }

  /**
   * Resume recording
   */
  resumeRecording(): void {
    if (this.mediaRecorder && this.isRecording && this.mediaRecorder.state === 'paused') {
      this.mediaRecorder.resume();
    }
  }

  /**
   * Get audio stream for live monitoring
   */
  getAudioStream(): MediaStream | null {
    return this.audioStream;
  }

  /**
   * Convert recorded blob to AudioBuffer
   */
  async blobToAudioBuffer(blob: Blob, audioContext: AudioContext): Promise<AudioBuffer> {
    const arrayBuffer = await blob.arrayBuffer();
    return await audioContext.decodeAudioData(arrayBuffer);
  }

  /**
   * Convert recorded blob to WAV file
   */
  async blobToWav(blob: Blob): Promise<Blob> {
    // If already WAV, return as-is
    if (blob.type === 'audio/wav') {
      return blob;
    }

    // For WebM/Ogg, we need to decode and re-encode as WAV
    // This requires an AudioContext
    const audioContext = new AudioContext();
    const arrayBuffer = await blob.arrayBuffer();
    const audioBuffer = await audioContext.decodeAudioData(arrayBuffer);

    // Convert AudioBuffer to WAV
    const wavBlob = await this.audioBufferToWav(audioBuffer);
    await audioContext.close();

    return wavBlob;
  }

  /**
   * Convert AudioBuffer to WAV Blob
   */
  private async audioBufferToWav(audioBuffer: AudioBuffer): Promise<Blob> {
    const numberOfChannels = audioBuffer.numberOfChannels;
    const sampleRate = audioBuffer.sampleRate;
    const length = audioBuffer.length * numberOfChannels * 2;

    const buffer = new ArrayBuffer(44 + length);
    const view = new DataView(buffer);

    // WAV header
    const writeString = (offset: number, string: string) => {
      for (let i = 0; i < string.length; i++) {
        view.setUint8(offset + i, string.charCodeAt(i));
      }
    };

    writeString(0, 'RIFF');
    view.setUint32(4, 36 + length, true);
    writeString(8, 'WAVE');
    writeString(12, 'fmt ');
    view.setUint32(16, 16, true); // Subchunk1Size
    view.setUint16(20, 1, true); // AudioFormat (PCM)
    view.setUint16(22, numberOfChannels, true);
    view.setUint32(24, sampleRate, true);
    view.setUint32(28, sampleRate * numberOfChannels * 2, true); // ByteRate
    view.setUint16(32, numberOfChannels * 2, true); // BlockAlign
    view.setUint16(34, 16, true); // BitsPerSample
    writeString(36, 'data');
    view.setUint32(40, length, true);

    // Write audio data
    const channels = [];
    for (let i = 0; i < numberOfChannels; i++) {
      channels.push(audioBuffer.getChannelData(i));
    }

    let offset = 44;
    for (let i = 0; i < audioBuffer.length; i++) {
      for (let channel = 0; channel < numberOfChannels; channel++) {
        const sample = Math.max(-1, Math.min(1, channels[channel][i]));
        view.setInt16(offset, sample < 0 ? sample * 0x8000 : sample * 0x7fff, true);
        offset += 2;
      }
    }

    return new Blob([buffer], { type: 'audio/wav' });
  }

  /**
   * Get recording state
   */
  getState(): { isRecording: boolean; state: RecordingState | null } {
    return {
      isRecording: this.isRecording,
      state: this.mediaRecorder?.state || null,
    };
  }

  /**
   * Set callbacks
   */
  setOnDataAvailable(callback: (blob: Blob) => void): void {
    this.onDataAvailable = callback;
  }

  setOnRecordingComplete(callback: (blob: Blob) => void): void {
    this.onRecordingComplete = callback;
  }

  setOnError(callback: (error: Error) => void): void {
    this.onError = callback;
  }

  /**
   * Cleanup and release resources
   */
  dispose(): void {
    if (this.mediaRecorder && this.isRecording) {
      this.stopRecording();
    }

    if (this.audioStream) {
      this.audioStream.getTracks().forEach(track => track.stop());
      this.audioStream = null;
    }

    this.mediaRecorder = null;
    this.recordedChunks = [];
    this.isRecording = false;
  }
}

// Export recording state type
export type RecordingState = 'inactive' | 'recording' | 'paused';
