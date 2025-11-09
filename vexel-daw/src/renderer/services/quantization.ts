/**
 * Advanced Quantization Service
 * Combines Ableton Live and Logic Pro quantization features
 */

import { MIDINote, QuantizationSettings } from '../types/recording';
import { SCALE_PATTERNS } from '../types/piano-roll';

export class QuantizationService {
  /**
   * Quantize MIDI notes with advanced options
   * Combines Ableton-style separate start/end quantization with Logic Pro-style strength and swing
   */
  static quantizeNotes(
    notes: MIDINote[],
    settings: QuantizationSettings
  ): MIDINote[] {
    if (!settings.enabled || notes.length === 0) {
      return notes;
    }

    let quantizedNotes = [...notes];

    // Apply rhythmic quantization (start and/or end)
    if (settings.quantizeNoteStart || settings.quantizeNoteEnd) {
      quantizedNotes = this.applyRhythmicQuantization(quantizedNotes, settings);
    }

    // Apply scale quantization if enabled
    if (settings.scaleQuantization.enabled) {
      quantizedNotes = this.applyScaleQuantization(quantizedNotes, settings);
    }

    return quantizedNotes;
  }

  /**
   * Apply rhythmic quantization (timing grid)
   * Supports separate start/end quantization, swing, and strength
   */
  private static applyRhythmicQuantization(
    notes: MIDINote[],
    settings: QuantizationSettings
  ): MIDINote[] {
    const { gridValue, strength, swing, quantizeNoteStart, quantizeNoteEnd, triplets } = settings;

    // Calculate effective grid value
    const effectiveGridValue = triplets ? (gridValue * 2) / 3 : gridValue;

    return notes.map((note) => {
      let newStart = note.start;
      let newLength = note.length;

      // Quantize note start
      if (quantizeNoteStart) {
        const gridPosition = Math.floor(note.start / effectiveGridValue);
        const nextGridPosition = gridPosition + 1;
        const currentGrid = gridPosition * effectiveGridValue;
        const nextGrid = nextGridPosition * effectiveGridValue;

        // Apply swing
        const swingAmount = this.calculateSwingOffset(gridPosition, swing, effectiveGridValue);
        const swungCurrentGrid = currentGrid + swingAmount;
        const swungNextGrid = nextGrid + this.calculateSwingOffset(nextGridPosition, swing, effectiveGridValue);

        // Find closest grid point
        const distToCurrent = Math.abs(note.start - swungCurrentGrid);
        const distToNext = Math.abs(note.start - swungNextGrid);
        const targetGrid = distToCurrent < distToNext ? swungCurrentGrid : swungNextGrid;

        // Apply strength (0% = no quantization, 100% = full quantization)
        const strengthFactor = strength / 100;
        newStart = note.start + (targetGrid - note.start) * strengthFactor;
      }

      // Quantize note end (Ableton feature)
      if (quantizeNoteEnd) {
        const noteEnd = note.start + note.length;
        const gridPosition = Math.floor(noteEnd / effectiveGridValue);
        const nextGridPosition = gridPosition + 1;
        const currentGrid = gridPosition * effectiveGridValue;
        const nextGrid = nextGridPosition * effectiveGridValue;

        // Apply swing to end position
        const swingAmount = this.calculateSwingOffset(gridPosition, swing, effectiveGridValue);
        const swungCurrentGrid = currentGrid + swingAmount;
        const swungNextGrid = nextGrid + this.calculateSwingOffset(nextGridPosition, swing, effectiveGridValue);

        // Find closest grid point for end
        const distToCurrent = Math.abs(noteEnd - swungCurrentGrid);
        const distToNext = Math.abs(noteEnd - swungNextGrid);
        const targetGrid = distToCurrent < distToNext ? swungCurrentGrid : swungNextGrid;

        // Apply strength
        const strengthFactor = strength / 100;
        const quantizedEnd = noteEnd + (targetGrid - noteEnd) * strengthFactor;

        // Calculate new length
        newLength = quantizedEnd - newStart;

        // Ensure minimum note length
        if (newLength < effectiveGridValue / 4) {
          newLength = effectiveGridValue / 4;
        }
      }

      return {
        ...note,
        start: newStart,
        length: newLength,
      };
    });
  }

  /**
   * Calculate swing offset for a grid position
   * Logic Pro-style swing implementation
   */
  private static calculateSwingOffset(
    gridPosition: number,
    swing: number,
    gridValue: number
  ): number {
    // Swing affects every second grid position
    if (gridPosition % 2 === 0) {
      return 0; // No swing on even positions
    }

    // Convert swing percentage to offset
    // 50% = no swing, 0% = maximum early, 99% = maximum late
    const swingNormalized = (swing - 50) / 50; // -1 to 0.98
    const maxSwingOffset = gridValue * 0.33; // Maximum swing is 1/3 of grid value

    return swingNormalized * maxSwingOffset;
  }

  /**
   * Apply scale quantization (pitch correction)
   * Maps notes to nearest note in selected scale
   */
  private static applyScaleQuantization(
    notes: MIDINote[],
    settings: QuantizationSettings
  ): MIDINote[] {
    const { scaleQuantization } = settings;
    const { root, scale } = scaleQuantization;

    const scalePattern = SCALE_PATTERNS[scale];

    if (!scalePattern) {
      console.warn('Unknown scale type:', scale);
      return notes;
    }

    return notes.map((note) => {
      // Get pitch class (0-11) relative to root
      const pitchClass = note.pitch % 12;
      const relativeToRoot = (pitchClass - root + 12) % 12;

      // Check if note is already in scale
      if (scalePattern.includes(relativeToRoot)) {
        return note; // Already in scale
      }

      // Find nearest note in scale
      let nearestNote = scalePattern[0];
      let minDistance = Math.abs(relativeToRoot - nearestNote);

      for (const scaleNote of scalePattern) {
        // Calculate distance considering octave wrapping
        const dist = Math.abs(relativeToRoot - scaleNote);
        const distWrap = 12 - dist;
        const minDist = Math.min(dist, distWrap);

        if (minDist < minDistance) {
          minDistance = minDist;
          nearestNote = scaleNote;
        }
      }

      // Calculate direction (up or down)
      const direction = nearestNote > relativeToRoot ? 1 : -1;

      // Adjust for wrapping
      let offset = nearestNote - relativeToRoot;
      if (Math.abs(offset) > 6) {
        offset = direction * (12 - Math.abs(offset)) * -1;
      }

      // Calculate new pitch
      const newPitch = note.pitch + offset;

      // Ensure pitch stays in valid MIDI range (0-127)
      const clampedPitch = Math.max(0, Math.min(127, newPitch));

      return {
        ...note,
        pitch: clampedPitch,
      };
    });
  }

  /**
   * Quantize to a specific grid value (for manual quantization)
   */
  static quantizeToGrid(
    position: number,
    gridValue: number,
    swing: number = 50
  ): number {
    const gridPosition = Math.round(position / gridValue);
    const quantized = gridPosition * gridValue;

    // Apply swing
    const swingOffset = this.calculateSwingOffset(gridPosition, swing, gridValue);

    return quantized + swingOffset;
  }

  /**
   * Get snap value for grid (for UI display)
   */
  static getSnapValueDisplay(gridValue: number, triplets: boolean): string {
    const baseValue = 1 / gridValue;

    if (triplets) {
      return `1/${Math.round(baseValue * (2 / 3))}T`;
    }

    return `1/${baseValue}`;
  }

  /**
   * Calculate grid lines for visual display
   */
  static getGridLines(
    startBeat: number,
    endBeat: number,
    gridValue: number,
    swing: number = 50
  ): number[] {
    const lines: number[] = [];
    const firstGrid = Math.floor(startBeat / gridValue) * gridValue;

    for (let beat = firstGrid; beat <= endBeat + gridValue; beat += gridValue) {
      const gridPosition = Math.round(beat / gridValue);
      const swingOffset = this.calculateSwingOffset(gridPosition, swing, gridValue);
      lines.push(beat + swingOffset);
    }

    return lines;
  }

  /**
   * Humanize notes (add subtle timing and velocity variations)
   */
  static humanizeNotes(notes: MIDINote[], amount: number): MIDINote[] {
    if (amount === 0) {
      return notes;
    }

    return notes.map((note) => ({
      ...note,
      start: note.start + (Math.random() - 0.5) * amount * 0.05,
      velocity: Math.max(
        1,
        Math.min(127, note.velocity + (Math.random() - 0.5) * amount * 40)
      ),
    }));
  }

  /**
   * Apply groove template (extract timing from existing notes and apply to others)
   */
  static applyGroove(
    notes: MIDINote[],
    grooveTemplate: MIDINote[],
    strength: number
  ): MIDINote[] {
    if (grooveTemplate.length === 0 || strength === 0) {
      return notes;
    }

    const strengthFactor = strength / 100;

    return notes.map((note) => {
      // Find nearest note in groove template
      let nearestGrooveNote = grooveTemplate[0];
      let minDistance = Math.abs(note.start - grooveTemplate[0].start);

      for (const grooveNote of grooveTemplate) {
        const dist = Math.abs(note.start - grooveNote.start);
        if (dist < minDistance) {
          minDistance = dist;
          nearestGrooveNote = grooveNote;
        }
      }

      // Calculate offset from grid
      const gridValue = 1 / 16; // Use 1/16 as default
      const noteGrid = Math.round(note.start / gridValue) * gridValue;
      const grooveOffset = nearestGrooveNote.start - noteGrid;

      // Apply groove offset with strength
      const newStart = note.start + grooveOffset * strengthFactor;

      // Optionally apply velocity from groove
      const velocityDiff = nearestGrooveNote.velocity - 100; // Assume 100 is "neutral"
      const newVelocity = Math.max(
        1,
        Math.min(127, note.velocity + velocityDiff * strengthFactor)
      );

      return {
        ...note,
        start: newStart,
        velocity: newVelocity,
      };
    });
  }
}

export const quantizationService = QuantizationService;
