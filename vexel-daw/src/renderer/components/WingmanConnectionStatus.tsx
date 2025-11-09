/**
 * Wingman Connection Status Indicator
 * Shows connection state and provides connect/disconnect controls
 */

import { useState } from 'react';
import { useWingmanBridge } from '../hooks/useWingmanBridge';

export default function WingmanConnectionStatus() {
  const { connectionState, isConnected, isConnecting, error, connect, disconnect } = useWingmanBridge();
  const [showDetails, setShowDetails] = useState(false);
  const [customConfig, setCustomConfig] = useState({
    websocketUrl: 'ws://localhost:8765',
    udpHost: 'localhost',
    udpPort: 8766,
  });

  const handleConnect = async () => {
    try {
      await connect({
        websocket: {
          enabled: true,
          url: customConfig.websocketUrl,
          reconnect: true,
          reconnectInterval: 5000,
          heartbeatInterval: 30000,
        },
        udp: {
          enabled: true,
          host: customConfig.udpHost,
          port: customConfig.udpPort,
          receivePort: 8767,
        },
        messageTimeout: 30000,
        maxRetries: 3,
      });
    } catch (err) {
      console.error('Failed to connect:', err);
    }
  };

  const handleDisconnect = () => {
    disconnect();
  };

  const getStatusColor = () => {
    if (isConnecting) return 'bg-yellow-500';
    if (isConnected) return 'bg-green-500';
    return 'bg-red-500';
  };

  const getStatusText = () => {
    if (isConnecting) return 'Connecting...';
    if (isConnected) {
      const protocol = connectionState.protocol;
      if (protocol === 'both') return 'WebSocket + UDP';
      if (protocol === 'websocket') return 'WebSocket';
      if (protocol === 'udp') return 'UDP';
      return 'Connected';
    }
    return 'Disconnected';
  };

  return (
    <div className="relative">
      {/* Status Indicator Button */}
      <button
        onClick={() => setShowDetails(!showDetails)}
        className="flex items-center gap-2 px-3 py-1.5 rounded-md bg-card hover:bg-card/80 border border-border transition-colors"
        title="Wingman AI Connection"
      >
        <div className={`w-2 h-2 rounded-full ${getStatusColor()} ${isConnected ? 'animate-pulse' : ''}`} />
        <span className="text-xs font-medium">Wingman AI</span>
        <span className="text-xs text-muted-foreground">{getStatusText()}</span>
      </button>

      {/* Details Panel */}
      {showDetails && (
        <div className="absolute top-full right-0 mt-2 w-80 bg-card border border-border rounded-lg shadow-xl z-50 p-4">
          <div className="flex items-center justify-between mb-3">
            <h3 className="text-sm font-semibold">Wingman AI Bridge</h3>
            <button
              onClick={() => setShowDetails(false)}
              className="text-muted-foreground hover:text-foreground"
            >
              ×
            </button>
          </div>

          {/* Connection Status */}
          <div className="mb-4 p-3 bg-background rounded-md">
            <div className="flex items-center gap-2 mb-2">
              <div className={`w-3 h-3 rounded-full ${getStatusColor()}`} />
              <span className="text-sm font-medium">{getStatusText()}</span>
            </div>

            {isConnected && (
              <div className="text-xs text-muted-foreground space-y-1">
                {connectionState.websocketUrl && (
                  <div>WebSocket: {connectionState.websocketUrl}</div>
                )}
                {connectionState.udpHost && connectionState.udpPort && (
                  <div>UDP: {connectionState.udpHost}:{connectionState.udpPort}</div>
                )}
                {connectionState.latency !== undefined && (
                  <div>Latency: {connectionState.latency}ms</div>
                )}
              </div>
            )}

            {error && (
              <div className="mt-2 p-2 bg-red-500/10 border border-red-500/20 rounded text-xs text-red-400">
                {error}
              </div>
            )}
          </div>

          {/* Connection Controls */}
          {!isConnected ? (
            <div className="space-y-3">
              <div>
                <label className="text-xs text-muted-foreground block mb-1">
                  WebSocket URL
                </label>
                <input
                  type="text"
                  value={customConfig.websocketUrl}
                  onChange={(e) => setCustomConfig(prev => ({ ...prev, websocketUrl: e.target.value }))}
                  className="w-full px-2 py-1.5 text-xs bg-background border border-border rounded focus:outline-none focus:ring-2 focus:ring-primary"
                  placeholder="ws://localhost:8765"
                />
              </div>

              <div className="grid grid-cols-2 gap-2">
                <div>
                  <label className="text-xs text-muted-foreground block mb-1">
                    UDP Host
                  </label>
                  <input
                    type="text"
                    value={customConfig.udpHost}
                    onChange={(e) => setCustomConfig(prev => ({ ...prev, udpHost: e.target.value }))}
                    className="w-full px-2 py-1.5 text-xs bg-background border border-border rounded focus:outline-none focus:ring-2 focus:ring-primary"
                    placeholder="localhost"
                  />
                </div>
                <div>
                  <label className="text-xs text-muted-foreground block mb-1">
                    UDP Port
                  </label>
                  <input
                    type="number"
                    value={customConfig.udpPort}
                    onChange={(e) => setCustomConfig(prev => ({ ...prev, udpPort: parseInt(e.target.value) }))}
                    className="w-full px-2 py-1.5 text-xs bg-background border border-border rounded focus:outline-none focus:ring-2 focus:ring-primary"
                    placeholder="8766"
                  />
                </div>
              </div>

              <button
                onClick={handleConnect}
                disabled={isConnecting}
                className="w-full px-4 py-2 bg-primary text-primary-foreground rounded-md hover:bg-primary/90 disabled:opacity-50 disabled:cursor-not-allowed transition-colors text-sm font-medium"
              >
                {isConnecting ? 'Connecting...' : 'Connect'}
              </button>
            </div>
          ) : (
            <button
              onClick={handleDisconnect}
              className="w-full px-4 py-2 bg-red-500 text-white rounded-md hover:bg-red-600 transition-colors text-sm font-medium"
            >
              Disconnect
            </button>
          )}

          {/* Info */}
          <div className="mt-4 p-3 bg-blue-500/10 border border-blue-500/20 rounded text-xs text-blue-400">
            <p className="font-semibold mb-1">💡 Protocol Support</p>
            <ul className="list-disc list-inside space-y-1 text-blue-300/80">
              <li>WebSocket: Real-time bidirectional communication</li>
              <li>UDP: Low-latency command streaming</li>
              <li>Both protocols can run simultaneously</li>
            </ul>
          </div>
        </div>
      )}
    </div>
  );
}
