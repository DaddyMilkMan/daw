// Recording and Audio Engine Types

export interface AudioClip {
  id: string;
  trackId: string;
  name: string;
  start: number; // in beats
  length: number; // in beats
  audioBuffer: AudioBuffer | null;
  waveformData: Float32Array | null;
  fadeIn: number; // in beats
  fadeOut: number; // in beats
  gain: number; // 0-2
  offset: number; // trim start in seconds
}

export interface MIDIClip {
  id: string;
  trackId: string;
  name: string;
  start: number; // in beats
  length: number; // in beats
  notes: MIDINote[];
}

export interface MIDINote {
  id: string;
  pitch: number; // 0-127
  start: number; // in beats (relative to clip)
  length: number; // in beats
  velocity: number; // 0-127
  selected: boolean;
  muted: boolean;
}

export interface RecordingState {
  isRecording: boolean;
  isArmed: boolean; // At least one track is armed
  armedTracks: Set<string>; // Track IDs
  preCountBars: number;
  preCountRemaining: number; // Bars remaining before recording starts
  recordingTracks: Map<string, RecordingTrackState>; // Active recording sessions
  inputMonitoring: boolean; // Global input monitoring toggle
  latencyCompensation: number; // in samples
}

export interface RecordingTrackState {
  trackId: string;
  startTime: number; // AudioContext time
  startBeat: number; // Position in timeline
  recordedData: Float32Array[]; // Chunks of audio data
  sampleRate: number;
  channelCount: number;
  midiEvents: MIDIMIDIEvent[];
}

export interface MIDIEvent {
  type: 'noteOn' | 'noteOff' | 'cc' | 'pitchBend' | 'aftertouch';
  timestamp: number; // in beats
  data1: number; // note number or CC number
  data2?: number; // velocity or CC value
  channel: number;
}

export interface AudioSettings {
  sampleRate: number; // 44100, 48000, 96000, 192000
  bitDepth: 16 | 24 | 32; // For export (internal is always 32-bit float)
  bufferSize: number; // 64, 128, 256, 512, 1024
  inputDevice: string | null;
  outputDevice: string | null;
  inputLatency: number; // in ms
  outputLatency: number; // in ms
}

export interface MIDISettings {
  inputDevices: string[]; // Selected MIDI input device IDs
  outputDevices: string[]; // Selected MIDI output device IDs
  midiThru: boolean; // Pass MIDI input to output
}

export interface QuantizationSettings {
  enabled: boolean;
  gridValue: number; // 1/4, 1/8, 1/16, 1/32, 1/64 (stored as decimal)
  strength: number; // 0-100 (percentage)
  swing: number; // 0-99 (50 = no swing)
  quantizeNoteStart: boolean;
  quantizeNoteEnd: boolean;
  scaleQuantization: {
    enabled: boolean;
    root: number; // 0-11 (C=0)
    scale: 'major' | 'minor' | 'dorian' | 'phrygian' | 'lydian' | 'mixolydian' | 'locrian' | 'harmonic-minor' | 'melodic-minor';
  };
  triplets: boolean;
}

export interface InputLevel {
  trackId: string;
  peak: number; // 0-1
  rms: number; // 0-1
  clipping: boolean;
}

export interface ProjectMetadata {
  name: string;
  author: string;
  created: Date;
  modified: Date;
  tempo: number;
  timeSignature: {
    numerator: number;
    denominator: number;
  };
  sampleRate: number;
  bitDepth: number;
  key: string; // Musical key
  genre: string;
  notes: string;
}

export interface Project {
  metadata: ProjectMetadata;
  tracks: ExtendedTrack[];
  audioClips: AudioClip[];
  midiClips: MIDIClip[];
  masterVolume: number;
  masterPan: number;
}

export interface ExtendedTrack {
  id: string;
  name: string;
  type: 'midi' | 'audio' | 'instrument';
  volume: number; // 0-1
  pan: number; // -1 to 1
  muted: boolean;
  solo: boolean;
  armed: boolean; // Record arm
  color: string; // Hex color
  height: number; // Track height in pixels
  inputMonitoring: boolean; // Per-track monitoring
  inputGain: number; // Input gain 0-2
  sends: {
    id: string;
    amount: number; // 0-1
  }[];
  inserts: {
    id: string;
    enabled: boolean;
    bypassed: boolean;
  }[];
}

// Web Audio API Types
export interface AudioContextState {
  context: AudioContext | null;
  masterGain: GainNode | null;
  analyser: AnalyserNode | null;
  compressor: DynamicsCompressorNode | null;
  initialized: boolean;
}

export interface AudioInputState {
  stream: MediaStream | null;
  source: MediaStreamAudioSourceNode | null;
  processor: AudioWorkletNode | null;
  analyser: AnalyserNode | null;
}

export interface MIDIInputState {
  access: MIDIAccess | null;
  inputs: Map<string, MIDIInput>;
  outputs: Map<string, MIDIOutput>;
  selectedInput: MIDIInput | null;
}
