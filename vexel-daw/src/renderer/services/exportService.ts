/**
 * Export Service
 * Handles WAV audio export and MIDI file export
 */

import { AudioClip, MIDIClip, MIDINote } from '../types/recording';

export class ExportService {
  /**
   * Export AudioBuffer to WAV file
   */
  static async exportToWAV(
    audioBuffer: AudioBuffer,
    fileName: string = 'recording.wav',
    bitDepth: 16 | 24 | 32 = 32
  ): Promise<Blob> {
    const numberOfChannels = audioBuffer.numberOfChannels;
    const sampleRate = audioBuffer.sampleRate;
    const length = audioBuffer.length;

    // Interleave channels
    const interleaved = this.interleaveChannels(audioBuffer);

    // Convert to appropriate bit depth
    let audioData: ArrayBuffer;

    if (bitDepth === 16) {
      audioData = this.floatTo16BitPCM(interleaved);
    } else if (bitDepth === 24) {
      audioData = this.floatTo24BitPCM(interleaved);
    } else {
      audioData = this.floatTo32BitPCM(interleaved);
    }

    // Create WAV file
    const wavBlob = this.encodeWAV(audioData, numberOfChannels, sampleRate, bitDepth);

    return wavBlob;
  }

  /**
   * Download WAV file
   */
  static downloadWAV(blob: Blob, fileName: string): void {
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = fileName;
    a.click();
    URL.revokeObjectURL(url);
  }

  /**
   * Export audio clip to WAV
   */
  static async exportAudioClipToWAV(
    clip: AudioClip,
    fileName?: string,
    bitDepth: 16 | 24 | 32 = 32
  ): Promise<Blob> {
    if (!clip.audioBuffer) {
      throw new Error('Audio clip has no audio buffer');
    }

    const name = fileName || `${clip.name}.wav`;
    return this.exportToWAV(clip.audioBuffer, name, bitDepth);
  }

  /**
   * Export MIDI clip to .mid file
   */
  static exportMIDIClipToFile(
    clip: MIDIClip,
    tempo: number = 120,
    timeSignature: { numerator: number; denominator: number } = { numerator: 4, denominator: 4 }
  ): Blob {
    const midiData = this.createMIDIFile(clip.notes, tempo, timeSignature);
    return new Blob([midiData], { type: 'audio/midi' });
  }

  /**
   * Download MIDI file
   */
  static downloadMIDI(blob: Blob, fileName: string): void {
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = fileName;
    a.click();
    URL.revokeObjectURL(url);
  }

  /**
   * Create MIDI file from notes
   */
  private static createMIDIFile(
    notes: MIDINote[],
    tempo: number,
    timeSignature: { numerator: number; denominator: number }
  ): Uint8Array {
    // MIDI file format
    const header = this.createMIDIHeader(1, 1, 480); // Format 1, 1 track, 480 PPQ
    const track = this.createMIDITrack(notes, tempo, timeSignature, 480);

    const midiFile = new Uint8Array(header.length + track.length);
    midiFile.set(header, 0);
    midiFile.set(track, header.length);

    return midiFile;
  }

  /**
   * Create MIDI file header
   */
  private static createMIDIHeader(
    format: number,
    numTracks: number,
    ticksPerBeat: number
  ): Uint8Array {
    const header = new Uint8Array(14);

    // MThd chunk
    header[0] = 0x4d; // M
    header[1] = 0x54; // T
    header[2] = 0x68; // h
    header[3] = 0x64; // d

    // Header length (always 6)
    header[4] = 0x00;
    header[5] = 0x00;
    header[6] = 0x00;
    header[7] = 0x06;

    // Format
    header[8] = 0x00;
    header[9] = format;

    // Number of tracks
    header[10] = (numTracks >> 8) & 0xff;
    header[11] = numTracks & 0xff;

    // Ticks per beat (PPQ)
    header[12] = (ticksPerBeat >> 8) & 0xff;
    header[13] = ticksPerBeat & 0xff;

    return header;
  }

  /**
   * Create MIDI track from notes
   */
  private static createMIDITrack(
    notes: MIDINote[],
    tempo: number,
    timeSignature: { numerator: number; denominator: number },
    ticksPerBeat: number
  ): Uint8Array {
    const events: number[] = [];

    // Track header
    events.push(0x4d, 0x54, 0x72, 0x6b); // MTrk

    // Placeholder for track length (will be filled later)
    const trackLengthPos = events.length;
    events.push(0x00, 0x00, 0x00, 0x00);

    // Time signature meta event
    events.push(...this.writeVariableLength(0)); // Delta time
    events.push(0xff, 0x58, 0x04); // Time signature meta event
    events.push(timeSignature.numerator);
    events.push(Math.log2(timeSignature.denominator));
    events.push(24); // MIDI clocks per metronome click
    events.push(8); // 32nd notes per quarter note

    // Tempo meta event
    events.push(...this.writeVariableLength(0)); // Delta time
    events.push(0xff, 0x51, 0x03); // Tempo meta event
    const microsecondsPerBeat = Math.floor(60000000 / tempo);
    events.push((microsecondsPerBeat >> 16) & 0xff);
    events.push((microsecondsPerBeat >> 8) & 0xff);
    events.push(microsecondsPerBeat & 0xff);

    // Sort notes by start time
    const sortedNotes = [...notes].sort((a, b) => a.start - b.start);

    // Convert notes to MIDI events
    const midiEvents: Array<{ time: number; type: 'noteOn' | 'noteOff'; pitch: number; velocity: number }> = [];

    for (const note of sortedNotes) {
      // Note On
      midiEvents.push({
        time: note.start,
        type: 'noteOn',
        pitch: note.pitch,
        velocity: note.velocity,
      });

      // Note Off
      midiEvents.push({
        time: note.start + note.length,
        type: 'noteOff',
        pitch: note.pitch,
        velocity: 0,
      });
    }

    // Sort all events by time
    midiEvents.sort((a, b) => a.time - b.time);

    // Write events with delta times
    let lastTime = 0;
    for (const event of midiEvents) {
      const ticks = Math.floor(event.time * ticksPerBeat);
      const deltaTime = ticks - lastTime;

      events.push(...this.writeVariableLength(deltaTime));

      if (event.type === 'noteOn') {
        events.push(0x90); // Note On, channel 0
        events.push(event.pitch);
        events.push(event.velocity);
      } else {
        events.push(0x80); // Note Off, channel 0
        events.push(event.pitch);
        events.push(0x40); // Default release velocity
      }

      lastTime = ticks;
    }

    // End of track
    events.push(...this.writeVariableLength(0)); // Delta time
    events.push(0xff, 0x2f, 0x00); // End of track meta event

    // Fill in track length
    const trackLength = events.length - trackLengthPos - 4;
    events[trackLengthPos] = (trackLength >> 24) & 0xff;
    events[trackLengthPos + 1] = (trackLength >> 16) & 0xff;
    events[trackLengthPos + 2] = (trackLength >> 8) & 0xff;
    events[trackLengthPos + 3] = trackLength & 0xff;

    return new Uint8Array(events);
  }

  /**
   * Write variable length quantity (MIDI format)
   */
  private static writeVariableLength(value: number): number[] {
    const bytes: number[] = [];

    bytes.push(value & 0x7f);

    while (value >>= 7) {
      bytes.unshift((value & 0x7f) | 0x80);
    }

    return bytes;
  }

  /**
   * Interleave audio buffer channels
   */
  private static interleaveChannels(audioBuffer: AudioBuffer): Float32Array {
    const numberOfChannels = audioBuffer.numberOfChannels;
    const length = audioBuffer.length * numberOfChannels;
    const result = new Float32Array(length);

    for (let channel = 0; channel < numberOfChannels; channel++) {
      const channelData = audioBuffer.getChannelData(channel);
      for (let i = 0; i < audioBuffer.length; i++) {
        result[i * numberOfChannels + channel] = channelData[i];
      }
    }

    return result;
  }

  /**
   * Convert float32 to 16-bit PCM
   */
  private static floatTo16BitPCM(input: Float32Array): ArrayBuffer {
    const buffer = new ArrayBuffer(input.length * 2);
    const view = new DataView(buffer);

    for (let i = 0; i < input.length; i++) {
      const sample = Math.max(-1, Math.min(1, input[i]));
      view.setInt16(i * 2, sample < 0 ? sample * 0x8000 : sample * 0x7fff, true);
    }

    return buffer;
  }

  /**
   * Convert float32 to 24-bit PCM
   */
  private static floatTo24BitPCM(input: Float32Array): ArrayBuffer {
    const buffer = new ArrayBuffer(input.length * 3);
    const view = new DataView(buffer);

    for (let i = 0; i < input.length; i++) {
      const sample = Math.max(-1, Math.min(1, input[i]));
      const int24 = sample < 0 ? sample * 0x800000 : sample * 0x7fffff;

      view.setUint8(i * 3, int24 & 0xff);
      view.setUint8(i * 3 + 1, (int24 >> 8) & 0xff);
      view.setUint8(i * 3 + 2, (int24 >> 16) & 0xff);
    }

    return buffer;
  }

  /**
   * Convert float32 to 32-bit PCM (float)
   */
  private static floatTo32BitPCM(input: Float32Array): ArrayBuffer {
    const buffer = new ArrayBuffer(input.length * 4);
    const view = new DataView(buffer);

    for (let i = 0; i < input.length; i++) {
      view.setFloat32(i * 4, input[i], true);
    }

    return buffer;
  }

  /**
   * Encode WAV file
   */
  private static encodeWAV(
    audioData: ArrayBuffer,
    numberOfChannels: number,
    sampleRate: number,
    bitDepth: number
  ): Blob {
    const bytesPerSample = bitDepth / 8;
    const blockAlign = numberOfChannels * bytesPerSample;

    const buffer = new ArrayBuffer(44 + audioData.byteLength);
    const view = new DataView(buffer);

    // RIFF chunk descriptor
    this.writeString(view, 0, 'RIFF');
    view.setUint32(4, 36 + audioData.byteLength, true);
    this.writeString(view, 8, 'WAVE');

    // fmt sub-chunk
    this.writeString(view, 12, 'fmt ');
    view.setUint32(16, 16, true); // Sub-chunk size
    view.setUint16(20, bitDepth === 32 ? 3 : 1, true); // Audio format (1 = PCM, 3 = IEEE float)
    view.setUint16(22, numberOfChannels, true);
    view.setUint32(24, sampleRate, true);
    view.setUint32(28, sampleRate * blockAlign, true); // Byte rate
    view.setUint16(32, blockAlign, true);
    view.setUint16(34, bitDepth, true);

    // data sub-chunk
    this.writeString(view, 36, 'data');
    view.setUint32(40, audioData.byteLength, true);

    // Copy audio data
    const audioView = new Uint8Array(audioData);
    const resultView = new Uint8Array(buffer);
    resultView.set(audioView, 44);

    return new Blob([buffer], { type: 'audio/wav' });
  }

  /**
   * Write string to DataView
   */
  private static writeString(view: DataView, offset: number, string: string): void {
    for (let i = 0; i < string.length; i++) {
      view.setUint8(offset + i, string.charCodeAt(i));
    }
  }
}

export const exportService = ExportService;
