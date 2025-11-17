# ⚠️ LEGACY: UI Freeze Documentation (Electron/React Prototype)

---

## ⚠️ LEGACY DOCUMENT NOTICE

**This document describes an OBSOLETE Electron/React prototype that was never shipped.**

**Current Reality:** Zenith DAW is a **100% native JUCE 8 C++ application**. No Electron, no React, no web UI exists.

**What This Doc Describes:**
- Early Electron/React UI prototype for experimentation
- Mock audio engine (no real audio processing)
- UI state management via IPC to Electron main process

**Why This Exists:**
- Historical record of early prototyping phase
- Documents lessons learned before migrating to JUCE
- Reference for understanding architectural evolution

**See instead:**
- `/zenith-core/` for the current JUCE-native implementation
- Project root `README.md` for actual architecture
- `docs/tech-briefs/01-juce-framework-guide.md` for JUCE patterns

---

## Original Overview (Legacy Prototype)

**Date**: 2025-11-10
**Purpose**: Documented the Electron/React UI prototype state before considering migration paths.

This document captured the complete state of an early Electron-based DAW UI prototype. **This prototype was superseded by the JUCE-native implementation.**

---

## Architecture Summary

### Current State Management (Before Freeze)

- **No centralized state library** (no Redux, Context, Zustand)
- **Props drilling** for shared state (audioState, tracks)
- **Component-level useState** for local state
- **Direct IPC calls** via `window.electron.*` methods
- **Mock engine** in main process (`src/main/index.js`)

### Files

| Component | Path | Lines | Purpose |
|-----------|------|-------|---------|
| **Main App** | `src/renderer/App.tsx` | 219 | Root component, manages audioState |
| **Transport** | `src/renderer/components/TransportBar.tsx` | 333 | Playback controls, tempo, metronome |
| **Left Panel** | `src/renderer/components/LeftPanel.tsx` | - | Browser with samples/presets |
| **Center Panel** | `src/renderer/components/CenterPanel.tsx` | - | Arrangement & Session views |
| **Right Panel** | `src/renderer/components/RightPanel.tsx` | - | Mixer & Inspector |
| **Wingman** | `src/renderer/components/WingmanSidebar.tsx` | 422 | AI assistant (currently used) |
| **Piano Roll** | `src/renderer/components/PianoRoll.tsx` | - | MIDI note editor |
| **Automation** | `src/renderer/components/AutomationLane.tsx` | 236 | Per-track automation editing |
| **Timeline** | `src/renderer/components/TimelineRuler.tsx` | - | Timeline ruler with markers |

**Total UI Code**: ~3,647 lines across all TSX files

---

## UI Panels & Components

### 1. Transport Bar (Top)

**Location**: `TransportBar.tsx`

**Buttons & Controls**:
- ⏮️ **Previous** - Go to previous marker/section
- ⏯️ **Play/Pause** - Toggle playback (`isPlaying` state)
- ⏹️ **Stop** - Stop and reset playhead
- ⏭️ **Next** - Go to next marker/section
- ⏺️ **Record** - Enable recording mode
- 🔁 **Loop** - Toggle loop mode (`isLooping` local state)
- 🎵 **Metronome** - Toggle click track (`isMetronomeOn` local state)
- 👆 **Tap Tempo** - Calculate tempo from taps (`tapTimes` array)
- 🎛️ **Tempo Input** - BPM value (20-999 range)
- 🎹 **Time Signature** - Display only (4/4 hardcoded)
- 📊 **CPU Meter** - Mock usage display (random 5-15%)
- ⚙️ **Record Arm Indicator** - Shows armed track count

**State Mutations**:
- `window.electron.transportPlay()` → Sets `isPlaying: true` in engine
- `window.electron.transportPause()` → Sets `isPlaying: false`
- `window.electron.transportStop()` → Resets playback
- `window.electron.setTempo(tempo)` → Updates BPM globally

**Local State** (not synced with engine):
- `isLooping: boolean`
- `isMetronomeOn: boolean`
- `tapTimes: number[]`
- `cpuUsage: number` (mock random value)

---

### 2. Left Panel - Browser

**Location**: `LeftPanel.tsx`

**Features**:
- **Search bar** - Filter by name/tags
- **Category tabs** - Samples, Instruments, Effects, Loops, MIDI, Projects, Collections
- **Tag filters** - Multi-select genre/mood tags
- **View modes** - List, Grid, Tree
- **Sort options** - Name, Date, Type, Rating, Size
- **Preview system** - Play samples before adding (mock only)
- **Favorites** - Toggle favorite flag on items
- **Collections** - User-created item groups
- **Context menu** - Add to project, Add to collection, Add to favorites, Show in folder

**Data Source**:
- `SAMPLE_BROWSER_ITEMS` constant (171 hardcoded items)
- Local state: `items`, `collections`, `searchQuery`, `selectedTags`

**No Engine Integration**: Browser data is entirely UI-side, no IPC calls

---

### 3. Center Panel - Arrangement View

**Location**: `CenterPanel.tsx` (Arrangement mode)

**Features**:
- **Track lanes** - Horizontal tracks with clips
- **Add Track button** - Creates MIDI/Audio/Instrument tracks
- **Track controls per lane**:
  - 🔴 **Record Arm** - Toggle record-ready state (`recordArmed` Set)
  - 📝 **Rename** - Inline track name editing
  - 🔇 **Mute** - Silence track
  - 🔊 **Solo** - Solo track
  - 📈 **Automation** - Toggle automation lane visibility
  - 🗑️ **Delete** - Remove track
- **Automation lanes** - Expandable per-track automation envelopes
  - Add automation button
  - Parameter selection (Volume, Pan, Filter, etc.)
  - Breakpoint editing (not yet implemented)
- **Timeline ruler** - Bar/beat grid
- **Context menu** - Track operations

**State Mutations**:
- `window.electron.createTrack(name, type)` → Adds track to engine
- Local state: `recordArmed` Set, `expandedAutomation` Set, `automationLanes` Map
- Track name changes are **local only** (not sent to engine)

---

### 4. Center Panel - Session View

**Location**: `CenterPanel.tsx` (Session mode)

**Features**:
- **Clip grid** - Scenes (rows) × Tracks (columns)
- **Launch clips** - Click to trigger
- **Scene launch** - Launch entire horizontal scene
- **Stop buttons** - Per-track and global stop
- **Context menu**:
  - Duplicate clip
  - Delete clip
  - Edit MIDI (opens Piano Roll)
  - Export audio
  - Set color

**Data Source**:
- `clips: ClipGridItem[]` local state (not synced with engine)
- Mock clips created in component state

**No Engine Integration**: Clip launching is purely visual (no audio playback)

---

### 5. Right Panel - Mixer

**Location**: `RightPanel.tsx` (Mixer tab)

**Features**:
- **Master channel** - Global output controls
- **Per-track channels**:
  - 🎚️ **Volume fader** - 0-100 range
  - 🔄 **Pan knob** - L-R positioning
  - 🔇 **Mute button**
  - 🔊 **Solo button**
  - 📊 **Level meter** - Mock audio levels (0-100)
  - 📈 **Peak indicator** - Overload warning
  - 🎛️ **EQ button** - Show/hide EQ section
  - 📤 **Sends button** - Show/hide aux sends

**State**: All mixer state is **local only** (not sent to engine)
- No `window.electron.*` calls for volume/pan/mute/solo
- Mock meters: `level` (random) and `peak` (random > 85)

---

### 6. Right Panel - Inspector

**Location**: `RightPanel.tsx` (Inspector tab)

**Features**:
- **Track properties** - Name, color, type
- **Input/Output routing** - Dropdown selectors (UI only)
- **Plugin list** - Add/remove effects
- **MIDI settings** - Channel, transpose, velocity
- **Audio settings** - Warp mode, pitch shift

**State**: All inspector data is **local/mock** (no engine integration)

---

### 7. Wingman AI Assistant

**Location**: `WingmanSidebar.tsx` (active), `WingmanPanel.tsx` (alternate)

**Features**:
- **Chat interface** - User messages + AI responses
- **Command buttons** - Quick actions ("Create drums", "Add bass", etc.)
- **Settings panel** - Model selection, creativity slider
- **Context display** - Shows current project state
- **Voice input** - Mock microphone button

**Mock AI Commands**:
```typescript
// Keyword-based responses (no real AI)
if (input.includes('drum') || input.includes('beat')) {
  dawActions: [
    { type: 'setTempo', value: 140 },
    { type: 'createTrack', name: 'Trap Drums', trackType: 'midi' }
  ]
}

if (input.includes('bass')) {
  dawActions: [
    { type: 'createTrack', name: 'Bass', trackType: 'midi' }
  ]
}
```

**Current Limitations**:
- Cannot query project state (no `requestProjectState()`)
- Cannot batch multiple commands atomically
- Cannot see existing tracks/clips/automation
- Responses are canned, not AI-generated

---

### 8. Piano Roll Editor

**Location**: `PianoRoll.tsx`

**Features**:
- **Note grid** - Piano keys (Y) × Time (X)
- **Drawing tools** - Pencil, Select, Erase, Slice
- **Note operations**:
  - Add notes (click & drag)
  - Move notes (drag)
  - Resize notes (drag edges)
  - Delete notes (erase tool or delete key)
- **Scale highlighting** - Show in-scale notes (C minor, C major, etc.)
- **Quantize** - Snap notes to grid
- **Velocity editor** - Bottom panel for note velocity
- **Zoom controls** - Horizontal and vertical zoom

**State**: `PianoRollState` object (notes, scale, tool, zoom, snap, velocity)
- Passed as prop from `App.tsx`
- Changes via `onClose` callback (passes full state back)

**No Engine Integration**: All note data is local, no audio playback

---

### 9. Automation Lanes

**Location**: `AutomationLane.tsx`

**Features**:
- **Parameter selector** - Volume, Pan, Filter Cutoff, Resonance, etc.
- **Breakpoint graph** - Automation curve visualization
- **Add points** - Click to add breakpoints (not yet implemented)
- **Edit points** - Drag breakpoints (not yet implemented)
- **Delete** - Remove automation lane

**State**: `automationLanes` Map stored in `CenterPanel.tsx`
- Local only, not synced to engine

---

### 10. Context Menus

**Location**: `ContextMenu.tsx`

**Features**:
- **Track context menu** - Rename, Duplicate, Color, Delete, Record Arm, Show Automation
- **Clip context menu** - Duplicate, Delete, Edit MIDI, Export, Set Color
- **Browser context menu** - Add to Project, Add to Collection, Add to Favorites, Show in Folder

**Implementation**: Absolute-positioned div with click handlers

---

## Mock Engine State

**Location**: `src/main/index.js`

```javascript
const audioState = {
  tempo: 120,
  timeSignature: { numerator: 4, denominator: 4 },
  isPlaying: false,
  currentBar: 0,
  tracks: []
};
```

**IPC Commands** (all in `main/index.js`):
- `transport-play` → `isPlaying = true`, broadcast state
- `transport-pause` → `isPlaying = false`, broadcast state
- `transport-stop` → `isPlaying = false`, `currentBar = 0`, broadcast
- `set-tempo` → `tempo = data.tempo`, broadcast state
- `create-track` → `tracks.push({id, name, type})`, broadcast state
- `get-audio-state` → Return current `audioState` object

**No Real Audio**:
- No WebAudio API integration
- No audio file loading/playback
- No MIDI processing
- No plugin hosting
- No real-time audio meters

---

## IPC Bridge

**Location**: `src/main/preload.js`

**Exposed Methods** (via `window.electron`):
```typescript
interface ElectronAPI {
  // Transport
  transportPlay: () => void;
  transportPause: () => void;
  transportStop: () => void;

  // Project
  setTempo: (tempo: number) => void;
  createTrack: (name: string, type: 'midi' | 'audio' | 'instrument') => void;
  getAudioState: () => Promise<AudioState>;

  // Window management
  windowMinimize: () => void;
  windowMaximize: () => void;
  windowClose: () => void;

  // Events
  onAudioStateUpdate: (callback: (state: AudioState) => void) => () => void;
}
```

---

## State Not Synced to Engine

These UI features have **local state only** and are **not** sent to the mock engine:

1. **Browser data** - All 171 sample items, favorites, collections
2. **Mixer controls** - Volume, pan, mute, solo, meters
3. **Automation lanes** - All automation data and breakpoints
4. **Piano Roll notes** - MIDI notes are local to `pianoRollTrack` state
5. **Session View clips** - Clip grid is local state
6. **Track record-arm state** - `recordArmed` Set is local
7. **Loop/Metronome toggles** - Transport flags are local
8. **Inspector data** - Routing, plugins, MIDI/audio settings
9. **Context menu state** - Position and items
10. **Wingman chat history** - Messages array is local

**Why**: The mock engine only tracks `tempo`, `timeSignature`, `isPlaying`, `currentBar`, and `tracks[]` (just ID, name, type).

---

## Behaviors

### Playback Flow

1. User clicks **Play** in Transport Bar
2. `TransportBar.tsx` calls `window.electron.transportPlay()`
3. Main process sets `audioState.isPlaying = true`
4. Main process broadcasts `audio-state-update` event
5. `App.tsx` receives update via `onAudioStateUpdate` callback
6. `App.tsx` calls `setAudioState(newState)`
7. Re-render propagates to all components
8. UI shows **Pause** button (based on `audioState.isPlaying`)

**No actual audio plays** - this is just state changes.

---

### Track Creation Flow

1. User clicks **Add Track** in Arrangement View
2. `CenterPanel.tsx` calls `window.electron.createTrack('Audio 1', 'audio')`
3. Main process pushes track to `audioState.tracks[]`
4. Main process broadcasts `audio-state-update`
5. `App.tsx` receives update
6. `CenterPanel.tsx` re-renders with new track in list

---

### Wingman Command Flow

1. User types "create drums" in Wingman chat
2. `WingmanSidebar.tsx` generates mock response (keyword matching)
3. If response has `dawActions`, execute each:
   ```typescript
   response.dawActions.forEach((action: any) => {
     if (action.type === 'setTempo') {
       window.electron.setTempo(action.value);
     } else if (action.type === 'createTrack') {
       window.electron.createTrack(action.name, action.trackType);
     }
   });
   ```
4. Each action follows standard IPC flow above

**Current Limitation**: Wingman cannot query project state, so it doesn't know what tracks already exist.

---

### Mixer Changes (Local Only)

1. User drags volume fader in Mixer
2. `RightPanel.tsx` updates local `volume` state
3. **No IPC call** - change is purely visual
4. No other component knows about the change

**Result**: Mixer controls are not persisted or shared.

---

## Keyboard Shortcuts

**Defined in**: `CenterPanel.tsx`

| Shortcut | Action |
|----------|--------|
| `Space` | Play/Pause |
| `Ctrl+T` | Create new track |
| `Delete` | Delete selected track |
| `Ctrl+D` | Duplicate selected track |
| `Ctrl+Z` | Undo (not yet implemented) |
| `Ctrl+Y` | Redo (not yet implemented) |

**Global shortcuts** handled in `App.tsx`:
- `Ctrl+S` - Save project (not yet implemented)
- `Ctrl+O` - Open project (not yet implemented)

---

## Visual Theme

**Colors** (from Tailwind classes):
- Background: `bg-gray-900` (dark)
- Panels: `bg-gray-800`
- Borders: `border-gray-700`
- Accent: `bg-purple-600` (primary actions)
- Hover: `hover:bg-gray-700`
- Text: `text-gray-200`

**Layout**:
- Top: TransportBar (fixed height ~60px)
- Main: 3-column layout (Left 20%, Center 50%, Right 30%)
- Bottom: Status bar (not yet implemented)

---

## TypeScript Interfaces

**Core Audio Types** (`types/audio.ts`):
```typescript
export interface AudioState {
  tempo: number;
  timeSignature: { numerator: number; denominator: number };
  isPlaying: boolean;
  currentBar: number;
  tracks: Track[];
}

export interface Track {
  id: string;
  name: string;
  type: 'midi' | 'audio' | 'instrument';
  color?: string;
  muted?: boolean;
  solo?: boolean;
  volume?: number;
  pan?: number;
}
```

**Note**: Current engine only stores `id`, `name`, `type`. Other fields are UI-only.

---

## Known Issues & Gaps

1. **No undo/redo** - `useHistory` hook exists but not integrated
2. **No project save/load** - State is ephemeral
3. **No audio playback** - Mock engine only
4. **No plugin system** - No VST/AU support
5. **Mixer not synced** - Volume/pan/mute/solo are local only
6. **Automation not functional** - Lanes display but don't affect audio
7. **Session View not functional** - Clips don't trigger audio
8. **Piano Roll not synced** - Notes are local, no MIDI output
9. **Wingman limited** - Can't query state, canned responses only
10. **No collaboration** - Single-user only

---

## Adapter Requirements

To enable future native engine swap, the adapter must:

### 1. Command API

Replace all `window.electron.*` calls with unified adapter:

```typescript
// Before
window.electron.transportPlay();
window.electron.setTempo(140);
window.electron.createTrack('Bass', 'midi');

// After
engineClient.sendCommand('transport:play');
engineClient.sendCommand('transport:setTempo', { tempo: 140 });
engineClient.sendCommand('track:create', { name: 'Bass', type: 'midi' });
```

### 2. Event System

Replace `onAudioStateUpdate` with generic event listener:

```typescript
// Before
window.electron.onAudioStateUpdate((state) => setAudioState(state));

// After
engineClient.onEvent((event) => {
  if (event.type === 'state:updated') {
    store.setState(event.data);
  }
});
```

### 3. State Query

Enable Wingman to query project state:

```typescript
const state = await engineClient.requestProjectState();
// Returns: { tempo, tracks, clips, automation, mixer, ... }
```

### 4. Batch Commands

Enable atomic multi-command operations:

```typescript
await engineClient.sendCommand('batch:start');
await engineClient.sendCommand('track:create', { name: 'Drums' });
await engineClient.sendCommand('track:create', { name: 'Bass' });
await engineClient.sendCommand('transport:setTempo', { tempo: 140 });
await engineClient.sendCommand('batch:commit');
```

### 5. Feature Flag

Support switching between mock and real engine:

```typescript
// .env
ENGINE_MODE=mock  // or 'ipc' for real engine

// Adapter internals
if (import.meta.env.ENGINE_MODE === 'mock') {
  // Use mock implementation
} else {
  // Use IPC to native engine
}
```

---

## Migration Plan

### Phase 1: Create Adapter Layer ✅ (This document)

- Document all UI panels and behaviors
- Map all `window.electron.*` calls
- Identify state sync gaps

### Phase 2: Build Adapter

- Create `src/renderer/lib/engineClient.ts`
- Implement `connect()`, `sendCommand()`, `onEvent()`, `requestProjectState()`
- Create mock implementation behind the scenes

### Phase 3: Add Zustand Store

- Create `src/renderer/lib/store.ts`
- Move `audioState` from `App.tsx` to store
- Subscribe store to `engineClient.onEvent()`

### Phase 4: Refactor Components

- TransportBar: Replace `window.electron.transportPlay()` → `sendCommand('transport:play')`
- CenterPanel: Replace `createTrack()` → `sendCommand('track:create', ...)`
- RightPanel: Add `sendCommand()` for mixer changes
- WingmanSidebar: Replace direct IPC → `sendCommand()` batches

### Phase 5: Add Feature Flag

- Create `.env` with `ENGINE_MODE=mock`
- Update `engineClient.ts` to check flag
- Keep mock path working exactly as before

### Phase 6: Validate

- Ensure all features work identically
- No direct `window.electron.*` calls remain
- Single switchover point in `engineClient.ts`

---

## Success Criteria

✅ App behaves **exactly as before** after refactor
✅ No React component mutates global state directly
✅ All mutations go through `engineClient.sendCommand()`
✅ One place (`engineClient.ts`) to switch mock → real engine
✅ Wingman can query full project state
✅ Zustand store is single source of truth for UI state
✅ `ENGINE_MODE` flag works (mock/ipc)

---

## References

- Current codebase: `vexel-daw/src/renderer/`
- Mock engine: `vexel-daw/src/main/index.js`
- Type definitions: `vexel-daw/src/renderer/types/`
- Hooks: `vexel-daw/src/renderer/hooks/useHistory.ts`

---

**End of UI Freeze Documentation**
