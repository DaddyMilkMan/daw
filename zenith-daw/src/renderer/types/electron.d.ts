export interface ElectronAPI {
  transportPlay: () => void;
  transportPause: () => void;
  transportStop: () => void;
  setTempo: (tempo: number) => void;
  createTrack: (name: string, type: 'midi' | 'audio' | 'instrument') => void;
  getAudioState: () => Promise<AudioState>;
  windowMinimize: () => void;
  windowMaximize: () => void;
  windowClose: () => void;
  onAudioStateUpdate: (callback: (state: AudioState) => void) => () => void;
}

declare global {
  interface Window {
    electron: ElectronAPI;
  }
}
