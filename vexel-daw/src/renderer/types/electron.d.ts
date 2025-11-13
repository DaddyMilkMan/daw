export interface ProjectData {
  clips: any[];
  scenes: any[];
  automation: any[];
  markers: any[];
  metadata: {
    name: string;
    createdAt: string;
    modifiedAt: string;
    author?: string;
    description?: string;
  };
}

export interface SaveProjectResult {
  success: boolean;
  canceled?: boolean;
  filePath?: string;
  fileName?: string;
  error?: string;
}

export interface LoadProjectResult {
  success: boolean;
  canceled?: boolean;
  filePath?: string;
  fileName?: string;
  data?: any;
  error?: string;
}

export interface ProjectInfo {
  currentPath: string | null;
  metadata: ProjectData['metadata'];
}

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

  // Wingman AI Bridge
  wingman: {
    connect: (config?: any) => Promise<{ success: boolean; state?: any; error?: string }>;
    disconnect: () => void;
    sendCommand: (command: string, payload?: any) => Promise<{ success: boolean; data?: any; error?: string }>;
    sendEvent: (event: string, data?: any) => void;
    getConnectionState: () => Promise<any>;
    onConnectionStateChanged: (callback: (state: any) => void) => () => void;
    onEvent: (callback: (event: any) => void) => () => void;
    onError: (callback: (error: any) => void) => () => void;
    onGenerationCompleted: (callback: (data: any) => void) => () => void;
    onInsertClip: (callback: (data: any) => void) => () => void;
    onLaunchClip: (callback: (data: any) => void) => () => void;
  };

  // Project operations
  onProjectDataUpdate: (callback: (data: ProjectData) => void) => () => void;
  saveProject: (filePath?: string, data?: any) => Promise<SaveProjectResult>;
  loadProject: (filePath?: string) => Promise<LoadProjectResult>;
  newProject: () => Promise<{ success: boolean; data?: any; error?: string }>;
  getProjectInfo: () => Promise<ProjectInfo>;
}

declare global {
  interface Window {
    electron: ElectronAPI;
  }
}
