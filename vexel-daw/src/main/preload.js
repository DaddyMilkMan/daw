const { contextBridge, ipcRenderer } = require('electron');

// Expose protected methods that allow the renderer process to use
// the ipcRenderer without exposing the entire object
contextBridge.exposeInMainWorld('electron', {
  // Transport controls
  transportPlay: () => ipcRenderer.send('transport-play'),
  transportPause: () => ipcRenderer.send('transport-pause'),
  transportStop: () => ipcRenderer.send('transport-stop'),

  // Audio controls
  setTempo: (tempo) => ipcRenderer.send('set-tempo', { tempo }),
  createTrack: (name, type) => ipcRenderer.send('create-track', { name, type }),
  getAudioState: () => ipcRenderer.invoke('get-audio-state'),

  // Window controls
  windowMinimize: () => ipcRenderer.send('window-minimize'),
  windowMaximize: () => ipcRenderer.send('window-maximize'),
  windowClose: () => ipcRenderer.send('window-close'),

  // Listen for audio state updates
  onAudioStateUpdate: (callback) => {
    const subscription = (event, data) => callback(data);
    ipcRenderer.on('audio-state-update', subscription);

    // Return unsubscribe function
    return () => {
      ipcRenderer.removeListener('audio-state-update', subscription);
    };
  },

  // Wingman AI Bridge
  wingman: {
    // Connect to Wingman AI
    connect: (config) => ipcRenderer.invoke('wingman-connect', config),

    // Disconnect from Wingman AI
    disconnect: () => ipcRenderer.send('wingman-disconnect'),

    // Send a command to Wingman AI
    sendCommand: (command, payload) => ipcRenderer.invoke('wingman-send-command', { command, payload }),

    // Send an event notification (fire-and-forget)
    sendEvent: (event, data) => ipcRenderer.send('wingman-send-event', { event, data }),

    // Get connection state
    getConnectionState: () => ipcRenderer.invoke('wingman-get-connection-state'),

    // Listen for connection state changes
    onConnectionStateChanged: (callback) => {
      const subscription = (event, data) => callback(data);
      ipcRenderer.on('connection-state-changed', subscription);
      return () => {
        ipcRenderer.removeListener('connection-state-changed', subscription);
      };
    },

    // Listen for Wingman events
    onEvent: (callback) => {
      const subscription = (event, data) => callback(data);
      ipcRenderer.on('wingman-event', subscription);
      return () => {
        ipcRenderer.removeListener('wingman-event', subscription);
      };
    },

    // Listen for Wingman errors
    onError: (callback) => {
      const subscription = (event, data) => callback(data);
      ipcRenderer.on('wingman-error', subscription);
      return () => {
        ipcRenderer.removeListener('wingman-error', subscription);
      };
    },

    // Listen for generation completed events
    onGenerationCompleted: (callback) => {
      const subscription = (event, data) => callback(data);
      ipcRenderer.on('generation-completed', subscription);
      return () => {
        ipcRenderer.removeListener('generation-completed', subscription);
      };
    },

    // Listen for clip insertion requests
    onInsertClip: (callback) => {
      const subscription = (event, data) => callback(data);
      ipcRenderer.on('insert-clip', subscription);
      return () => {
        ipcRenderer.removeListener('insert-clip', subscription);
      };
    },

    // Listen for clip launch requests
    onLaunchClip: (callback) => {
      const subscription = (event, data) => callback(data);
      ipcRenderer.on('launch-clip', subscription);
      return () => {
        ipcRenderer.removeListener('launch-clip', subscription);
      };
    },
  },
});
