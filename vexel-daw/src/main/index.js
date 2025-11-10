const { app, BrowserWindow, ipcMain } = require('electron');
const path = require('path');
const WingmanBridgeService = require('./services/WingmanBridgeService');

let mainWindow;
let wingmanBridge;

// Audio engine state (stub for now)
const audioState = {
  tempo: 120,
  timeSignature: { numerator: 4, denominator: 4 },
  isPlaying: false,
  currentBar: 0,
  tracks: [],
};

function createWindow() {
  mainWindow = new BrowserWindow({
    width: 1600,
    height: 1000,
    minWidth: 1200,
    minHeight: 700,
    backgroundColor: '#1a1a1a',
    webPreferences: {
      nodeIntegration: false,
      contextIsolation: true,
      preload: path.join(__dirname, 'preload.js'),
    },
    titleBarStyle: 'hidden',
    frame: false,
  });

  // Load the app
  if (process.env.NODE_ENV === 'development') {
    mainWindow.loadURL('http://localhost:5173');
    mainWindow.webContents.openDevTools();
  } else {
    mainWindow.loadFile(path.join(__dirname, '../../dist/index.html'));
  }

  // Send initial state to renderer
  mainWindow.webContents.on('did-finish-load', () => {
    mainWindow.webContents.send('audio-state-update', audioState);
  });
}

app.whenReady().then(() => {
  createWindow();

  // Initialize Wingman AI Bridge
  wingmanBridge = new WingmanBridgeService(mainWindow, audioState);
  console.log('🤖 Wingman AI Bridge initialized');

  app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) {
      createWindow();
    }
  });
});

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') {
    app.quit();
  }
});

// IPC Handlers
ipcMain.on('transport-play', () => {
  console.log('▶️ Transport: Play');
  audioState.isPlaying = true;
  mainWindow.webContents.send('audio-state-update', audioState);
});

ipcMain.on('transport-pause', () => {
  console.log('⏸️ Transport: Pause');
  audioState.isPlaying = false;
  mainWindow.webContents.send('audio-state-update', audioState);
});

ipcMain.on('transport-stop', () => {
  console.log('⏹️ Transport: Stop');
  audioState.isPlaying = false;
  audioState.currentBar = 0;
  mainWindow.webContents.send('audio-state-update', audioState);
});

ipcMain.on('set-tempo', (event, { tempo }) => {
  console.log(`🎵 Set tempo: ${tempo} BPM`);
  audioState.tempo = tempo;
  mainWindow.webContents.send('audio-state-update', audioState);
});

ipcMain.on('create-track', (event, { name, type }) => {
  console.log(`➕ Create track: ${name} (${type})`);
  const track = {
    id: Date.now().toString(),
    name,
    type,
    volume: 0.8,
    pan: 0,
    muted: false,
    solo: false,
  };
  audioState.tracks.push(track);
  mainWindow.webContents.send('audio-state-update', audioState);
});

ipcMain.handle('get-audio-state', () => {
  return audioState;
});

// Window controls
ipcMain.on('window-minimize', () => {
  mainWindow.minimize();
});

ipcMain.on('window-maximize', () => {
  if (mainWindow.isMaximized()) {
    mainWindow.unmaximize();
  } else {
    mainWindow.maximize();
  }
});

ipcMain.on('window-close', () => {
  mainWindow.close();
});

// ============================================================================
// Wingman AI Bridge IPC Handlers
// ============================================================================

// Connect to Wingman AI
ipcMain.handle('wingman-connect', async (event, config) => {
  try {
    console.log('🤖 Connecting to Wingman AI...', config);
    await wingmanBridge.connect(config);
    return { success: true, state: wingmanBridge.getConnectionState() };
  } catch (err) {
    console.error('❌ Failed to connect to Wingman AI:', err);
    return { success: false, error: err.message };
  }
});

// Disconnect from Wingman AI
ipcMain.on('wingman-disconnect', () => {
  console.log('🤖 Disconnecting from Wingman AI...');
  wingmanBridge.disconnect();
});

// Send command to Wingman AI
ipcMain.handle('wingman-send-command', async (event, { command, payload }) => {
  try {
    console.log('🤖 Sending command to Wingman AI:', command);
    const result = await wingmanBridge.sendCommand(command, payload);
    return { success: true, data: result };
  } catch (err) {
    console.error('❌ Failed to send command to Wingman AI:', err);
    return { success: false, error: err.message };
  }
});

// Send event to Wingman AI
ipcMain.on('wingman-send-event', (event, { event: eventType, data }) => {
  console.log('🤖 Sending event to Wingman AI:', eventType);
  wingmanBridge.sendEvent(eventType, data);
});

// Get connection state
ipcMain.handle('wingman-get-connection-state', () => {
  return wingmanBridge.getConnectionState();
});

// Cleanup on app quit
app.on('before-quit', () => {
  if (wingmanBridge) {
    console.log('🤖 Cleaning up Wingman AI Bridge...');
    wingmanBridge.disconnect();
  }
});
