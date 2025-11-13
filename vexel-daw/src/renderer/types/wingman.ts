/**
 * Wingman AI Bridge - Type Definitions
 * Protocol for communication between DAW and external AI core
 */

// ============================================================================
// Message Protocol
// ============================================================================

/**
 * Base message structure for all Wingman communications
 */
export interface WingmanMessage {
  id: string;                          // Unique message ID for request/response matching
  timestamp: number;                   // Unix timestamp
  type: 'command' | 'response' | 'event' | 'error';
}

/**
 * Command message sent from DAW to AI
 */
export interface WingmanCommand extends WingmanMessage {
  type: 'command';
  command: CommandType;
  payload: any;
}

/**
 * Response message from AI to DAW
 */
export interface WingmanResponse extends WingmanMessage {
  type: 'response';
  requestId: string;                   // ID of the command this responds to
  success: boolean;
  data?: any;
  error?: string;
}

/**
 * Event message (can be initiated by either side)
 */
export interface WingmanEvent extends WingmanMessage {
  type: 'event';
  event: EventType;
  data: any;
}

/**
 * Error message
 */
export interface WingmanError extends WingmanMessage {
  type: 'error';
  code: string;
  message: string;
  details?: any;
}

// ============================================================================
// Command Types
// ============================================================================

export type CommandType =
  // Transport Commands
  | 'TRANSPORT_PLAY'
  | 'TRANSPORT_PAUSE'
  | 'TRANSPORT_STOP'
  | 'TRANSPORT_RECORD'
  | 'SET_TEMPO'
  | 'SET_TIME_SIGNATURE'
  | 'SET_LOOP'
  | 'SET_METRONOME'

  // Track Commands
  | 'CREATE_TRACK'
  | 'DELETE_TRACK'
  | 'RENAME_TRACK'
  | 'SET_TRACK_VOLUME'
  | 'SET_TRACK_PAN'
  | 'MUTE_TRACK'
  | 'SOLO_TRACK'
  | 'ARM_TRACK'

  // Clip Commands
  | 'INSERT_CLIP'
  | 'DELETE_CLIP'
  | 'LAUNCH_CLIP'
  | 'STOP_CLIP'
  | 'LAUNCH_SCENE'
  | 'STOP_SCENE'

  // Generation Commands
  | 'GENERATE_MIDI'
  | 'GENERATE_AUDIO'
  | 'GENERATE_DRUMS'
  | 'GENERATE_BASS'
  | 'GENERATE_MELODY'
  | 'GENERATE_HARMONY'

  // Query Commands
  | 'GET_STATE'
  | 'GET_TRACKS'
  | 'GET_CLIPS'
  | 'GET_TEMPO'

  // System Commands
  | 'PING'
  | 'DISCONNECT';

// ============================================================================
// Event Types
// ============================================================================

export type EventType =
  // Transport Events
  | 'PLAYBACK_STARTED'
  | 'PLAYBACK_STOPPED'
  | 'TEMPO_CHANGED'
  | 'POSITION_CHANGED'

  // Track Events
  | 'TRACK_CREATED'
  | 'TRACK_DELETED'
  | 'TRACK_UPDATED'

  // Clip Events
  | 'CLIP_INSERTED'
  | 'CLIP_DELETED'
  | 'CLIP_LAUNCHED'
  | 'CLIP_STOPPED'

  // Generation Events
  | 'GENERATION_STARTED'
  | 'GENERATION_PROGRESS'
  | 'GENERATION_COMPLETED'
  | 'GENERATION_FAILED'

  // System Events
  | 'CONNECTED'
  | 'DISCONNECTED'
  | 'ERROR';

// ============================================================================
// Command Payloads
// ============================================================================

export interface TransportCommandPayload {
  position?: number;                   // In beats
}

export interface SetTempoPayload {
  tempo: number;                       // BPM (20-999)
}

export interface SetTimeSignaturePayload {
  numerator: number;                   // e.g., 4
  denominator: number;                 // e.g., 4
}

export interface SetLoopPayload {
  enabled: boolean;
  start: number;                       // In beats
  end: number;                         // In beats
}

export interface CreateTrackPayload {
  name: string;
  type: 'midi' | 'audio' | 'instrument';
  index?: number;                      // Position in track list
}

export interface DeleteTrackPayload {
  trackId: string;
}

export interface RenameTrackPayload {
  trackId: string;
  name: string;
}

export interface SetTrackVolumePayload {
  trackId: string;
  volume: number;                      // 0-1
}

export interface SetTrackPanPayload {
  trackId: string;
  pan: number;                         // -1 to 1
}

export interface MuteTrackPayload {
  trackId: string;
  muted: boolean;
}

export interface SoloTrackPayload {
  trackId: string;
  solo: boolean;
}

export interface ArmTrackPayload {
  trackId: string;
  armed: boolean;
}

export interface InsertClipPayload {
  trackId: string;
  sceneIndex?: number;                 // For session view
  startTime?: number;                  // For arrangement view (in beats)
  clip: GeneratedClip;
}

export interface DeleteClipPayload {
  clipId: string;
}

export interface LaunchClipPayload {
  clipId: string;
  mode?: 'trigger' | 'gate' | 'toggle';
}

export interface LaunchScenePayload {
  sceneIndex: number;
}

export interface GenerateMidiPayload {
  prompt?: string;
  trackId?: string;
  duration?: number;                   // In beats
  key?: string;                        // e.g., "C", "Am"
  scale?: string;                      // e.g., "major", "minor"
  style?: string;                      // e.g., "jazz", "techno"
}

export interface GenerateAudioPayload {
  prompt?: string;
  trackId?: string;
  duration?: number;                   // In seconds
  style?: string;
}

// ============================================================================
// Generated Content
// ============================================================================

export interface MIDINote {
  pitch: number;                       // 0-127 (MIDI note number)
  velocity: number;                    // 0-127
  startTime: number;                   // In beats
  duration: number;                    // In beats
}

export interface GeneratedClip {
  id?: string;                         // Optional, will be generated if not provided
  name: string;
  type: 'midi' | 'audio';
  length: number;                      // In beats
  color?: string;                      // Hex color (e.g., "#3b82f6")

  // MIDI-specific
  notes?: MIDINote[];

  // Audio-specific
  audioData?: Float32Array;            // Raw audio samples
  sampleRate?: number;
  audioUrl?: string;                   // URL to audio file
}

// ============================================================================
// Connection State
// ============================================================================

export interface WingmanConnectionState {
  connected: boolean;
  protocol: 'websocket' | 'udp' | 'both' | 'none';
  websocketUrl?: string;
  udpHost?: string;
  udpPort?: number;
  lastPingTime?: number;
  latency?: number;                    // In milliseconds
  error?: string;
}

// ============================================================================
// Configuration
// ============================================================================

export interface WingmanBridgeConfig {
  websocket: {
    enabled: boolean;
    url: string;                       // e.g., "ws://localhost:8765"
    reconnect: boolean;
    reconnectInterval: number;         // In milliseconds
    heartbeatInterval: number;         // In milliseconds
  };
  udp: {
    enabled: boolean;
    host: string;                      // e.g., "localhost"
    port: number;                      // e.g., 8766
    receivePort: number;               // Local port for receiving
  };
  messageTimeout: number;              // Command timeout in milliseconds
  maxRetries: number;
}

// ============================================================================
// Default Configuration
// ============================================================================

export const DEFAULT_WINGMAN_CONFIG: WingmanBridgeConfig = {
  websocket: {
    enabled: true,
    url: 'ws://localhost:8765',
    reconnect: true,
    reconnectInterval: 5000,
    heartbeatInterval: 30000,
  },
  udp: {
    enabled: true,
    host: 'localhost',
    port: 8766,
    receivePort: 8767,
  },
  messageTimeout: 30000,
  maxRetries: 3,
};
