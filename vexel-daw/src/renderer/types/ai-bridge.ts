/**
 * AI Bridge Types
 *
 * Protocol definitions for communication between the DAW and external AI core.
 * Supports both WebSocket and UDP transport layers.
 */

import { Note } from '../lib/MagentaService';

// ============================================================================
// Message Protocol
// ============================================================================

export enum MessageType {
  // Generation Commands
  GENERATE_MELODY = 'generate_melody',
  GENERATE_CHORDS = 'generate_chords',
  GENERATE_DRUMS = 'generate_drums',
  GENERATE_BASS = 'generate_bass',

  // Control Commands
  PLAY = 'play',
  STOP = 'stop',
  PAUSE = 'pause',
  SET_TEMPO = 'set_tempo',
  SET_KEY = 'set_key',

  // Query Commands
  GET_STATUS = 'get_status',
  GET_CAPABILITIES = 'get_capabilities',

  // Responses
  SUCCESS = 'success',
  ERROR = 'error',
  STATUS = 'status',
  CAPABILITIES = 'capabilities',
  GENERATION_COMPLETE = 'generation_complete',
}

export interface BaseMessage {
  id: string; // Unique message ID for request/response matching
  type: MessageType;
  timestamp: number;
}

// ============================================================================
// Request Messages
// ============================================================================

export interface GenerateMelodyRequest extends BaseMessage {
  type: MessageType.GENERATE_MELODY;
  payload: {
    seed?: Note[];
    temperature?: number;
    steps?: number;
    key?: string;
    scale?: 'major' | 'minor' | 'pentatonic' | 'blues';
  };
}

export interface GenerateChordsRequest extends BaseMessage {
  type: MessageType.GENERATE_CHORDS;
  payload: {
    key?: string;
    progression?: string; // e.g., 'I-V-vi-IV'
    bars?: number;
  };
}

export interface GenerateDrumsRequest extends BaseMessage {
  type: MessageType.GENERATE_DRUMS;
  payload: {
    style?: string; // 'trap', 'house', 'techno', 'rock', etc.
    bars?: number;
    complexity?: number; // 0-1
  };
}

export interface ControlRequest extends BaseMessage {
  type: MessageType.PLAY | MessageType.STOP | MessageType.PAUSE;
  payload?: {
    position?: number; // Playback position in beats
  };
}

export interface SetTempoRequest extends BaseMessage {
  type: MessageType.SET_TEMPO;
  payload: {
    tempo: number; // BPM
  };
}

export interface SetKeyRequest extends BaseMessage {
  type: MessageType.SET_KEY;
  payload: {
    key: string;
    scale?: 'major' | 'minor';
  };
}

export interface QueryRequest extends BaseMessage {
  type: MessageType.GET_STATUS | MessageType.GET_CAPABILITIES;
  payload?: Record<string, unknown>;
}

export type RequestMessage =
  | GenerateMelodyRequest
  | GenerateChordsRequest
  | GenerateDrumsRequest
  | ControlRequest
  | SetTempoRequest
  | SetKeyRequest
  | QueryRequest;

// ============================================================================
// Response Messages
// ============================================================================

export interface SuccessResponse extends BaseMessage {
  type: MessageType.SUCCESS;
  requestId: string; // ID of the request this responds to
  payload?: Record<string, unknown>;
}

export interface ErrorResponse extends BaseMessage {
  type: MessageType.ERROR;
  requestId: string;
  payload: {
    code: string;
    message: string;
    details?: unknown;
  };
}

export interface GenerationCompleteResponse extends BaseMessage {
  type: MessageType.GENERATION_COMPLETE;
  requestId: string;
  payload: {
    notes: Note[];
    metadata?: {
      key?: string;
      scale?: string;
      style?: string;
      bars?: number;
      [key: string]: unknown;
    };
  };
}

export interface StatusResponse extends BaseMessage {
  type: MessageType.STATUS;
  requestId: string;
  payload: {
    isPlaying: boolean;
    tempo: number;
    position: number; // in beats
    key?: string;
    activeModels: string[];
  };
}

export interface CapabilitiesResponse extends BaseMessage {
  type: MessageType.CAPABILITIES;
  requestId: string;
  payload: {
    models: {
      melody: boolean;
      chords: boolean;
      drums: boolean;
      bass: boolean;
    };
    maxSequenceLength: number;
    supportedScales: string[];
    supportedDrumStyles: string[];
  };
}

export type ResponseMessage =
  | SuccessResponse
  | ErrorResponse
  | GenerationCompleteResponse
  | StatusResponse
  | CapabilitiesResponse;

// ============================================================================
// AI Bridge Configuration
// ============================================================================

export interface AIBridgeConfig {
  // WebSocket configuration
  websocket: {
    enabled: boolean;
    host: string;
    port: number;
    secure: boolean; // Use WSS
    reconnectInterval: number; // milliseconds
    maxReconnectAttempts: number;
  };

  // UDP configuration (fallback)
  udp: {
    enabled: boolean;
    host: string;
    port: number;
    timeout: number; // milliseconds
  };

  // General settings
  preferredTransport: 'websocket' | 'udp';
  messageTimeout: number; // milliseconds
  maxRetries: number;
}

// ============================================================================
// AI Bridge Status
// ============================================================================

export enum ConnectionStatus {
  DISCONNECTED = 'disconnected',
  CONNECTING = 'connecting',
  CONNECTED = 'connected',
  RECONNECTING = 'reconnecting',
  ERROR = 'error',
}

export interface AIBridgeStatus {
  status: ConnectionStatus;
  transport: 'websocket' | 'udp' | null;
  lastMessageTime: number;
  reconnectAttempts: number;
  error?: string;
}

// ============================================================================
// Event Types
// ============================================================================

export interface AIBridgeEvents {
  connected: () => void;
  disconnected: (reason?: string) => void;
  error: (error: Error) => void;
  message: (message: ResponseMessage) => void;
  statusChange: (status: AIBridgeStatus) => void;
}
