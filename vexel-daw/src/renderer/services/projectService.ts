/**
 * Project Service
 * Handles project save/load with Ableton-style project folder structure
 */

import { Project, ProjectMetadata, ExtendedTrack, AudioClip, MIDIClip } from '../types/recording';
import { exportService } from './exportService';

export interface ProjectFile {
  version: string;
  metadata: ProjectMetadata;
  tracks: ExtendedTrack[];
  audioClips: SerializedAudioClip[];
  midiClips: MIDIClip[];
  masterVolume: number;
  masterPan: number;
}

export interface SerializedAudioClip {
  id: string;
  trackId: string;
  name: string;
  start: number;
  length: number;
  fadeIn: number;
  fadeOut: number;
  gain: number;
  offset: number;
  audioFileName: string; // Reference to WAV file in Samples folder
  waveformData: number[]; // Serialized waveform
}

export class ProjectService {
  private static readonly PROJECT_VERSION = '1.0.0';
  private static readonly PROJECT_EXTENSION = '.zenith';

  /**
   * Save project to JSON format
   */
  static async saveProject(project: Project, saveAsFolder: boolean = false): Promise<string> {
    const projectFile: ProjectFile = {
      version: this.PROJECT_VERSION,
      metadata: {
        ...project.metadata,
        modified: new Date(),
      },
      tracks: project.tracks,
      audioClips: await this.serializeAudioClips(project.audioClips),
      midiClips: project.midiClips,
      masterVolume: project.masterVolume,
      masterPan: project.masterPan,
    };

    if (saveAsFolder) {
      return this.saveAsProjectFolder(projectFile, project);
    } else {
      return this.saveAsProjectFile(projectFile);
    }
  }

  /**
   * Save as Ableton-style project folder
   */
  private static async saveAsProjectFolder(
    projectFile: ProjectFile,
    project: Project
  ): Promise<string> {
    // In Electron, we would use the file system API
    // For now, we'll create a structure that can be downloaded

    const folderStructure = {
      projectName: projectFile.metadata.name,
      files: {
        // Main project file
        [`${projectFile.metadata.name}${this.PROJECT_EXTENSION}`]: JSON.stringify(projectFile, null, 2),

        // Samples folder structure
        'Samples/Recorded/': {},
        'Samples/Imported/': {},

        // Project info folder
        'Zenith Project Info/': {
          'metadata.json': JSON.stringify({
            created: projectFile.metadata.created,
            modified: projectFile.metadata.modified,
            version: this.PROJECT_VERSION,
          }, null, 2),
        },
      },
      audioFiles: [] as { path: string; blob: Blob }[],
    };

    // Export audio clips to WAV files
    for (const clip of project.audioClips) {
      if (clip.audioBuffer) {
        const fileName = `${clip.name}.wav`;
        const filePath = `Samples/Recorded/${fileName}`;

        const wavBlob = await exportService.exportToWAV(
          clip.audioBuffer,
          fileName,
          projectFile.metadata.bitDepth
        );

        folderStructure.audioFiles.push({
          path: filePath,
          blob: wavBlob,
        });
      }
    }

    // In a real Electron app, we would write these files to disk
    // For demonstration, we'll return a JSON representation
    return JSON.stringify(folderStructure, null, 2);
  }

  /**
   * Save as single project file
   */
  private static saveAsProjectFile(projectFile: ProjectFile): string {
    return JSON.stringify(projectFile, null, 2);
  }

  /**
   * Load project from JSON
   */
  static async loadProject(jsonData: string): Promise<Project> {
    const projectFile: ProjectFile = JSON.parse(jsonData);

    // Validate version
    if (!projectFile.version) {
      throw new Error('Invalid project file: missing version');
    }

    // Deserialize audio clips (AudioBuffers need to be reconstructed)
    const audioClips = await this.deserializeAudioClips(projectFile.audioClips);

    const project: Project = {
      metadata: {
        ...projectFile.metadata,
        created: new Date(projectFile.metadata.created),
        modified: new Date(projectFile.metadata.modified),
      },
      tracks: projectFile.tracks,
      audioClips,
      midiClips: projectFile.midiClips,
      masterVolume: projectFile.masterVolume,
      masterPan: projectFile.masterPan,
    };

    return project;
  }

  /**
   * Serialize audio clips for JSON export
   */
  private static async serializeAudioClips(
    clips: AudioClip[]
  ): Promise<SerializedAudioClip[]> {
    return clips.map((clip) => ({
      id: clip.id,
      trackId: clip.trackId,
      name: clip.name,
      start: clip.start,
      length: clip.length,
      fadeIn: clip.fadeIn,
      fadeOut: clip.fadeOut,
      gain: clip.gain,
      offset: clip.offset,
      audioFileName: `${clip.name}.wav`,
      waveformData: clip.waveformData ? Array.from(clip.waveformData) : [],
    }));
  }

  /**
   * Deserialize audio clips from JSON
   */
  private static async deserializeAudioClips(
    serializedClips: SerializedAudioClip[]
  ): Promise<AudioClip[]> {
    // Note: AudioBuffers would need to be loaded from the WAV files
    // This is a simplified version that creates empty buffers

    return serializedClips.map((clip) => ({
      id: clip.id,
      trackId: clip.trackId,
      name: clip.name,
      start: clip.start,
      length: clip.length,
      fadeIn: clip.fadeIn,
      fadeOut: clip.fadeOut,
      gain: clip.gain,
      offset: clip.offset,
      audioBuffer: null, // Would be loaded from WAV file
      waveformData: clip.waveformData ? new Float32Array(clip.waveformData) : null,
    }));
  }

  /**
   * Export project as ZIP file (for web-based download)
   */
  static async exportProjectAsZip(project: Project): Promise<Blob> {
    // This would require a ZIP library like JSZip
    // For now, we'll create a placeholder

    const projectJson = await this.saveProject(project, true);

    // In a real implementation, we would:
    // 1. Create ZIP archive
    // 2. Add project file
    // 3. Add all audio files from Samples folder
    // 4. Add metadata files
    // 5. Return ZIP blob

    return new Blob([projectJson], { type: 'application/json' });
  }

  /**
   * Create new empty project
   */
  static createNewProject(name: string = 'Untitled Project'): Project {
    return {
      metadata: {
        name,
        author: '',
        created: new Date(),
        modified: new Date(),
        tempo: 120,
        timeSignature: {
          numerator: 4,
          denominator: 4,
        },
        sampleRate: 48000,
        bitDepth: 32,
        key: 'C',
        genre: '',
        notes: '',
      },
      tracks: [],
      audioClips: [],
      midiClips: [],
      masterVolume: 0.8,
      masterPan: 0,
    };
  }

  /**
   * Download project file
   */
  static downloadProjectFile(jsonData: string, fileName: string): void {
    const blob = new Blob([jsonData], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = fileName.endsWith(this.PROJECT_EXTENSION)
      ? fileName
      : `${fileName}${this.PROJECT_EXTENSION}`;
    a.click();
    URL.revokeObjectURL(url);
  }

  /**
   * Import project file
   */
  static async importProjectFile(file: File): Promise<Project> {
    const text = await file.text();
    return this.loadProject(text);
  }

  /**
   * Auto-save project (for background saving)
   */
  static async autoSaveProject(project: Project): Promise<void> {
    const jsonData = await this.saveProject(project, false);

    // Save to localStorage as backup
    try {
      localStorage.setItem('zenith_autosave', jsonData);
      localStorage.setItem('zenith_autosave_timestamp', new Date().toISOString());
    } catch (error) {
      console.warn('Failed to auto-save project:', error);
    }
  }

  /**
   * Recover auto-saved project
   */
  static async recoverAutoSavedProject(): Promise<Project | null> {
    try {
      const jsonData = localStorage.getItem('zenith_autosave');
      const timestamp = localStorage.getItem('zenith_autosave_timestamp');

      if (!jsonData || !timestamp) {
        return null;
      }

      // Check if auto-save is recent (within last 24 hours)
      const saveTime = new Date(timestamp);
      const now = new Date();
      const hoursDiff = (now.getTime() - saveTime.getTime()) / (1000 * 60 * 60);

      if (hoursDiff > 24) {
        return null; // Too old
      }

      return this.loadProject(jsonData);
    } catch (error) {
      console.error('Failed to recover auto-saved project:', error);
      return null;
    }
  }

  /**
   * Clear auto-save
   */
  static clearAutoSave(): void {
    localStorage.removeItem('zenith_autosave');
    localStorage.removeItem('zenith_autosave_timestamp');
  }

  /**
   * Collect all external files into project folder (like Ableton's "Collect All and Save")
   */
  static async collectAllAndSave(project: Project): Promise<string> {
    // This would:
    // 1. Find all referenced audio files
    // 2. Copy them into the project's Samples folder
    // 3. Update references in the project file
    // 4. Save the project

    return this.saveProject(project, true);
  }

  /**
   * Get project file size (for display)
   */
  static getProjectSize(project: Project): number {
    let size = 0;

    // Estimate JSON size
    const jsonStr = JSON.stringify({
      metadata: project.metadata,
      tracks: project.tracks,
      midiClips: project.midiClips,
    });
    size += jsonStr.length;

    // Add audio buffer sizes
    for (const clip of project.audioClips) {
      if (clip.audioBuffer) {
        const bytesPerSample = 4; // 32-bit float
        const totalSamples = clip.audioBuffer.length * clip.audioBuffer.numberOfChannels;
        size += totalSamples * bytesPerSample;
      }
    }

    return size;
  }

  /**
   * Format file size for display
   */
  static formatFileSize(bytes: number): string {
    if (bytes === 0) return '0 Bytes';

    const k = 1024;
    const sizes = ['Bytes', 'KB', 'MB', 'GB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));

    return Math.round((bytes / Math.pow(k, i)) * 100) / 100 + ' ' + sizes[i];
  }
}

export const projectService = ProjectService;
