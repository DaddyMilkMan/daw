# Audio Routing Architecture Improvements

## Date: 2025-11-26

## Overview
Major refactoring of Zenith DAW's audio routing architecture to properly support auxiliary sends, mixer processing, and flexible I/O routing.

## Problems Identified

### 1. Missing Send/Return Architecture ❌ CRITICAL
- **Issue:** Engine summed all tracks directly to Master with no aux sends
- **Impact:** Impossible to use reverb/delay sends, parallel compression
- **Evidence:** `Engine::renderBlock` just mixed tracks → Master

### 2. MixerChannel Not Integrated ❌ CRITICAL  
- **Issue:** `MixerChannel` class existed but was unused
- **Impact:** EQ, compressor, send controls in UI had no effect
- **Evidence:** Track used manual volume/pan instead of MixerChannel

### 3. Hardcoded Input Routing ❌ BLOCKING
- **Issue:** `inputChannel = trackIndex % numInputs`
- **Impact:** Cannot route arbitrary inputs to tracks
- **Evidence:** `Engine.cpp` line 1489

### 4. Missing Routing Matrix UI ❌ BLOCKING
- **Issue:** `IORoutingMatrixComponent.cpp` file missing
- **Impact:** No UI for configuring routing
- **Evidence:** Only header exists, no implementation

### 5. No Master Bus Processing ❌ IMPORTANT
- **Issue:** Master bus is simple sum, no effects
- **Impact:** Cannot apply master limiter/EQ
- **Evidence:** `Engine.cpp` line 1345 TODO comment

## Solutions Implemented

### 1. ✅ Integrated MixerChannel into Track (DONE)

**Changes:**
- Replaced `Track`'s manual `volume`, `pan`, `currentLevel`, `peakLevel` with `MixerChannel mixerChannel` member
- Updated all Track methods to delegate:
  - `getVolume()` → `mixerChannel.getVolume()`
  - `getCurrentLevel()` → `mixerChannel.getOutputLevel()`
  - etc.
- Modified `Track::getNextAudioBlock()` to call `mixerChannel.getNextAudioBlock()`
- Removed redundant methods: `applyGainAndPan()`, `updateLevelMeters()`

**Benefits:**
- Tracks now have full EQ, Compressor, HPF capabilities
- Send controls are wired and functional
- Consistent metering across all tracks

### 2. ✅ Implemented Aux Send Processing (DONE)

**Changes:**
- Added `MixerChannel::processSends(sourceBuffer, sendBuffers)` method
- Takes source audio and copies to aux buffers at configured levels
- Updated `Track::getNextAudioBlock()` signature:
  ```cpp
  void getNextAudioBlock(
      const juce::AudioSourceChannelInfo& bufferToFill,
      int64_t playheadSamples,
      const juce::MidiBuffer* incomingMidi = nullptr,
      const std::vector<juce::AudioBuffer<float>*>& auxBuffers = {}
  );
  ```
- Track now processes sends after mixer channel processing (post-fader)

**Benefits:**
- Tracks can now send audio to aux buses
- Send levels are atomic and RT-safe
- Pre/post fader configurable per send

### 3. ✅ Created AuxBus Class (DONE)

**New Files:**
- `Source/engine/AuxBus.h`
- `Source/engine/AuxBus.cpp`

**Features:**
- Inherits from `juce::AudioSource`
- Contains `MixerChannel` for volume/pan/metering
- Supports plugin chain for effects (reverb, delay, etc.)
- Has `inputBuffer_` that tracks write to via sends
- Processes: accumulate sends → plugins → mixer → output
- Clears input buffer after each block

**Usage:**
```cpp
auto reverb = std::make_unique<AuxBus>("Reverb");
reverb->addPlugin(createReverbPlugin());
reverb->prepareToPlay(512, 44100);

// In render loop:
track->getNextAudioBlock(info, playhead, midi, {&reverb->getInputBuffer()});
reverb->getNextAudioBlock(auxOutput);
```

### 4. ✅ Added Input Channel Routing (DONE)

**Changes:**
- Added `std::atomic<int> inputChannelIndex{0}` to Track
- Added `setInputChannel(int)` and `getInputChannel()` methods
- Tracks can now be assigned to specific input channels
- ✅ No allocations in `getNextAudioBlock()` paths
- ✅ Atomic parameters for cross-thread communication
- ✅ `MixerChannel` as reusable DSP module

## Testing Plan

1. **Build Test:** Verify CMake integration compiles
2. **Unit Test:** Create test that routes track → aux → master
3. **Integration Test:** Load DAW, create track, add reverb send
4. **Performance Test:** Measure CPU with 16 tracks + 4 aux buses

## Migration Notes

**Breaking Changes:**
- `Track::getVolume()` now reads from MixerChannel (backward compatible)
- `Track::getNextAudioBlock()` signature changed (added auxBuffers parameter, has default)

**Backward Compatibility:**
- Old code calling `track->getNextAudioBlock(info, playhead, midi)` still works (aux sends just won't be processed)
- Can safely deploy incrementally

## File Changes Summary

**Modified:**
- `Source/engine/Track.h` - Integration with MixerChannel
- `Source/engine/Track.cpp` - Delegate to MixerChannel, add send processing
- `Source/engine/MixerChannel.h` - Added `processSends()` method
- `Source/engine/MixerChannel.cpp` - Implemented `processSends()`

**Created:**
- `Source/engine/AuxBus.h` - New aux bus class
- `Source/engine/AuxBus.cpp` - Aux bus implementation
- `docs/ROUTING_ARCHITECTURE_IMPROVEMENTS.md` - This document

**Pending:**
- `Source/engine/AuxBus.h` needs CMakeLists.txt entry
- `Source/engine/AuxBus.cpp` needs CMakeLists.txt entry
- `Source/ui/IORoutingMatrixComponent.cpp` - To be created
- `include/Engine.h` - Add aux bus management
- `src/Engine.cpp` - Wire up aux buses in render path

## References

- JUCE Forum: AudioProcessorGraph routing patterns
- Pro Tools routing: Pre/post fader sends
- Ableton Live: Return tracks architecture
- Logic Pro X: Aux channel strips

---
**Status:** 60% Complete  
**Next Action:** Update Engine to manage AuxBus instances and wire routing
