# API Coverage Analysis
## Current State vs. Desired State

**Date**: 2025-12-03  
**Analysis**: CommandAPI feature completeness

---

## 1. Mixer Control

### ✅ Currently Exposed
```json
{
  "command": "set_track_volume",
  "params": { "trackId": "track_1", "volumeDb": -6.0 }
}

{
  "command": "set_track_pan",
  "params": { "trackId": "track_1", "pan": -0.5 }
}
```

### ❌ Missing (But Implemented in MixerChannel.h)
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

{
  "command": "set_track_eq",
  "params": {
    "trackId": "track_1",
    "band": 0,
    "frequency": 1000.0,
    "gain": 3.0,
    "q": 0.707
  }
}

{
  "command": "set_track_compressor",
  "params": {
    "trackId": "track_1",
    "threshold": -10.0,
    "ratio": 4.0,
    "attack": 10.0,
    "release": 100.0
  }
}
```

**Evidence**: 
- `MixerChannel.h:116` - `void setSendLevel(int sendIndex, float level)`
- `MixerChannel.h:79` - `struct EQBand` with 4 bands
- `MixerChannel.h:94` - Full compressor implementation

**Impact**: AI cannot create reverb buses, setup sidechains, or use built-in EQ/comp

---

## 2. Routing & Signal Flow

### ✅ Currently Exposed
```json
// NOTHING - Tracks exist in isolation
```

### ❌ Missing (Core Architecture Gap)
```json
{
  "command": "create_bus",
  "params": {
    "name": "Reverb Bus",
    "type": "stereo"
  }
}

{
  "command": "connect_nodes",
  "params": {
    "source": "track_1_output",
    "destination": "bus_reverb_input"
  }
}

{
  "command": "get_routing_graph",
  "params": {}
}
// Returns: { nodes: [...], connections: [...] }
```

**Evidence**: No routing graph system exists
**Impact**: Cannot create sends/returns, aux buses, sidechain routing, track groups

---

## 3. Plugin Control

### ✅ Currently Exposed
```json
{
  "command": "set_plugin_param",
  "params": {
    "trackId": "track_1",
    "pluginIndex": 0,
    "paramIndex": 42,    // ← AI must GUESS this
    "value": 0.5
  }
}

{
  "command": "get_plugin_params",
  "params": {
    "trackId": "track_1",
    "pluginIndex": 0
  }
}
// Returns: [{ name: "Volume", index: 0, value: 0.5 }, ...]
```

### ❌ Missing (Critical Gap)
```json
{
  "command": "set_plugin_param_by_name",
  "params": {
    "trackId": "track_1",
    "pluginIndex": 0,
    "paramName": "warpMode",  // ← Use name, not index
    "value": 0.5
  }
}

{
  "command": "get_plugin_state_chunk",
  "params": {
    "trackId": "track_1",
    "pluginIndex": 0
  }
}
// Returns: { state: "VkVTVDAwMDEAA..." (base64 blob) }

{
  "command": "set_plugin_state_chunk",
  "params": {
    "trackId": "track_1",
    "pluginIndex": 0,
    "state": "VkVTVDAwMDEAA..."
  }
}
```

**Evidence**: 
- `CommandAPI.cpp:1155` - Only supports `paramIndex`, not `paramName`
- No state chunk handling exists

**Impact**: 
- AI must reverse-engineer parameter indices
- Cannot load presets (must set 50+ params individually)
- Cannot save/restore exact plugin states

---

## 4. Clip Editing

### ✅ Currently Exposed
```json
{
  "command": "split_clip",
  "params": { "trackId": "track_1", "clipId": "clip_0", "splitSamples": 44100 }
}

{
  "command": "move_clip",
  "params": { "trackId": "track_1", "clipId": "clip_0", "newPosition": 88200 }
}

{
  "command": "resize_clip",
  "params": { "trackId": "track_1", "clipId": "clip_0", "newLengthSamples": 176400 }
}
```

### ❌ Missing (Audio DSP Gap)
```json
{
  "command": "set_clip_gain",
  "params": {
    "trackId": "track_1",
    "clipId": "clip_0",
    "gain": -3.0  // dB, separate from track gain
  }
}

{
  "command": "set_clip_fade",
  "params": {
    "trackId": "track_1",
    "clipId": "clip_0",
    "fadeIn": { "lengthSamples": 2205, "curve": "exponential" },
    "fadeOut": { "lengthSamples": 4410, "curve": "linear" }
  }
}

{
  "command": "set_clip_pitch",
  "params": {
    "trackId": "track_1",
    "clipId": "clip_0",
    "pitchShiftSemitones": 5.0  // +5 semitones
  }
}

{
  "command": "set_clip_time_stretch",
  "params": {
    "trackId": "track_1",
    "clipId": "clip_0",
    "timeStretchRatio": 0.5  // Half speed, maintain pitch
  }
}
```

**Evidence**: `Track.h` Clip class has no DSP parameters
**Impact**: 
- All clip boundaries pop/click (no fades)
- Cannot transpose audio
- Cannot fit loops to tempo
- Cannot adjust clip gain independently

---

## 5. AI Feedback

### ✅ Currently Exposed
```json
{
  "command": "get_session_graph",
  "params": {}
}
// Returns: { tracks: [...], tempo: 120, ... } (text summary)
```

### ❌ Missing (AI Blindness Gap)
```json
{
  "command": "get_ui_state",
  "params": {}
}
// Returns: { 
//   currentView: "arranger",
//   selectedTracks: ["track_1"],
//   viewportStart: 0,
//   viewportEnd: 192000,
//   zoomLevel: 1.5
// }

{
  "command": "analyze_track_audio",
  "params": { "trackId": "track_1" }
}
// Returns: {
//   waveformPng: "base64_image_data",
//   rmsDb: -18.5,
//   peakDb: -6.2,
//   spectralCentroid: 2500.0
// }

{
  "command": "render_spectrogram",
  "params": { "trackId": "track_1", "width": 512, "height": 256 }
}
// Returns: { spectrogramPng: "base64_image_data" }
```

**Evidence**: 
- `GrokDAWController.cpp` only receives text summary
- No visual/audio analysis exported

**Impact**:
- AI cannot "see" waveforms
- AI cannot "hear" spectral content
- AI doesn't know what user is looking at
- Flying blind based on text description

---

## Coverage Scorecard

| Category | Implemented | Exposed to AI | Missing | % Coverage |
|----------|-------------|---------------|---------|-----------|
| **Mixer** | 15 capabilities | 2 commands | 13 | 13% |
| **Routing** | 0 capabilities | 0 commands | 5+ | 0% |
| **Plugins** | 5 capabilities | 2 commands | 3 | 40% |
| **Clips** | 3 capabilities | 3 commands | 5 | 38% |
| **Feedback** | 1 capability | 1 command | 3+ | 25% |
| **TOTAL** | 24 capabilities | 8 commands | 29+ | **21%** |

**Conclusion**: The AI has access to ~21% of the DAW's implemented functionality. The remaining 79% is "dark matter" - it exists in the codebase but is invisible to the AI.

---

## Quick Wins (Low-Hanging Fruit)

These are already implemented in the engine but just need CommandAPI wrappers:

### 1. Expose MixerChannel Sends (30 minutes)
```cpp
// CommandAPI.cpp - Add wrapper
juce::var CommandAPI::setTrackSend(const juce::var& params) {
    auto* track = findTrackById(params["trackId"]);
    int sendIndex = params["sendIndex"];
    float level = params["level"];
    track->getMixerChannel().setSendLevel(sendIndex, level);
    return createSuccessResponse(juce::var());
}
```

### 2. Expose EQ Bands (30 minutes)
```cpp
juce::var CommandAPI::setTrackEQ(const juce::var& params) {
    auto* track = findTrackById(params["trackId"]);
    int band = params["band"];
    auto& eqBand = track->getMixerChannel().getEQBand(band);
    eqBand.frequency = params["frequency"];
    eqBand.gain = params["gain"];
    eqBand.q = params["q"];
    return createSuccessResponse(juce::var());
}
```

### 3. Expose Compressor (30 minutes)
```cpp
juce::var CommandAPI::setTrackCompressor(const juce::var& params) {
    auto* track = findTrackById(params["trackId"]);
    auto& mixer = track->getMixerChannel();
    mixer.setCompressorThreshold(params["threshold"]);
    mixer.setCompressorRatio(params["ratio"]);
    mixer.setCompressorAttack(params["attack"]);
    mixer.setCompressorRelease(params["release"]);
    return createSuccessResponse(juce::var());
}
```

**Total Quick Win Time**: 90 minutes  
**Coverage Boost**: 21% → 35%

---

## Architecture Gaps (Deep Work)

These require new systems to be built:

### 1. Routing Graph (Week 1)
- Create `RoutingGraph.h/cpp`
- Implement node connection system
- Add bus creation

### 2. Plugin State Chunks (Week 2)
- Implement base64 state serialization
- Add named parameter access
- Create preset management

### 3. Clip-Level DSP (Week 3)
- Extend Clip struct with DSP params
- Implement fade processing
- Add pitch/time stretch (use rubberband library)

### 4. AI Feedback (Week 4)
- Create UI state capture
- Generate waveform/spectrogram images
- Export audio analysis features

---

## Recommended Implementation Order

**Phase 0 (Day 1)**: Quick Wins
- Expose sends, EQ, compressor
- Boost coverage to 35%
- Immediate AI capability improvement

**Phase 1 (Week 1)**: Routing Graph
- Biggest architectural gap
- Blocks real mixing workflows

**Phase 2 (Week 2)**: Plugin State
- Most painful current limitation
- Enables preset loading

**Phase 3 (Week 3)**: Clip DSP
- Enables professional editing
- Eliminates pops/clicks

**Phase 4 (Week 4)**: AI Feedback
- Reduces "blindness"
- Improves AI decision quality
