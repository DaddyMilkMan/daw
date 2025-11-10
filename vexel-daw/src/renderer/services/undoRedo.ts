/**
 * Undo/Redo System
 * Command pattern implementation for history management
 *
 * Based on best practices:
 * - Command pattern for each operation
 * - Past/Present/Future stack structure
 * - Immutable state snapshots
 * - TypeScript type safety
 */

import { useAudioStore } from '../store/audioStore';
import { AudioClip, MIDIClip, ExtendedTrack } from '../types/recording';

// Command interface
export interface Command {
  execute(): void;
  undo(): void;
  description: string;
}

// Command types

/**
 * Add Clip Command
 */
export class AddClipCommand implements Command {
  description: string;
  private clipType: 'audio' | 'midi';
  private clip: AudioClip | MIDIClip;

  constructor(clipType: 'audio' | 'midi', clip: AudioClip | MIDIClip) {
    this.clipType = clipType;
    this.clip = clip;
    this.description = `Add ${clipType} clip: ${clip.name}`;
  }

  execute(): void {
    if (this.clipType === 'audio') {
      useAudioStore.getState().addAudioClip(this.clip as AudioClip);
    } else {
      useAudioStore.getState().addMIDIClip(this.clip as MIDIClip);
    }
  }

  undo(): void {
    if (this.clipType === 'audio') {
      useAudioStore.getState().removeAudioClip(this.clip.id);
    } else {
      useAudioStore.getState().removeMIDIClip(this.clip.id);
    }
  }
}

/**
 * Delete Clip Command
 */
export class DeleteClipCommand implements Command {
  description: string;
  private clipType: 'audio' | 'midi';
  private clip: AudioClip | MIDIClip;

  constructor(clipType: 'audio' | 'midi', clip: AudioClip | MIDIClip) {
    this.clipType = clipType;
    this.clip = clip;
    this.description = `Delete ${clipType} clip: ${clip.name}`;
  }

  execute(): void {
    if (this.clipType === 'audio') {
      useAudioStore.getState().removeAudioClip(this.clip.id);
    } else {
      useAudioStore.getState().removeMIDIClip(this.clip.id);
    }
  }

  undo(): void {
    if (this.clipType === 'audio') {
      useAudioStore.getState().addAudioClip(this.clip as AudioClip);
    } else {
      useAudioStore.getState().addMIDIClip(this.clip as MIDIClip);
    }
  }
}

/**
 * Move Clip Command
 */
export class MoveClipCommand implements Command {
  description: string;
  private clipType: 'audio' | 'midi';
  private clipId: string;
  private oldStart: number;
  private newStart: number;

  constructor(
    clipType: 'audio' | 'midi',
    clipId: string,
    oldStart: number,
    newStart: number
  ) {
    this.clipType = clipType;
    this.clipId = clipId;
    this.oldStart = oldStart;
    this.newStart = newStart;
    this.description = `Move ${clipType} clip`;
  }

  execute(): void {
    this.moveClip(this.newStart);
  }

  undo(): void {
    this.moveClip(this.oldStart);
  }

  private moveClip(start: number): void {
    const store = useAudioStore.getState();

    if (this.clipType === 'audio') {
      const clips = store.audioClips.map((clip) =>
        clip.id === this.clipId ? { ...clip, start } : clip
      );
      useAudioStore.setState({ audioClips: clips });
    } else {
      const clips = store.midiClips.map((clip) =>
        clip.id === this.clipId ? { ...clip, start } : clip
      );
      useAudioStore.setState({ midiClips: clips });
    }
  }
}

/**
 * Add Track Command
 */
export class AddTrackCommand implements Command {
  description: string;
  private track: ExtendedTrack;

  constructor(track: ExtendedTrack) {
    this.track = track;
    this.description = `Add track: ${track.name}`;
  }

  execute(): void {
    const store = useAudioStore.getState();
    useAudioStore.setState({
      tracks: [...store.tracks, this.track],
    });
  }

  undo(): void {
    useAudioStore.getState().removeTrack(this.track.id);
  }
}

/**
 * Delete Track Command
 */
export class DeleteTrackCommand implements Command {
  description: string;
  private track: ExtendedTrack;
  private audioClips: AudioClip[];
  private midiClips: MIDIClip[];

  constructor(track: ExtendedTrack) {
    this.track = track;

    const store = useAudioStore.getState();
    // Store associated clips for undo
    this.audioClips = store.audioClips.filter((c) => c.trackId === track.id);
    this.midiClips = store.midiClips.filter((c) => c.trackId === track.id);

    this.description = `Delete track: ${track.name}`;
  }

  execute(): void {
    useAudioStore.getState().removeTrack(this.track.id);
  }

  undo(): void {
    const store = useAudioStore.getState();

    // Restore track
    useAudioStore.setState({
      tracks: [...store.tracks, this.track],
    });

    // Restore clips
    useAudioStore.setState({
      audioClips: [...store.audioClips, ...this.audioClips],
      midiClips: [...store.midiClips, ...this.midiClips],
    });
  }
}

/**
 * Update Track Property Command
 */
export class UpdateTrackCommand implements Command {
  description: string;
  private trackId: string;
  private property: keyof ExtendedTrack;
  private oldValue: any;
  private newValue: any;

  constructor(
    trackId: string,
    property: keyof ExtendedTrack,
    oldValue: any,
    newValue: any
  ) {
    this.trackId = trackId;
    this.property = property;
    this.oldValue = oldValue;
    this.newValue = newValue;
    this.description = `Update track ${property}`;
  }

  execute(): void {
    this.updateProperty(this.newValue);
  }

  undo(): void {
    this.updateProperty(this.oldValue);
  }

  private updateProperty(value: any): void {
    const store = useAudioStore.getState();
    const tracks = store.tracks.map((track) =>
      track.id === this.trackId
        ? { ...track, [this.property]: value }
        : track
    );
    useAudioStore.setState({ tracks });
  }
}

/**
 * Batch Command (for multiple operations)
 */
export class BatchCommand implements Command {
  description: string;
  private commands: Command[];

  constructor(commands: Command[], description: string = 'Batch operation') {
    this.commands = commands;
    this.description = description;
  }

  execute(): void {
    this.commands.forEach((cmd) => cmd.execute());
  }

  undo(): void {
    // Undo in reverse order
    for (let i = this.commands.length - 1; i >= 0; i--) {
      this.commands[i].undo();
    }
  }
}

/**
 * History Manager
 */
export class HistoryManager {
  private static instance: HistoryManager | null = null;

  private history: Command[] = [];
  private currentIndex: number = -1;
  private maxHistorySize: number = 100;

  private constructor() {}

  static getInstance(): HistoryManager {
    if (!HistoryManager.instance) {
      HistoryManager.instance = new HistoryManager();
    }
    return HistoryManager.instance;
  }

  /**
   * Execute command and add to history
   */
  execute(command: Command): void {
    // Execute the command
    command.execute();

    // Remove any commands after current index (clearing redo stack)
    this.history = this.history.slice(0, this.currentIndex + 1);

    // Add command to history
    this.history.push(command);
    this.currentIndex++;

    // Limit history size
    if (this.history.length > this.maxHistorySize) {
      this.history.shift();
      this.currentIndex--;
    }

    console.log(`Executed: ${command.description}`);
  }

  /**
   * Undo last command
   */
  undo(): boolean {
    if (!this.canUndo()) {
      return false;
    }

    const command = this.history[this.currentIndex];
    command.undo();
    this.currentIndex--;

    console.log(`Undid: ${command.description}`);
    return true;
  }

  /**
   * Redo last undone command
   */
  redo(): boolean {
    if (!this.canRedo()) {
      return false;
    }

    this.currentIndex++;
    const command = this.history[this.currentIndex];
    command.execute();

    console.log(`Redid: ${command.description}`);
    return true;
  }

  /**
   * Check if can undo
   */
  canUndo(): boolean {
    return this.currentIndex >= 0;
  }

  /**
   * Check if can redo
   */
  canRedo(): boolean {
    return this.currentIndex < this.history.length - 1;
  }

  /**
   * Get history status
   */
  getStatus(): {
    canUndo: boolean;
    canRedo: boolean;
    historySize: number;
    currentIndex: number;
  } {
    return {
      canUndo: this.canUndo(),
      canRedo: this.canRedo(),
      historySize: this.history.length,
      currentIndex: this.currentIndex,
    };
  }

  /**
   * Clear history
   */
  clear(): void {
    this.history = [];
    this.currentIndex = -1;
  }

  /**
   * Get history list (for debugging/display)
   */
  getHistory(): string[] {
    return this.history.map((cmd, i) => {
      const marker = i === this.currentIndex ? '→ ' : '  ';
      return `${marker}${cmd.description}`;
    });
  }
}

// Export singleton instance
export const historyManager = HistoryManager.getInstance();

// Convenience functions
export const undoRedo = {
  execute: (command: Command) => historyManager.execute(command),
  undo: () => historyManager.undo(),
  redo: () => historyManager.redo(),
  canUndo: () => historyManager.canUndo(),
  canRedo: () => historyManager.canRedo(),
  clear: () => historyManager.clear(),
  getStatus: () => historyManager.getStatus(),
  getHistory: () => historyManager.getHistory(),
};
