---
description: Fix AI Control Architecture - From Remote Control to Brain
---

# AI Control Architecture Implementation

**Context**: Based on roast in `docs/NUCLEAR_CODEBASE_ROAST_2025.md` and analysis in `docs/AI_CONTROL_ARCHITECTURE_FIX.md`

**Goal**: Transform CommandAPI from a "remote control" into a true "brain interface"

---

## Phase 1: Routing Graph Foundation (Week 1)

### Task 1.1: Create Routing Node Architecture
**Files to Create:**
- `apps/desktop/Source/engine/RoutingGraph.h`
- `apps/desktop/Source/engine/RoutingGraph.cpp`

**Implementation:**
```cpp
class RoutingNode {
    enum class Type { Track, Bus, Send, Return, Master };
    juce::String nodeId;
    Type nodeType;
    std::vector<juce::String> inputs;
    std::vector<juce::String> outputs;
};

class RoutingGraph {
    void addNode(const RoutingNode& node);
    void connect(const juce::String& sourceId, const juce::String& destId);
    void disconnect(const juce::String& sourceId, const juce::String& destId);
    juce::var toVar() const;  // For AI inspection
};
```

### Task 1.2: Expose MixerChannel Sends to CommandAPI
**Files to Modify:**
- `apps/desktop/Source/commands/CommandAPI.h` (add method declarations)
- `apps/desktop/Source/commands/CommandAPI.cpp` (add implementations)

**New Commands:**
- `set_track_send` - Set send level for a track
- `get_track_sends` - Get all send levels
- `set_send_pre_fader` - Configure pre/post fader

**Example JSON:**
```json
{
  "command": "set_track_send",
  "params": {
    "trackId": "track_1",
    "sendIndex": 0,
    "level": 0.5,
    "preFader": false
  }
}
```

### Task 1.3: Implement Bus Creation
**Files to Modify:**
- `apps/desktop/Source/engine/Engine.h` - Add bus management
- `apps/desktop/Source/engine/Engine.cpp` - Implement bus creation
- `apps/desktop/Source/commands/CommandAPI.cpp` - Add `create_bus` command

**Command Spec:**
```json
{
  "command": "create_bus",
  "params": {
    "name": "Reverb Bus",
    "type": "stereo",
    "color": "#3498db"
  }
}
```

### Task 1.4: Implement Node Connection Commands
**New CommandAPI Methods:**
- `connect_nodes` - Connect two routing nodes
- `disconnect_nodes` - Disconnect nodes
- `get_routing_graph` - Export full routing graph for AI

---

## Phase 2: Plugin State Management (Week 2)

### Task 2.1: Plugin Parameter Introspection
**Files to Modify:**
- `apps/desktop/Source/commands/CommandAPI.h`
- `apps/desktop/Source/commands/CommandAPI.cpp`

**New Commands:**
- `get_plugin_parameter_map` - Return name→index mapping
- `set_plugin_param_by_name` - Set parameter by name instead of index

**Example:**
```json
// Before (AI must guess paramIndex)
{
  "command": "set_plugin_param",
  "params": { "pluginIndex": 0, "paramIndex": 42, "value": 0.5 }
}

// After (AI uses parameter name)
{
  "command": "set_plugin_param_by_name",
  "params": { "pluginIndex": 0, "paramName": "warpMode", "value": 0.5 }
}
```

### Task 2.2: Plugin State Chunk Support
**Files to Modify:**
- `apps/desktop/Source/engine/Track.h` - Add state chunk methods
- `apps/desktop/Source/engine/Track.cpp` - Implement state serialization
- `apps/desktop/Source/commands/CommandAPI.cpp` - Expose via API

**New Commands:**
- `get_plugin_state_chunk` - Returns base64-encoded VST state
- `set_plugin_state_chunk` - Loads base64-encoded VST state

**Example:**
```json
{
  "command": "set_plugin_state_chunk",
  "params": {
    "trackId": "track_1",
    "pluginIndex": 0,
    "state": "VkVTVDAwMDEAA..." // base64 blob
  }
}
```

---

## Phase 3: Clip-Level DSP (Week 3)

### Task 3.1: Extend Clip Structure
**Files to Modify:**
- `apps/desktop/Source/engine/Track.h` - Add Clip DSP members
- `apps/desktop/Source/engine/Clip.cpp` - Implement processing

**New Clip Parameters:**
```cpp
class Track::Clip {
    std::atomic<float> clipGain{1.0f};
    std::atomic<float> pitchShiftSemitones{0.0f};
    std::atomic<float> timeStretchRatio{1.0f};
    std::atomic<bool> reverse{false};
    
    struct Fade {
        std::atomic<float> lengthSamples{0.0f};
        enum class Curve { Linear, Exponential, SCurve } curve;
    };
    Fade fadeIn, fadeOut;
};
```

### Task 3.2: Implement Fade Processing
**Files to Modify:**
- `apps/desktop/Source/engine/Clip.cpp` - Add fade application in audio processing

**Fade Curves:**
- Linear: `gain = progress`
- Exponential: `gain = pow(progress, 2)`
- S-Curve: `gain = smoothstep(progress)`

### Task 3.3: Expose Clip DSP to CommandAPI
**New Commands:**
- `set_clip_gain` - Set clip-level gain (independent of track gain)
- `set_clip_pitch` - Pitch shift in semitones
- `set_clip_time_stretch` - Time stretch ratio
- `set_clip_fade` - Configure fade in/out
- `set_clip_reverse` - Enable reverse playback

**Example:**
```json
{
  "command": "set_clip_fade",
  "params": {
    "trackId": "track_1",
    "clipId": "clip_0",
    "fadeIn": { "lengthSamples": 2205, "curve": "exponential" },
    "fadeOut": { "lengthSamples": 4410, "curve": "linear" }
  }
}
```

---

## Phase 4: AI Visual/Audio Feedback (Week 4)

### Task 4.1: UI State Capture
**Files to Create:**
- `apps/desktop/Source/ui/UIStateCapture.h`
- `apps/desktop/Source/ui/UIStateCapture.cpp`

**New CommandAPI Method:**
- `get_ui_state` - Returns current view, selection, zoom level

**Example Response:**
```json
{
  "success": true,
  "result": {
    "currentView": "arranger",
    "selectedTracks": ["track_1", "track_3"],
    "viewportStart": 0,
    "viewportEnd": 192000,
    "zoomLevel": 1.5
  }
}
```

### Task 4.2: Audio Analysis Export
**Files to Modify:**
- `apps/desktop/Source/commands/CommandAPI.cpp`

**New Command:**
- `analyze_track_audio` - Returns waveform PNG + audio features

**Example Response:**
```json
{
  "success": true,
  "result": {
    "waveformPng": "iVBORw0KGgoAAAANS...", // base64 PNG (256x64)
    "rmsDb": -18.5,
    "peakDb": -6.2,
    "spectralCentroid": 2500.0,
    "duration": 4.5
  }
}
```

### Task 4.3: Enhanced Session Graph
**Files to Modify:**
- `apps/desktop/Source/network/SessionGraph.cpp`

**Enhancements:**
- Add routing graph visualization
- Include plugin state summaries (not full chunks)
- Add visual bounding boxes for UI elements

---

## Phase 5: Command Dispatch Cleanup (Bonus)

### Task 5.1: Make Dispatch Table More Obvious
**Files to Modify:**
- `apps/desktop/Source/commands/CommandAPI.cpp`

**Current (Hidden Complexity):**
```cpp
juce::var executeCommand(const juce::var& request) {
    // commandMap lookup hidden in initializeCommandMap()
    // ...500 line switch statement
}
```

**After (Explicit Dispatch):**
```cpp
class CommandAPI {
private:
    using Handler = juce::var (CommandAPI::*)(const juce::var&);
    static const std::unordered_map<CommandID, Handler> handlers_;
    
    juce::var executeCommand(const juce::var& request) {
        auto id = parseCommand(request["command"]);
        return (this->*handlers_.at(id))(request["params"]);
    }
};
```

---

## Testing Strategy

### Integration Tests
**File to Create:** `test/CommandAPIRoutingTest.cpp`

**Test Cases:**
1. `CanCreateBusAndRouteTracks` - Create bus, route multiple tracks
2. `CanSetupSidechainCompression` - Route kick to sidechain bass
3. `CanLoadPluginByStateChunk` - Load Serum preset via state blob
4. `CanApplyClipFades` - Verify fade processing in audio output
5. `CanCaptureUIState` - Verify UI state matches actual state

---

## Success Metrics

### Before (Remote Control)
- ❌ Cannot create reverb bus with sends
- ❌ Cannot setup sidechain compression
- ❌ Cannot load plugin presets (only tweak params)
- ❌ Clips pop and click at boundaries
- ❌ AI has no visual/audio feedback

### After (Brain Integration)
- ✅ Full signal routing control
- ✅ Plugin presets load instantly
- ✅ Professional audio editing (fades, pitch, time stretch)
- ✅ AI can "see" waveforms and "hear" analysis
- ✅ 80%+ DAW feature coverage via API

---

## Migration Notes

1. **Backward Compatibility**: All existing commands continue to work
2. **Incremental Rollout**: Implement phases independently
3. **Testing**: Each phase has dedicated integration tests
4. **Documentation**: Update `docs/AI_COMMAND_REFERENCE.md` as you go

---

## Priority Order

**Week 1**: Phase 1 (Routing Graph) - Biggest architectural gap  
**Week 2**: Phase 2 (Plugin State) - Most painful current limitation  
**Week 3**: Phase 3 (Clip DSP) - Enables professional editing  
**Week 4**: Phase 4 (AI Feedback) - Reduces AI "blindness"  
**Bonus**: Phase 5 (Dispatch Cleanup) - Code quality improvement
