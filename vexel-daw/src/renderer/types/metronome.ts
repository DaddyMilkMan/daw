/**
 * Metronome Types
 * Professional DAW metronome with multiple sounds, pre-count, and MIDI clock
 */

export type MetronomeSoundType = 'wood' | 'beep' | 'click' | 'custom';

export interface MetronomeSettings {
  // Basic Settings
  enabled: boolean;
  volume: number; // 0-1
  soundType: MetronomeSoundType;
  customSoundUrl?: string; // For user-uploaded sounds

  // Pre-count (Count-in) Settings - Logic Pro style
  preCount: {
    enabled: boolean;
    bars: number; // Number of bars to count (1-8)
    onlyWhenRecording: boolean; // Only use pre-count when recording
    showVisualCountdown: boolean; // Show big numbers "1 2 3 GO"
  };

  // Accent Patterns
  accentDownbeat: boolean; // Accent beat 1 of each bar
  accentGrouping: number[]; // Custom grouping for odd meters e.g., [2, 2, 3] for 7/8
  useTimeSignatureAccents: boolean; // Automatically accent based on time signature

  // Click Behavior
  clickDuringPlayback: boolean;
  clickDuringRecording: boolean;
  clickDuringCountIn: boolean;

  // Sound Parameters (for synthesis)
  sounds: {
    wood: {
      highFreq: number; // Frequency for downbeat/accent
      lowFreq: number;  // Frequency for regular beats
    };
    beep: {
      highFreq: number;
      lowFreq: number;
    };
    click: {
      highFreq: number;
      lowFreq: number;
    };
  };
}

export interface MIDIClockSettings {
  enabled: boolean; // On by default
  sendStart: boolean;
  sendStop: boolean;
  sendContinue: boolean;
  sendSongPosition: boolean;
  timeSignatureAware: boolean; // Change accent pattern with time signature
}

export interface MetronomeState {
  isPlaying: boolean;
  isInPreCount: boolean;
  preCountBeat: number; // Current beat in pre-count (0-based)
  currentBeat: number; // Current beat in bar (0-based)
  nextClickTime: number; // Next scheduled click time (AudioContext time)
}

// Default settings matching professional DAWs
export const DEFAULT_METRONOME_SETTINGS: MetronomeSettings = {
  enabled: false,
  volume: 0.7,
  soundType: 'wood',

  preCount: {
    enabled: false,
    bars: 1,
    onlyWhenRecording: false,
    showVisualCountdown: true,
  },

  accentDownbeat: true,
  accentGrouping: [],
  useTimeSignatureAccents: false,

  clickDuringPlayback: true,
  clickDuringRecording: true,
  clickDuringCountIn: true,

  sounds: {
    wood: {
      highFreq: 1200,
      lowFreq: 800,
    },
    beep: {
      highFreq: 1000,
      lowFreq: 800,
    },
    click: {
      highFreq: 2000,
      lowFreq: 1500,
    },
  },
};

export const DEFAULT_MIDI_CLOCK_SETTINGS: MIDIClockSettings = {
  enabled: true, // On by default as requested
  sendStart: true,
  sendStop: true,
  sendContinue: true,
  sendSongPosition: true,
  timeSignatureAware: false,
};
