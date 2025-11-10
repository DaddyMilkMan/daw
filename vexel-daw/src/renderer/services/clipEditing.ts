/**
 * Clip Editing Service
 * Audio and MIDI clip editing operations (trim, split, fade, etc.)
 */

import { AudioClip, MIDIClip, MIDINote } from '../types/recording';
import { useAudioStore } from '../store/audioStore';

export interface ClipEditOperation {
  type: 'trim' | 'split' | 'fade' | 'move' | 'delete' | 'duplicate' | 'normalize';
  clipId: string;
  params: any;
}

export class ClipEditingService {
  /**
   * Trim audio clip (adjust start/end points)
   */
  static trimAudioClip(
    clip: AudioClip,
    options: {
      trimStart?: number; // beats to trim from start
      trimEnd?: number; // beats to trim from end
    }
  ): AudioClip {
    const store = useAudioStore.getState();
    const tempo = store.tempo;
    const newClip = { ...clip };

    if (options.trimStart !== undefined && options.trimStart > 0) {
      newClip.start += options.trimStart;
      newClip.offset += this.beatsToSeconds(options.trimStart, tempo);
      newClip.length -= options.trimStart;
    }

    if (options.trimEnd !== undefined && options.trimEnd > 0) {
      newClip.length -= options.trimEnd;
    }

    // Ensure positive length
    newClip.length = Math.max(0.1, newClip.length);

    return newClip;
  }

  /**
   * Split audio clip at position
   */
  static splitAudioClip(clip: AudioClip, splitBeat: number): [AudioClip, AudioClip] {
    const store = useAudioStore.getState();
    const tempo = store.tempo;

    if (splitBeat <= clip.start || splitBeat >= clip.start + clip.length) {
      throw new Error('Split position must be within clip bounds');
    }

    const splitOffset = splitBeat - clip.start;

    // First part
    const clip1: AudioClip = {
      ...clip,
      id: `${clip.id}-1`,
      length: splitOffset,
    };

    // Second part
    const clip2: AudioClip = {
      ...clip,
      id: `${clip.id}-2`,
      start: splitBeat,
      length: clip.length - splitOffset,
      offset: clip.offset + this.beatsToSeconds(splitOffset, tempo),
    };

    return [clip1, clip2];
  }

  /**
   * Add fade in to audio clip
   */
  static addFadeIn(clip: AudioClip, fadeLength: number): AudioClip {
    return {
      ...clip,
      fadeIn: Math.min(fadeLength, clip.length / 2),
    };
  }

  /**
   * Add fade out to audio clip
   */
  static addFadeOut(clip: AudioClip, fadeLength: number): AudioClip {
    return {
      ...clip,
      fadeOut: Math.min(fadeLength, clip.length / 2),
    };
  }

  /**
   * Move clip to new position
   */
  static moveClip<T extends AudioClip | MIDIClip>(clip: T, newStart: number): T {
    return {
      ...clip,
      start: newStart,
    };
  }

  /**
   * Duplicate clip
   */
  static duplicateClip<T extends AudioClip | MIDIClip>(clip: T, offset: number = 0): T {
    const newClip = {
      ...clip,
      id: `${clip.id}-copy-${Date.now()}`,
      start: clip.start + offset,
    };

    // Deep copy notes if it's a MIDI clip
    if ('notes' in newClip) {
      newClip.notes = (newClip.notes as MIDINote[]).map((note) => ({
        ...note,
        id: `${note.id}-copy-${Date.now()}`,
      }));
    }

    return newClip as T;
  }

  /**
   * Normalize audio clip (adjust gain to maximize volume without clipping)
   */
  static normalizeAudioClip(clip: AudioClip): AudioClip {
    if (!clip.audioBuffer) {
      return clip;
    }

    // Find peak amplitude
    let peak = 0;
    for (let channel = 0; channel < clip.audioBuffer.numberOfChannels; channel++) {
      const channelData = clip.audioBuffer.getChannelData(channel);
      for (let i = 0; i < channelData.length; i++) {
        peak = Math.max(peak, Math.abs(channelData[i]));
      }
    }

    // Calculate normalization gain (target 0dB, leave 0.5dB headroom)
    const targetPeak = 0.95;
    const normalizationGain = peak > 0 ? targetPeak / peak : 1.0;

    return {
      ...clip,
      gain: clip.gain * normalizationGain,
    };
  }

  /**
   * Reverse audio clip
   */
  static async reverseAudioClip(clip: AudioClip): Promise<AudioClip> {
    if (!clip.audioBuffer) {
      return clip;
    }

    const context = useAudioStore.getState().audioContext.context;
    if (!context) {
      throw new Error('AudioContext not initialized');
    }

    // Create new buffer
    const reversedBuffer = context.createBuffer(
      clip.audioBuffer.numberOfChannels,
      clip.audioBuffer.length,
      clip.audioBuffer.sampleRate
    );

    // Reverse each channel
    for (let channel = 0; channel < clip.audioBuffer.numberOfChannels; channel++) {
      const originalData = clip.audioBuffer.getChannelData(channel);
      const reversedData = reversedBuffer.getChannelData(channel);

      for (let i = 0; i < originalData.length; i++) {
        reversedData[i] = originalData[originalData.length - 1 - i];
      }
    }

    return {
      ...clip,
      audioBuffer: reversedBuffer,
    };
  }

  /**
   * Trim MIDI clip
   */
  static trimMIDIClip(
    clip: MIDIClip,
    options: {
      trimStart?: number;
      trimEnd?: number;
    }
  ): MIDIClip {
    let newClip = { ...clip, notes: [...clip.notes] };

    if (options.trimStart !== undefined && options.trimStart > 0) {
      newClip.start += options.trimStart;
      newClip.length -= options.trimStart;

      // Shift and filter notes
      newClip.notes = newClip.notes
        .map((note) => ({
          ...note,
          start: note.start - options.trimStart!,
        }))
        .filter((note) => note.start >= 0 && note.start < newClip.length);
    }

    if (options.trimEnd !== undefined && options.trimEnd > 0) {
      newClip.length -= options.trimEnd;

      // Filter notes that extend beyond new length
      newClip.notes = newClip.notes.filter((note) => note.start < newClip.length);
    }

    return newClip;
  }

  /**
   * Split MIDI clip at position
   */
  static splitMIDIClip(clip: MIDIClip, splitBeat: number): [MIDIClip, MIDIClip] {
    if (splitBeat <= clip.start || splitBeat >= clip.start + clip.length) {
      throw new Error('Split position must be within clip bounds');
    }

    const splitOffset = splitBeat - clip.start;

    // First part
    const clip1: MIDIClip = {
      ...clip,
      id: `${clip.id}-1`,
      length: splitOffset,
      notes: clip.notes
        .filter((note) => note.start < splitOffset)
        .map((note) => ({ ...note })),
    };

    // Second part
    const clip2: MIDIClip = {
      ...clip,
      id: `${clip.id}-2`,
      start: splitBeat,
      length: clip.length - splitOffset,
      notes: clip.notes
        .filter((note) => note.start >= splitOffset)
        .map((note) => ({
          ...note,
          id: `${note.id}-2`,
          start: note.start - splitOffset,
        })),
    };

    return [clip1, clip2];
  }

  /**
   * Quantize MIDI clip notes
   */
  static quantizeMIDIClip(clip: MIDIClip, gridValue: number, strength: number = 100): MIDIClip {
    const quantizedNotes = clip.notes.map((note) => {
      const gridPosition = Math.round(note.start / gridValue) * gridValue;
      const offset = gridPosition - note.start;
      const quantizedStart = note.start + offset * (strength / 100);

      return {
        ...note,
        start: Math.max(0, quantizedStart),
      };
    });

    return {
      ...clip,
      notes: quantizedNotes,
    };
  }

  /**
   * Transpose MIDI clip
   */
  static transposeMIDIClip(clip: MIDIClip, semitones: number): MIDIClip {
    const transposedNotes = clip.notes.map((note) => ({
      ...note,
      pitch: Math.max(0, Math.min(127, note.pitch + semitones)),
    }));

    return {
      ...clip,
      notes: transposedNotes,
    };
  }

  /**
   * Adjust MIDI note velocities
   */
  static adjustVelocities(clip: MIDIClip, amount: number): MIDIClip {
    const adjustedNotes = clip.notes.map((note) => ({
      ...note,
      velocity: Math.max(1, Math.min(127, note.velocity + amount)),
    }));

    return {
      ...clip,
      notes: adjustedNotes,
    };
  }

  /**
   * Delete notes in MIDI clip
   */
  static deleteNotes(clip: MIDIClip, noteIds: string[]): MIDIClip {
    const noteIdSet = new Set(noteIds);

    return {
      ...clip,
      notes: clip.notes.filter((note) => !noteIdSet.has(note.id)),
    };
  }

  /**
   * Helper: Convert beats to seconds
   */
  private static beatsToSeconds(beats: number, tempo: number): number {
    return (beats * 60.0) / tempo;
  }
}

export const clipEditingService = ClipEditingService;
