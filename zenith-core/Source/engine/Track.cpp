/*
  ==============================================================================

    Track.cpp
    Ported from: ZenithDAW-Native/Source/Audio/Track.cpp (2025-11-11)
    Author:  Zenith DAW → Zenith DAW

    Audio/MIDI track implementation

    JUCE 8 / C++20 adaptations:
    - Wrapped in namespace zenith
    - OwnedArray<Clip> → std::vector<std::unique_ptr<Clip>>
    - Plugin hosting stubbed for Phase 2

  ==============================================================================
*/

#include "Track.h"
#include "../instruments/Instrument.h"
#include "Clip.h"
#include "PluginHost.h"
#include <algorithm>


namespace zenith {

//==============================================================================
Track::Track(const juce::String &name, Type type)
    : trackName(name), trackType(type) {
  // Initialize empty clip snapshot
  clipsSnapshot_ = std::make_shared<const ClipSnapshot>();
}

Track::~Track() {
  // Ensure we're not in the middle of audio processing
  const juce::ScopedLock sl1(pluginLock);

  // Phase 2A: Clips are automatically destroyed via std::unique_ptr
  // No lock needed - destructor is called from message thread only
  clipsOwned_.clear();
  clipsSnapshot_ = std::make_shared<const ClipSnapshot>();
}

//==============================================================================
void Track::setInstrument(std::unique_ptr<Instrument> instrument) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  // Release old instrument if present
  if (instrument_ != nullptr) {
    auto *processor = instrument_->getAudioProcessor();
    if (processor != nullptr) {
      processor->releaseResources();
    }
  }

  // Set new instrument
  instrument_ = std::move(instrument);

  // Prepare new instrument if audio is running
  if (instrument_ != nullptr && currentSampleRate > 0) {
    auto *processor = instrument_->getAudioProcessor();
    if (processor != nullptr) {
      processor->setPlayConfigDetails(0, 2, currentSampleRate,
                                      currentBlockSize);
      processor->prepareToPlay(currentSampleRate, currentBlockSize);
    }
  }

  sendChangeMessage();
}

//==============================================================================
void Track::prepareToPlay(int samplesPerBlockExpected, double sampleRate) {
  currentSampleRate = sampleRate;
  currentBlockSize = samplesPerBlockExpected;

  // Prepare instrument buffer (fixed size, no reallocation on audio thread)
  instrumentBuffer_.setSize(2, samplesPerBlockExpected, false, true, true);

  // Phase 1: Pre-allocate clip buffer to avoid RT allocations
  clipBuffer_.setSize(2, samplesPerBlockExpected, false, true, true);

  // Prepare plugin buffer
  pluginBuffer.setSize(2, samplesPerBlockExpected);

  // Prepare instrument if present
  if (instrument_ != nullptr) {
    auto *processor = instrument_->getAudioProcessor();
    if (processor != nullptr) {
      processor->setPlayConfigDetails(0, 2, sampleRate,
                                      samplesPerBlockExpected);
      processor->prepareToPlay(sampleRate, samplesPerBlockExpected);
    }
  }

  // Phase 3: Prepare all plugins
  {
    const juce::ScopedLock sl(pluginLock);
    for (auto &plugin : plugins) {
      if (plugin != nullptr) {
        plugin->prepareToPlay(sampleRate, samplesPerBlockExpected);
        plugin->setNonRealtime(false);
      }
    }
  }

  // Phase 2A: Prepare all clips (message thread only, no lock needed)
  for (auto &clip : clipsOwned_) {
    if (clip != nullptr) {
      clip->prepareToPlay(samplesPerBlockExpected, sampleRate);
    }
  }
}

void Track::releaseResources() {
  // Release instrument if present
  if (instrument_ != nullptr) {
    auto *processor = instrument_->getAudioProcessor();
    if (processor != nullptr) {
      processor->releaseResources();
    }
  }

  // Phase 3: Release all plugins
  {
    const juce::ScopedLock sl(pluginLock);
    for (auto &plugin : plugins) {
      if (plugin != nullptr) {
        plugin->releaseResources();
      }
    }
  }

  // Phase 2A: Release all clips (message thread only, no lock needed)
  for (auto &clip : clipsOwned_) {
    if (clip != nullptr) {
      clip->releaseResources();
    }
  }
}

// Phase 1.3 / 2A: Process with explicit playhead position (lock-free)
void Track::getNextAudioBlock(const juce::AudioSourceChannelInfo &bufferToFill,
                              int64_t playheadSamples,
                              const juce::MidiBuffer *incomingMidi) {
  // Clear the buffer first
  bufferToFill.clearActiveBufferRegion();

  // If track is disabled or muted, return silence
  if (!enabled.load() || muted.load()) {
    currentLevel.store(0.0f);
    midiBuffer_.clear(); // Phase 2A: Clear MIDI buffer too
    return;
  }

  // Phase 2A: Clear MIDI buffer for this block
  midiBuffer_.clear();

  // Add incoming MIDI (e.g. from external controller)
  if (incomingMidi != nullptr && !incomingMidi->isEmpty()) {
    midiBuffer_.addEvents(*incomingMidi, 0, bufferToFill.numSamples, 0);
  }

  // Phase 2A: Get current clip snapshot (RT-safe atomic load, no lock!)
  std::shared_ptr<const ClipSnapshot> currentSnapshot;
  {
    const juce::SpinLock::ScopedLockType sl(snapshotLock);
    currentSnapshot = clipsSnapshot_;
  }

  if (currentSnapshot) {
    // Iterate clips from snapshot (no lock needed!)
    for (auto *clip : currentSnapshot->clips) {
      if (clip != nullptr && clip->isPlaying() &&
          clip->isActiveAt(playheadSamples)) {
        if (clip->getType() == Clip::Type::Audio) {
          // Phase 1: Use pre-allocated clipBuffer_ to avoid RT allocations
          clipBuffer_.clear();

          juce::AudioSourceChannelInfo clipInfo(&clipBuffer_, 0,
                                                bufferToFill.numSamples);

          // Phase 1.3: Pass playhead to clip for timing
          clip->processAudioClip(clipInfo, playheadSamples);

          // Mix clip into main buffer
          const int channelsToMix =
              juce::jmin(bufferToFill.buffer->getNumChannels(),
                         clipBuffer_.getNumChannels());

          for (int ch = 0; ch < channelsToMix; ++ch) {
            bufferToFill.buffer->addFrom(ch, bufferToFill.startSample,
                                         clipBuffer_, ch, 0,
                                         bufferToFill.numSamples);
          }
        } else if (clip->getType() == Clip::Type::MIDI) {
          // Phase 2A: Process MIDI clip into track's MIDI buffer
          clip->processMidiClip(midiBuffer_, playheadSamples,
                                bufferToFill.numSamples);
        }
      }
    }
  }

  // Create a local buffer reference for processing
  juce::AudioBuffer<float> localBuffer(
      bufferToFill.buffer->getArrayOfWritePointers(),
      bufferToFill.buffer->getNumChannels(), bufferToFill.startSample,
      bufferToFill.numSamples);

  // Process instrument if present (for Instrument tracks)
  if (instrument_ != nullptr && trackType == Type::Instrument) {
    auto *processor = instrument_->getAudioProcessor();
    if (processor != nullptr) {
      // Use preallocated buffer (RT-safe, no reallocation)
      // instrumentBuffer_ was sized in prepareToPlay
      const int numSamples = bufferToFill.numSamples;

      // Clear instrument buffer for this block
      instrumentBuffer_.clear();

      // Process instrument (RT-safe as long as numSamples <= currentBlockSize)
      processor->processBlock(instrumentBuffer_, midiBuffer_);

      // Mix instrument output into track buffer
      for (int ch = 0; ch < juce::jmin(localBuffer.getNumChannels(),
                                       instrumentBuffer_.getNumChannels());
           ++ch) {
        localBuffer.addFrom(ch, 0, instrumentBuffer_, ch, 0, numSamples);
      }

      // Clear MIDI buffer for next block
      midiBuffer_.clear();
    }
  }

  // Process through plugin chain with MIDI support
  processPluginChain(localBuffer, midiBuffer_, bufferToFill.numSamples);

  // Apply volume and pan
  applyGainAndPan(localBuffer, bufferToFill.numSamples);

  // Update level meters
  updateLevelMeters(localBuffer, bufferToFill.numSamples);
}

// Legacy overload: uses default playhead of 0
void Track::getNextAudioBlock(
    const juce::AudioSourceChannelInfo &bufferToFill) {
  getNextAudioBlock(bufferToFill, 0);
}

//==============================================================================
void Track::setName(const juce::String &newName) {
  trackName = newName;
  sendChangeMessage();
}

juce::String Track::getTypeString() const {
  switch (trackType) {
  case Type::Audio:
    return "Audio";
  case Type::MIDI:
    return "MIDI";
  case Type::Instrument:
    return "Instrument";
  default:
    return "Unknown";
  }
}

//==============================================================================
void Track::setVolume(float newVolume) {
  volume.store(juce::jlimit(0.0f, 1.0f, newVolume));
  sendChangeMessage();
}

void Track::setPan(float newPan) {
  pan.store(juce::jlimit(-1.0f, 1.0f, newPan));
  sendChangeMessage();
}

void Track::setMuted(bool shouldBeMuted) {
  muted.store(shouldBeMuted);
  sendChangeMessage();
}

void Track::setSolo(bool shouldBeSolo) {
  solo.store(shouldBeSolo);
  sendChangeMessage();
}

void Track::setArmed(bool shouldBeArmed) {
  armed.store(shouldBeArmed);
  sendChangeMessage();
}

void Track::setEnabled(bool shouldBeEnabled) {
  enabled.store(shouldBeEnabled);
  sendChangeMessage();
}

//==============================================================================
// Plugin chain management (Phase 3: VST3 hosting MVP)
//==============================================================================

void Track::addPlugin(std::unique_ptr<juce::AudioPluginInstance> plugin) {
  if (plugin == nullptr)
    return;

  const juce::ScopedLock sl(pluginLock);

  // Prepare the plugin if we're already initialized
  if (currentSampleRate > 0) {
    plugin->prepareToPlay(currentSampleRate, currentBlockSize);
    plugin->setNonRealtime(false);
  }

  plugins.push_back(std::move(plugin));
  sendChangeMessage();
}

void Track::removePlugin(int pluginIndex) {
  const juce::ScopedLock sl(pluginLock);

  if (pluginIndex >= 0 && pluginIndex < static_cast<int>(plugins.size())) {
    auto &plugin = plugins[pluginIndex];
    if (plugin != nullptr) {
      plugin->releaseResources();
    }
    plugins.erase(plugins.begin() + pluginIndex);
    sendChangeMessage();
  }
}

void Track::clearPlugins() {
  const juce::ScopedLock sl(pluginLock);

  for (auto &plugin : plugins) {
    if (plugin != nullptr) {
      plugin->releaseResources();
    }
  }

  plugins.clear();
  sendChangeMessage();
}

int Track::getNumPlugins() const {
  const juce::ScopedLock sl(pluginLock);
  return static_cast<int>(plugins.size());
}

juce::AudioPluginInstance *Track::getPlugin(int index) const {
  const juce::ScopedLock sl(pluginLock);
  if (index >= 0 && index < static_cast<int>(plugins.size()))
    return plugins[index].get();
  return nullptr;
}

//==============================================================================
// Phase 2A: Lock-free clip management (message thread only)
//==============================================================================

void Track::updateClipSnapshot() {
  // Called from message thread only - no lock needed
  // Create new snapshot from current ownership
  auto newSnapshot = std::make_shared<const ClipSnapshot>(clipsOwned_);

  // Atomically swap snapshot (audio thread will see new snapshot on next load)
  {
    const juce::SpinLock::ScopedLockType sl(snapshotLock);
    clipsSnapshot_ = newSnapshot;
  }
}

void Track::addClip(std::unique_ptr<Clip> clip) {
  if (clip != nullptr) {
    // Prepare the clip if we're already initialized
    if (currentSampleRate > 0) {
      clip->prepareToPlay(currentBlockSize, currentSampleRate);
    }

    // Add to ownership vector
    clipsOwned_.push_back(std::move(clip));

    // Update snapshot for audio thread
    updateClipSnapshot();

    sendChangeMessage();
  }
}

void Track::removeClip(int clipIndex) {
  if (clipIndex >= 0 && clipIndex < static_cast<int>(clipsOwned_.size())) {
    auto &clip = clipsOwned_[clipIndex];
    if (clip != nullptr) {
      clip->releaseResources();
    }

    // Remove from ownership vector
    clipsOwned_.erase(clipsOwned_.begin() + clipIndex);

    // Update snapshot for audio thread
    updateClipSnapshot();

    sendChangeMessage();
  }
}

void Track::removeClip(Clip *clip) {
  auto it = std::find_if(
      clipsOwned_.begin(), clipsOwned_.end(),
      [clip](const std::unique_ptr<Clip> &c) { return c.get() == clip; });

  if (it != clipsOwned_.end()) {
    (*it)->releaseResources();

    // Remove from ownership vector
    clipsOwned_.erase(it);

    // Update snapshot for audio thread
    updateClipSnapshot();

    sendChangeMessage();
  }
}

void Track::clearClips() {
  for (auto &clip : clipsOwned_) {
    if (clip != nullptr) {
      clip->releaseResources();
    }
  }

  // Clear ownership vector
  clipsOwned_.clear();

  // Update snapshot for audio thread
  updateClipSnapshot();

  sendChangeMessage();
}

int Track::getNumClips() const {
  // Message thread access - read ownership vector directly
  return static_cast<int>(clipsOwned_.size());
}

Track::Clip *Track::getClip(int index) const {
  // Message thread access - read ownership vector directly
  if (index >= 0 && index < static_cast<int>(clipsOwned_.size()))
    return clipsOwned_[index].get();
  return nullptr;
}

//==============================================================================
void Track::resetPeakLevel() { peakLevel.store(0.0f); }

//==============================================================================
juce::ValueTree Track::getState() const {
  juce::ValueTree state("Track");

  state.setProperty("name", trackName, nullptr);
  state.setProperty("type", static_cast<int>(trackType), nullptr);
  state.setProperty("volume", volume.load(), nullptr);
  state.setProperty("pan", pan.load(), nullptr);
  state.setProperty("muted", muted.load(), nullptr);
  state.setProperty("solo", solo.load(), nullptr);
  state.setProperty("armed", armed.load(), nullptr);
  state.setProperty("enabled", enabled.load(), nullptr);

  // Phase 3: Save plugin states
  juce::ValueTree pluginsState("Plugins");
  {
    const juce::ScopedLock sl(pluginLock);
    for (auto &plugin : plugins) {
      if (plugin != nullptr) {
        juce::ValueTree pluginState("Plugin");

        // Store plugin identifier
        auto description = plugin->getPluginDescription();
        pluginState.setProperty("identifier",
                                description.createIdentifierString(), nullptr);
        pluginState.setProperty("name", description.name, nullptr);

        // Store plugin state as binary data
        juce::MemoryBlock stateData;
        plugin->getStateInformation(stateData);

        if (stateData.getSize() > 0) {
          pluginState.setProperty("state", stateData.toBase64Encoding(),
                                  nullptr);
        }

        pluginsState.appendChild(pluginState, nullptr);
      }
    }
  }
  state.appendChild(pluginsState, nullptr);

  // Phase 2A: Save clip states (message thread, no lock needed)
  juce::ValueTree clipsState("Clips");
  for (auto &clip : clipsOwned_) {
    if (clip != nullptr) {
      clipsState.appendChild(clip->getState(), nullptr);
    }
  }
  state.appendChild(clipsState, nullptr);

  return state;
}

void Track::loadState(const juce::ValueTree &state) {
  if (!state.hasType("Track"))
    return;

  trackName = state.getProperty("name", "Untitled Track");
  trackType = static_cast<Type>(static_cast<int>(state.getProperty("type", 0)));
  volume.store(state.getProperty("volume", 0.8f));
  pan.store(state.getProperty("pan", 0.0f));
  muted.store(state.getProperty("muted", false));
  solo.store(state.getProperty("solo", false));
  armed.store(state.getProperty("armed", false));
  enabled.store(state.getProperty("enabled", true));

  // Phase 3: Load plugin states
  // NOTE: Plugin loading requires PluginHost to recreate instances.
  // This should be called from Engine/ProjectState level where PluginHost is
  // available. For now, we just clear plugins and document the requirement.
  // TODO(Phase 3+): Add loadPluginState(ValueTree, PluginHost&) method
  auto pluginsState = state.getChildWithName("Plugins");
  if (pluginsState.isValid()) {
    clearPlugins();
    // Plugin recreation needs to happen at Engine level with access to
    // PluginHost See ProjectState integration for proper plugin loading
  }

  // Load clip states
  auto clipsState = state.getChildWithName("Clips");
  if (clipsState.isValid()) {
    clearClips();
    for (int i = 0; i < clipsState.getNumChildren(); ++i) {
      auto clipState = clipsState.getChild(i);
      auto clip = std::make_unique<Clip>();
      clip->loadState(clipState);
      addClip(std::move(clip));
    }
  }

  sendChangeMessage();
}

void Track::loadPluginStates(const juce::ValueTree &state,
                             PluginHost &pluginHost) {
  if (!state.hasType("Track"))
    return;

  auto pluginsState = state.getChildWithName("Plugins");
  if (!pluginsState.isValid())
    return;

  DBG("Track: Loading plugin states for " + trackName);

  // Clear existing plugins
  clearPlugins();

  // Recreate each plugin
  for (int i = 0; i < pluginsState.getNumChildren(); ++i) {
    auto pluginState = pluginsState.getChild(i);

    if (!pluginState.hasType("Plugin"))
      continue;

    // Get plugin identifier
    juce::String identifier = pluginState.getProperty("identifier", "");
    juce::String name = pluginState.getProperty("name", "Unknown");

    if (identifier.isEmpty()) {
      DBG("Track: Skipping plugin with no identifier");
      continue;
    }

    // Try to create plugin instance
    juce::String errorMessage;
    auto instance = pluginHost.createInstance(
        identifier, currentSampleRate > 0 ? currentSampleRate : 44100.0,
        currentBlockSize > 0 ? currentBlockSize : 512, errorMessage);

    if (instance == nullptr) {
      DBG("Track: WARNING - Failed to load plugin '" + name +
          "': " + errorMessage);
      continue;
    }

    // Restore plugin state
    juce::String stateBase64 = pluginState.getProperty("state", "");
    if (stateBase64.isNotEmpty()) {
      juce::MemoryBlock stateData;
      if (stateData.fromBase64Encoding(stateBase64)) {
        instance->setStateInformation(stateData.getData(),
                                      static_cast<int>(stateData.getSize()));
        DBG("Track: Restored state for plugin '" + name + "'");
      }
    }

    // Add to track
    addPlugin(std::move(instance));
    DBG("Track: Loaded plugin '" + name + "'");
  }

  DBG("Track: Loaded " + juce::String(getNumPlugins()) + " plugins");
}

//==============================================================================
void Track::processPluginChain(juce::AudioBuffer<float> &buffer,
                               juce::MidiBuffer &midi, int numSamples) {
  // Phase 3: Process plugin chain
  // RT-SAFE: We only read the plugins vector here, no modifications
  // The pluginLock is only used when adding/removing plugins (message thread)

  if (plugins.empty())
    return;

  // For MVP: Simple linear plugin chain processing
  // Audio tracks: audio in → plugins → audio out
  // Instrument tracks: MIDI in → first plugin (synth) → audio → remaining
  // plugins → audio out

  for (auto &plugin : plugins) {
    if (plugin != nullptr) {
      // CODEX FEEDBACK APPLIED: Use max(inputs, outputs) for channel sizing
      // This ensures instrument plugins (0 inputs, >0 outputs) get writable
      // channels instead of receiving an empty buffer.
      const int bufferChannels = buffer.getNumChannels();
      const int pluginInputs = plugin->getTotalNumInputChannels();
      const int pluginOutputs = plugin->getTotalNumOutputChannels();

      // Use max(inputs, outputs) so instrument plugins (0 in, >0 out) get
      // actual audio
      const int numChannels =
          juce::jmin(bufferChannels, juce::jmax(pluginInputs, pluginOutputs));

      // Create a view of the buffer with the correct number of channels
      juce::AudioBuffer<float> pluginView(buffer.getArrayOfWritePointers(),
                                          numChannels, 0, numSamples);

      // Process this plugin
      // Note: processBlock expects the full buffer, not just a section
      // We're processing in-place
      // MIDI buffer is passed through the chain:
      // - Instrument plugins consume note-on/off events
      // - Effect plugins typically ignore MIDI (but some use it for modulation)
      plugin->processBlock(pluginView, midi);
    }
  }
}

void Track::applyGainAndPan(juce::AudioBuffer<float> &buffer, int numSamples) {
  const float vol = volume.load();
  const float panValue = pan.load();

  // Calculate left and right gains from pan
  // Pan law: -3dB center, constant power
  const float piOver4 = juce::MathConstants<float>::pi / 4.0f;
  const float leftGain = vol * std::cos(piOver4 * (1.0f + panValue));
  const float rightGain = vol * std::sin(piOver4 * (1.0f + panValue));

  if (buffer.getNumChannels() >= 2) {
    // Stereo: apply pan
    buffer.applyGain(0, 0, numSamples, leftGain);
    buffer.applyGain(1, 0, numSamples, rightGain);
  } else if (buffer.getNumChannels() == 1) {
    // Mono: apply volume only
    buffer.applyGain(0, 0, numSamples, vol);
  }
}

void Track::updateLevelMeters(const juce::AudioBuffer<float> &buffer,
                              int numSamples) {
  float maxLevel = 0.0f;

  for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
    const float *channelData = buffer.getReadPointer(ch);
    for (int i = 0; i < numSamples; ++i) {
      const float absValue = std::abs(channelData[i]);
      if (absValue > maxLevel) {
        maxLevel = absValue;
      }
    }
  }

  // Update current level (RMS-style with smoothing)
  const float currentLevelValue = currentLevel.load();
  const float smoothingFactor = 0.3f;
  const float newLevel =
      currentLevelValue * (1.0f - smoothingFactor) + maxLevel * smoothingFactor;
  currentLevel.store(newLevel);

  // Update peak level
  if (maxLevel > peakLevel.load()) {
    peakLevel.store(maxLevel);
  }
}

//==============================================================================
// MIDI Scheduling
//==============================================================================

void Track::generateMidiForBlock(const juce::ValueTree &trackState,
                                 double tempo, double sampleRate,
                                 juce::int64 blockStartSample, int blockSize,
                                 juce::MidiBuffer &midiOut) {
  // Calculate block boundaries in samples
  juce::int64 blockEndSample = blockStartSample + blockSize;

  // Convert to beats
  // Formula: beats = (samples / sampleRate) * (tempo / 60)
  double beatsPerSample = (tempo / 60.0) / sampleRate;
  double blockStartBeats = blockStartSample * beatsPerSample;
  double blockEndBeats = blockEndSample * beatsPerSample;

  // Get CLIPS node from track state
  auto clipsNode = trackState.getChildWithName(juce::Identifier("CLIPS"));
  if (!clipsNode.isValid())
    return;

  // Process each clip
  for (const auto &clip : clipsNode) {
    // Get clip position (in beats or samples - need to check)
    // For now, assume clip.start is in beats
    double clipStartBeats = clip.getProperty("start", 0.0);
    double clipLengthBeats = clip.getProperty("length", 0.0);

    // Skip clips that don't overlap this block
    if (clipStartBeats + clipLengthBeats < blockStartBeats ||
        clipStartBeats > blockEndBeats)
      continue;

    // Get NOTES node from clip
    auto notesNode = clip.getChildWithName(juce::Identifier("NOTES"));
    if (!notesNode.isValid())
      continue;

    // Process each note in the clip
    for (const auto &note : notesNode) {
      // Get note properties
      double noteStartBeats = note.getProperty("startBeats", 0.0);
      double noteLengthBeats = note.getProperty("lengthBeats", 0.0);
      int pitch = note.getProperty("pitch", 60);
      int velocity = note.getProperty("velocity", 100);
      juce::String noteId = note.getProperty("id", "");

      // Convert note times to absolute timeline beats (relative to clip)
      double absNoteStartBeats = clipStartBeats + noteStartBeats;
      double absNoteEndBeats = absNoteStartBeats + noteLengthBeats;

      // Convert to samples
      double samplesPerBeat = sampleRate * 60.0 / tempo;
      juce::int64 noteStartSample =
          static_cast<juce::int64>(absNoteStartBeats * samplesPerBeat);
      juce::int64 noteEndSample =
          static_cast<juce::int64>(absNoteEndBeats * samplesPerBeat);

      // Check if note-on happens in this block
      if (noteStartSample >= blockStartSample &&
          noteStartSample < blockEndSample) {
        // Calculate offset within this block
        int sampleOffset = static_cast<int>(noteStartSample - blockStartSample);

        // Create note-on event
        juce::MidiMessage noteOn = juce::MidiMessage::noteOn(
            1, pitch, static_cast<juce::uint8>(velocity));
        midiOut.addEvent(noteOn, sampleOffset);

        // Track this note as active (with thread synchronization)
        ActiveNote activeNote;
        activeNote.pitch = pitch;
        activeNote.channel = 1;
        activeNote.noteId = noteId;
        {
          const juce::ScopedLock sl(activeNotesLock);
          activeNotes.push_back(activeNote);
        }
      }

      // Check if note-off happens in this block
      if (noteEndSample >= blockStartSample && noteEndSample < blockEndSample) {
        // Calculate offset within this block
        int sampleOffset = static_cast<int>(noteEndSample - blockStartSample);

        // Create note-off event
        juce::MidiMessage noteOff =
            juce::MidiMessage::noteOff(1, pitch, static_cast<juce::uint8>(0));
        midiOut.addEvent(noteOff, sampleOffset);

        // Remove from active notes (with thread synchronization)
        {
          const juce::ScopedLock sl(activeNotesLock);
          activeNotes.erase(std::remove_if(activeNotes.begin(),
                                           activeNotes.end(),
                                           [&](const ActiveNote &n) {
                                             return n.noteId == noteId;
                                           }),
                            activeNotes.end());
        }
      }
    }
  }
}

} // namespace zenith
