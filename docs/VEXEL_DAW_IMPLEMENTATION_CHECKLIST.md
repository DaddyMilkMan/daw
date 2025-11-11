# Vexel-DAW React UI Implementation Checklist

**Target:** Connect to JUCE Engine via WebSocket
**Date:** 2025-11-11
**Status:** 95% Complete → Final Integration

---

## Overview

The React UI in `vexel-daw/` is **95% complete** and already implements the Perfect DAW UI Design. This checklist focuses on:
1. Connecting to the JUCE engine via WebSocket
2. Adapting to embedded CEF environment
3. Minor UI adjustments for the hybrid architecture

---

## Phase 1: WebSocket Integration (Weeks 1-2)

### 1.1 Update engineClient.ts for JUCE Connection

**File to Modify:**
- [ ] `vexel-daw/src/renderer/lib/engineClient.ts`

**Current State:**
```typescript
// Placeholder implementation
class EngineClient {
    sendCommand(command: string, params: any): Promise<any> {
        // TODO: Implement WebSocket connection
    }
}
```

**Target State:**
```typescript
class EngineClient {
    private ws: WebSocket;
    private commandId = 0;
    private pendingCommands = new Map<string, CommandCallback>();

    constructor() {
        this.connect();
    }

    private connect() {
        this.ws = new WebSocket('ws://127.0.0.1:9001');
        this.ws.onopen = () => console.log('Connected to JUCE engine');
        this.ws.onmessage = (event) => this.handleMessage(event.data);
        this.ws.onerror = (error) => console.error('WebSocket error:', error);
        this.ws.onclose = () => this.reconnect();
    }

    sendCommand(command: string, params: any): Promise<any> {
        const id = `cmd-${this.commandId++}`;
        const message = JSON.stringify({ id, command, params });
        this.ws.send(message);

        return new Promise((resolve, reject) => {
            this.pendingCommands.set(id, { resolve, reject, timeout: setTimeout(() => {
                reject(new Error('Command timeout'));
                this.pendingCommands.delete(id);
            }, 5000) });
        });
    }

    private handleMessage(data: string) {
        const message = JSON.parse(data);

        if (message.id && this.pendingCommands.has(message.id)) {
            // Command response
            const { resolve, timeout } = this.pendingCommands.get(message.id)!;
            clearTimeout(timeout);
            resolve(message.payload || message);
            this.pendingCommands.delete(message.id);
        } else if (message.event) {
            // Event from engine
            this.handleEvent(message.event, message.payload);
        }
    }

    private handleEvent(eventType: string, payload: any) {
        switch (eventType) {
            case 'playhead.update':
                useTransportStore.getState().updatePlayhead(payload);
                break;
            case 'meters.update':
                useMixerStore.getState().updateMeters(payload);
                break;
            case 'track.created':
                useProjectStore.getState().addTrack(payload);
                break;
            case 'clip.created':
                useProjectStore.getState().addClip(payload);
                break;
            // ... more event handlers
        }
    }

    private reconnect() {
        console.warn('Disconnected from JUCE engine, reconnecting...');
        setTimeout(() => this.connect(), 1000);
    }
}

export const engineClient = new EngineClient();
```

### 1.2 Implement Command Methods

**Commands to Implement:**
- [ ] `transport.play()`
- [ ] `transport.stop()`
- [ ] `transport.record()`
- [ ] `transport.setTempo(bpm: number)`
- [ ] `transport.setTimeSignature(numerator: number, denominator: number)`
- [ ] `track.create(name: string, type: 'audio' | 'midi', numChannels: number)`
- [ ] `track.delete(trackId: string)`
- [ ] `track.setVolume(trackId: string, volume: number)`
- [ ] `track.setPan(trackId: string, pan: number)`
- [ ] `track.setMute(trackId: string, mute: boolean)`
- [ ] `track.setSolo(trackId: string, solo: boolean)`
- [ ] `clip.create(trackId: string, startTime: number, length: number)`
- [ ] `clip.delete(clipId: string)`
- [ ] `clip.move(clipId: string, startTime: number)`
- [ ] `clip.resize(clipId: string, length: number)`
- [ ] `plugin.load(trackId: string, pluginId: string)`
- [ ] `plugin.remove(trackId: string, slotIndex: number)`
- [ ] `plugin.setParameter(pluginId: string, paramId: string, value: number)`
- [ ] `project.save(path: string)`
- [ ] `project.load(path: string)`

**Example Implementation:**
```typescript
class EngineClient {
    // ... existing code ...

    async transportPlay() {
        return this.sendCommand('transport.play', {});
    }

    async transportStop() {
        return this.sendCommand('transport.stop', {});
    }

    async createTrack(name: string, type: 'audio' | 'midi', numChannels: number = 2) {
        return this.sendCommand('track.create', { name, type, numChannels });
    }

    async setTrackVolume(trackId: string, volume: number) {
        return this.sendCommand('track.set_volume', { trackId, volume });
    }

    // ... more methods
}
```

### 1.3 Update React Components to Use Engine Client

**Files to Modify:**

**TransportBar.tsx:**
- [ ] Replace local state with `engineClient.transportPlay()`, `.transportStop()`, etc.
- [ ] Update BPM control to call `engineClient.setTempo()`

```typescript
// Before
const handlePlay = () => {
    setIsPlaying(true);
};

// After
const handlePlay = async () => {
    try {
        await engineClient.transportPlay();
        // State will update via 'transport.state_changed' event
    } catch (error) {
        showNotification('Failed to start playback', 'error');
    }
};
```

**LeftPanel.tsx / Browser:**
- [ ] Drag sample → calls `engineClient.importAudio(path, trackId)`
- [ ] Drag plugin → calls `engineClient.loadPlugin(trackId, pluginId)`

**CenterPanel.tsx / ArrangementView:**
- [ ] Create clip → calls `engineClient.createClip()`
- [ ] Move clip → calls `engineClient.moveClip()`
- [ ] Resize clip → calls `engineClient.resizeClip()`

**RightPanel.tsx / Mixer:**
- [ ] Fader change → calls `engineClient.setTrackVolume()`
- [ ] Pan change → calls `engineClient.setTrackPan()`
- [ ] Mute/Solo → calls `engineClient.setTrackMute/Solo()`

**PianoRoll.tsx:**
- [ ] Add MIDI note → calls `engineClient.addMidiNote()`
- [ ] Delete MIDI note → calls `engineClient.deleteMidiNote()`
- [ ] Update velocity → calls `engineClient.updateMidiNoteVelocity()`

### 1.4 Update Zustand Stores for Engine Events

**Files to Modify:**

**transportStore.ts:**
- [ ] Add `updatePlayhead(payload)` method
- [ ] Listen for `playhead.update` events

```typescript
interface TransportState {
    isPlaying: boolean;
    isRecording: boolean;
    tempo: number;
    timeSignature: { numerator: number; denominator: number };
    playheadPosition: number;
    playheadBar: number;
    playheadBeat: number;

    updatePlayhead: (payload: PlayheadUpdate) => void;
}

export const useTransportStore = create<TransportState>((set) => ({
    // ... existing state ...
    updatePlayhead: (payload) => set({
        playheadPosition: payload.position,
        playheadBar: payload.bar,
        playheadBeat: payload.beat,
    }),
}));
```

**mixerStore.ts:**
- [ ] Add `updateMeters(payload)` method
- [ ] Listen for `meters.update` events

**projectStore.ts:**
- [ ] Add `addTrack(payload)` method
- [ ] Add `removeTrack(payload)` method
- [ ] Add `updateTrack(payload)` method
- [ ] Add `addClip(payload)` method
- [ ] Listen for corresponding events

### 1.5 Testing

- [ ] Verify WebSocket connection establishes on app start
- [ ] Click Play → verify JUCE engine starts playback
- [ ] Adjust fader → verify JUCE engine volume changes
- [ ] Create track → verify track appears in UI and engine
- [ ] Verify playhead updates at 30-60 Hz

---

## Phase 2: CEF Environment Adaptation (Weeks 3-4)

### 2.1 Detect CEF vs Electron Environment

**File to Create:**
- [ ] `vexel-daw/src/renderer/lib/environment.ts`

```typescript
export const isCEF = () => {
    return typeof (window as any).chrome !== 'undefined' &&
           typeof (window as any).chrome.cef !== 'undefined';
};

export const isElectron = () => {
    return typeof (window as any).electron !== 'undefined';
};

export const isStandalone = () => {
    return !isCEF() && !isElectron();
};
```

### 2.2 Disable Electron-Specific Features When Running in CEF

**Files to Modify:**
- [ ] `vexel-daw/src/renderer/App.tsx` - Conditionally disable Electron IPC
- [ ] `vexel-daw/src/renderer/components/TransportBar.tsx` - Use WebSocket instead of Electron IPC
- [ ] `vexel-daw/src/main/main.ts` - Skip if running in CEF (no Electron main process)

**Example:**
```typescript
// Before
import { ipcRenderer } from 'electron';

const saveProject = () => {
    ipcRenderer.send('save-project', projectData);
};

// After
import { engineClient } from '@/lib/engineClient';
import { isElectron, isCEF } from '@/lib/environment';

const saveProject = async () => {
    if (isCEF()) {
        await engineClient.saveProject(projectPath);
    } else if (isElectron()) {
        ipcRenderer.send('save-project', projectData);
    } else {
        showNotification('Save not available in web mode', 'warning');
    }
};
```

### 2.3 Update File Dialogs

**Files to Modify:**
- [ ] `vexel-daw/src/renderer/components/TransportBar.tsx` - File > Save/Load

**Changes:**
- When running in CEF, call JUCE engine for file dialogs
- JUCE returns selected path via WebSocket event

```typescript
const openProjectDialog = async () => {
    if (isCEF()) {
        const path = await engineClient.showOpenDialog({ filters: [{ name: 'Zenith Projects', extensions: ['zen'] }] });
        if (path) {
            await engineClient.loadProject(path);
        }
    } else {
        // Use Electron dialog
    }
};
```

### 2.4 Handle Window Resize

**Files to Modify:**
- [ ] `vexel-daw/src/renderer/App.tsx` - Listen for window resize events

**Changes:**
- CEF may send custom events when JUCE window resizes
- Adjust layout accordingly

### 2.5 Testing

- [ ] Verify UI renders correctly in CEF
- [ ] File dialogs work (JUCE native dialogs)
- [ ] Window resize updates layout
- [ ] All WebSocket commands work in CEF environment

---

## Phase 3: UI Enhancements (Weeks 5-6)

### 3.1 Implement Missing Perfect DAW Features

**Features Already Implemented:**
- ✅ Transport bar with BPM, time signature, CPU/RAM meters
- ✅ Left panel browser with search, favorites, drag-and-drop
- ✅ Session View and Arrangement View toggle
- ✅ Piano Roll with scale highlighting, velocity editing
- ✅ Right panel Mixer and Inspector
- ✅ Wingman AI sidebar
- ✅ Automation lanes
- ✅ Waveform rendering
- ✅ Keyboard shortcuts
- ✅ Dark theme
- ✅ Smooth animations (Framer Motion)

**Minor Enhancements Needed:**

**Command Palette (Cmd+K):**
- [ ] Already partially implemented in `vexel-daw`?
- [ ] If not, add command palette component
- [ ] Integrate with engine commands

**Grid View (Bitwig-style routing):**
- [ ] Not yet implemented
- [ ] Lower priority (optional advanced feature)

**Comp Lanes in Audio Editor:**
- [ ] Check if `AudioClipComponent.tsx` supports multiple takes
- [ ] If not, add comp lane UI

### 3.2 Wingman AI Integration with Engine

**Files to Modify:**
- [ ] `vexel-daw/src/renderer/components/WingmanSidebar.tsx`
- [ ] `vexel-daw/src/renderer/services/wingman.ts` (if exists)

**Changes:**
- When Wingman generates audio/MIDI, send to JUCE engine
- Engine creates clips and returns IDs
- Update UI with new clips

```typescript
const handleWingmanCommand = async (command: string) => {
    if (command.includes('create trap beat')) {
        const result = await engineClient.sendCommand('ai.generate_midi', {
            prompt: command,
            trackId: selectedTrackId,
        });

        showNotification('Trap beat created!', 'success');
    }
};
```

### 3.3 Plugin UI Integration

**Challenge:** VST3/AU plugin UIs are native windows managed by JUCE

**Solution:**
- When user opens plugin UI, send command to JUCE
- JUCE creates native window (outside React/CEF)
- React shows placeholder or "Plugin UI open in separate window" message

**Files to Modify:**
- [ ] `vexel-daw/src/renderer/components/RightPanel.tsx` (Mixer → Plugin slot)

```typescript
const openPluginUI = async (pluginId: string) => {
    await engineClient.openPluginUI(pluginId);
    showNotification('Plugin UI opened in separate window', 'info');
};
```

### 3.4 Testing

- [ ] Verify all Perfect DAW features work
- [ ] Wingman generates audio and creates clips
- [ ] Plugin UIs open correctly
- [ ] All keyboard shortcuts work

---

## Phase 4: Performance & Polish (Weeks 7-8)

### 4.1 Optimize WebSocket Message Frequency

**Files to Modify:**
- [ ] `vexel-daw/src/renderer/lib/engineClient.ts`

**Changes:**
- Throttle meter updates to 30 Hz (currently may be 60 Hz)
- Batch parameter changes when dragging faders

```typescript
// Throttle meter updates
private lastMeterUpdate = 0;
private handleMeterUpdate(payload: any) {
    const now = Date.now();
    if (now - this.lastMeterUpdate < 33) return; // 30 Hz
    this.lastMeterUpdate = now;
    useMixerStore.getState().updateMeters(payload);
}
```

### 4.2 Add Loading States

**Files to Modify:**
- [ ] All components that call `engineClient` methods

**Changes:**
- Show loading spinner when waiting for engine response
- Disable controls during operations

```typescript
const [isLoading, setIsLoading] = useState(false);

const handlePlay = async () => {
    setIsLoading(true);
    try {
        await engineClient.transportPlay();
    } finally {
        setIsLoading(false);
    }
};
```

### 4.3 Error Handling

**Files to Modify:**
- [ ] `vexel-daw/src/renderer/lib/engineClient.ts`

**Changes:**
- Show user-friendly error messages
- Log errors to console
- Handle WebSocket disconnection gracefully

```typescript
private handleError(error: any) {
    console.error('Engine error:', error);
    showNotification(`Error: ${error.message}`, 'error');
}
```

### 4.4 Add Undo/Redo UI Indicators

**Files to Modify:**
- [ ] `vexel-daw/src/renderer/components/TransportBar.tsx` (or toolbar)

**Changes:**
- Show undo/redo buttons
- Call `engineClient.undo()` / `engineClient.redo()`
- Disable when no undo/redo available

### 4.5 Testing

- [ ] Test with 100 tracks, 1000 clips
- [ ] Verify no UI lag when dragging faders
- [ ] Error messages display correctly
- [ ] Undo/redo works for all operations

---

## Phase 5: Standalone Electron Mode (Optional)

**If you want to keep Electron as an option:**

### 5.1 Dual-Mode Support

**Files to Modify:**
- [ ] `vexel-daw/src/renderer/lib/engineClient.ts`

**Changes:**
- Detect environment (CEF vs Electron vs Web)
- Use WebSocket for CEF/Web, use Electron IPC for Electron mode

```typescript
class EngineClient {
    private mode: 'cef' | 'electron' | 'web';

    constructor() {
        this.mode = this.detectMode();
        if (this.mode === 'cef' || this.mode === 'web') {
            this.connectWebSocket();
        } else {
            this.connectElectron();
        }
    }

    private detectMode(): 'cef' | 'electron' | 'web' {
        if (isCEF()) return 'cef';
        if (isElectron()) return 'electron';
        return 'web';
    }
}
```

---

## Summary Checklist

### Must-Have for Phase 1 (Weeks 1-2)
- [ ] `engineClient.ts` WebSocket connection working
- [ ] All transport controls call engine commands
- [ ] Playhead updates received from engine
- [ ] Meter updates received from engine
- [ ] Track creation/deletion working

### Must-Have for Phase 2 (Weeks 3-4)
- [ ] UI runs correctly in CEF environment
- [ ] File dialogs use JUCE native dialogs
- [ ] Window resize handled correctly
- [ ] All Electron-specific code conditionally disabled

### Must-Have for Phase 3 (Weeks 5-6)
- [ ] Wingman AI sends commands to engine
- [ ] Plugin UIs open in native windows
- [ ] All Perfect DAW features functional

### Must-Have for Phase 4 (Weeks 7-8)
- [ ] Performance optimized (30 Hz updates, batching)
- [ ] Loading states for all operations
- [ ] Error handling complete
- [ ] Undo/redo UI

### Optional (Future)
- [ ] Dual-mode support (CEF + Electron)
- [ ] Web-only mode (demo/preview)

---

## Component Modification Summary

| Component | Changes Needed | Priority |
|-----------|---------------|----------|
| **engineClient.ts** | Implement full WebSocket protocol | 🔴 Critical |
| **TransportBar.tsx** | Call engine commands instead of local state | 🔴 Critical |
| **LeftPanel.tsx** | Integrate drag-and-drop with engine | 🟡 High |
| **CenterPanel.tsx** | Call engine for clip operations | 🔴 Critical |
| **RightPanel.tsx** | Call engine for mixer operations | 🔴 Critical |
| **PianoRoll.tsx** | Send MIDI edits to engine | 🟡 High |
| **WingmanSidebar.tsx** | Send AI commands to engine | 🟡 High |
| **AutomationLane.tsx** | Send automation data to engine | 🟢 Medium |
| **WaveformCanvas.tsx** | Receive audio data from engine | 🟢 Medium |
| **PluginManager.tsx** | Get plugin list from engine | 🟡 High |
| **App.tsx** | Detect CEF environment, conditionally load features | 🔴 Critical |

---

## Testing Plan

### Unit Tests
- [ ] `engineClient.ts` - WebSocket connection, message parsing, command sending
- [ ] Store updates from engine events
- [ ] Error handling and reconnection logic

### Integration Tests
- [ ] Full workflow: Create track → Add clip → Play
- [ ] Plugin loading and parameter automation
- [ ] Project save/load
- [ ] Wingman AI command execution

### End-to-End Tests
- [ ] Complete music production workflow
- [ ] Multi-track recording session
- [ ] Mixing with plugins and automation
- [ ] Export final mixdown

---

## Additional Notes

### Dependencies Already in Place
- ✅ React 18.3
- ✅ Tailwind CSS 3.4
- ✅ Framer Motion 11
- ✅ Zustand 5.0
- ✅ Radix UI components
- ✅ Lucide icons

### No New Dependencies Needed
- WebSocket is native browser API
- All other dependencies already installed

### Build Process
- **Dev Mode:** `npm run dev` (runs Vite dev server on :3000)
- **Production Build:** `npm run build` (static files to `dist/`)
- **CEF Integration:** JUCE serves from `dist/` or uses custom scheme

---

**Document Version:** 1.0
**Last Updated:** 2025-11-11
**Next Review:** After Phase 1 completion
