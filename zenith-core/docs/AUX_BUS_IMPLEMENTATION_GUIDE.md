# Aux Bus Integration - Implementation Guide

## Overview
This document provides step-by-step instructions for completing the aux bus integration into the Engine class.

## Prerequisites - Already Completed ✅
1. ✅ `AuxBus.h` and `AuxBus.cpp` created
2. ✅ `MixerChannel::processSends()` implemented
3. ✅ `Track::getNextAudioBlock()` updated with aux buffer parameter
4. ✅ Engine.h updated with aux bus management methods

## Remaining Implementation Steps

### Step 1: Add AuxBus Include to Engine.cpp

**File:** `src/Engine.cpp`  
**Location:** After line 16 (after PluginHost.h include)

```cpp
#include "../Source/engine/AuxBus.h"
```

### Step 2: Implement Aux Bus Management Methods

**File:** `src/Engine.cpp`  
**Location:** After `Engine::createTrack()` (around line 875)

```cpp
//==============================================================================
// Aux Bus Management (MESSAGE THREAD ONLY)
//==============================================================================

int Engine::createAuxBus(const juce::String& name)
{
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
  
  DBG("Engine: Creating aux bus '" + name + "'");
  
  auto auxBus = std::make_unique<zenith::AuxBus>(name);
  
  // Prepare the aux bus if audio is already running
  if (currentSampleRate.load() > 0)
  {
    auxBus->prepareToPlay(currentBufferSize.load(), currentSampleRate.load());
  }
  
  auxBuses_.push_back(std::move(auxBus));
  
  const int auxIndex = static_cast<int>(auxBuses_.size()) - 1;
  DBG("Engine: Created aux bus '" + name + "' at index " + juce::String(auxIndex));
  
  return auxIndex;
}

void Engine::removeAuxBus(int auxIndex)
{
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
  
  if (auxIndex >= 0 && auxIndex < static_cast<int>(auxBuses_.size()))
  {
    DBG("Engine: Removing aux bus at index " + juce::String(auxIndex));
    auxBuses_.erase(auxBuses_.begin() + auxIndex);
  }
}

int Engine::getNumAuxBuses() const noexcept
{
  return static_cast<int>(auxBuses_.size());
}

zenith::AuxBus* Engine::getAuxBus(int auxIndex) noexcept
{
  if (auxIndex >= 0 && auxIndex < static_cast<int>(auxBuses_.size()))
  {
    return auxBuses_[auxIndex].get();
  }
  return nullptr;
}

float Engine::getAuxBusLevel(int auxIndex) const
{
  if (auxIndex >= 0 && auxIndex < static_cast<int>(auxBuses_.size()))
  {
    return auxBuses_[auxIndex]->getCurrentLevel();
  }
  return 0.0f;
}

float Engine::getAuxBusPeakLevel(int auxIndex) const
{
  if (auxIndex >= 0 && auxIndex < static_cast<int>(auxBuses_.size()))
  {
    return auxBuses_[auxIndex]->getPeakLevel();
  }
  return 0.0f;
}
```

### Step 3: Update audioDeviceAboutToStart

**File:** `src/Engine.cpp`  
**Location:** In `Engine::audioDeviceAboutToStart()`, after track preparation (around line 853)

**Add this code block:**

```cpp
  // Prepare aux buses
  auxBusBuffers_.clear();
  auxBusBuffers_.resize(auxBuses_.size());

  for (size_t i = 0; i < auxBuses_.size(); ++i)
  {
    // Allocate stereo buffer for each aux bus
    auxBusBuffers_[i].setSize(2, bufferSize);
    auxBusBuffers_[i].clear();

    // Prepare aux bus for playback
    if (auxBuses_[i] != nullptr)
    {
      auxBuses_[i]->prepareToPlay(bufferSize, currentSampleRate.load());
    }
  }

  DBG("Engine: Aux Bus Buffers: " + juce::String(auxBusBuffers_.size()));
```

### Step 4: Update audioDeviceStopped

**File:** `src/Engine.cpp`  
**Location:** In `Engine::audioDeviceStopped()`, after track cleanup (around line 863)

**Add this code:**

```cpp
  // Release aux bus resources
  for (auto& auxBus : auxBuses_)
  {
    if (auxBus != nullptr)
    {
      auxBus->releaseResources();
    }
  }
```

### Step 5: Update renderBlock to Process Aux Buses

**File:** `src/Engine.cpp`  
**Location:** In `Engine::renderBlock()`, after track processing loop (around line 1343)

**Replace the final output mixing section with:**

```cpp
    // Mix track buffer into output buffer
    for (int channel = 0; channel < juce::jmin(outputBuffer.getNumChannels(),
                                                trackBuffer.getNumChannels()); ++channel)
    {
      outputBuffer.addFrom(channel, 0,
                           trackBuffer.getReadPointer(channel),
                           numSamples);
    }
  }

  // ============================================================================
  // Process Aux Buses
  // ============================================================================

  // Clear all aux bus input buffers
  for (auto& auxBuffer : auxBusBuffers_)
  {
    auxBuffer.clear();
  }

  // Build list of aux buffer pointers for sends
  std::vector<juce::AudioBuffer<float>*> auxBufferPtrs;
  for (auto& auxBuffer : auxBusBuffers_)
  {
    auxBufferPtrs.push_back(&auxBuffer);
  }

  // Re-render tracks WITH aux sends this time
  for (size_t trackIdx = 0; trackIdx < tracks_.size(); ++trackIdx)
  {
    if (trackIdx >= trackBuffers_.size())
      continue;

    auto& trackBuffer = trackBuffers_[trackIdx];

    if (trackBuffer.getNumSamples() < numSamples)
      continue;

    // Clear track buffer
    trackBuffer.clear();

    // Create AudioSourceChannelInfo for the track
    juce::AudioSourceChannelInfo trackInfo(&trackBuffer, 0, numSamples);

    // Get the track pointer
    auto* track = tracks_[trackIdx].get();

    // Determine MIDI input (same logic as before)
    const juce::MidiBuffer* trackMidiInput = nullptr;
    if (incomingMidi != nullptr && !incomingMidi->isEmpty() &&
        track->getType() == zenith::Track::Type::Instrument &&
        track->isArmed())
    {
      trackMidiInput = incomingMidi;
    }

    // Render track audio WITH aux sends
    track->getNextAudioBlock(trackInfo, playheadPosition, trackMidiInput, auxBufferPtrs);

    // Don't add to output - we already did that in first pass
  }

  // Process each aux bus and mix into output
  for (size_t i = 0; i < auxBuses_.size(); ++i)
  {
    auto* auxBus = auxBuses_[i].get();
    if (auxBus == nullptr || auxBus->isMuted())
      continue;

   // Get input buffer that was filled by track sends
    auto& auxInputBuffer = auxBusBuffers_[i];

    // Process aux bus (applies effects, volume, pan)
    juce::AudioSourceChannelInfo auxInfo(&auxInputBuffer, 0, numSamples);
    auxBus->getNextAudioBlock(auxInfo);

    // Mix aux output into master
    for (int ch = 0; ch < juce::jmin(outputBuffer.getNumChannels(),
                                     auxInputBuffer.getNumChannels()); ++ch)
    {
      outputBuffer.addFrom(ch, 0, auxInputBuffer, ch, 0, numSamples);
    }
  }
```

### Step 6: Fix processAudioRecording to Use Track Input Routing

**File:** `src/Engine.cpp`  
**Location:** In `Engine::processAudioRecording()` (around line 1489)

**Replace the hardcoded input channel selection:**

```cpp
// OLD CODE (around line 1489):
const int inputChannel = session.trackIndex % numInputChannels;

// NEW CODE:
// Get track's configured input channel
int inputChannel = 0;
if (session.trackIndex >= 0 && session.trackIndex < static_cast<int>(tracks_.size()))
{
  inputChannel = tracks_[session.trackIndex]->getInputChannel();
}

// Clamp to valid input range
if (inputChannel >= numInputChannels)
{
  inputChannel = 0;  // Fallback to first input
}
```

### Step 7: Add Aux Buses to CMakeLists.txt

**File:** `zenith-core/CMakeLists.txt`  
**Location:** In the `ZENITH_CORE_SOURCES` list (around line with Track.cpp, MixerChannel.cpp)

```cmake
    Source/engine/AuxBus.cpp
```

## Testing Plan

### Unit Test - Create and Test Aux Bus

```cpp
// In a test file
TEST_CASE("Aux Bus Creation and Routing")
{
  Engine engine;
  engine.initialize();
  
  // Create a track
  auto trackId = engine.createTrack("Test Track", "audio");
  REQUIRE(engine.getNumTracks() == 1);
  
  // Create an aux bus
  int auxIdx = engine.createAuxBus("Reverb");
  REQUIRE(engine.getNumAuxBuses() == 1);
  
  auto* auxBus = engine.getAuxBus(auxIdx);
  REQUIRE(auxBus != nullptr);
  REQUIRE(auxBus->getName() == "Reverb");
  
  // Set send level
  auto* track = engine.tracks()[0].get();
  track->getMixerChannel().setSendLevel(0, 0.5f);  // 50% to aux 1
  
  // Verify
  REQUIRE(track->getMixerChannel().getSendLevel(0) == Approx(0.5f));
}
```

### Integration Test - Full Signal Flow

```cpp
TEST_CASE("Track -> Aux -> Master Signal Flow")
{
  Engine engine;
  engine.setProjectState(&projectState);
  engine.initialize();
  
  // Create track with audio clip
  auto trackId = engine.createTrack("Drums", "audio");
  // ... add audio clip ...
  
  // Create reverb aux
  int reverbIdx = engine.createAuxBus("Reverb");
  
  // Set 30% send to reverb
  engine.tracks()[0]->getMixerChannel().setSendLevel(0, 0.3f);
  
  // Process one block
  float outputData[2][512] = {};
  float* outputPtrs[2] = { outputData[0], outputData[1] };
  engine.processAudio(nullptr, 0, outputPtrs, 2, 512);
  
  // Verify output is not silent
  bool hasSignal = false;
  for (int i = 0; i < 512; ++i)
  {
    if (std::abs(outputData[0][i]) > 0.001f)
    {
      hasSignal = true;
      break;
    }
  }
  REQUIRE(hasSignal);
}
```

## Validation Checklist

- [ ] CMakeLists.txt updated with AuxBus.cpp
- [ ] Engine.cpp includes AuxBus.h
- [ ] Aux bus management methods implemented
- [ ] audioDeviceAboutToStart allocates aux buffers
- [ ] audioDeviceStopped releases aux resources
- [ ] renderBlock processes aux buses and mixes to master
- [ ] processAudioRecording uses track->getInputChannel()
- [ ] Unit tests pass
- [ ] Integration test shows signal flow working
- [ ] No crashes during playback
- [ ] CPU usage acceptable with 4 aux buses

## Common Issues and Solutions

### Issue 1: Aux bus output is silent
**Cause:** Send levels are 0, or aux bus is muted  
**Solution:** Check `MixerChannel::getSendLevel(0)` returns > 0.0f

### Issue 2: Doubled audio in master
**Cause:** Track audio added twice (once in first pass, once with sends)  
**Solution:** Only add track audio in first pass, second pass just fills aux buffers

### Issue 3: Clicks/pops in aux bus output
**Cause:** Buffer size mismatch or uninitialized memory  
**Solution:** Verify aux buffers are cleared before each render block

### Issue 4: Crash when removing aux bus
**Cause:** Tracks still referencing aux bus index  
**Solution:** When removing aux bus, reset all tracks' send levels for that index

## Performance Considerations

- **Pre-allocate everything:** All aux buffers allocated in `audioDeviceAboutToStart()`
- **No heap allocations in audio thread:** All processing uses pre-allocated buffers
- **Minimize copies:** Use `addFrom()` to mix, not copy+add
- **Skip muted buses:** Check `aux Bus->isMuted()` before processing

## Next Steps After Completion

1. Create UI for aux bus visualization in mixer
2. Implement pre/post fader send switching
3. Add aux bus solo/mute to IORoutingMatrixComponent
4. Support more than 4 sends per track (configurable)
5. Add aux bus insert plugin support (already works via AuxBus::addPlugin)

---

**Document Status:** Ready for Implementation  
**Estimated Time:** 2-3 hours
**Risk Level:** Medium (affects core audio rendering)
