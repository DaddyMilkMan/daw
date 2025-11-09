# Audio Clip Editing Features for Zenith DAW

This document describes the comprehensive audio clip editing features implemented for Zenith DAW.

## 🎵 Features Implemented

### 1. Trim and Split
- **Non-destructive trim**: Drag clip edges to trim start/end while preserving original audio
- **Split at playhead**: Press `S` to split selected clips at the current playhead position
- **Visual feedback**: Trim handles appear on hover with intuitive edge dragging

### 2. Fade In/Out and Crossfades
- **Draggable fade handles**: Create smooth fades by dragging handles at clip edges
- **Multiple fade curves**:
  - Linear
  - Exponential (recommended for most cases)
  - Logarithmic
  - S-Curve
- **Automatic crossfades**: When clips overlap, crossfades are created automatically
- **Equal-power crossfade**: Default crossfade curve maintains constant perceived volume
- **Configurable**: Disable auto-crossfade or adjust curve type in audio settings

### 3. Time-Stretch and Pitch Shift
- **Playback rate control**: Adjust from 25% to 400% speed
- **Pitch shifting**: ±12 semitones range
- **Simple Web Audio API implementation**: Uses `playbackRate` for initial version
- **Architecture ready for advanced algorithms**: Planned support for Rubber Band or Elastique

### 4. Waveform Display and Zoom
- **Canvas-based rendering**: High-performance waveform visualization
- **RMS peak calculation**: Shows detailed audio peaks for better visibility
- **Automatic caching**: Waveforms are cached for 7 days (configurable)
- **Visual feedback**: Fades and trims are visualized on the waveform
- **Customizable colors**: Change waveform color in audio settings

### 5. Clipboard Operations
- **Copy** (`Ctrl+C`): Copy selected clips to clipboard
- **Cut** (`Ctrl+X`): Cut selected clips
- **Paste** (`Ctrl+V`): Paste clips at playhead position
- **Duplicate** (`Ctrl+D`): Duplicate selected clips adjacent to originals
- **Delete** (`Del`/`Backspace`): Remove selected clips
- **Multi-selection**: Hold `Shift` or `Ctrl` to select multiple clips

### 6. UI Integration
- **Dark Apple-like theme**: Consistent with existing Zenith DAW design
- **Drag-and-drop support**: Drop audio files (MP3, WAV, OGG, FLAC, etc.) into arrangement view
- **Context menus**: Right-click clips for quick actions
- **Tooltips**: Hover over controls for helpful information
- **Icons**: Clear visual indicators for all editing functions
- **Grid snapping**: Toggle grid snap with dedicated button (enabled by default)

### 7. Serialization
- **Base64 audio embedding**: Audio files are embedded in project JSON
- **Progressive loading**: UI loads first, then audio files render progressively
- **Full state preservation**: All clip properties saved (fades, trim, stretch, pitch, etc.)

## 📁 New Files Created

### Type Definitions
- `src/renderer/types/clip.ts` - AudioClip types with editing properties

### Core Engine
- `src/renderer/lib/audioEngine.ts` - Web Audio API infrastructure
- `src/renderer/lib/waveformGenerator.ts` - Canvas-based waveform rendering with caching

### State Management
- `src/renderer/store/clipStore.ts` - Zustand store for clip state management

### Components
- `src/renderer/components/WaveformCanvas.tsx` - Waveform visualization component
- `src/renderer/components/AudioClipComponent.tsx` - Interactive clip with trim/fade handles
- `src/renderer/components/ArrangementViewEnhanced.tsx` - Enhanced arrangement view
- `src/renderer/components/ClipPropertiesPanel.tsx` - Time-stretch/pitch controls
- `src/renderer/components/AudioSettingsPanel.tsx` - Crossfade and audio preferences

### Hooks
- `src/renderer/hooks/useAudioFileLoader.ts` - Audio file loading with progress
- `src/renderer/hooks/useAudioFileDrop.ts` - Drag-and-drop handler

### Modified Files
- `src/renderer/components/CenterPanel.tsx` - Integrated enhanced arrangement view
- `src/renderer/App.tsx` - Pass BPM to CenterPanel

## ⌨️ Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| `S` | Split clip at playhead |
| `Ctrl+C` | Copy selected clips |
| `Ctrl+X` | Cut selected clips |
| `Ctrl+V` | Paste clips |
| `Ctrl+D` | Duplicate selected clips |
| `Del` / `Backspace` | Delete selected clips |
| `Escape` | Deselect all clips |
| `Shift+Click` | Add to selection |

## 🎨 UI Features

### Clip Display
- Color-coded clips with waveform overlay
- Selection indicators (ring highlight)
- Mute/lock status badges
- Time-stretch indicator (percentage)
- Crossfade indicators

### Editing Handles
- **Trim handles**: Left/right edges for trimming
- **Fade handles**: Blue gradient handles for fades
- **Move handle**: Entire clip is draggable
- All handles show on hover with visual feedback

### Grid Snap
- Toggle button in arrangement toolbar
- Snaps to configurable grid (default: 16th notes)
- Affects clip movement, trimming, and splitting

## 🔧 Technical Implementation

### Non-Destructive Editing
All clip editing is non-destructive:
- Original audio files are never modified
- Trim is achieved via `trimStart` and `trimEnd` offsets
- Fades are applied using Web Audio API `AudioParam` automation
- Time-stretch uses `playbackRate` property
- All settings are stored in clip state

### Waveform Caching
- Peaks are pre-calculated using RMS or max peak algorithms
- Cached with file hash (name + size + modified date)
- Automatic cleanup after 7 days (configurable)
- Significant performance improvement for large files

### Equal-Power Crossfade
Based on research, equal-power crossfade is the default because:
- Maintains constant perceived volume (prevents dip in middle)
- Works well for 90% of use cases
- Uses cos/sin curves: `out = cos(angle)`, `in = sin(angle)`
- Can be changed to linear (for coherent material) or logarithmic

### State Management
- Zustand store for global clip state
- Reactive updates across all components
- Undo/redo ready (hooks exist in codebase)
- TypeScript for type safety

## 🚀 Future Enhancements

### Planned Features
1. **Advanced time-stretch**: Integrate Rubber Band or similar library via Web Workers
2. **Undo/redo integration**: Connect clip editing to existing undo/redo system
3. **Clip quantization**: Snap MIDI or audio to grid with timing correction
4. **Audio warping**: Time-stretch sections of clips independently
5. **Clip gain automation**: Per-clip volume envelopes
6. **Audio effects per clip**: Add effects to individual clips
7. **Spectral editing**: Visual frequency domain editing
8. **Clip grouping**: Group multiple clips for synchronized editing

### Performance Optimizations
1. Web Workers for waveform generation
2. Virtualized rendering for large projects
3. Incremental waveform rendering
4. GPU-accelerated waveform drawing

## 📊 Architecture

```
┌─────────────────────────────────────┐
│     ArrangementViewEnhanced         │
│  (Main container with drag-drop)    │
└──────────┬──────────────────────────┘
           │
           ├─► AudioClipComponent (per clip)
           │   ├─► WaveformCanvas
           │   ├─► Trim handles
           │   └─► Fade handles
           │
           ├─► useClipStore (Zustand)
           │   ├─► Clip state
           │   ├─► Audio settings
           │   └─► Clipboard
           │
           ├─► audioEngine (Web Audio API)
           │   ├─► Audio loading
           │   ├─► Playback
           │   └─► Fade curves
           │
           └─► WaveformGenerator
               ├─► Peak calculation
               └─► Canvas rendering
```

## 🧪 Testing

To test the features:

1. **Install dependencies**:
   ```bash
   npm install
   ```

2. **Run in development**:
   ```bash
   npm run dev
   ```

3. **Create a track**: Click "Add Track" in arrangement view

4. **Add audio**:
   - Drag and drop an audio file from your system
   - Or use the browser panel (left side) to load samples

5. **Edit clips**:
   - Drag edges to trim
   - Drag fade handles to create fades
   - Press `S` to split at playhead
   - Right-click for context menu
   - Use keyboard shortcuts

6. **Adjust settings**:
   - Open audio settings (gear icon in toolbar)
   - Configure crossfade preferences
   - Customize waveform colors

## 📝 Notes

- Audio files are embedded as base64 in projects for portability
- First load may be slow for large files (peaks generation)
- Subsequent loads are fast (cached peaks)
- Grid snap is enabled by default for musical alignment
- All editing operations preserve original audio files

## 🎯 Credits

Implemented following DAW industry best practices:
- Crossfade curves based on audio engineering research
- Waveform rendering inspired by BBC Peaks.js
- Non-destructive editing following Pro Tools model
- Keyboard shortcuts matching Logic Pro / Ableton conventions

---

**Built for Zenith DAW** 🚀
