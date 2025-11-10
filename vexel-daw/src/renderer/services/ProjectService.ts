import { AudioState } from '../types/audio';
import { Clip, Scene } from '../types/session';
import { Note, PianoRollState } from '../types/piano-roll';

/**
 * Automation lane for track parameters
 */
export interface AutomationLane {
  id: string;
  trackId: string;
  parameter: 'volume' | 'pan' | 'send1' | 'send2' | 'send3' | 'send4';
  points: AutomationPoint[];
  enabled: boolean;
}

export interface AutomationPoint {
  time: number; // in beats
  value: number; // normalized 0-1
  curve: 'linear' | 'exponential' | 'logarithmic' | 'stepped';
}

/**
 * Marker for timeline navigation
 */
export interface Marker {
  id: string;
  name: string;
  time: number; // in beats
  color: string;
}

/**
 * Plugin/Effect in a track's chain
 */
export interface PluginInstance {
  id: string;
  pluginId: string; // identifier for the plugin type
  name: string;
  enabled: boolean;
  parameters: Record<string, number>; // parameter name -> value
  presetName?: string;
}

/**
 * Extended track data with full DAW features
 */
export interface ExtendedTrack {
  id: string;
  name: string;
  type: 'midi' | 'audio' | 'instrument';
  volume: number;
  pan: number;
  muted: boolean;
  solo: boolean;
  recordEnabled: boolean;
  monitorMode: 'off' | 'in' | 'auto';
  color: string;
  height: number; // UI height in pixels
  plugins: PluginInstance[];
  sends: number[]; // send levels to aux tracks
  notes?: Note[]; // MIDI notes (for MIDI tracks)
  audioClipPath?: string; // file path for audio clips
}

/**
 * Complete project state
 */
export interface ProjectData {
  version: string; // project format version
  metadata: {
    name: string;
    createdAt: string;
    modifiedAt: string;
    author?: string;
    description?: string;
  };
  transport: {
    tempo: number;
    timeSignature: {
      numerator: number;
      denominator: number;
    };
  };
  tracks: ExtendedTrack[];
  clips: Clip[];
  scenes: Scene[];
  automation: AutomationLane[];
  markers: Marker[];
  pianoRollSettings?: Partial<PianoRollState>; // global piano roll preferences
}

/**
 * ProjectService handles serialization and deserialization of DAW projects
 * Operates off the main audio thread
 */
export class ProjectService {
  private static readonly CURRENT_VERSION = '1.0.0';
  private static readonly FILE_EXTENSION = '.vxl';

  /**
   * Serialize the current project state to a JSON-compatible object
   */
  static serializeProject(
    audioState: AudioState,
    clips: Clip[] = [],
    scenes: Scene[] = [],
    automation: AutomationLane[] = [],
    markers: Marker[] = [],
    metadata: Partial<ProjectData['metadata']> = {}
  ): ProjectData {
    const now = new Date().toISOString();

    // Convert basic tracks to extended tracks
    const extendedTracks: ExtendedTrack[] = audioState.tracks.map((track) => ({
      ...track,
      recordEnabled: false,
      monitorMode: 'auto' as const,
      color: this.getDefaultTrackColor(track.type),
      height: 120,
      plugins: [],
      sends: [0, 0, 0, 0],
      notes: [], // Will be populated if track has MIDI data
    }));

    const projectData: ProjectData = {
      version: this.CURRENT_VERSION,
      metadata: {
        name: metadata.name || 'Untitled Project',
        createdAt: metadata.createdAt || now,
        modifiedAt: now,
        author: metadata.author,
        description: metadata.description,
      },
      transport: {
        tempo: audioState.tempo,
        timeSignature: audioState.timeSignature,
      },
      tracks: extendedTracks,
      clips,
      scenes,
      automation,
      markers,
    };

    return projectData;
  }

  /**
   * Deserialize a project from JSON and return the state objects
   */
  static deserializeProject(data: ProjectData): {
    audioState: Partial<AudioState>;
    clips: Clip[];
    scenes: Scene[];
    automation: AutomationLane[];
    markers: Marker[];
    metadata: ProjectData['metadata'];
  } {
    // Validate version compatibility
    if (!this.isVersionCompatible(data.version)) {
      throw new Error(
        `Project version ${data.version} is not compatible with current version ${this.CURRENT_VERSION}`
      );
    }

    // Convert extended tracks back to basic tracks
    const tracks = data.tracks.map((track) => ({
      id: track.id,
      name: track.name,
      type: track.type,
      volume: track.volume,
      pan: track.pan,
      muted: track.muted,
      solo: track.solo,
    }));

    const audioState: Partial<AudioState> = {
      tempo: data.transport.tempo,
      timeSignature: data.transport.timeSignature,
      tracks,
    };

    return {
      audioState,
      clips: data.clips || [],
      scenes: data.scenes || [],
      automation: data.automation || [],
      markers: data.markers || [],
      metadata: data.metadata,
    };
  }

  /**
   * Validate project data structure
   */
  static validateProjectData(data: any): data is ProjectData {
    if (!data || typeof data !== 'object') {
      return false;
    }

    // Check required fields
    const hasVersion = typeof data.version === 'string';
    const hasMetadata = data.metadata && typeof data.metadata === 'object';
    const hasTransport = data.transport && typeof data.transport === 'object';
    const hasTracks = Array.isArray(data.tracks);

    if (!hasVersion || !hasMetadata || !hasTransport || !hasTracks) {
      return false;
    }

    // Validate transport
    const validTransport =
      typeof data.transport.tempo === 'number' &&
      data.transport.timeSignature &&
      typeof data.transport.timeSignature.numerator === 'number' &&
      typeof data.transport.timeSignature.denominator === 'number';

    return validTransport;
  }

  /**
   * Check if a project version is compatible with the current version
   */
  private static isVersionCompatible(version: string): boolean {
    const [major] = version.split('.').map(Number);
    const [currentMajor] = this.CURRENT_VERSION.split('.').map(Number);

    // Compatible if major versions match
    return major === currentMajor;
  }

  /**
   * Get default color for track type
   */
  private static getDefaultTrackColor(type: string): string {
    const colors: Record<string, string> = {
      midi: '#3b82f6', // blue
      audio: '#10b981', // green
      instrument: '#8b5cf6', // purple
    };
    return colors[type] || '#6b7280'; // gray fallback
  }

  /**
   * Create a new empty project
   */
  static createEmptyProject(): ProjectData {
    return this.serializeProject(
      {
        tempo: 120,
        timeSignature: { numerator: 4, denominator: 4 },
        isPlaying: false,
        currentBar: 0,
        tracks: [],
      },
      [],
      [],
      [],
      [],
      {
        name: 'New Project',
      }
    );
  }

  /**
   * Get the recommended file extension
   */
  static getFileExtension(): string {
    return this.FILE_EXTENSION;
  }

  /**
   * Convert project data to JSON string
   */
  static toJSON(data: ProjectData, prettify: boolean = true): string {
    return JSON.stringify(data, null, prettify ? 2 : 0);
  }

  /**
   * Parse JSON string to project data
   */
  static fromJSON(json: string): ProjectData {
    try {
      const data = JSON.parse(json);

      if (!this.validateProjectData(data)) {
        throw new Error('Invalid project data structure');
      }

      return data;
    } catch (error) {
      if (error instanceof SyntaxError) {
        throw new Error('Invalid JSON format');
      }
      throw error;
    }
  }
}
