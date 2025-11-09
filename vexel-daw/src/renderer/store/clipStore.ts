// Zustand store for managing audio clips in arrangement view

import { create } from 'zustand';
import {
  AudioClip,
  ArrangementState,
  ClipboardData,
  ClipEditAction,
  createAudioClip,
  snapToGrid,
  AudioSettings,
  DEFAULT_AUDIO_SETTINGS,
} from '../types/clip';
import { audioEngine } from '../lib/audioEngine';
import { WaveformGenerator } from '../lib/waveformGenerator';

interface ClipStore extends ArrangementState {
  // Audio settings
  audioSettings: AudioSettings;

  // Actions - Clip management
  addClip: (clip: AudioClip) => void;
  removeClip: (clipId: string) => void;
  updateClip: (clipId: string, updates: Partial<AudioClip>) => void;
  getClip: (clipId: string) => AudioClip | undefined;
  getClipsForTrack: (trackId: string) => AudioClip[];

  // Actions - Selection
  selectClip: (clipId: string, addToSelection?: boolean) => void;
  deselectClip: (clipId: string) => void;
  selectAll: () => void;
  deselectAll: () => void;

  // Actions - Clipboard operations
  copySelectedClips: () => void;
  cutSelectedClips: () => void;
  pasteClips: (targetTrackId: string, targetBeat: number) => void;
  duplicateSelectedClips: () => void;
  deleteSelectedClips: () => void;

  // Actions - Editing
  trimClip: (clipId: string, trimStart: number, trimEnd: number) => void;
  splitClipAtBeat: (clipId: string, beat: number) => void;
  moveClip: (clipId: string, newStartBeat: number, newTrackId?: string) => void;
  setClipFade: (clipId: string, fadeType: 'fadeIn' | 'fadeOut', duration: number) => void;
  setClipGain: (clipId: string, gain: number) => void;
  setClipPlaybackRate: (clipId: string, rate: number) => void;
  setClipPitchShift: (clipId: string, semitones: number) => void;

  // Actions - Grid and snap
  setGridSnap: (enabled: boolean) => void;
  setGridSnapValue: (beats: number) => void;
  snapBeatToGrid: (beat: number) => number;

  // Actions - Zoom and view
  setZoom: (zoom: number) => void;
  setVerticalZoom: (zoom: number) => void;

  // Actions - Playhead
  setPlayheadPosition: (beat: number) => void;

  // Actions - Loop
  setLoop: (start: number | null, end: number | null) => void;

  // Actions - Audio settings
  updateAudioSettings: (settings: Partial<AudioSettings>) => void;

  // Actions - Crossfades
  detectAndCreateCrossfades: (trackId: string) => void;

  // Utility
  reset: () => void;
}

const INITIAL_STATE: ArrangementState = {
  clips: [],
  selectedClipIds: [],
  clipboard: null,
  gridSnapEnabled: true,
  gridSnapValue: 0.25, // 16th notes
  zoom: 1,
  verticalZoom: 1,
  playheadPosition: 0,
  loopStart: null,
  loopEnd: null,
};

export const useClipStore = create<ClipStore>((set, get) => ({
  ...INITIAL_STATE,
  audioSettings: DEFAULT_AUDIO_SETTINGS,

  // Clip management
  addClip: (clip) => {
    set((state) => ({
      clips: [...state.clips, { ...clip, modifiedAt: new Date() }],
    }));

    // Auto-detect crossfades if enabled
    const { audioSettings } = get();
    if (audioSettings.autoCrossfadeEnabled) {
      get().detectAndCreateCrossfades(clip.trackId);
    }
  },

  removeClip: (clipId) => {
    set((state) => ({
      clips: state.clips.filter((c) => c.id !== clipId),
      selectedClipIds: state.selectedClipIds.filter((id) => id !== clipId),
    }));
  },

  updateClip: (clipId, updates) => {
    set((state) => ({
      clips: state.clips.map((clip) =>
        clip.id === clipId ? { ...clip, ...updates, modifiedAt: new Date() } : clip
      ),
    }));
  },

  getClip: (clipId) => {
    return get().clips.find((c) => c.id === clipId);
  },

  getClipsForTrack: (trackId) => {
    return get().clips.filter((c) => c.trackId === trackId);
  },

  // Selection
  selectClip: (clipId, addToSelection = false) => {
    set((state) => {
      const selectedIds = addToSelection
        ? [...state.selectedClipIds, clipId]
        : [clipId];

      return {
        selectedClipIds: selectedIds,
        clips: state.clips.map((clip) => ({
          ...clip,
          isSelected: selectedIds.includes(clip.id),
        })),
      };
    });
  },

  deselectClip: (clipId) => {
    set((state) => ({
      selectedClipIds: state.selectedClipIds.filter((id) => id !== clipId),
      clips: state.clips.map((clip) =>
        clip.id === clipId ? { ...clip, isSelected: false } : clip
      ),
    }));
  },

  selectAll: () => {
    set((state) => ({
      selectedClipIds: state.clips.map((c) => c.id),
      clips: state.clips.map((clip) => ({ ...clip, isSelected: true })),
    }));
  },

  deselectAll: () => {
    set((state) => ({
      selectedClipIds: [],
      clips: state.clips.map((clip) => ({ ...clip, isSelected: false })),
    }));
  },

  // Clipboard operations
  copySelectedClips: () => {
    const { clips, selectedClipIds } = get();
    const selectedClips = clips.filter((c) => selectedClipIds.includes(c.id));

    if (selectedClips.length > 0) {
      set({
        clipboard: {
          clips: selectedClips.map((c) => ({ ...c })),
          copiedAt: new Date(),
          sourceTrackId: selectedClips[0].trackId,
        },
      });
    }
  },

  cutSelectedClips: () => {
    get().copySelectedClips();
    get().deleteSelectedClips();
  },

  pasteClips: (targetTrackId, targetBeat) => {
    const { clipboard, gridSnapEnabled, gridSnapValue } = get();
    if (!clipboard || clipboard.clips.length === 0) return;

    const snappedBeat = snapToGrid(targetBeat, gridSnapValue, gridSnapEnabled);

    // Calculate offset from first clip
    const firstClipStart = Math.min(...clipboard.clips.map((c) => c.startBeat));
    const offset = snappedBeat - firstClipStart;

    // Create new clips
    const newClips = clipboard.clips.map((clip) => {
      const duration = clip.endBeat - clip.startBeat;
      return createAudioClip(targetTrackId, clip.startBeat + offset, clip.audioFile);
    });

    set((state) => ({
      clips: [...state.clips, ...newClips],
    }));
  },

  duplicateSelectedClips: () => {
    const { clips, selectedClipIds } = get();
    const selectedClips = clips.filter((c) => selectedClipIds.includes(c.id));

    const newClips = selectedClips.map((clip) => {
      const duration = clip.endBeat - clip.startBeat;
      const newStartBeat = clip.endBeat; // Place right after original

      return {
        ...clip,
        id: `clip-${Date.now()}-${Math.random().toString(36).substr(2, 9)}`,
        startBeat: newStartBeat,
        endBeat: newStartBeat + duration,
        isSelected: true,
        createdAt: new Date(),
        modifiedAt: new Date(),
      };
    });

    set((state) => ({
      clips: [...state.clips, ...newClips],
      selectedClipIds: newClips.map((c) => c.id),
    }));
  },

  deleteSelectedClips: () => {
    const { selectedClipIds } = get();
    set((state) => ({
      clips: state.clips.filter((c) => !selectedClipIds.includes(c.id)),
      selectedClipIds: [],
    }));
  },

  // Editing operations
  trimClip: (clipId, trimStart, trimEnd) => {
    const clip = get().getClip(clipId);
    if (!clip || !clip.audioFile) return;

    const maxTrim = clip.audioFile.duration - 0.01; // Keep at least 10ms
    const validTrimStart = Math.max(0, Math.min(trimStart, maxTrim));
    const validTrimEnd = Math.max(0, Math.min(trimEnd, maxTrim - validTrimStart));

    get().updateClip(clipId, {
      trimStart: validTrimStart,
      trimEnd: validTrimEnd,
    });
  },

  splitClipAtBeat: (clipId, beat) => {
    const clip = get().getClip(clipId);
    if (!clip || beat <= clip.startBeat || beat >= clip.endBeat) return;

    // Calculate split point in audio time
    const clipDuration = clip.endBeat - clip.startBeat;
    const splitRatio = (beat - clip.startBeat) / clipDuration;

    if (!clip.audioFile) return;

    const audioFileDuration = clip.audioFile.duration - clip.trimStart - clip.trimEnd;
    const splitPointInAudio = clip.trimStart + audioFileDuration * splitRatio;

    // Create first half
    const firstClip: AudioClip = {
      ...clip,
      id: `clip-${Date.now()}-${Math.random().toString(36).substr(2, 9)}-1`,
      endBeat: beat,
      trimEnd: clip.audioFile.duration - splitPointInAudio,
      fadeOut: 0, // Remove fade out from first clip
      createdAt: new Date(),
      modifiedAt: new Date(),
    };

    // Create second half
    const secondClip: AudioClip = {
      ...clip,
      id: `clip-${Date.now()}-${Math.random().toString(36).substr(2, 9)}-2`,
      startBeat: beat,
      trimStart: splitPointInAudio,
      fadeIn: 0, // Remove fade in from second clip
      createdAt: new Date(),
      modifiedAt: new Date(),
    };

    // Remove original and add new clips
    set((state) => ({
      clips: [
        ...state.clips.filter((c) => c.id !== clipId),
        firstClip,
        secondClip,
      ],
    }));
  },

  moveClip: (clipId, newStartBeat, newTrackId) => {
    const clip = get().getClip(clipId);
    if (!clip) return;

    const { gridSnapEnabled, gridSnapValue } = get();
    const snappedBeat = snapToGrid(newStartBeat, gridSnapValue, gridSnapEnabled);

    const duration = clip.endBeat - clip.startBeat;
    const updates: Partial<AudioClip> = {
      startBeat: snappedBeat,
      endBeat: snappedBeat + duration,
    };

    if (newTrackId && newTrackId !== clip.trackId) {
      updates.trackId = newTrackId;
    }

    get().updateClip(clipId, updates);
  },

  setClipFade: (clipId, fadeType, duration) => {
    const clip = get().getClip(clipId);
    if (!clip) return;

    const maxFade = audioEngine.getClipDuration(clip);
    const validDuration = Math.max(0, Math.min(duration, maxFade));

    get().updateClip(clipId, {
      [fadeType]: validDuration,
    });
  },

  setClipGain: (clipId, gain) => {
    get().updateClip(clipId, { gain: Math.max(0, Math.min(2, gain)) });
  },

  setClipPlaybackRate: (clipId, rate) => {
    get().updateClip(clipId, { playbackRate: Math.max(0.25, Math.min(4, rate)) });
  },

  setClipPitchShift: (clipId, semitones) => {
    get().updateClip(clipId, { pitchShift: Math.max(-12, Math.min(12, semitones)) });
  },

  // Grid and snap
  setGridSnap: (enabled) => set({ gridSnapEnabled: enabled }),

  setGridSnapValue: (beats) => set({ gridSnapValue: beats }),

  snapBeatToGrid: (beat) => {
    const { gridSnapEnabled, gridSnapValue } = get();
    return snapToGrid(beat, gridSnapValue, gridSnapEnabled);
  },

  // Zoom and view
  setZoom: (zoom) => set({ zoom: Math.max(0.1, Math.min(10, zoom)) }),

  setVerticalZoom: (zoom) => set({ verticalZoom: Math.max(0.5, Math.min(3, zoom)) }),

  // Playhead
  setPlayheadPosition: (beat) => set({ playheadPosition: Math.max(0, beat) }),

  // Loop
  setLoop: (start, end) => set({ loopStart: start, loopEnd: end }),

  // Audio settings
  updateAudioSettings: (settings) =>
    set((state) => ({
      audioSettings: { ...state.audioSettings, ...settings },
    })),

  // Crossfades
  detectAndCreateCrossfades: (trackId) => {
    const clips = get().getClipsForTrack(trackId).sort((a, b) => a.startBeat - b.startBeat);
    const { audioSettings } = get();

    if (!audioSettings.autoCrossfadeEnabled) return;

    // Detect overlapping clips
    for (let i = 0; i < clips.length - 1; i++) {
      const currentClip = clips[i];
      const nextClip = clips[i + 1];

      // Check for overlap
      if (currentClip.endBeat > nextClip.startBeat) {
        const overlapDuration = currentClip.endBeat - nextClip.startBeat;
        const crossfadeDuration = Math.min(
          overlapDuration,
          audioSettings.defaultCrossfadeDuration
        );

        // Add crossfade to current clip
        get().updateClip(currentClip.id, {
          crossfade: {
            withClipId: nextClip.id,
            duration: crossfadeDuration,
            curve: audioSettings.defaultCrossfadeCurve,
          },
        });
      }
    }
  },

  // Utility
  reset: () => set(INITIAL_STATE),
}));
