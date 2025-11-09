/**
 * Project Save/Load System for Zenith DAW
 * Handles serialization and deserialization of DAW projects
 */

import { AudioState } from '@/types/audio';
import { AudioClip } from './audio-engine';

export interface ProjectData {
  version: string;
  name: string;
  tempo: number;
  timeSignature: {
    numerator: number;
    denominator: number;
  };
  tracks: ProjectTrack[];
  masterVolume: number;
  createdAt: string;
  modifiedAt: string;
}

export interface ProjectTrack {
  id: string;
  name: string;
  type: 'midi' | 'audio' | 'instrument';
  volume: number;
  pan: number;
  muted: boolean;
  solo: boolean;
  color?: string;
  clips: ProjectClip[];
  effects?: ProjectEffect[];
}

export interface ProjectClip {
  id: string;
  name: string;
  startTime: number;
  duration: number;
  offset: number;
  gain: number;
  muted: boolean;
  color: string;
  // Audio data stored as base64 for portability
  audioData?: string;
  // MIDI data
  midiNotes?: MIDINote[];
}

export interface MIDINote {
  pitch: number;
  start: number;
  length: number;
  velocity: number;
}

export interface ProjectEffect {
  id: string;
  type: string;
  enabled: boolean;
  parameters: Record<string, number>;
}

export class ProjectManager {
  private currentProject: ProjectData | null = null;
  private projectName: string = 'Untitled Project';

  /**
   * Create a new empty project
   */
  createNew(name: string = 'Untitled Project'): ProjectData {
    this.projectName = name;
    this.currentProject = {
      version: '1.0.0',
      name,
      tempo: 120,
      timeSignature: { numerator: 4, denominator: 4 },
      tracks: [],
      masterVolume: 0.8,
      createdAt: new Date().toISOString(),
      modifiedAt: new Date().toISOString(),
    };

    return this.currentProject;
  }

  /**
   * Save current project to JSON
   */
  async saveProject(
    audioState: AudioState,
    clips: Map<string, AudioClip[]>,
    audioBuffers: Map<string, AudioBuffer>
  ): Promise<ProjectData> {
    const projectData: ProjectData = {
      version: '1.0.0',
      name: this.projectName,
      tempo: audioState.tempo,
      timeSignature: audioState.timeSignature,
      masterVolume: 0.8,
      tracks: [],
      createdAt: this.currentProject?.createdAt || new Date().toISOString(),
      modifiedAt: new Date().toISOString(),
    };

    // Serialize tracks
    for (const track of audioState.tracks) {
      const trackClips = clips.get(track.id) || [];

      const projectTrack: ProjectTrack = {
        id: track.id,
        name: track.name,
        type: track.type,
        volume: track.volume,
        pan: track.pan,
        muted: track.muted,
        solo: track.solo,
        clips: [],
      };

      // Serialize clips
      for (const clip of trackClips) {
        const projectClip: ProjectClip = {
          id: clip.id,
          name: clip.name,
          startTime: clip.startTime,
          duration: clip.duration,
          offset: clip.offset,
          gain: clip.gain,
          muted: clip.muted,
          color: clip.color,
        };

        // Convert audio buffer to base64 (if not too large)
        if (clip.buffer) {
          try {
            const audioData = await this.audioBufferToBase64(clip.buffer);
            // Only store if reasonable size (< 10MB base64)
            if (audioData.length < 10 * 1024 * 1024) {
              projectClip.audioData = audioData;
            }
          } catch (error) {
            console.warn(`Failed to serialize audio for clip ${clip.id}:`, error);
          }
        }

        projectTrack.clips.push(projectClip);
      }

      projectData.tracks.push(projectTrack);
    }

    this.currentProject = projectData;
    return projectData;
  }

  /**
   * Load project from JSON
   */
  async loadProject(projectData: ProjectData): Promise<{
    audioState: Partial<AudioState>;
    clips: Map<string, AudioClip[]>;
    audioBuffers: Map<string, AudioBuffer>;
  }> {
    this.currentProject = projectData;
    this.projectName = projectData.name;

    const clips = new Map<string, AudioClip[]>();
    const audioBuffers = new Map<string, AudioBuffer>();

    // Reconstruct audio state
    const audioState: Partial<AudioState> = {
      tempo: projectData.tempo,
      timeSignature: projectData.timeSignature,
      tracks: projectData.tracks.map(track => ({
        id: track.id,
        name: track.name,
        type: track.type,
        volume: track.volume,
        pan: track.pan,
        muted: track.muted,
        solo: track.solo,
      })),
    };

    // Reconstruct clips and audio buffers
    for (const track of projectData.tracks) {
      const trackClips: AudioClip[] = [];

      for (const projectClip of track.clips) {
        let buffer: AudioBuffer | null = null;

        // Decode audio data if present
        if (projectClip.audioData) {
          try {
            buffer = await this.base64ToAudioBuffer(projectClip.audioData);
            audioBuffers.set(projectClip.id, buffer);
          } catch (error) {
            console.warn(`Failed to load audio for clip ${projectClip.id}:`, error);
          }
        }

        const clip: AudioClip = {
          id: projectClip.id,
          trackId: track.id,
          buffer,
          startTime: projectClip.startTime,
          duration: projectClip.duration,
          offset: projectClip.offset,
          gain: projectClip.gain,
          muted: projectClip.muted,
          name: projectClip.name,
          color: projectClip.color,
        };

        trackClips.push(clip);
      }

      clips.set(track.id, trackClips);
    }

    return { audioState, clips, audioBuffers };
  }

  /**
   * Export project to JSON string
   */
  exportToJSON(projectData: ProjectData): string {
    return JSON.stringify(projectData, null, 2);
  }

  /**
   * Import project from JSON string
   */
  importFromJSON(json: string): ProjectData {
    return JSON.parse(json) as ProjectData;
  }

  /**
   * Save project to file (download)
   */
  async saveToFile(projectData: ProjectData): Promise<void> {
    const json = this.exportToJSON(projectData);
    const blob = new Blob([json], { type: 'application/json' });
    const url = URL.createObjectURL(blob);

    const a = document.createElement('a');
    a.href = url;
    a.download = `${projectData.name.replace(/[^a-z0-9]/gi, '_').toLowerCase()}.zndaw`;
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);
  }

  /**
   * Load project from file
   */
  async loadFromFile(file: File): Promise<ProjectData> {
    const text = await file.text();
    return this.importFromJSON(text);
  }

  /**
   * Convert AudioBuffer to base64 string
   */
  private async audioBufferToBase64(buffer: AudioBuffer): Promise<string> {
    // Serialize audio buffer to WAV format
    const wavBlob = await this.audioBufferToWav(buffer);
    const arrayBuffer = await wavBlob.arrayBuffer();
    const bytes = new Uint8Array(arrayBuffer);

    // Convert to base64
    let binary = '';
    for (let i = 0; i < bytes.length; i++) {
      binary += String.fromCharCode(bytes[i]);
    }
    return btoa(binary);
  }

  /**
   * Convert base64 string to AudioBuffer
   */
  private async base64ToAudioBuffer(base64: string): Promise<AudioBuffer> {
    // Decode base64
    const binary = atob(base64);
    const bytes = new Uint8Array(binary.length);
    for (let i = 0; i < binary.length; i++) {
      bytes[i] = binary.charCodeAt(i);
    }

    // Decode WAV to AudioBuffer
    const audioContext = new AudioContext();
    const audioBuffer = await audioContext.decodeAudioData(bytes.buffer);
    await audioContext.close();

    return audioBuffer;
  }

  /**
   * Convert AudioBuffer to WAV Blob
   */
  private async audioBufferToWav(audioBuffer: AudioBuffer): Promise<Blob> {
    const numberOfChannels = audioBuffer.numberOfChannels;
    const sampleRate = audioBuffer.sampleRate;
    const length = audioBuffer.length * numberOfChannels * 2;

    const buffer = new ArrayBuffer(44 + length);
    const view = new DataView(buffer);

    // WAV header
    const writeString = (offset: number, string: string) => {
      for (let i = 0; i < string.length; i++) {
        view.setUint8(offset + i, string.charCodeAt(i));
      }
    };

    writeString(0, 'RIFF');
    view.setUint32(4, 36 + length, true);
    writeString(8, 'WAVE');
    writeString(12, 'fmt ');
    view.setUint32(16, 16, true);
    view.setUint16(20, 1, true);
    view.setUint16(22, numberOfChannels, true);
    view.setUint32(24, sampleRate, true);
    view.setUint32(28, sampleRate * numberOfChannels * 2, true);
    view.setUint16(32, numberOfChannels * 2, true);
    view.setUint16(34, 16, true);
    writeString(36, 'data');
    view.setUint32(40, length, true);

    // Write audio data
    const channels = [];
    for (let i = 0; i < numberOfChannels; i++) {
      channels.push(audioBuffer.getChannelData(i));
    }

    let offset = 44;
    for (let i = 0; i < audioBuffer.length; i++) {
      for (let channel = 0; channel < numberOfChannels; channel++) {
        const sample = Math.max(-1, Math.min(1, channels[channel][i]));
        view.setInt16(offset, sample < 0 ? sample * 0x8000 : sample * 0x7fff, true);
        offset += 2;
      }
    }

    return new Blob([buffer], { type: 'audio/wav' });
  }

  /**
   * Get current project
   */
  getCurrentProject(): ProjectData | null {
    return this.currentProject;
  }

  /**
   * Save to localStorage
   */
  saveToLocalStorage(key: string = 'zenith-daw-project'): void {
    if (this.currentProject) {
      localStorage.setItem(key, this.exportToJSON(this.currentProject));
      localStorage.setItem(`${key}-timestamp`, new Date().toISOString());
    }
  }

  /**
   * Load from localStorage
   */
  loadFromLocalStorage(key: string = 'zenith-daw-project'): ProjectData | null {
    const json = localStorage.getItem(key);
    if (json) {
      try {
        return this.importFromJSON(json);
      } catch (error) {
        console.error('Failed to load project from localStorage:', error);
        return null;
      }
    }
    return null;
  }

  /**
   * Auto-save to localStorage
   */
  enableAutoSave(intervalMs: number = 30000): () => void {
    const interval = setInterval(() => {
      this.saveToLocalStorage();
    }, intervalMs);

    return () => clearInterval(interval);
  }
}

// Create singleton instance
export const projectManager = new ProjectManager();
