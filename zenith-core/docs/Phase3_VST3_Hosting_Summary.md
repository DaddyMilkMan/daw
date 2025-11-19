# Phase 3: VST3 Plugin Hosting MVP - Implementation Summary

**Date:** 2025-11-14
**Status:** ✅ Complete

---

## Overview

This document summarizes the VST3 plugin hosting implementation for Zenith DAW Phase 3.

## Components Implemented

### 1. PluginHost (Engine-Level Manager)

**Location:** `Source/engine/PluginHost.{h,cpp}`

**Responsibilities:**
- Manages `AudioPluginFormatManager` (VST3 format only for MVP)
- Maintains `KnownPluginList` of scanned plugins
- Provides plugin scanning (`scanDefaultLocations()`, `scanPath()`)
- Creates plugin instances on demand (`createInstance()`)

**Thread Safety:**
- ✅ All operations are **MESSAGE THREAD ONLY**
- ✅ No audio thread access
- ✅ Scanning is synchronous/blocking (acceptable for MVP)

**Key Methods:**
- `scanDefaultLocations()` - Scans standard VST3 directories
- `createInstance(identifier, sampleRate, blockSize)` - Creates plugin instance
- `getKnownPlugins()` - Returns list of available plugins

### 2. Track Plugin Chain

**Location:** `Source/engine/Track.{h,cpp}`

**Modifications:**
- Added `std::vector<std::unique_ptr<AudioPluginInstance>> plugins`
- Replaced stubbed plugin methods with real implementations
- Added `pluginMidiBuffer` for MIDI processing

**Plugin Management (MESSAGE THREAD ONLY):**
- `addPlugin(plugin)` - Add plugin to chain
- `removePlugin(index)` - Remove plugin from chain
- `clearPlugins()` - Clear all plugins
- `getNumPlugins()` / `getPlugin(index)` - Access plugins

**Audio Processing (AUDIO THREAD):**
- `processPluginChain(buffer, numSamples)` - Process plugins in linear chain
- ✅ RT-safe: Only reads plugin vector (no modifications during playback)
- ✅ No locks during processing (modifications only on message thread when stopped)

**Processing Order:**
- Audio tracks: `clips → MixerChannel → plugins → output`
- Instrument tracks (future): `MIDI → instrument plugin → FX plugins → MixerChannel → output`

### 3. Plugin Editor Windows

**Location:** `Source/ui/PluginEditorWindow.{h,cpp}`

**Components:**

#### PluginEditorWindow
- Simple `DocumentWindow` wrapper for plugin editors
- Displays custom plugin UI or generic editor
- Self-deleting on close

#### PluginEditorWindowManager
- Manages all open plugin editor windows
- Ensures one window per plugin instance
- Integrated into `Engine` for global access

**Thread Safety:**
- ✅ All operations are **MESSAGE THREAD ONLY**
- ✅ Plugin editors only created/destroyed on message thread (JUCE requirement)

### 4. ProjectState Persistence

**Track State Serialization:**

**Saving (`Track::getState()`):**
- Saves plugin chain as `"Plugins"` ValueTree child
- For each plugin:
  - Plugin identifier (for recreation)
  - Plugin name (for debugging)
  - Plugin state as Base64-encoded binary blob (`getStateInformation()`)

**Loading (`Track::loadPluginStates()`):**
- Recreates plugins via `PluginHost::createInstance()`
- Restores plugin parameters from binary state (`setStateInformation()`)
- Gracefully handles missing plugins (logs warning, continues loading)

**Integration:**
- Must be called from Engine/ProjectState level where `PluginHost` is accessible
- Called after basic track state is loaded (`loadState()`)

---

## RT-Safety Verification

### ✅ Thread Safety Checklist

#### Message Thread Operations (Safe)
- ✅ Plugin scanning (`PluginHost::scanDefaultLocations()`)
- ✅ Plugin instantiation (`PluginHost::createInstance()`)
- ✅ Adding/removing plugins from tracks (`Track::addPlugin/removePlugin()`)
- ✅ Opening/closing plugin editor windows
- ✅ Project save/load with plugin states

#### Audio Thread Operations (RT-Safe)
- ✅ `Track::processPluginChain()` - Only reads plugin vector
- ✅ `AudioPluginInstance::processBlock()` - JUCE RT-safe method
- ✅ No file I/O in audio thread
- ✅ No memory allocation in audio thread
- ✅ No mutex locks in audio thread (plugins vector only modified on message thread)

#### Cross-Thread Communication
- ✅ Plugin vector only modified when audio is stopped (or with proper synchronization)
- ✅ `CriticalSection pluginLock` only used for add/remove operations
- ✅ No lock contention during playback

---

## Manual Test Plan

### Test 1: Plugin Scanning

**Steps:**
1. Build and run Zenith DAW
2. Open MainWindow
3. Call `Engine::scanForPlugins()` (via debug button or menu)

**Expected Results:**
- Console shows "Scanning default VST3 locations..."
- Lists all found plugins with names
- Returns count of discovered plugins
- No crashes or errors

**Verification:**
- Check console output for plugin list
- Verify known VST3 plugins on system are found

---

### Test 2: Adding Plugin to Track

**Steps:**
1. Create a test track via `Engine::addTestTracks(1)`
2. Get plugin descriptions from `PluginHost::getKnownPlugins()`
3. Create a plugin instance for a simple effect (e.g., gain/EQ)
4. Add plugin to track via `track->addPlugin(instance)`
5. Verify `track->getNumPlugins()` returns 1

**Expected Results:**
- Plugin instance created successfully
- Plugin added to track
- No crashes

**Verification:**
- `track->getNumPlugins()` == 1
- `track->getPlugin(0)` returns valid pointer
- Console logs plugin name

---

### Test 3: Audio Processing with Plugin (Audio Track)

**Steps:**
1. Create audio track with a simple VST3 effect (e.g., gain, filter)
2. Add an audio clip to the track
3. Start playback via `Engine::play()`
4. Listen for effect processing

**Expected Results:**
- Audio plays through plugin
- Effect is audible (e.g., gain change, filtering)
- No audio dropouts or glitches
- No crashes during playback

**Verification:**
- Visual: CPU usage stays reasonable
- Audio: Effect processing is audible
- No console errors during playback

---

### Test 4: Plugin Editor Window

**Steps:**
1. Create track with plugin (from Test 2)
2. Get plugin instance pointer
3. Call `PluginEditorWindowManager::openEditor(plugin)`
4. Interact with plugin UI (adjust parameters)
5. Close window via close button

**Expected Results:**
- Plugin editor window opens
- UI displays correctly (custom or generic)
- Parameter changes work
- Window closes cleanly without crashes

**Verification:**
- Window appears centered on screen
- Controls are interactive
- Window deletes itself on close
- No memory leaks (check `PluginEditorWindowManager` map)

---

### Test 5: Project Save/Load with Plugins

**Steps:**
1. Create track with 2-3 plugins
2. Adjust plugin parameters
3. Save project via `ProjectState::saveToFile()`
4. Clear track plugins
5. Load project via `ProjectState::loadFromFile()`
6. Call `track->loadPluginStates(state, pluginHost)`

**Expected Results:**
- Project saves successfully
- XML contains `<Plugins>` section with plugin identifiers and states
- Project loads successfully
- Plugins recreated with same parameters

**Verification:**
- Inspect saved XML file for plugin data
- `track->getNumPlugins()` matches original count
- Plugin parameters match saved values
- Audio processing sounds identical to before save

---

### Test 6: Missing Plugin Handling

**Steps:**
1. Create project with a plugin
2. Save project
3. Manually edit XML to reference a non-existent plugin ID
4. Load project

**Expected Results:**
- Project loads without crashing
- Console shows warning: "Failed to load plugin: ..."
- Track loads with 0 plugins (gracefully skips missing plugin)
- Other plugins (if any) load correctly

**Verification:**
- No crash
- Warning logged
- Application remains stable

---

### Test 7: Multiple Plugin Chain

**Steps:**
1. Create audio track
2. Add 3+ plugins in series (e.g., EQ → Compressor → Reverb)
3. Start playback
4. Verify all plugins process in order

**Expected Results:**
- All plugins process audio
- Processing order is correct (first → last)
- No audio glitches
- CPU usage reasonable

**Verification:**
- Audio output sounds correct (all effects applied)
- No dropouts during playback

---

### Test 8: RT-Safety Stress Test

**Steps:**
1. Create 8 tracks with 3-4 plugins each (~24-32 plugins total)
2. Start playback
3. Monitor CPU usage
4. Let play for 1-2 minutes

**Expected Results:**
- No audio dropouts
- CPU usage stable
- No crashes
- No console errors

**Verification:**
- Check CPU meter (should be <80% on modern system)
- Listen for xruns/dropouts
- No RT-safety violations logged

---

## Known Limitations / Future Work

### Current MVP Limitations

1. **VST3 Only:**
   - No AudioUnit support (macOS)
   - No VST2 support
   - **Future:** Add AU format on macOS

2. **Simple Instrument Track Handling:**
   - Instrument vs. Audio track processing is simplified
   - MIDI routing is basic (empty MIDI buffer for audio tracks)
   - **Future:** Proper MIDI clip → instrument plugin routing

3. **No Sidechain Support:**
   - Plugins process in simple linear chain
   - **Future:** Implement aux routing and sidechaining

4. **Synchronous Plugin Scanning:**
   - Blocking operation (may freeze UI briefly)
   - **Future:** Async scanning with progress UI

5. **No Plugin Delay Compensation:**
   - Plugins with latency not compensated
   - **Future:** Implement PDC (Plugin Delay Compensation)

6. **No Plugin Preset Management:**
   - No built-in preset browser
   - **Future:** Add preset save/load/browser

---

## Architecture Summary

### PluginHost Design

```
Engine
  └── PluginHost
      ├── AudioPluginFormatManager (VST3 only)
      ├── KnownPluginList (scanned plugins)
      └── Methods:
          ├── scanDefaultLocations()
          └── createInstance(identifier)
```

### Track Plugin Chain Design

```
Track
  ├── std::vector<unique_ptr<AudioPluginInstance>> plugins
  ├── CriticalSection pluginLock (message thread only)
  └── Processing:
      Audio Track:  clips → MixerChannel → plugins → output
      Instrument:   MIDI → instrument → FX plugins → MixerChannel → output (future)
```

### State Persistence Design

```
ValueTree Structure:
  TRACK
    ├── (track properties)
    ├── PLUGINS
    │   ├── PLUGIN
    │   │   ├── identifier: "VST3:..."
    │   │   ├── name: "Plugin Name"
    │   │   └── state: "Base64EncodedBinary..."
    │   └── ...
    └── CLIPS
        └── ...
```

---

## Assumptions & Design Decisions

1. **Message Thread Plugin Modifications:**
   - Plugins only added/removed when audio is stopped or with proper sync
   - This is safe for MVP; future may need lockless queue for realtime changes

2. **Linear Plugin Chain:**
   - Simple first-to-last processing order
   - No complex routing (sufficient for MVP)

3. **Generic Editor Fallback:**
   - All plugins can use JUCE's GenericAudioProcessorEditor if custom UI fails
   - Ensures every plugin is editable

4. **Base64 State Encoding:**
   - Plugin states stored as Base64 in XML
   - Human-readable XML format
   - Slightly larger than binary, but acceptable

5. **Graceful Missing Plugin Handling:**
   - Projects load even if plugins are missing
   - Warns user but doesn't fail
   - Prevents data loss

---

## Files Modified/Created

### New Files
- `Source/engine/PluginHost.h`
- `Source/engine/PluginHost.cpp`
- `Source/ui/PluginEditorWindow.h`
- `Source/ui/PluginEditorWindow.cpp`
- `docs/Phase3_VST3_Hosting_Summary.md`

### Modified Files
- `Source/engine/Track.h` (replaced stubs, added plugin chain)
- `Source/engine/Track.cpp` (implemented plugin methods, processing, state)
- `include/Engine.h` (added PluginHost & PluginEditorWindowManager)
- `src/Engine.cpp` (initialized plugin systems)
- `CMakeLists.txt` (added new files, enabled VST3 hosting)

---

## Build Configuration

**CMakeLists.txt Changes:**
- Enabled `JUCE_PLUGINHOST_VST3=1`
- Added `Source/engine/PluginHost.{h,cpp}`
- Added `Source/ui/PluginEditorWindow.{h,cpp}`
- Added `${CMAKE_CURRENT_SOURCE_DIR}/Source/ui` to include paths

**Required JUCE Modules:**
- `juce_audio_processors` (plugin hosting)
- All existing modules (audio_basics, audio_devices, etc.)

---

## Next Steps (Phase 4+)

1. **Timeline UI + Piano Roll:**
   - Visual clip editing
   - MIDI note editor
   - Automation curves

2. **Wingman Integration:**
   - Use existing Ableton command schema
   - Map to Zenith's Track/Clip/Plugin API

3. **Advanced Plugin Features:**
   - AudioUnit support (macOS)
   - Plugin delay compensation
   - Preset management
   - Sidechain routing

4. **Performance Optimizations:**
   - Lock-free plugin chain updates
   - Async plugin scanning with progress UI
   - SIMD optimizations

---

## Conclusion

✅ **Phase 3 Complete:** Zenith DAW now has a fully functional VST3 plugin hosting system that is:
- RT-safe
- Persistent (save/load projects with plugins)
- User-editable (plugin UI windows)
- Robust (handles missing plugins gracefully)

The implementation follows JUCE best practices and maintains the existing RT-safe architecture established in previous phases.
