/**
 * Engine Client Adapter
 *
 * Single interface for all UI → Engine communication.
 * Allows swapping between mock (Electron IPC) and native engine
 * without touching UI code.
 *
 * Feature flag: ENGINE_MODE=mock|ipc (default: mock)
 */

import type { AudioState } from '../types/audio';

// Event types from engine
export interface EngineEvent {
  type: string;
  data?: any;
  timestamp?: number;
}

// Command types (unified interface)
export type EngineCommand =
  // Transport commands
  | { type: 'transport:play' }
  | { type: 'transport:pause' }
  | { type: 'transport:stop' }
  | { type: 'transport:setTempo'; data: { tempo: number } }
  | { type: 'transport:setTimeSignature'; data: { numerator: number; denominator: number } }
  | { type: 'transport:setLoop'; data: { enabled: boolean; start: number; end: number } }
  | { type: 'transport:setMetronome'; data: { enabled: boolean } }

  // Track commands
  | { type: 'track:create'; data: { name: string; type: 'midi' | 'audio' | 'instrument' } }
  | { type: 'track:delete'; data: { trackId: string } }
  | { type: 'track:rename'; data: { trackId: string; name: string } }
  | { type: 'track:setVolume'; data: { trackId: string; volume: number } }
  | { type: 'track:setPan'; data: { trackId: string; pan: number } }
  | { type: 'track:setMute'; data: { trackId: string; muted: boolean } }
  | { type: 'track:setSolo'; data: { trackId: string; solo: boolean } }
  | { type: 'track:setRecordArm'; data: { trackId: string; armed: boolean } }
  | { type: 'track:setColor'; data: { trackId: string; color: string } }

  // Automation commands
  | { type: 'automation:add'; data: { trackId: string; parameter: string; points: Array<{ time: number; value: number }> } }
  | { type: 'automation:update'; data: { trackId: string; parameter: string; points: Array<{ time: number; value: number }> } }
  | { type: 'automation:delete'; data: { trackId: string; parameter: string } }

  // Clip commands (Session View)
  | { type: 'clip:create'; data: { trackId: string; sceneIndex: number; length: number } }
  | { type: 'clip:delete'; data: { clipId: string } }
  | { type: 'clip:launch'; data: { clipId: string } }
  | { type: 'clip:stop'; data: { trackId: string } }

  // MIDI commands
  | { type: 'midi:addNote'; data: { trackId: string; pitch: number; time: number; duration: number; velocity: number } }
  | { type: 'midi:removeNote'; data: { trackId: string; noteId: string } }
  | { type: 'midi:updateNote'; data: { trackId: string; noteId: string; pitch?: number; time?: number; duration?: number; velocity?: number } }

  // Batch commands
  | { type: 'batch:start' }
  | { type: 'batch:commit' }
  | { type: 'batch:rollback' }

  // Project commands
  | { type: 'project:save'; data: { path?: string } }
  | { type: 'project:load'; data: { path: string } }
  | { type: 'project:new' };

// Project state (for Wingman queries)
export interface ProjectState {
  audio: AudioState;
  mixer: {
    tracks: Record<string, {
      volume: number;
      pan: number;
      muted: boolean;
      solo: boolean;
    }>;
  };
  automation: {
    lanes: Record<string, Array<{
      parameter: string;
      points: Array<{ time: number; value: number }>;
    }>>;
  };
  clips?: any[]; // Session view clips
  notes?: any[]; // MIDI notes
}

type EventCallback = (event: EngineEvent) => void;

class EngineClient {
  private connected = false;
  private eventCallbacks: Set<EventCallback> = new Set();
  private mode: 'mock' | 'ipc' = 'mock';
  private unsubscribeElectron?: () => void;

  constructor() {
    // Read ENGINE_MODE from environment
    const envMode = import.meta.env.VITE_ENGINE_MODE || 'mock';
    this.mode = envMode as 'mock' | 'ipc';

    console.log(`[EngineClient] Initialized in ${this.mode} mode`);
  }

  /**
   * Connect to the engine
   * In mock mode: Sets up Electron IPC listeners
   * In ipc mode: Would connect to native engine
   */
  async connect(): Promise<void> {
    if (this.connected) {
      console.warn('[EngineClient] Already connected');
      return;
    }

    if (this.mode === 'mock') {
      // Use existing Electron IPC bridge
      this.unsubscribeElectron = window.electron.onAudioStateUpdate((state) => {
        this.emitEvent({
          type: 'state:updated',
          data: state,
          timestamp: Date.now(),
        });
      });

      // Fetch initial state
      const initialState = await window.electron.getAudioState();
      this.emitEvent({
        type: 'state:initial',
        data: initialState,
        timestamp: Date.now(),
      });
    } else {
      // Future: Connect to native engine via IPC/WebSocket/etc
      throw new Error('Native IPC mode not yet implemented');
    }

    this.connected = true;
    console.log('[EngineClient] Connected successfully');
  }

  /**
   * Send a command to the engine
   * In mock mode: Translates to Electron IPC calls
   * In ipc mode: Would send to native engine
   */
  async sendCommand(type: string, data?: any): Promise<void> {
    if (!this.connected) {
      console.warn('[EngineClient] Not connected, attempting to connect...');
      await this.connect();
    }

    if (this.mode === 'mock') {
      // Translate unified commands to Electron IPC
      await this.handleMockCommand(type, data);
    } else {
      // Future: Send to native engine
      throw new Error('Native IPC mode not yet implemented');
    }
  }

  /**
   * Subscribe to engine events
   * Returns unsubscribe function
   */
  onEvent(callback: EventCallback): () => void {
    this.eventCallbacks.add(callback);

    return () => {
      this.eventCallbacks.delete(callback);
    };
  }

  /**
   * Request full project state
   * Used by Wingman to query current state before making decisions
   */
  async requestProjectState(): Promise<ProjectState> {
    if (!this.connected) {
      await this.connect();
    }

    if (this.mode === 'mock') {
      // In mock mode, only audio state is tracked by engine
      const audioState = await window.electron.getAudioState();

      // Build mixer state from tracks (if they have volume/pan/etc)
      const mixer: ProjectState['mixer'] = { tracks: {} };
      audioState.tracks.forEach((track) => {
        mixer.tracks[track.id] = {
          volume: track.volume ?? 75,
          pan: track.pan ?? 0,
          muted: track.muted ?? false,
          solo: track.solo ?? false,
        };
      });

      return {
        audio: audioState,
        mixer,
        automation: { lanes: {} }, // Not yet synced to engine
        clips: [], // Not yet synced to engine
        notes: [], // Not yet synced to engine
      };
    } else {
      // Future: Query native engine
      throw new Error('Native IPC mode not yet implemented');
    }
  }

  /**
   * Disconnect from engine
   */
  disconnect(): void {
    if (this.unsubscribeElectron) {
      this.unsubscribeElectron();
      this.unsubscribeElectron = undefined;
    }

    this.eventCallbacks.clear();
    this.connected = false;
    console.log('[EngineClient] Disconnected');
  }

  /**
   * Get connection status
   */
  isConnected(): boolean {
    return this.connected;
  }

  /**
   * Get current mode
   */
  getMode(): 'mock' | 'ipc' {
    return this.mode;
  }

  // Private: Emit event to all subscribers
  private emitEvent(event: EngineEvent): void {
    this.eventCallbacks.forEach((callback) => {
      try {
        callback(event);
      } catch (error) {
        console.error('[EngineClient] Error in event callback:', error);
      }
    });
  }

  // Private: Handle mock mode commands (translate to Electron IPC)
  private async handleMockCommand(type: string, data?: any): Promise<void> {
    try {
      switch (type) {
        // Transport
        case 'transport:play':
          window.electron.transportPlay();
          break;

        case 'transport:pause':
          window.electron.transportPause();
          break;

        case 'transport:stop':
          window.electron.transportStop();
          break;

        case 'transport:setTempo':
          window.electron.setTempo(data.tempo);
          break;

        // Track commands
        case 'track:create':
          window.electron.createTrack(data.name, data.type);
          break;

        // Commands not yet supported by mock engine
        case 'transport:setTimeSignature':
        case 'transport:setLoop':
        case 'transport:setMetronome':
        case 'track:delete':
        case 'track:rename':
        case 'track:setVolume':
        case 'track:setPan':
        case 'track:setMute':
        case 'track:setSolo':
        case 'track:setRecordArm':
        case 'track:setColor':
        case 'automation:add':
        case 'automation:update':
        case 'automation:delete':
        case 'clip:create':
        case 'clip:delete':
        case 'clip:launch':
        case 'clip:stop':
        case 'midi:addNote':
        case 'midi:removeNote':
        case 'midi:updateNote':
        case 'batch:start':
        case 'batch:commit':
        case 'batch:rollback':
        case 'project:save':
        case 'project:load':
        case 'project:new':
          console.warn(`[EngineClient] Command "${type}" not yet implemented in mock engine`);
          // Emit a mock event so UI can still update locally if needed
          this.emitEvent({
            type: 'command:notImplemented',
            data: { commandType: type, commandData: data },
            timestamp: Date.now(),
          });
          break;

        default:
          console.error(`[EngineClient] Unknown command type: ${type}`);
      }
    } catch (error) {
      console.error(`[EngineClient] Error executing command ${type}:`, error);
      throw error;
    }
  }
}

// Singleton instance
export const engineClient = new EngineClient();

// Export types
export type { EngineCommand, EventCallback };
