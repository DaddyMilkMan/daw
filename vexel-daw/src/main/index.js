const { app, BrowserWindow, ipcMain, dialog } = require('electron');
const path = require('path');
const fs = require('fs').promises;

let mainWindow;
let currentProjectPath = null; // Track the current project file path

// Extended audio engine state with full DAW features
const audioState = {
  tempo: 120,
  timeSignature: { numerator: 4, denominator: 4 },
  isPlaying: false,
  currentBar: 0,
  tracks: [],
};

// Extended project data
const projectData = {
  clips: [],
  scenes: [],
  automation: [],
  markers: [],
  metadata: {
    name: 'Untitled Project',
    createdAt: new Date().toISOString(),
    modifiedAt: new Date().toISOString(),
  },
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

// Project File I/O Handlers

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
