// Session View types
export interface Clip {
  id: string;
  name: string;
  trackId: string;
  sceneIndex: number;
  color: string;
  length: number; // in beats
  state: 'empty' | 'stopped' | 'playing' | 'recording' | 'queued';
  type: 'midi' | 'audio';
  loopEnabled: boolean;
  startOffset: number;

  // MIDI-specific
  midiNotes?: import('./recording').MIDINote[];

  // Audio-specific
  audioBuffer?: AudioBuffer;

  // Optional launch behavior
  launchMode?: 'trigger' | 'gate' | 'toggle' | 'repeat';
  followAction?: {
    enabled: boolean;
    action: 'next' | 'previous' | 'first' | 'last' | 'any' | 'other' | 'stop';
    chance: number; // 0-100
    time: number; // in beats
  };
}

export interface Scene {
  id: string;
  name: string;
  index: number;
  tempo?: number;
  timeSignature?: { numerator: number; denominator: number };
}

export const CLIP_COLORS = [
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

export function getRandomColor(): string {
  return CLIP_COLORS[Math.floor(Math.random() * CLIP_COLORS.length)];
}
