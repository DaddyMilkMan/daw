/**
 * React Hook for Wingman AI Bridge
 * Manages connection, commands, and event handling
 */

import { useState, useEffect, useCallback, useRef } from 'react';
import type {
  WingmanConnectionState,
  WingmanBridgeConfig,
  CommandType,
  WingmanEvent,
  WingmanError,
  DEFAULT_WINGMAN_CONFIG,
} from '../types/wingman';

interface UseWingmanBridgeReturn {
  // Connection state
  connectionState: WingmanConnectionState;
  isConnected: boolean;
  isConnecting: boolean;
  error: string | null;

  // Actions
  connect: (config?: Partial<WingmanBridgeConfig>) => Promise<void>;
  disconnect: () => void;
  sendCommand: (command: CommandType, payload?: any) => Promise<any>;
  sendEvent: (event: string, data?: any) => void;

  // Event listeners
  lastEvent: WingmanEvent | null;
  lastError: WingmanError | null;
}

/**
 * Hook for managing Wingman AI Bridge connection
 */
export function useWingmanBridge(): UseWingmanBridgeReturn {
  const [connectionState, setConnectionState] = useState<WingmanConnectionState>({
    connected: false,
    protocol: 'none',
  });
  const [isConnecting, setIsConnecting] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [lastEvent, setLastEvent] = useState<WingmanEvent | null>(null);
  const [lastError, setLastError] = useState<WingmanError | null>(null);

  // Track if component is mounted to prevent state updates after unmount
  const isMountedRef = useRef(true);

  // Listen for connection state changes
  useEffect(() => {
    const unsubscribe = window.electron.wingman.onConnectionStateChanged((state) => {
      if (isMountedRef.current) {
        setConnectionState(state);
        setError(state.error || null);
      }
    });

    return () => {
      unsubscribe();
    };
  }, []);

  // Listen for Wingman events
  useEffect(() => {
    const unsubscribe = window.electron.wingman.onEvent((event) => {
      if (isMountedRef.current) {
        console.log('Wingman event received:', event);
        setLastEvent(event);
      }
    });

    return () => {
      unsubscribe();
    };
  }, []);

  // Listen for Wingman errors
  useEffect(() => {
    const unsubscribe = window.electron.wingman.onError((error) => {
      if (isMountedRef.current) {
        console.error('Wingman error received:', error);
        setLastError(error);
        setError(error.message);
      }
    });

    return () => {
      unsubscribe();
    };
  }, []);

  // Get initial connection state on mount
  useEffect(() => {
    window.electron.wingman.getConnectionState().then((state) => {
      if (isMountedRef.current) {
        setConnectionState(state);
      }
    });

    return () => {
      isMountedRef.current = false;
    };
  }, []);

  // Connect to Wingman AI
  const connect = useCallback(async (config?: Partial<WingmanBridgeConfig>) => {
    setIsConnecting(true);
    setError(null);

    try {
      const result = await window.electron.wingman.connect(config);

      if (result.success) {
        console.log('✅ Connected to Wingman AI');
        if (isMountedRef.current) {
          setConnectionState(result.state);
          setIsConnecting(false);
        }
      } else {
        throw new Error(result.error || 'Connection failed');
      }
    } catch (err) {
      console.error('❌ Failed to connect to Wingman AI:', err);
      if (isMountedRef.current) {
        setError(err instanceof Error ? err.message : 'Connection failed');
        setIsConnecting(false);
      }
    }
  }, []);

  // Disconnect from Wingman AI
  const disconnect = useCallback(() => {
    console.log('Disconnecting from Wingman AI...');
    window.electron.wingman.disconnect();
    setConnectionState({
      connected: false,
      protocol: 'none',
    });
  }, []);

  // Send command to Wingman AI
  const sendCommand = useCallback(async (command: CommandType, payload?: any) => {
    if (!connectionState.connected) {
      throw new Error('Not connected to Wingman AI');
    }

    try {
      const result = await window.electron.wingman.sendCommand(command, payload);

      if (result.success) {
        return result.data;
      } else {
        throw new Error(result.error || 'Command failed');
      }
    } catch (err) {
      console.error('❌ Failed to send command:', err);
      throw err;
    }
  }, [connectionState.connected]);

  // Send event to Wingman AI
  const sendEvent = useCallback((event: string, data?: any) => {
    if (connectionState.connected) {
      window.electron.wingman.sendEvent(event, data);
    } else {
      console.warn('Cannot send event: Not connected to Wingman AI');
    }
  }, [connectionState.connected]);

  return {
    connectionState,
    isConnected: connectionState.connected,
    isConnecting,
    error,
    connect,
    disconnect,
    sendCommand,
    sendEvent,
    lastEvent,
    lastError,
  };
}

/**
 * Hook specifically for handling AI-generated clips
 */
export function useWingmanClipInsertion(onClipInserted: (data: any) => void) {
  useEffect(() => {
    const unsubscribeInsert = window.electron.wingman.onInsertClip((data) => {
      console.log('Inserting AI-generated clip:', data);
      onClipInserted(data);
    });

    const unsubscribeGeneration = window.electron.wingman.onGenerationCompleted((data) => {
      console.log('AI generation completed:', data);
      // Also trigger clip insertion if clip data is included
      if (data.clip) {
        onClipInserted(data);
      }
    });

    return () => {
      unsubscribeInsert();
      unsubscribeGeneration();
    };
  }, [onClipInserted]);
}

/**
 * Hook for automatically syncing transport state with Wingman AI
 */
export function useWingmanTransportSync(isPlaying: boolean, tempo: number) {
  const { isConnected, sendEvent } = useWingmanBridge();
  const prevIsPlayingRef = useRef(isPlaying);
  const prevTempoRef = useRef(tempo);

  useEffect(() => {
    if (isConnected) {
      // Send playback state changes
      if (isPlaying !== prevIsPlayingRef.current) {
        sendEvent(isPlaying ? 'PLAYBACK_STARTED' : 'PLAYBACK_STOPPED', {
          position: 0, // TODO: Get actual playback position
        });
        prevIsPlayingRef.current = isPlaying;
      }

      // Send tempo changes
      if (tempo !== prevTempoRef.current) {
        sendEvent('TEMPO_CHANGED', { tempo });
        prevTempoRef.current = tempo;
      }
    }
  }, [isConnected, isPlaying, tempo, sendEvent]);
}
