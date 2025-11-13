const { app, BrowserWindow, ipcMain, dialog } = require('electron');
const path = require('path');
const WingmanBridgeService = require('./services/WingmanBridgeService');
const fs = require('fs').promises;

let mainWindow;
let wingmanBridge;
let currentProjectPath = null; // Track the current project file path

// Extended audio engine state with full DAW features
const audioState = {
  tempo: 120,
  timeSignature: { numerator: 4, denominator: 4 },
  loop: { enabled: false, start: 0, end: 4 },
  metronome: { enabled: false },
  isPlaying: false,
  currentBar: 0,
  tracks: [],
};

// Extended project data
const projectData = {
  clips: [],
  scenes: [],
  automation: {},  // Map of trackId -> automation lanes
  midiNotes: {},   // Map of trackId -> MIDI notes
  markers: [],
  metadata: {
    name: 'Untitled Project',
    createdAt: new Date().toISOString(),
    modifiedAt: new Date().toISOString(),
  },
};

// Batch operation state
let batchState = {
  active: false,
  operations: [],
  snapshot: null,
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
    recordArm: false,
    color: '#4a90e2',
  };
  audioState.tracks.push(track);
  mainWindow.webContents.send('audio-state-update', audioState);
});

// Time signature
ipcMain.on('set-time-signature', (event, { numerator, denominator }) => {
  console.log(`🎼 Set time signature: ${numerator}/${denominator}`);
  audioState.timeSignature = { numerator, denominator };
  mainWindow.webContents.send('audio-state-update', audioState);
});

// Loop
ipcMain.on('set-loop', (event, { enabled, start, end }) => {
  console.log(`🔁 Set loop: ${enabled ? 'ON' : 'OFF'} (${start}-${end})`);
  audioState.loop = { enabled, start, end };
  mainWindow.webContents.send('audio-state-update', audioState);
});

// Metronome
ipcMain.on('set-metronome', (event, { enabled }) => {
  console.log(`🎶 Metronome: ${enabled ? 'ON' : 'OFF'}`);
  audioState.metronome = { enabled };
  mainWindow.webContents.send('audio-state-update', audioState);
});

// Track operations
ipcMain.on('delete-track', (event, { trackId }) => {
  console.log(`🗑️ Delete track: ${trackId}`);
  const index = audioState.tracks.findIndex(t => t.id === trackId);
  if (index !== -1) {
    audioState.tracks.splice(index, 1);
    // Clean up associated data
    delete projectData.automation[trackId];
    delete projectData.midiNotes[trackId];
    mainWindow.webContents.send('audio-state-update', audioState);
  }
});

ipcMain.on('rename-track', (event, { trackId, name }) => {
  console.log(`✏️ Rename track: ${trackId} -> ${name}`);
  const track = audioState.tracks.find(t => t.id === trackId);
  if (track) {
    track.name = name;
    mainWindow.webContents.send('audio-state-update', audioState);
  }
});

ipcMain.on('set-track-volume', (event, { trackId, volume }) => {
  const track = audioState.tracks.find(t => t.id === trackId);
  if (track) {
    track.volume = Math.max(0, Math.min(1, volume));
    mainWindow.webContents.send('audio-state-update', audioState);
  }
});

ipcMain.on('set-track-pan', (event, { trackId, pan }) => {
  const track = audioState.tracks.find(t => t.id === trackId);
  if (track) {
    track.pan = Math.max(-1, Math.min(1, pan));
    mainWindow.webContents.send('audio-state-update', audioState);
  }
});

ipcMain.on('set-track-mute', (event, { trackId, muted }) => {
  const track = audioState.tracks.find(t => t.id === trackId);
  if (track) {
    track.muted = muted;
    mainWindow.webContents.send('audio-state-update', audioState);
  }
});

ipcMain.on('set-track-solo', (event, { trackId, solo }) => {
  const track = audioState.tracks.find(t => t.id === trackId);
  if (track) {
    track.solo = solo;
    mainWindow.webContents.send('audio-state-update', audioState);
  }
});

ipcMain.on('set-track-record-arm', (event, { trackId, armed }) => {
  const track = audioState.tracks.find(t => t.id === trackId);
  if (track) {
    track.recordArm = armed;
    mainWindow.webContents.send('audio-state-update', audioState);
  }
});

ipcMain.on('set-track-color', (event, { trackId, color }) => {
  console.log(`🎨 Set track color: ${trackId} -> ${color}`);
  const track = audioState.tracks.find(t => t.id === trackId);
  if (track) {
    track.color = color;
    mainWindow.webContents.send('audio-state-update', audioState);
  }
});

// Automation operations
ipcMain.on('add-automation', (event, { trackId, parameter, points }) => {
  console.log(`📈 Add automation: ${trackId}.${parameter}`);
  if (!projectData.automation[trackId]) {
    projectData.automation[trackId] = [];
  }
  projectData.automation[trackId].push({ parameter, points });
  mainWindow.webContents.send('project-data-update', projectData);
});

ipcMain.on('update-automation', (event, { trackId, parameter, points }) => {
  console.log(`📈 Update automation: ${trackId}.${parameter}`);
  if (projectData.automation[trackId]) {
    const lane = projectData.automation[trackId].find(l => l.parameter === parameter);
    if (lane) {
      lane.points = points;
    }
  }
  mainWindow.webContents.send('project-data-update', projectData);
});

ipcMain.on('delete-automation', (event, { trackId, parameter }) => {
  console.log(`🗑️ Delete automation: ${trackId}.${parameter}`);
  if (projectData.automation[trackId]) {
    projectData.automation[trackId] = projectData.automation[trackId].filter(
      l => l.parameter !== parameter
    );
  }
  mainWindow.webContents.send('project-data-update', projectData);
});

// Clip operations (Session View)
ipcMain.on('create-clip', (event, { trackId, sceneIndex, length }) => {
  console.log(`🎬 Create clip: track=${trackId}, scene=${sceneIndex}, length=${length}`);
  const clip = {
    id: `clip-${Date.now()}`,
    trackId,
    sceneIndex,
    length,
    startTime: 0,
    notes: [],
  };
  projectData.clips.push(clip);
  mainWindow.webContents.send('project-data-update', projectData);
});

ipcMain.on('delete-clip', (event, { clipId }) => {
  console.log(`🗑️ Delete clip: ${clipId}`);
  const index = projectData.clips.findIndex(c => c.id === clipId);
  if (index !== -1) {
    projectData.clips.splice(index, 1);
    mainWindow.webContents.send('project-data-update', projectData);
  }
});

ipcMain.on('launch-clip', (event, { clipId }) => {
  console.log(`▶️ Launch clip: ${clipId}`);
  const clip = projectData.clips.find(c => c.id === clipId);
  if (clip) {
    // Notify renderer to trigger clip playback
    mainWindow.webContents.send('clip-launched', clip);
  }
});

ipcMain.on('stop-clip', (event, { trackId }) => {
  console.log(`⏹️ Stop clips on track: ${trackId}`);
  mainWindow.webContents.send('clip-stopped', { trackId });
});

// MIDI operations
ipcMain.on('add-midi-note', (event, { trackId, pitch, time, duration, velocity }) => {
  console.log(`🎹 Add MIDI note: track=${trackId}, pitch=${pitch}`);
  if (!projectData.midiNotes[trackId]) {
    projectData.midiNotes[trackId] = [];
  }
  const note = {
    id: `note-${Date.now()}`,
    pitch,
    time,
    duration,
    velocity,
  };
  projectData.midiNotes[trackId].push(note);
  mainWindow.webContents.send('project-data-update', projectData);
});

ipcMain.on('remove-midi-note', (event, { trackId, noteId }) => {
  console.log(`🗑️ Remove MIDI note: ${noteId}`);
  if (projectData.midiNotes[trackId]) {
    projectData.midiNotes[trackId] = projectData.midiNotes[trackId].filter(
      n => n.id !== noteId
    );
    mainWindow.webContents.send('project-data-update', projectData);
  }
});

ipcMain.on('update-midi-note', (event, { trackId, noteId, pitch, time, duration, velocity }) => {
  console.log(`✏️ Update MIDI note: ${noteId}`);
  if (projectData.midiNotes[trackId]) {
    const note = projectData.midiNotes[trackId].find(n => n.id === noteId);
    if (note) {
      if (pitch !== undefined) note.pitch = pitch;
      if (time !== undefined) note.time = time;
      if (duration !== undefined) note.duration = duration;
      if (velocity !== undefined) note.velocity = velocity;
      mainWindow.webContents.send('project-data-update', projectData);
    }
  }
});

// Batch operations
ipcMain.on('batch-start', () => {
  console.log('📦 Batch start');
  batchState.active = true;
  batchState.operations = [];
  // Take snapshot of current state
  batchState.snapshot = {
    audioState: JSON.parse(JSON.stringify(audioState)),
    projectData: JSON.parse(JSON.stringify(projectData)),
  };
});

ipcMain.on('batch-commit', () => {
  console.log('✅ Batch commit');
  batchState.active = false;
  batchState.operations = [];
  batchState.snapshot = null;
  // Broadcast final state
  mainWindow.webContents.send('audio-state-update', audioState);
  mainWindow.webContents.send('project-data-update', projectData);
});

ipcMain.on('batch-rollback', () => {
  console.log('↩️ Batch rollback');
  if (batchState.snapshot) {
    // Restore from snapshot
    Object.assign(audioState, batchState.snapshot.audioState);
    Object.assign(projectData, batchState.snapshot.projectData);
    mainWindow.webContents.send('audio-state-update', audioState);
    mainWindow.webContents.send('project-data-update', projectData);
  }
  batchState.active = false;
  batchState.operations = [];
  batchState.snapshot = null;
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

// ============================================================================
// Project File I/O Handlers
// ============================================================================

/**
 * Save project to file
 */
ipcMain.handle('save-project', async (event, { filePath, data }) => {
  try {
    // If no filePath provided, show save dialog
    let targetPath = filePath;

    if (!targetPath) {
      const result = await dialog.showSaveDialog(mainWindow, {
        title: 'Save Project',
        defaultPath: path.join(app.getPath('documents'), `${projectData.metadata.name}.vxl`),
        filters: [
          { name: 'Vexel DAW Project', extensions: ['vxl'] },
          { name: 'All Files', extensions: ['*'] },
        ],
      });

      if (result.canceled) {
        return { success: false, canceled: true };
      }

      targetPath = result.filePath;
    }

    // Merge current state with provided data
    const fullProjectData = {
      version: '1.0.0',
      metadata: {
        ...projectData.metadata,
        modifiedAt: new Date().toISOString(),
        ...data?.metadata,
      },
      transport: {
        tempo: audioState.tempo,
        timeSignature: audioState.timeSignature,
      },
      tracks: audioState.tracks,
      clips: data?.clips || projectData.clips,
      scenes: data?.scenes || projectData.scenes,
      automation: data?.automation || projectData.automation,
      markers: data?.markers || projectData.markers,
    };

    // Write to file
    await fs.writeFile(targetPath, JSON.stringify(fullProjectData, null, 2), 'utf-8');

    // Update current project path
    currentProjectPath = targetPath;
    projectData.metadata.name = path.basename(targetPath, '.vxl');

    console.log(`💾 Project saved: ${targetPath}`);

    return {
      success: true,
      filePath: targetPath,
      fileName: path.basename(targetPath),
    };
  } catch (error) {
    console.error('Error saving project:', error);
    return {
      success: false,
      error: error.message,
    };
  }
});

/**
 * Load project from file
 */
ipcMain.handle('load-project', async (event, { filePath }) => {
  try {
    // If no filePath provided, show open dialog
    let targetPath = filePath;

    if (!targetPath) {
      const result = await dialog.showOpenDialog(mainWindow, {
        title: 'Load Project',
        defaultPath: app.getPath('documents'),
        filters: [
          { name: 'Vexel DAW Project', extensions: ['vxl'] },
          { name: 'All Files', extensions: ['*'] },
        ],
        properties: ['openFile'],
      });

      if (result.canceled || result.filePaths.length === 0) {
        return { success: false, canceled: true };
      }

      targetPath = result.filePaths[0];
    }

    // Check if file exists
    try {
      await fs.access(targetPath);
    } catch {
      return {
        success: false,
        error: 'File not found',
      };
    }

    // Read and parse file
    const fileContent = await fs.readFile(targetPath, 'utf-8');
    const data = JSON.parse(fileContent);

    // Validate basic structure
    if (!data.version || !data.transport || !data.tracks) {
      return {
        success: false,
        error: 'Invalid project file format',
      };
    }

    // Update state
    audioState.tempo = data.transport.tempo;
    audioState.timeSignature = data.transport.timeSignature;
    audioState.tracks = data.tracks;
    audioState.isPlaying = false;
    audioState.currentBar = 0;

    projectData.clips = data.clips || [];
    projectData.scenes = data.scenes || [];
    projectData.automation = data.automation || [];
    projectData.markers = data.markers || [];
    projectData.metadata = data.metadata || {
      name: path.basename(targetPath, '.vxl'),
      createdAt: new Date().toISOString(),
      modifiedAt: new Date().toISOString(),
    };

    // Update current project path
    currentProjectPath = targetPath;

    // Broadcast updated state to renderer
    mainWindow.webContents.send('audio-state-update', audioState);
    mainWindow.webContents.send('project-data-update', projectData);

    console.log(`📂 Project loaded: ${targetPath}`);

    return {
      success: true,
      filePath: targetPath,
      fileName: path.basename(targetPath),
      data: {
        audioState,
        ...projectData,
      },
    };
  } catch (error) {
    console.error('Error loading project:', error);
    return {
      success: false,
      error: error.message,
    };
  }
});

/**
 * Create new project
 */
ipcMain.handle('new-project', async () => {
  try {
    // Reset to default state
    audioState.tempo = 120;
    audioState.timeSignature = { numerator: 4, denominator: 4 };
    audioState.isPlaying = false;
    audioState.currentBar = 0;
    audioState.tracks = [];

    projectData.clips = [];
    projectData.scenes = [];
    projectData.automation = [];
    projectData.markers = [];
    projectData.metadata = {
      name: 'Untitled Project',
      createdAt: new Date().toISOString(),
      modifiedAt: new Date().toISOString(),
    };

    currentProjectPath = null;

    // Broadcast updated state to renderer
    mainWindow.webContents.send('audio-state-update', audioState);
    mainWindow.webContents.send('project-data-update', projectData);

    console.log('📄 New project created');

    return {
      success: true,
      data: {
        audioState,
        ...projectData,
      },
    };
  } catch (error) {
    console.error('Error creating new project:', error);
    return {
      success: false,
      error: error.message,
    };
  }
});

/**
 * Get current project info
 */
ipcMain.handle('get-project-info', () => {
  return {
    currentPath: currentProjectPath,
    metadata: projectData.metadata,
  };
});

// Cleanup on app quit
app.on('before-quit', () => {
  if (wingmanBridge) {
    console.log('🤖 Cleaning up Wingman AI Bridge...');
    wingmanBridge.disconnect();
  }
});
