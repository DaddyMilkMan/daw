/*
  ==============================================================================

    Track.cpp
    Created: 26 May 2024 1:35:46pm
    Author:  Administrator

  ==============================================================================
*/

#include "Track.h"
#include "../../include/ProjectState.h"
#include "../../include/TempoMap.h"
#include "../instruments/Instrument.h"
#include "Clip.h"
#include <JuceHeader.h> // For juce::MessageManager
#include "../include/Engine.h"        // For Engine access (if needed)
#include "../network/NetworkManager.h" // For network features (AI, Cloud)

namespace zenith {

//==============================================================================
Track::Track(const juce::String &name, Type type)
    : trackName(name), trackType(type),
      // PDC State (from HEAD)
      latencyCompensationSamples{0},
      currentSampleRate(0.0), // Initialize to safe default
      currentBlockSize(0)     // Initialize to safe default
{
  // Initialize empty clip snapshot (lock-free RCU pattern)
  currentClipSnapshot_ = std::make_shared<ClipSnapshot>();
  activeClipSnapshot_.store(currentClipSnapshot_.get());

  // Initialize empty plugin snapshot (lock-free RCU pattern)
  currentPluginSnapshot_ = std::make_shared<PluginSnapshot>();
  activePluginSnapshot_.store(currentPluginSnapshot_.get());

  // Initialize empty automation snapshot
  currentAutomationSnapshot_ = std::make_shared<AutomationSnapshot>();
  activeAutomationSnapshot_.store(currentAutomationSnapshot_.get());
}

Track::~Track() {
  // Phase 2A: Clips are automatically destroyed via std::unique_ptr
  // No lock needed - destructor is called from message thread only
  clipsOwned_.clear();
  currentClipSnapshot_ = std::make_shared<ClipSnapshot>();
  activeClipSnapshot_.store(currentClipSnapshot_.get());

  // Clear plugins snapshot
  pluginsOwned_.clear();
  currentPluginSnapshot_ = std::make_shared<PluginSnapshot>();
  activePluginSnapshot_.store(currentPluginSnapshot_.get());

  // Clear automation snapshot
  automationLanesOwned_.clear();
  currentAutomationSnapshot_ = std::make_shared<AutomationSnapshot>();
  activeAutomationSnapshot_.store(currentAutomationSnapshot_.get());
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
  // Store these for later
  currentSampleRate = sampleRate;
  currentBlockSize = samplesPerBlockExpected;

  // Prepare plugins
  for (auto &plugin : pluginsOwned_) {
    if (plugin != nullptr)
      plugin->prepareToPlay(sampleRate, samplesPerBlockExpected);
  }

  // Prepare mixer channel
  mixerChannel.prepareToPlay(samplesPerBlockExpected, sampleRate);

  // Buffer for PDC (2 seconds max) - from HEAD
  compensationBuffer.setSize(2, static_cast<int>(sampleRate * 2.0));
  compensationBuffer.clear();
  compensationWritePos = 0;
}

void Track::releaseResources() {
  // Release plugins
  for (auto &plugin : pluginsOwned_) {
    if (plugin != nullptr)
      plugin->releaseResources();
  }

  // Release mixer channel
  mixerChannel.releaseResources();
}

// Phase 1.3 / 2A: Process with explicit playhead position (lock-free)
void Track::getNextAudioBlock(
    const juce::AudioSourceChannelInfo &bufferToFill, juce::int64 playheadSamples,
    const juce::MidiBuffer *incomingMidi,
    const std::vector<juce::AudioBuffer<float> *> &auxBuffers,
    const TempoMap *tempoMap) {
  // Clear the buffer first
  bufferToFill.clearActiveBufferRegion();

  // If track is disabled, return silence
  if (!enabled.load() || isMuted() || isSilencedBySolo()) { // Add my mute/solo logic
    midiBuffer_.clear();
    return;
  }

  // Automation Application (Tier 1 Feature)
  // We apply automation at the start of the block (Control Rate)
  if (tempoMap != nullptr) {
    auto *automationSnapshot =
        activeAutomationSnapshot_.load(std::memory_order_acquire);
    if (automationSnapshot) {
      // Calculate time in beats at start of block
      double startBeats =
          tempoMap->samplesToBeats(playheadSamples, currentSampleRate);

      // Volume Automation
      auto volIt = automationSnapshot->lanes.find("volume");
      if (volIt != automationSnapshot->lanes.end()) {
        float val = volIt->second->getValueAt(startBeats);
        mixerChannel.setVolume(val);
      }

      // Pan Automation
      auto panIt = automationSnapshot->lanes.find("pan");
      if (panIt != automationSnapshot->lanes.end()) {
        float val = panIt->second->getValueAt(startBeats);
        mixerChannel.setPan(val);
      }

      // Plugin parameter automation
      // Iterate over plugin-specific automation lanes (format:
      // "plugin_X_paramY")
      for (const auto &[laneId, lane] : automationSnapshot->lanes) {
        if (laneId.startsWith("plugin_")) {
          // Parse plugin index and param index from laneId
          auto parts = juce::StringArray::fromTokens(laneId, "_", "");
          if (parts.size() >= 3) {
            int pluginIdx = parts[1].getIntValue();
            int paramIdx = parts[2].getIntValue();

            auto *pluginSnapshot =
                activePluginSnapshot_.load(std::memory_order_acquire);
            if (pluginSnapshot && pluginIdx >= 0 &&
                pluginIdx < (int)pluginSnapshot->plugins.size()) {
              auto &plugin = pluginSnapshot->plugins[pluginIdx];
              if (plugin) {
                auto params = plugin->getParameters();
                if (paramIdx >= 0 && paramIdx < params.size()) {
                  float val = lane->getValueAt(startBeats);
                  params[paramIdx]->setValueNotifyingHost(val);
                }
              }
            }
          }
        }
      }
    }
  }

  // Phase 2A: Clear MIDI buffer for this block
  midiBuffer_.clear();

  // Add incoming MIDI (e.g. from external controller)
  if (incomingMidi != nullptr && !incomingMidi->isEmpty()) {
    midiBuffer_.addEvents(*incomingMidi, 0, bufferToFill.numSamples, 0);
  }

  // TRULY LOCK-FREE: Atomic load of raw pointer - no SpinLock!
  const ClipSnapshot *currentClipSnapshot =
      activeClipSnapshot_.load(std::memory_order_acquire);

  if (currentClipSnapshot) {
    // Iterate clips from snapshot (no lock needed!)
    for (auto *clip : currentClipSnapshot->clips) {
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

  // If it's an instrument track, render instrument audio from MIDI input
  if (trackType == Type::Instrument && instrument_ != nullptr) {
    instrument_->processBlock(*bufferToFill.buffer, midiBuffer_);
  }

  // Process plugin chain (using lock-free snapshot)
  processPluginChain(*bufferToFill.buffer, midiBuffer_, bufferToFill.numSamples);

  // Delegate all mixer processing to MixerChannel
  // This includes: input gain, HPF, EQ, compressor, volume, pan, metering
  // Also processes Aux Sends (Pre and Post fader)
  juce::AudioSourceChannelInfo mixerInfo(bufferToFill.buffer, bufferToFill.startSample,
                                         bufferToFill.numSamples); // Pass original buffer
  mixerChannel.getNextAudioBlock(mixerInfo, auxBuffers);

  // Apply PDC (Plugin Delay Compensation) - from HEAD
  const int delaySamples = latencyCompensationSamples.load();
  if (delaySamples > 0 && compensationBuffer.getNumSamples() > 0) {
      const int numSamples = bufferToFill.numSamples;
      const int bufferLen = compensationBuffer.getNumSamples();
      const int numChannels = juce::jmin(bufferToFill.buffer->getNumChannels(), compensationBuffer.getNumChannels());

      for (int ch = 0; ch < numChannels; ++ch) {
          float* channelData = bufferToFill.buffer->getWritePointer(ch, bufferToFill.startSample);
          const float* compRead = compensationBuffer.getReadPointer(ch);
          float* compWrite = compensationBuffer.getWritePointer(ch);

          for (int i = 0; i < numSamples; ++i) {
              // Write current sample to delay line
              compWrite[(compensationWritePos + i) % bufferLen] = channelData[i];

              // Read delayed sample
              int readIndex = (compensationWritePos + i - delaySamples);
              while (readIndex < 0) readIndex += bufferLen;
              readIndex %= bufferLen;

              channelData[i] = compRead[readIndex];
          }
      }
      compensationWritePos = (compensationWritePos + numSamples) % bufferLen;
  }
}

// Legacy overload: uses default playhead of 0 (from HEAD)
void Track::getNextAudioBlock(juce::AudioSourceChannelInfo &bufferToFill) {
  juce::MidiBuffer midiBuffer; // Empty midi buffer
  AuxBufferList emptyAuxBuffers;
  getNextAudioBlock(bufferToFill, 0, &midiBuffer, emptyAuxBuffers,
                    nullptr); // Pass nullptr for tempoMap
}

//==============================================================================
void Track::updateClipSnapshot() {
  // Create new snapshot
  auto newSnapshot = std::make_shared<ClipSnapshot>(clipsOwned_);

  // Atomically swap (release semantics for the store)
  // The audio thread will see the new pointer immediately
  activeClipSnapshot_.store(newSnapshot.get(), std::memory_order_release); // Use std::memory_order_release

  // Manage lifetime of old snapshots
  // We keep the previous snapshot alive in clipSnapshotTrash_
  // because the audio thread might still be reading it.
  clipSnapshotTrash_.push_back(currentClipSnapshot_); // Use currentClipSnapshot_ from origin/master

  // Update current holder to the new snapshot
  currentClipSnapshot_ = newSnapshot; // Use currentClipSnapshot_ from origin/master

  // Garbage collection: Keep last 10 snapshots (from origin/master)
  // At 60Hz updates, this gives plenty of margin for the audio thread to finish
  while (clipSnapshotTrash_.size() > 10) { // Use while loop from origin/master
    clipSnapshotTrash_.erase(clipSnapshotTrash_.begin());
  }
}

void Track::addClip(std::unique_ptr<Clip> clip) {
  // THREAD SAFETY: Message thread only
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

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
  // THREAD SAFETY: Message thread only
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  if (clipIndex >= 0 && clipIndex < static_cast<int>(clipsOwned_.size())) {
    auto &clip = clipsOwned_[clipIndex];
    if (clip != nullptr) {
      clip->releaseResources();
    }
    clipsOwned_.erase(clipsOwned_.begin() + clipIndex);

    // Update snapshot for audio thread
    updateClipSnapshot();

    sendChangeMessage();
  }
}

void Track::removeClip(Clip *clip) {
  // THREAD SAFETY: Message thread only
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

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
  // THREAD SAFETY: Message thread only
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

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
// Plugin chain management (Phase 3: VST3 hosting MVP)
//==============================================================================

// Lock-free RCU-style plugin snapshot update
void Track::updatePluginSnapshot() {
  // Called from message thread only
  auto newSnapshot = std::make_shared<PluginSnapshot>(pluginsOwned_);

  // Atomic swap - audio thread will see new snapshot on next load
  activePluginSnapshot_.store(newSnapshot.get(), std::memory_order_release);

  // Manage lifetime: keep old snapshots alive briefly for audio thread
  pluginSnapshotTrash_.push_back(currentPluginSnapshot_);
  currentPluginSnapshot_ = newSnapshot;

  // ACCUMULATION FIX: Increased limit from 5 to 10 to handle rapid updates
  // during offline rendering or automation. The snapshot is typically read
  // once per audio callback (~10ms at 48kHz/512 samples), so 10 snapshots
  // provides ~100ms grace period.
  while (pluginSnapshotTrash_.size() > 10) {
    pluginSnapshotTrash_.erase(pluginSnapshotTrash_.begin());
  }
}

// ROAST FIX #2: Add plugin using shared_ptr and snapshot pattern
void Track::addPlugin(std::unique_ptr<juce::AudioPluginInstance> plugin) {
  // THREAD SAFETY: Message thread only
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  if (plugin == nullptr)
    return;

  // Convert to shared_ptr for snapshot pattern
  auto sharedPlugin =
      std::shared_ptr<juce::AudioPluginInstance>(plugin.release());

  // Prepare the plugin if we're already initialized
  if (currentSampleRate > 0) {
    sharedPlugin->prepareToPlay(currentSampleRate, currentBlockSize);
    sharedPlugin->setNonRealtime(false);
  }

  pluginsOwned_.push_back(sharedPlugin);

  // Update snapshot for audio thread
  updatePluginSnapshot();

  sendChangeMessage();
}

// ROAST FIX #2: Use snapshot pattern instead of lock
void Track::removePlugin(int pluginIndex) {
  // THREAD SAFETY: Message thread only
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  if (pluginIndex >= 0 &&
      pluginIndex < static_cast<int>(pluginsOwned_.size())) {
    auto &plugin = pluginsOwned_[pluginIndex];
    if (plugin != nullptr) {
      plugin->releaseResources();
    }
    pluginsOwned_.erase(pluginsOwned_.begin() + pluginIndex);

    // Update snapshot for audio thread
    updatePluginSnapshot();

    sendChangeMessage();
  }
}

// ROAST FIX #2: Use snapshot pattern instead of lock
void Track::clearPlugins() {
  // THREAD SAFETY: Message thread only
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  for (auto &plugin : pluginsOwned_) {
    if (plugin != nullptr) {
      plugin->releaseResources();
    }
  }

  pluginsOwned_.clear();

  // Update snapshot for audio thread
  updatePluginSnapshot();

  sendChangeMessage();
}

// ROAST FIX #2: Read from ownership vector (message thread only)
int Track::getNumPlugins() const {
  return static_cast<int>(pluginsOwned_.size());
}

// ROAST FIX #2: Read from ownership vector (message thread only)
juce::AudioPluginInstance *Track::getPlugin(int index) const {
  if (index >= 0 && index < static_cast<int>(pluginsOwned_.size()))
    return pluginsOwned_[index].get();
  return nullptr;
}

/** Returns total plugin chain latency in samples for PDC (Plugin Delay Compensation). */
int Track::getLatencySamples() const {
  // If called from message thread, use owned list
  if (juce::MessageManager::getInstance()->isThisTheMessageThread()) {
    int latency = 0;
    for (size_t i = 0; i < pluginsOwned_.size(); ++i) {
      if (pluginsOwned_[i])
        latency += pluginsOwned_[i]->getLatencySamples();
    }
    return latency;
  }

  // If called from audio thread, use snapshot
  const auto* snapshot = activePluginSnapshot_.load(std::memory_order_acquire);
  if (snapshot == nullptr)
    return 0;

  int latency = 0;
  for (size_t i = 0; i < snapshot->plugins.size(); ++i) {
    if (snapshot->plugins[i])
      latency += snapshot->plugins[i]->getLatencySamples();
  }
  return latency;
}


//==============================================================================
// Automation Management (Message Thread)
//==============================================================================

void Track::updateAutomationSnapshot() {
  auto newSnapshot =
      std::make_shared<AutomationSnapshot>(automationLanesOwned_);
  activeAutomationSnapshot_.store(newSnapshot.get(), std::memory_order_release);

  automationSnapshotTrash_.push_back(currentAutomationSnapshot_);
  currentAutomationSnapshot_ = newSnapshot;

  while (automationSnapshotTrash_.size() > 10) {
    automationSnapshotTrash_.erase(automationSnapshotTrash_.begin());
  }
}

void Track::addAutomationLane(const juce::String &paramId,
                              std::shared_ptr<AutomationLane> lane) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
  automationLanesOwned_[paramId] = lane;
  updateAutomationSnapshot();
}

void Track::clearAutomationLanes() {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
  automationLanesOwned_.clear();
  updateAutomationSnapshot();
}

//==============================================================================
// MIDI Scheduling
//==============================================================================

void Track::generateMidiForBlock(const juce::ValueTree &trackState,
                                 double tempo, double sampleRate,
                                 juce::int64 blockStartSample, int blockSize,
                                 juce::MidiBuffer &midiOut) {
  // THREAD SAFETY FIX: This method uses a lock (activeNotesLock) and is
  // therefore NOT RT-safe. It must only be called from the message thread.
  // Note: This method is currently not called - actual MIDI scheduling uses
  // the lock-free Clip::processMidiClip() path instead.
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  // Calculate block boundaries in samples
  juce::int64 blockEndSample = blockStartSample + blockSize;

  // Convert to beats
  // Formula: beats = (samples / sampleRate) * (tempo / 60)
  double beatsPerSample = (tempo / 60.0) / sampleRate;
  double blockStartBeats = blockStartSample * beatsPerSample;
  double blockEndBeats = blockEndSample * beatsPerBeat; // Fix: was sampleRate

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


void Track::setLatencyCompensation(int samples) {
    latencyCompensationSamples.store(samples);
}

//==============================================================================
// State management
//==============================================================================

juce::ValueTree Track::getState() const {
  juce::ValueTree state("Track");

  state.setProperty("name", trackName, nullptr);
  state.setProperty("type", static_cast<int>(trackType), nullptr);
  state.setProperty("volume", mixerChannel.getVolume(), nullptr);
  state.setProperty("pan", mixerChannel.getPan(), nullptr);
  state.setProperty("muted", mixerChannel.isMuted(), nullptr);
  state.setProperty("solo", mixerChannel.isSolo(), nullptr);
  state.setProperty("armed", armed.load(), nullptr);
  state.setProperty("enabled", enabled.load(), nullptr);

  // Save plugin states (message thread only, no lock needed)
  juce::ValueTree pluginsState("Plugins");
  for (auto &plugin : pluginsOwned_) {
    if (plugin != nullptr) {
      juce::ValueTree pluginState("Plugin");
      savePluginState(plugin.get(), pluginState);
      pluginsState.appendChild(pluginState, nullptr);
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
  mixerChannel.setVolume(state.getProperty("volume", 0.8f));
  mixerChannel.setPan(state.getProperty("pan", 0.0f));
  mixerChannel.setMuted(state.getProperty("muted", false));
  mixerChannel.setSolo(state.getProperty("solo", false));
  armed.store(state.getProperty("armed", false));
  enabled.store(state.getProperty("enabled", true));

  // Load plugin states using the dedicated method if PluginHost is available
  // Note: This basic loadState doesn't have PluginHost context,
  // so plugin loading must be deferred to loadPluginStates() call from Engine
  auto pluginsState = state.getChildWithName("Plugins");
  if (pluginsState.isValid()) {
    // Store plugin state for later loading when PluginHost is available
    // For now, just clear - Engine will call loadPluginStates() after this
    clearPlugins();
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

  // Load Automation
  clearAutomationLanes();
  auto automationNode = state.getChildWithName(ProjectState::ID_AUTOMATION);
  if (automationNode.isValid()) {
    for (auto envNode : automationNode) {
      if (envNode.hasType(ProjectState::ID_ENVELOPE)) {
        juce::String paramId = envNode.getProperty(ProjectState::PROP_PARAM_ID);
        std::vector<AutomationPoint> points;
        for (auto pointNode : envNode) {
          if (pointNode.hasType(ProjectState::ID_POINT)) {
            points.push_back({
                static_cast<double>(
                    pointNode.getProperty(ProjectState::PROP_TIME_BEATS)),
                static_cast<float>(
                    pointNode.getProperty(ProjectState::PROP_VALUE)),
                0.5f // Curve default (linear)
            });
          }
        }
        // Ensure sorted
        std::sort(points.begin(), points.end(),
                  [](const auto &a, const auto &b) {
                    return a.timeBeats < b.timeBeats;
                  });

        if (!points.empty()) {
          addAutomationLane(paramId, std::make_shared<AutomationLane>(points));
        }
      }
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

  // Recreate each plugin using the helper
  for (int i = 0; i < pluginsState.getNumChildren(); ++i) {
    auto pluginState = pluginsState.getChild(i);
    if (pluginState.hasType("Plugin")) {
      loadPluginState(pluginState, pluginHost);
    }
  }

  DBG("Track: Loaded " + juce::String(getNumPlugins()) + " plugins");
}

} // namespace zenith
