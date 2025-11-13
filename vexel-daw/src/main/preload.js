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
  setTimeSignature: (numerator, denominator) => ipcRenderer.send('set-time-signature', { numerator, denominator }),
  setLoop: (enabled, start, end) => ipcRenderer.send('set-loop', { enabled, start, end }),
  setMetronome: (enabled) => ipcRenderer.send('set-metronome', { enabled }),

  // Track operations
  createTrack: (name, type) => ipcRenderer.send('create-track', { name, type }),
  deleteTrack: (trackId) => ipcRenderer.send('delete-track', { trackId }),
  renameTrack: (trackId, name) => ipcRenderer.send('rename-track', { trackId, name }),
  setTrackVolume: (trackId, volume) => ipcRenderer.send('set-track-volume', { trackId, volume }),
  setTrackPan: (trackId, pan) => ipcRenderer.send('set-track-pan', { trackId, pan }),
  setTrackMute: (trackId, muted) => ipcRenderer.send('set-track-mute', { trackId, muted }),
  setTrackSolo: (trackId, solo) => ipcRenderer.send('set-track-solo', { trackId, solo }),
  setTrackRecordArm: (trackId, armed) => ipcRenderer.send('set-track-record-arm', { trackId, armed }),
  setTrackColor: (trackId, color) => ipcRenderer.send('set-track-color', { trackId, color }),

  // Automation
  addAutomation: (trackId, parameter, points) => ipcRenderer.send('add-automation', { trackId, parameter, points }),
  updateAutomation: (trackId, parameter, points) => ipcRenderer.send('update-automation', { trackId, parameter, points }),
  deleteAutomation: (trackId, parameter) => ipcRenderer.send('delete-automation', { trackId, parameter }),

  // Clips (Session View)
  createClip: (trackId, sceneIndex, length) => ipcRenderer.send('create-clip', { trackId, sceneIndex, length }),
  deleteClip: (clipId) => ipcRenderer.send('delete-clip', { clipId }),
  launchClip: (clipId) => ipcRenderer.send('launch-clip', { clipId }),
  stopClip: (trackId) => ipcRenderer.send('stop-clip', { trackId }),

  // MIDI operations
  addMidiNote: (trackId, pitch, time, duration, velocity) => ipcRenderer.send('add-midi-note', { trackId, pitch, time, duration, velocity }),
  removeMidiNote: (trackId, noteId) => ipcRenderer.send('remove-midi-note', { trackId, noteId }),
  updateMidiNote: (trackId, noteId, updates) => ipcRenderer.send('update-midi-note', { trackId, noteId, ...updates }),

  // Batch operations
  batchStart: () => ipcRenderer.send('batch-start'),
  batchCommit: () => ipcRenderer.send('batch-commit'),
  batchRollback: () => ipcRenderer.send('batch-rollback'),

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

  // Listen for project data updates
  onProjectDataUpdate: (callback) => {
    const subscription = (event, data) => callback(data);
    ipcRenderer.on('project-data-update', subscription);

    // Return unsubscribe function
    return () => {
      ipcRenderer.removeListener('project-data-update', subscription);
    };
  },

  // Project file operations
  saveProject: (filePath, data) => ipcRenderer.invoke('save-project', { filePath, data }),
  loadProject: (filePath) => ipcRenderer.invoke('load-project', { filePath }),
  newProject: () => ipcRenderer.invoke('new-project'),
  getProjectInfo: () => ipcRenderer.invoke('get-project-info'),
});
