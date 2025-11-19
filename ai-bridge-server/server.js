/**
 * AI Bridge Server - Example Implementation
 *
 * WebSocket server for communication between Zenith DAW and external AI services.
 * This is a reference implementation showing how to build an AI Bridge server.
 *
 * Usage:
 *   npm install ws
 *   node server.js
 *
 * The server will listen on ws://localhost:8765
 */

const WebSocket = require('ws');

// Configuration
const PORT = 8765;
const HOST = 'localhost';

// Message types
const MessageType = {
  GENERATE_MELODY: 'generate_melody',
  GENERATE_CHORDS: 'generate_chords',
  GENERATE_DRUMS: 'generate_drums',
  GENERATE_BASS: 'generate_bass',
  PLAY: 'play',
  STOP: 'stop',
  PAUSE: 'pause',
  SET_TEMPO: 'set_tempo',
  SET_KEY: 'set_key',
  GET_STATUS: 'get_status',
  GET_CAPABILITIES: 'get_capabilities',
  SUCCESS: 'success',
  ERROR: 'error',
  STATUS: 'status',
  CAPABILITIES: 'capabilities',
  GENERATION_COMPLETE: 'generation_complete',
};

// Server state
let serverState = {
  isPlaying: false,
  tempo: 120,
  position: 0,
  key: 'C',
  scale: 'major',
  activeModels: ['melody', 'chords', 'drums'],
};

// Create WebSocket server
const wss = new WebSocket.Server({ port: PORT, host: HOST });

console.log(`🚀 AI Bridge Server started on ws://${HOST}:${PORT}`);
console.log('📡 Waiting for connections...\n');

// Handle connections
wss.on('connection', (ws) => {
  console.log('✅ Client connected');

  // Send welcome message
  ws.send(JSON.stringify({
    type: 'info',
    message: 'Connected to AI Bridge Server',
    timestamp: Date.now(),
  }));

  // Handle messages
  ws.on('message', async (data) => {
    try {
      const message = JSON.parse(data.toString());
      console.log(`📨 Received: ${message.type} (ID: ${message.id})`);

      // Route message to handler
      const response = await handleMessage(message);

      // Send response
      ws.send(JSON.stringify(response));
      console.log(`📤 Sent: ${response.type} (Request ID: ${response.requestId})\n`);

    } catch (error) {
      console.error('❌ Error handling message:', error);

      // Send error response
      ws.send(JSON.stringify({
        type: MessageType.ERROR,
        requestId: 'unknown',
        timestamp: Date.now(),
        payload: {
          code: 'PARSE_ERROR',
          message: error.message,
        },
      }));
    }
  });

  ws.on('close', () => {
    console.log('🔌 Client disconnected\n');
  });

  ws.on('error', (error) => {
    console.error('❌ WebSocket error:', error);
  });
});

// Handle server errors
wss.on('error', (error) => {
  console.error('❌ Server error:', error);
});

/**
 * Route and handle messages
 */
async function handleMessage(message) {
  const { id, type, payload } = message;

  switch (type) {
    case MessageType.GENERATE_MELODY:
      return await handleGenerateMelody(id, payload);

    case MessageType.GENERATE_CHORDS:
      return await handleGenerateChords(id, payload);

    case MessageType.GENERATE_DRUMS:
      return await handleGenerateDrums(id, payload);

    case MessageType.GENERATE_BASS:
      return await handleGenerateBass(id, payload);

    case MessageType.PLAY:
      return handlePlay(id);

    case MessageType.STOP:
      return handleStop(id);

    case MessageType.PAUSE:
      return handlePause(id);

    case MessageType.SET_TEMPO:
      return handleSetTempo(id, payload);

    case MessageType.SET_KEY:
      return handleSetKey(id, payload);

    case MessageType.GET_STATUS:
      return handleGetStatus(id);

    case MessageType.GET_CAPABILITIES:
      return handleGetCapabilities(id);

    default:
      return {
        type: MessageType.ERROR,
        requestId: id,
        timestamp: Date.now(),
        payload: {
          code: 'UNKNOWN_MESSAGE_TYPE',
          message: `Unknown message type: ${type}`,
        },
      };
  }
}

/**
 * Generate melody
 */
async function handleGenerateMelody(requestId, payload) {
  console.log('🎼 Generating melody...');

  // Simulate AI processing time
  await sleep(1000);

  // Generate sample melody (C major scale)
  const notes = [];
  const scale = [60, 62, 64, 65, 67, 69, 71, 72]; // C major
  const steps = payload?.steps || 16;

  let currentTime = 0;
  for (let i = 0; i < steps; i++) {
    const pitch = scale[Math.floor(Math.random() * scale.length)];
    const duration = Math.random() > 0.5 ? 0.5 : 0.25;

    notes.push({
      pitch,
      velocity: 70 + Math.floor(Math.random() * 30),
      startTime: currentTime,
      duration,
    });

    currentTime += duration;
  }

  return {
    type: MessageType.GENERATION_COMPLETE,
    requestId,
    timestamp: Date.now(),
    payload: {
      notes,
      metadata: {
        key: payload?.key || 'C',
        scale: payload?.scale || 'major',
        steps,
      },
    },
  };
}

/**
 * Generate chords
 */
async function handleGenerateChords(requestId, payload) {
  console.log('🎹 Generating chords...');

  await sleep(800);

  const key = payload?.key || 'C';
  const chords = ['C', 'Am', 'F', 'G'];
  const notes = [];

  // Generate chord voicings
  chords.forEach((chord, index) => {
    const startTime = index * 4;
    const root = 60; // C

    notes.push(
      { pitch: root, velocity: 80, startTime, duration: 4 },
      { pitch: root + 4, velocity: 75, startTime, duration: 4 },
      { pitch: root + 7, velocity: 75, startTime, duration: 4 }
    );
  });

  return {
    type: MessageType.GENERATION_COMPLETE,
    requestId,
    timestamp: Date.now(),
    payload: {
      notes,
      metadata: {
        key,
        chords,
        progression: 'I-vi-IV-V',
      },
    },
  };
}

/**
 * Generate drums
 */
async function handleGenerateDrums(requestId, payload) {
  console.log('🥁 Generating drums...');

  await sleep(1200);

  const style = payload?.style || 'trap';
  const bars = payload?.bars || 4;
  const pattern = [];

  // Simple drum pattern
  const kick = 36;
  const snare = 38;
  const hihat = 42;

  for (let bar = 0; bar < bars; bar++) {
    for (let step = 0; step < 16; step++) {
      const time = bar * 4 + (step / 16) * 4;

      // Kick on 1 and 3
      if (step === 0 || step === 8) {
        pattern.push({ pitch: kick, velocity: 100, startTime: time, duration: 0.25 });
      }

      // Snare on 2 and 4
      if (step === 4 || step === 12) {
        pattern.push({ pitch: snare, velocity: 90, startTime: time, duration: 0.25 });
      }

      // Hi-hat pattern
      if (step % 2 === 0) {
        pattern.push({ pitch: hihat, velocity: 70, startTime: time, duration: 0.125 });
      }
    }
  }

  return {
    type: MessageType.GENERATION_COMPLETE,
    requestId,
    timestamp: Date.now(),
    payload: {
      notes: pattern,
      metadata: {
        style,
        bars,
      },
    },
  };
}

/**
 * Generate bass
 */
async function handleGenerateBass(requestId, payload) {
  console.log('🎸 Generating bass...');

  await sleep(900);

  const notes = [];
  const bassNotes = [36, 38, 40, 41, 43]; // Bass range
  const steps = 16;

  let currentTime = 0;
  for (let i = 0; i < steps; i++) {
    const pitch = bassNotes[Math.floor(Math.random() * bassNotes.length)];
    const duration = 0.5;

    notes.push({
      pitch,
      velocity: 90,
      startTime: currentTime,
      duration,
    });

    currentTime += duration;
  }

  return {
    type: MessageType.GENERATION_COMPLETE,
    requestId,
    timestamp: Date.now(),
    payload: {
      notes,
      metadata: {
        key: payload?.key || 'C',
      },
    },
  };
}

/**
 * Control handlers
 */
function handlePlay(requestId) {
  serverState.isPlaying = true;
  console.log('▶️  Playing...');

  return {
    type: MessageType.SUCCESS,
    requestId,
    timestamp: Date.now(),
    payload: { action: 'play', state: serverState.isPlaying },
  };
}

function handleStop(requestId) {
  serverState.isPlaying = false;
  serverState.position = 0;
  console.log('⏹️  Stopped');

  return {
    type: MessageType.SUCCESS,
    requestId,
    timestamp: Date.now(),
    payload: { action: 'stop', state: serverState.isPlaying },
  };
}

function handlePause(requestId) {
  serverState.isPlaying = false;
  console.log('⏸️  Paused');

  return {
    type: MessageType.SUCCESS,
    requestId,
    timestamp: Date.now(),
    payload: { action: 'pause', state: serverState.isPlaying },
  };
}

function handleSetTempo(requestId, payload) {
  serverState.tempo = payload.tempo;
  console.log(`🎵 Tempo set to ${serverState.tempo} BPM`);

  return {
    type: MessageType.SUCCESS,
    requestId,
    timestamp: Date.now(),
    payload: { tempo: serverState.tempo },
  };
}

function handleSetKey(requestId, payload) {
  serverState.key = payload.key;
  serverState.scale = payload.scale || 'major';
  console.log(`🎼 Key set to ${serverState.key} ${serverState.scale}`);

  return {
    type: MessageType.SUCCESS,
    requestId,
    timestamp: Date.now(),
    payload: { key: serverState.key, scale: serverState.scale },
  };
}

function handleGetStatus(requestId) {
  return {
    type: MessageType.STATUS,
    requestId,
    timestamp: Date.now(),
    payload: {
      isPlaying: serverState.isPlaying,
      tempo: serverState.tempo,
      position: serverState.position,
      key: serverState.key,
      activeModels: serverState.activeModels,
    },
  };
}

function handleGetCapabilities(requestId) {
  return {
    type: MessageType.CAPABILITIES,
    requestId,
    timestamp: Date.now(),
    payload: {
      models: {
        melody: true,
        chords: true,
        drums: true,
        bass: true,
      },
      maxSequenceLength: 512,
      supportedScales: ['major', 'minor', 'pentatonic', 'blues'],
      supportedDrumStyles: ['trap', 'house', 'techno', 'rock'],
    },
  };
}

// Helper
function sleep(ms) {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

// Graceful shutdown
process.on('SIGINT', () => {
  console.log('\n👋 Shutting down AI Bridge Server...');
  wss.close(() => {
    console.log('✅ Server closed');
    process.exit(0);
  });
});
