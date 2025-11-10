/**
 * Wingman AI Bridge Service
 * Handles WebSocket and UDP communication between DAW and external AI core
 */

const WebSocket = require('ws');
const dgram = require('dgram');
const { v4: uuidv4 } = require('uuid');

class WingmanBridgeService {
  constructor(mainWindow, audioStateRef) {
    this.mainWindow = mainWindow;
    this.audioStateRef = audioStateRef;  // Reference to the mutable audioState object

    // WebSocket
    this.ws = null;
    this.wsUrl = 'ws://localhost:8765';
    this.wsReconnectTimer = null;
    this.wsReconnectInterval = 5000;
    this.wsHeartbeatTimer = null;
    this.wsHeartbeatInterval = 30000;

    // UDP
    this.udpSocket = null;
    this.udpHost = 'localhost';
    this.udpPort = 8766;
    this.udpReceivePort = 8767;

    // State
    this.connected = false;
    this.protocol = 'none';
    this.pendingRequests = new Map();    // requestId -> { resolve, reject, timeout }
    this.messageTimeout = 30000;
    this.maxRetries = 3;

    console.log('[WingmanBridge] Service initialized');
  }

  // =========================================================================
  // Public API
  // =========================================================================

  /**
   * Connect to Wingman AI (WebSocket + UDP)
   */
  async connect(config = {}) {
    console.log('[WingmanBridge] Connecting...');

    const {
      websocketUrl = this.wsUrl,
      udpHost = this.udpHost,
      udpPort = this.udpPort,
      udpReceivePort = this.udpReceivePort,
    } = config;

    this.wsUrl = websocketUrl;
    this.udpHost = udpHost;
    this.udpPort = udpPort;
    this.udpReceivePort = udpReceivePort;

    // Connect WebSocket
    await this.connectWebSocket();

    // Connect UDP
    await this.connectUDP();

    // Send initial state
    this.sendEvent('CONNECTED', {
      daw: 'Vexel DAW',
      version: '1.0.0',
      capabilities: ['midi', 'audio', 'automation', 'session-view'],
    });

    this.notifyRenderer('connection-state-changed', this.getConnectionState());
  }

  /**
   * Disconnect from Wingman AI
   */
  disconnect() {
    console.log('[WingmanBridge] Disconnecting...');

    // Clear timers
    if (this.wsReconnectTimer) {
      clearTimeout(this.wsReconnectTimer);
      this.wsReconnectTimer = null;
    }
    if (this.wsHeartbeatTimer) {
      clearInterval(this.wsHeartbeatTimer);
      this.wsHeartbeatTimer = null;
    }

    // Disconnect WebSocket
    if (this.ws) {
      this.ws.close();
      this.ws = null;
    }

    // Disconnect UDP
    if (this.udpSocket) {
      this.udpSocket.close();
      this.udpSocket = null;
    }

    // Reject all pending requests
    for (const [id, request] of this.pendingRequests.entries()) {
      clearTimeout(request.timeout);
      request.reject(new Error('Disconnected'));
    }
    this.pendingRequests.clear();

    this.connected = false;
    this.protocol = 'none';
    this.notifyRenderer('connection-state-changed', this.getConnectionState());
  }

  /**
   * Send a command to Wingman AI
   * @returns {Promise<any>} Response data
   */
  async sendCommand(command, payload = {}) {
    if (!this.connected) {
      throw new Error('Not connected to Wingman AI');
    }

    const message = {
      id: uuidv4(),
      timestamp: Date.now(),
      type: 'command',
      command,
      payload,
    };

    console.log('[WingmanBridge] Sending command:', command, payload);

    // Send via WebSocket (primary)
    if (this.ws && this.ws.readyState === WebSocket.OPEN) {
      return this.sendWebSocketMessage(message);
    }

    // Fallback to UDP
    if (this.udpSocket) {
      return this.sendUDPMessage(message);
    }

    throw new Error('No active connection');
  }

  /**
   * Send an event notification (fire-and-forget)
   */
  sendEvent(event, data = {}) {
    const message = {
      id: uuidv4(),
      timestamp: Date.now(),
      type: 'event',
      event,
      data,
    };

    console.log('[WingmanBridge] Sending event:', event);

    // Send via both protocols
    if (this.ws && this.ws.readyState === WebSocket.OPEN) {
      this.ws.send(JSON.stringify(message));
    }
    if (this.udpSocket) {
      const buffer = Buffer.from(JSON.stringify(message));
      this.udpSocket.send(buffer, 0, buffer.length, this.udpPort, this.udpHost);
    }
  }

  // =========================================================================
  // WebSocket
  // =========================================================================

  async connectWebSocket() {
    return new Promise((resolve, reject) => {
      console.log('[WingmanBridge] Connecting WebSocket to', this.wsUrl);

      try {
        this.ws = new WebSocket(this.wsUrl);

        this.ws.on('open', () => {
          console.log('[WingmanBridge] WebSocket connected');
          this.connected = true;
          this.protocol = this.udpSocket ? 'both' : 'websocket';

          // Start heartbeat
          this.startHeartbeat();

          resolve();
        });

        this.ws.on('message', (data) => {
          try {
            const message = JSON.parse(data.toString());
            this.handleMessage(message);
          } catch (err) {
            console.error('[WingmanBridge] Failed to parse WebSocket message:', err);
          }
        });

        this.ws.on('close', () => {
          console.log('[WingmanBridge] WebSocket disconnected');
          this.connected = false;
          this.protocol = this.udpSocket ? 'udp' : 'none';
          this.notifyRenderer('connection-state-changed', this.getConnectionState());

          // Auto-reconnect
          this.scheduleReconnect();
        });

        this.ws.on('error', (err) => {
          console.error('[WingmanBridge] WebSocket error:', err.message);
          reject(err);
        });

        // Timeout if connection takes too long
        setTimeout(() => {
          if (this.ws && this.ws.readyState !== WebSocket.OPEN) {
            reject(new Error('WebSocket connection timeout'));
          }
        }, 5000);

      } catch (err) {
        console.error('[WingmanBridge] Failed to create WebSocket:', err);
        reject(err);
      }
    });
  }

  sendWebSocketMessage(message) {
    return new Promise((resolve, reject) => {
      if (message.type === 'command') {
        // Set up response handler
        const timeout = setTimeout(() => {
          this.pendingRequests.delete(message.id);
          reject(new Error('Command timeout'));
        }, this.messageTimeout);

        this.pendingRequests.set(message.id, { resolve, reject, timeout });
      }

      this.ws.send(JSON.stringify(message), (err) => {
        if (err) {
          if (this.pendingRequests.has(message.id)) {
            const request = this.pendingRequests.get(message.id);
            clearTimeout(request.timeout);
            this.pendingRequests.delete(message.id);
          }
          reject(err);
        } else if (message.type !== 'command') {
          resolve();
        }
      });
    });
  }

  startHeartbeat() {
    if (this.wsHeartbeatTimer) {
      clearInterval(this.wsHeartbeatTimer);
    }

    this.wsHeartbeatTimer = setInterval(() => {
      if (this.ws && this.ws.readyState === WebSocket.OPEN) {
        this.sendCommand('PING').catch(err => {
          console.error('[WingmanBridge] Heartbeat failed:', err);
        });
      }
    }, this.wsHeartbeatInterval);
  }

  scheduleReconnect() {
    if (this.wsReconnectTimer) {
      clearTimeout(this.wsReconnectTimer);
    }

    this.wsReconnectTimer = setTimeout(() => {
      console.log('[WingmanBridge] Attempting to reconnect WebSocket...');
      this.connectWebSocket().catch(err => {
        console.error('[WingmanBridge] Reconnect failed:', err);
      });
    }, this.wsReconnectInterval);
  }

  // =========================================================================
  // UDP
  // =========================================================================

  async connectUDP() {
    return new Promise((resolve, reject) => {
      console.log('[WingmanBridge] Connecting UDP socket on port', this.udpReceivePort);

      try {
        this.udpSocket = dgram.createSocket('udp4');

        this.udpSocket.on('listening', () => {
          const address = this.udpSocket.address();
          console.log('[WingmanBridge] UDP listening on', address);
          this.protocol = this.ws ? 'both' : 'udp';
          resolve();
        });

        this.udpSocket.on('message', (msg, rinfo) => {
          try {
            const message = JSON.parse(msg.toString());
            this.handleMessage(message);
          } catch (err) {
            console.error('[WingmanBridge] Failed to parse UDP message:', err);
          }
        });

        this.udpSocket.on('error', (err) => {
          console.error('[WingmanBridge] UDP error:', err);
          reject(err);
        });

        this.udpSocket.bind(this.udpReceivePort);

      } catch (err) {
        console.error('[WingmanBridge] Failed to create UDP socket:', err);
        reject(err);
      }
    });
  }

  sendUDPMessage(message) {
    return new Promise((resolve, reject) => {
      const buffer = Buffer.from(JSON.stringify(message));

      if (message.type === 'command') {
        const timeout = setTimeout(() => {
          this.pendingRequests.delete(message.id);
          reject(new Error('Command timeout'));
        }, this.messageTimeout);

        this.pendingRequests.set(message.id, { resolve, reject, timeout });
      }

      this.udpSocket.send(buffer, 0, buffer.length, this.udpPort, this.udpHost, (err) => {
        if (err) {
          if (this.pendingRequests.has(message.id)) {
            const request = this.pendingRequests.get(message.id);
            clearTimeout(request.timeout);
            this.pendingRequests.delete(message.id);
          }
          reject(err);
        } else if (message.type !== 'command') {
          resolve();
        }
      });
    });
  }

  // =========================================================================
  // Message Handling
  // =========================================================================

  handleMessage(message) {
    console.log('[WingmanBridge] Received message:', message.type, message);

    switch (message.type) {
      case 'response':
        this.handleResponse(message);
        break;
      case 'event':
        this.handleEvent(message);
        break;
      case 'command':
        this.handleCommand(message);
        break;
      case 'error':
        this.handleError(message);
        break;
      default:
        console.warn('[WingmanBridge] Unknown message type:', message.type);
    }
  }

  handleResponse(message) {
    const request = this.pendingRequests.get(message.requestId);
    if (request) {
      clearTimeout(request.timeout);
      this.pendingRequests.delete(message.requestId);

      if (message.success) {
        request.resolve(message.data);
      } else {
        request.reject(new Error(message.error || 'Command failed'));
      }
    }
  }

  handleEvent(message) {
    console.log('[WingmanBridge] Event received:', message.event);

    // Forward to renderer
    this.notifyRenderer('wingman-event', message);

    // Handle specific events
    switch (message.event) {
      case 'GENERATION_COMPLETED':
        this.handleGenerationCompleted(message.data);
        break;
      case 'CLIP_INSERTED':
        this.notifyRenderer('clip-inserted', message.data);
        break;
    }
  }

  async handleCommand(message) {
    console.log('[WingmanBridge] Command received:', message.command);

    try {
      const result = await this.executeCommand(message.command, message.payload);

      // Send response
      const response = {
        id: uuidv4(),
        timestamp: Date.now(),
        type: 'response',
        requestId: message.id,
        success: true,
        data: result,
      };

      if (this.ws && this.ws.readyState === WebSocket.OPEN) {
        this.ws.send(JSON.stringify(response));
      }
      if (this.udpSocket) {
        const buffer = Buffer.from(JSON.stringify(response));
        this.udpSocket.send(buffer, 0, buffer.length, this.udpPort, this.udpHost);
      }
    } catch (err) {
      console.error('[WingmanBridge] Command execution failed:', err);

      const errorResponse = {
        id: uuidv4(),
        timestamp: Date.now(),
        type: 'response',
        requestId: message.id,
        success: false,
        error: err.message,
      };

      if (this.ws && this.ws.readyState === WebSocket.OPEN) {
        this.ws.send(JSON.stringify(errorResponse));
      }
    }
  }

  handleError(message) {
    console.error('[WingmanBridge] Error received:', message.code, message.message);
    this.notifyRenderer('wingman-error', message);
  }

  // =========================================================================
  // Command Execution
  // =========================================================================

  async executeCommand(command, payload) {
    const audioState = this.audioStateRef;

    switch (command) {
      // Transport Commands
      case 'TRANSPORT_PLAY':
        audioState.isPlaying = true;
        this.broadcastAudioState();
        this.sendEvent('PLAYBACK_STARTED', { position: payload.position || 0 });
        return { success: true };

      case 'TRANSPORT_PAUSE':
        audioState.isPlaying = false;
        this.broadcastAudioState();
        return { success: true };

      case 'TRANSPORT_STOP':
        audioState.isPlaying = false;
        this.broadcastAudioState();
        this.sendEvent('PLAYBACK_STOPPED');
        return { success: true };

      case 'SET_TEMPO':
        audioState.tempo = Math.max(20, Math.min(999, payload.tempo));
        this.broadcastAudioState();
        this.sendEvent('TEMPO_CHANGED', { tempo: audioState.tempo });
        return { tempo: audioState.tempo };

      case 'SET_LOOP':
        // TODO: Implement loop functionality in audioState
        this.sendEvent('LOOP_CHANGED', payload);
        return { success: true };

      case 'SET_METRONOME':
        // TODO: Implement metronome functionality
        return { success: true };

      // Track Commands
      case 'CREATE_TRACK':
        const newTrack = {
          id: Date.now().toString(),
          name: payload.name || 'New Track',
          type: payload.type || 'midi',
          volume: 0.8,
          pan: 0,
          muted: false,
          solo: false,
        };
        audioState.tracks.push(newTrack);
        this.broadcastAudioState();
        this.sendEvent('TRACK_CREATED', newTrack);
        return newTrack;

      case 'DELETE_TRACK':
        const trackIndex = audioState.tracks.findIndex(t => t.id === payload.trackId);
        if (trackIndex !== -1) {
          const deletedTrack = audioState.tracks.splice(trackIndex, 1)[0];
          this.broadcastAudioState();
          this.sendEvent('TRACK_DELETED', { trackId: payload.trackId });
          return deletedTrack;
        }
        throw new Error('Track not found');

      case 'RENAME_TRACK':
        const track = audioState.tracks.find(t => t.id === payload.trackId);
        if (track) {
          track.name = payload.name;
          this.broadcastAudioState();
          this.sendEvent('TRACK_UPDATED', track);
          return track;
        }
        throw new Error('Track not found');

      case 'SET_TRACK_VOLUME':
        const volTrack = audioState.tracks.find(t => t.id === payload.trackId);
        if (volTrack) {
          volTrack.volume = Math.max(0, Math.min(1, payload.volume));
          this.broadcastAudioState();
          return { volume: volTrack.volume };
        }
        throw new Error('Track not found');

      case 'SET_TRACK_PAN':
        const panTrack = audioState.tracks.find(t => t.id === payload.trackId);
        if (panTrack) {
          panTrack.pan = Math.max(-1, Math.min(1, payload.pan));
          this.broadcastAudioState();
          return { pan: panTrack.pan };
        }
        throw new Error('Track not found');

      case 'MUTE_TRACK':
        const muteTrack = audioState.tracks.find(t => t.id === payload.trackId);
        if (muteTrack) {
          muteTrack.muted = payload.muted;
          this.broadcastAudioState();
          return { muted: muteTrack.muted };
        }
        throw new Error('Track not found');

      case 'SOLO_TRACK':
        const soloTrack = audioState.tracks.find(t => t.id === payload.trackId);
        if (soloTrack) {
          soloTrack.solo = payload.solo;
          this.broadcastAudioState();
          return { solo: soloTrack.solo };
        }
        throw new Error('Track not found');

      // Clip Commands
      case 'INSERT_CLIP':
        // Forward to renderer for clip insertion
        this.notifyRenderer('insert-clip', payload);
        this.sendEvent('CLIP_INSERTED', payload);
        return { success: true };

      case 'LAUNCH_CLIP':
        this.notifyRenderer('launch-clip', payload);
        this.sendEvent('CLIP_LAUNCHED', payload);
        return { success: true };

      // Query Commands
      case 'GET_STATE':
        return audioState;

      case 'GET_TRACKS':
        return { tracks: audioState.tracks };

      case 'GET_TEMPO':
        return { tempo: audioState.tempo };

      case 'PING':
        return { pong: true, timestamp: Date.now() };

      default:
        throw new Error(`Unknown command: ${command}`);
    }
  }

  // =========================================================================
  // AI Generation Handling
  // =========================================================================

  handleGenerationCompleted(data) {
    console.log('[WingmanBridge] Generation completed:', data);

    // Forward to renderer for UI update
    this.notifyRenderer('generation-completed', data);

    // If clip data is included, insert it
    if (data.clip) {
      this.notifyRenderer('insert-clip', {
        trackId: data.trackId,
        clip: data.clip,
        sceneIndex: data.sceneIndex,
        startTime: data.startTime,
      });
    }
  }

  // =========================================================================
  // Utilities
  // =========================================================================

  broadcastAudioState() {
    this.notifyRenderer('audio-state-update', this.audioStateRef);
  }

  notifyRenderer(channel, data) {
    if (this.mainWindow && !this.mainWindow.isDestroyed()) {
      this.mainWindow.webContents.send(channel, data);
    }
  }

  getConnectionState() {
    return {
      connected: this.connected,
      protocol: this.protocol,
      websocketUrl: this.wsUrl,
      udpHost: this.udpHost,
      udpPort: this.udpPort,
    };
  }
}

module.exports = WingmanBridgeService;
