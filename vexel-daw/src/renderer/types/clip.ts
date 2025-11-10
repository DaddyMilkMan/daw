// Audio Clip types for arrangement view editing

export interface AudioClip {
  // Identity
  id: string;
  trackId: string;
  name: string;
  color: string;

  // Audio source
  audioFile: {
    path: string;
    name: string;
    buffer?: AudioBuffer; // Web Audio buffer
    duration: number; // Total duration in seconds
    sampleRate: number;
    numberOfChannels: number;
    base64Data?: string; // For project serialization
    peaksData?: PeaksData; // Cached waveform peaks
  } | null;

  // Timeline position (in beats)
  startBeat: number; // Position on timeline
  endBeat: number; // End position on timeline

  // Non-destructive editing (in seconds, relative to audio file)
  trimStart: number; // Offset from start of audio file
  trimEnd: number; // Offset from end of audio file

  // Fades (in seconds)
  fadeIn: number;
  fadeOut: number;
  fadeInCurve: FadeCurve;
  fadeOutCurve: FadeCurve;

  // Time-stretch and pitch
  playbackRate: number; // 1.0 = normal speed, affects pitch
  pitchShift: number; // Semitones (-12 to +12)
  preserveFormants: boolean; // For future advanced algorithms
  stretchAlgorithm: 'simple' | 'rubberband' | 'elastique'; // Future-proofing

  // Volume and pan
  gain: number; // 0-2 (1 = unity gain)
  pan: number; // -1 (left) to 1 (right)

  // Crossfade with adjacent clips
  crossfade?: {
    withClipId: string;
    duration: number; // in seconds
    curve: CrossfadeCurve;
  };

  // Metadata
  isSelected: boolean;
  isMuted: boolean;
  isLocked: boolean; // Prevent editing
  createdAt: Date;
  modifiedAt: Date;
}

export type FadeCurve = 'linear' | 'exponential' | 'logarithmic' | 'sCurve';
export type CrossfadeCurve = 'linear' | 'equalPower' | 'logarithmic';

// Waveform peaks data for efficient rendering
export interface PeaksData {
  version: number; // For cache invalidation
  length: number; // Number of peaks
  data: number[][]; // [channel][peak] - min/max pairs
  samplesPerPeak: number;
  generatedAt: Date;
  cacheExpiry: Date; // Auto-cleanup after 1 week
}

// Clipboard data structure
export interface ClipboardData {
  clips: AudioClip[];
  copiedAt: Date;
  sourceTrackId: string;
}

// Clip editing actions for undo/redo
export type ClipEditAction =
  | { type: 'create'; clip: AudioClip }
  | { type: 'delete'; clip: AudioClip }
  | { type: 'move'; clipId: string; oldStartBeat: number; oldEndBeat: number; newStartBeat: number; newEndBeat: number }
  | { type: 'trim'; clipId: string; oldTrimStart: number; oldTrimEnd: number; newTrimStart: number; newTrimEnd: number }
  | { type: 'split'; originalClip: AudioClip; newClips: [AudioClip, AudioClip] }
  | { type: 'fade'; clipId: string; property: 'fadeIn' | 'fadeOut'; oldValue: number; newValue: number }
  | { type: 'stretch'; clipId: string; oldRate: number; newRate: number }
  | { type: 'gain'; clipId: string; oldGain: number; newGain: number };

// Arrangement view state
export interface ArrangementState {
  clips: AudioClip[];
  selectedClipIds: string[];
  clipboard: ClipboardData | null;
  gridSnapEnabled: boolean;
  gridSnapValue: number; // in beats (e.g., 0.25 for 16th notes)
  zoom: number; // Horizontal zoom level
  verticalZoom: number; // Waveform height multiplier
  playheadPosition: number; // in beats
  loopStart: number | null;
  loopEnd: number | null;
}

// Audio settings
export interface AudioSettings {
  autoCrossfadeEnabled: boolean;
  defaultCrossfadeDuration: number; // in seconds
  defaultCrossfadeCurve: CrossfadeCurve;
  defaultFadeCurve: FadeCurve;
  waveformColor: string;
  waveformBackgroundColor: string;
  peaksCacheEnabled: boolean;
  peaksCacheDays: number; // Days before auto-cleanup
}

export const DEFAULT_AUDIO_SETTINGS: AudioSettings = {
  autoCrossfadeEnabled: true,
  defaultCrossfadeDuration: 0.01, // 10ms
  defaultCrossfadeCurve: 'equalPower',
  defaultFadeCurve: 'exponential',
  waveformColor: '#3b82f6',
  waveformBackgroundColor: '#1e293b',
  peaksCacheEnabled: true,
  peaksCacheDays: 7,
};

// Helper function to create a new clip
export function createAudioClip(
  trackId: string,
  startBeat: number,
  audioFile?: AudioClip['audioFile']
): AudioClip {
  const now = new Date();
  return {
    id: `clip-${Date.now()}-${Math.random().toString(36).substr(2, 9)}`,
    trackId,
    name: audioFile?.name || 'New Clip',
    color: getRandomClipColor(),
    audioFile: audioFile || null,
    startBeat,
    endBeat: startBeat + (audioFile ? beatsFromSeconds(audioFile.duration, 128) : 4),
    trimStart: 0,
    trimEnd: 0,
    fadeIn: 0,
    fadeOut: 0,
    fadeInCurve: 'exponential',
    fadeOutCurve: 'exponential',
    playbackRate: 1.0,
    pitchShift: 0,
    preserveFormants: false,
    stretchAlgorithm: 'simple',
    gain: 1.0,
    pan: 0,
    isSelected: false,
    isMuted: false,
    isLocked: false,
    createdAt: now,
    modifiedAt: now,
  };
}

// Helper functions
export function beatsFromSeconds(seconds: number, bpm: number): number {
  return (seconds / 60) * bpm;
}

export function secondsFromBeats(beats: number, bpm: number): number {
  return (beats * 60) / bpm;
}

const CLIP_COLORS = [
  '#ef4444', // red
  '#f97316', // orange
  '#f59e0b', // amber
  '#eab308', // yellow
  '#84cc16', // lime
  '#22c55e', // green
  '#10b981', // emerald
  '#14b8a6', // teal
  '#06b6d4', // cyan
  '#0ea5e9', // sky
  '#3b82f6', // blue
  '#6366f1', // indigo
  '#8b5cf6', // violet
  '#a855f7', // purple
  '#d946ef', // fuchsia
  '#ec4899', // pink
];

export function getRandomClipColor(): string {
  return CLIP_COLORS[Math.floor(Math.random() * CLIP_COLORS.length)];
}

// Snap to grid helper
export function snapToGrid(beat: number, gridSize: number, enabled: boolean): number {
  if (!enabled) return beat;
  return Math.round(beat / gridSize) * gridSize;
}
