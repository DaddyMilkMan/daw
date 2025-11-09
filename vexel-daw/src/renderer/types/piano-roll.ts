// Piano Roll types
export interface Note {
  id: string;
  pitch: number; // MIDI note number (0-127)
  start: number; // Position in beats
  length: number; // Duration in beats
  velocity: number; // 0-127
  selected: boolean;
  muted: boolean;
}

export interface PianoRollState {
  notes: Note[];
  selectedNotes: string[];
  scale: {
    root: number; // 0-11 (C=0, C#=1, etc)
    type: 'major' | 'minor' | 'dorian' | 'phrygian' | 'lydian' | 'mixolydian' | 'locrian' | 'harmonic-minor' | 'melodic-minor';
  };
  snapEnabled: boolean;
  snapValue: number; // 1/4, 1/8, 1/16, 1/32, 1/64
  ghostNotesEnabled: boolean;
  scaleHighlightEnabled: boolean;
  tool: 'select' | 'draw' | 'erase' | 'slice' | 'mute';
  zoom: {
    horizontal: number; // pixels per beat
    vertical: number; // pixels per key
  };
  viewRange: {
    startBeat: number;
    endBeat: number;
    lowestNote: number;
    highestNote: number;
  };
}

export const SCALE_PATTERNS: Record<PianoRollState['scale']['type'], number[]> = {
  'major': [0, 2, 4, 5, 7, 9, 11],
  'minor': [0, 2, 3, 5, 7, 8, 10],
  'dorian': [0, 2, 3, 5, 7, 9, 10],
  'phrygian': [0, 1, 3, 5, 7, 8, 10],
  'lydian': [0, 2, 4, 6, 7, 9, 11],
  'mixolydian': [0, 2, 4, 5, 7, 9, 10],
  'locrian': [0, 1, 3, 5, 6, 8, 10],
  'harmonic-minor': [0, 2, 3, 5, 7, 8, 11],
  'melodic-minor': [0, 2, 3, 5, 7, 9, 11],
};

export const NOTE_NAMES = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];

export function isNoteInScale(pitch: number, scale: PianoRollState['scale']): boolean {
  const pitchClass = pitch % 12;
  const relativeToRoot = (pitchClass - scale.root + 12) % 12;
  return SCALE_PATTERNS[scale.type].includes(relativeToRoot);
}

export function getNoteName(pitch: number): string {
  const octave = Math.floor(pitch / 12) - 1;
  const noteName = NOTE_NAMES[pitch % 12];
  return `${noteName}${octave}`;
}

export function snapToGrid(value: number, snapValue: number): number {
  return Math.round(value / snapValue) * snapValue;
}

export function humanizeNotes(notes: Note[], amount: number): Note[] {
  return notes.map(note => ({
    ...note,
    velocity: Math.max(1, Math.min(127, note.velocity + (Math.random() - 0.5) * amount * 40)),
    start: note.start + (Math.random() - 0.5) * amount * 0.05,
  }));
}

export function strumNotes(notes: Note[], direction: 'up' | 'down', amount: number): Note[] {
  const sorted = [...notes].sort((a, b) => direction === 'up' ? a.pitch - b.pitch : b.pitch - a.pitch);
  return sorted.map((note, index) => ({
    ...note,
    start: note.start + (index * amount * 0.02),
  }));
}
