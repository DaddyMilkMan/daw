/**
 * AI Bridge Client
 *
 * Manages communication between the DAW and external AI core.
 * Supports WebSocket (primary) and UDP (fallback) transports.
 */

import {
  AIBridgeConfig,
  AIBridgeStatus,
  ConnectionStatus,
  MessageType,
  RequestMessage,
  ResponseMessage,
  GenerationCompleteResponse,
} from '../types/ai-bridge';

type MessageCallback = (message: ResponseMessage) => void;
type ErrorCallback = (error: Error) => void;
type StatusCallback = (status: AIBridgeStatus) => void;

export class AIBridge {
  private config: AIBridgeConfig;
  private status: AIBridgeStatus;
  private websocket: WebSocket | null = null;
  private pendingRequests: Map<string, {
    resolve: (response: ResponseMessage) => void;
    reject: (error: Error) => void;
    timeout: NodeJS.Timeout;
  }> = new Map();

  // Event callbacks
  private messageCallbacks: MessageCallback[] = [];
  private errorCallbacks: ErrorCallback[] = [];
  private statusCallbacks: StatusCallback[] = [];

  constructor(config?: Partial<AIBridgeConfig>) {
    this.config = {
      websocket: {
        enabled: true,
        host: 'localhost',
        port: 8765,
        secure: false,
        reconnectInterval: 3000,
        maxReconnectAttempts: 5,
        ...config?.websocket,
      },
      udp: {
        enabled: true,
        host: 'localhost',
        port: 8766,
        timeout: 5000,
        ...config?.udp,
      },
      preferredTransport: config?.preferredTransport ?? 'websocket',
      messageTimeout: config?.messageTimeout ?? 10000,
      maxRetries: config?.maxRetries ?? 3,
    };

    this.status = {
      status: ConnectionStatus.DISCONNECTED,
      transport: null,
      lastMessageTime: 0,
      reconnectAttempts: 0,
    };
  }

  // ============================================================================
  // Connection Management
  // ============================================================================

  /**
   * Connect to the AI service
   */
  async connect(): Promise<void> {
    if (this.status.status === ConnectionStatus.CONNECTED) {
      console.log('🔗 Already connected to AI Bridge');
      return;
    }

    console.log('🔗 Connecting to AI Bridge...');
    this.updateStatus(ConnectionStatus.CONNECTING);

    if (this.config.preferredTransport === 'websocket' && this.config.websocket.enabled) {
      await this.connectWebSocket();
    } else if (this.config.udp.enabled) {
      await this.connectUDP();
    } else {
      throw new Error('No transport method enabled');
    }
  }

  /**
   * Disconnect from the AI service
   */
  disconnect(): void {
    console.log('🔌 Disconnecting from AI Bridge...');

    if (this.websocket) {
      this.websocket.close();
      this.websocket = null;
    }

    // Clear pending requests
    this.pendingRequests.forEach(({ reject, timeout }) => {
      clearTimeout(timeout);
      reject(new Error('Connection closed'));
    });
    this.pendingRequests.clear();

    this.updateStatus(ConnectionStatus.DISCONNECTED);
  }

  /**
   * Connect via WebSocket
   */
  private async connectWebSocket(): Promise<void> {
    const protocol = this.config.websocket.secure ? 'wss' : 'ws';
    const url = `${protocol}://${this.config.websocket.host}:${this.config.websocket.port}`;

    return new Promise((resolve, reject) => {
      try {
        this.websocket = new WebSocket(url);

        this.websocket.onopen = () => {
          console.log('✅ WebSocket connected');
          this.status.transport = 'websocket';
          this.status.reconnectAttempts = 0;
          this.updateStatus(ConnectionStatus.CONNECTED);
          resolve();
        };

        this.websocket.onmessage = (event) => {
          this.handleWebSocketMessage(event);
        };

        this.websocket.onerror = (error) => {
          console.error('❌ WebSocket error:', error);
          this.handleError(new Error('WebSocket connection error'));
          reject(error);
        };

        this.websocket.onclose = (event) => {
          console.log('🔌 WebSocket closed:', event.code, event.reason);
          this.handleDisconnect();
        };
      } catch (error) {
        console.error('❌ Failed to create WebSocket:', error);
        reject(error);
      }
    });
  }

  /**
   * Connect via UDP (fallback)
   * Note: UDP is not directly supported in browsers. This would need to be
   * implemented in the Electron main process and communicated via IPC.
   */
  private async connectUDP(): Promise<void> {
    console.log('📡 UDP transport not yet implemented (requires Electron IPC)');
    // TODO: Implement UDP via Electron main process
    throw new Error('UDP transport not yet implemented');
  }

  /**
   * Handle WebSocket message
   */
  private handleWebSocketMessage(event: MessageEvent): void {
    try {
      const message: ResponseMessage = JSON.parse(event.data);
      this.status.lastMessageTime = Date.now();

      console.log('📨 Received message:', message.type);

      // Notify message callbacks
      this.messageCallbacks.forEach((callback) => callback(message));

      // Resolve pending request if this is a response
      if ('requestId' in message && message.requestId) {
        const pending = this.pendingRequests.get(message.requestId);
        if (pending) {
          clearTimeout(pending.timeout);
          this.pendingRequests.delete(message.requestId);
          pending.resolve(message);
        }
      }
    } catch (error) {
      console.error('❌ Failed to parse message:', error);
      this.handleError(error as Error);
    }
  }

  /**
   * Handle disconnect
   */
  private handleDisconnect(): void {
    this.websocket = null;
    this.status.transport = null;

    // Attempt reconnection
    if (
      this.status.reconnectAttempts < this.config.websocket.maxReconnectAttempts
    ) {
      this.status.reconnectAttempts++;
      this.updateStatus(ConnectionStatus.RECONNECTING);

      console.log(
        `🔄 Reconnecting... (attempt ${this.status.reconnectAttempts}/${this.config.websocket.maxReconnectAttempts})`
      );

      setTimeout(() => {
        this.connectWebSocket().catch((error) => {
          console.error('❌ Reconnection failed:', error);
          this.handleError(error);
        });
      }, this.config.websocket.reconnectInterval);
    } else {
      this.updateStatus(ConnectionStatus.DISCONNECTED);
    }
  }

  /**
   * Handle error
   */
  private handleError(error: Error): void {
    this.status.error = error.message;
    this.updateStatus(ConnectionStatus.ERROR);
    this.errorCallbacks.forEach((callback) => callback(error));
  }

  /**
   * Update status and notify callbacks
   */
  private updateStatus(status: ConnectionStatus): void {
    this.status.status = status;
    this.statusCallbacks.forEach((callback) => callback(this.status));
  }

  // ============================================================================
  // Message Sending
  // ============================================================================

  /**
   * Send a message to the AI service
   */
  async sendMessage<T extends ResponseMessage>(
    message: RequestMessage
  ): Promise<T> {
    if (this.status.status !== ConnectionStatus.CONNECTED) {
      throw new Error('Not connected to AI Bridge');
    }

    if (!this.websocket) {
      throw new Error('No active transport');
    }

    return new Promise((resolve, reject) => {
      // Set up timeout
      const timeout = setTimeout(() => {
        this.pendingRequests.delete(message.id);
        reject(new Error(`Request timeout: ${message.type}`));
      }, this.config.messageTimeout);

      // Store pending request
      this.pendingRequests.set(message.id, {
        resolve: resolve as (response: ResponseMessage) => void,
        reject,
        timeout,
      });

      // Send message
      try {
        const data = JSON.stringify(message);
        this.websocket!.send(data);
        console.log('📤 Sent message:', message.type);
      } catch (error) {
        clearTimeout(timeout);
        this.pendingRequests.delete(message.id);
        reject(error);
      }
    });
  }

  /**
   * Create a request message with unique ID
   */
  createRequest<T extends RequestMessage>(
    type: MessageType,
    payload?: Record<string, unknown>
  ): T {
    return {
      id: this.generateMessageId(),
      type,
      timestamp: Date.now(),
      payload,
    } as T;
  }

  /**
   * Generate unique message ID
   */
  private generateMessageId(): string {
    return `${Date.now()}-${Math.random().toString(36).substr(2, 9)}`;
  }

  // ============================================================================
  // Event Listeners
  // ============================================================================

  onMessage(callback: MessageCallback): () => void {
    this.messageCallbacks.push(callback);
    return () => {
      this.messageCallbacks = this.messageCallbacks.filter((cb) => cb !== callback);
    };
  }

  onError(callback: ErrorCallback): () => void {
    this.errorCallbacks.push(callback);
    return () => {
      this.errorCallbacks = this.errorCallbacks.filter((cb) => cb !== callback);
    };
  }

  onStatusChange(callback: StatusCallback): () => void {
    this.statusCallbacks.push(callback);
    return () => {
      this.statusCallbacks = this.statusCallbacks.filter((cb) => cb !== callback);
    };
  }

  // ============================================================================
  // Getters
  // ============================================================================

  getStatus(): AIBridgeStatus {
    return { ...this.status };
  }

  isConnected(): boolean {
    return this.status.status === ConnectionStatus.CONNECTED;
  }
}

// Singleton instance
export const aiBridge = new AIBridge();
export default aiBridge;
