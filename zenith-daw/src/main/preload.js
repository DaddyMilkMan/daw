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
});
