export interface Track {
  id: string;
  name: string;
  type: 'midi' | 'audio' | 'instrument';
  volume: number;
  pan: number;
  muted: boolean;
  solo: boolean;
}

export interface AudioState {
  tempo: number;
  timeSignature: {
    numerator: number;
    denominator: number;
  };
  isPlaying: boolean;
  currentBar: number;
  tracks: Track[];
}

export interface TransportState {
  isPlaying: boolean;
  tempo: number;
  currentBar: number;
  timeSignature: {
    numerator: number;
    denominator: number;
  };
}
