/*
  ==============================================================================

    Track.cpp
    Created: 26 May 2024 1:35:46pm
    Author:  Administrator

  ==============================================================================
*/

#include "Track.h"
#include "Clip.h"
#include <JuceHeader.h> // For juce::MessageManager
#include "../include/Engine.h"        // For Engine access (if needed)

namespace zenith {

//==============================================================================
Track::Track(juce::String name, Type type)
    : trackName(std::move(name)), trackType(type),
      inputChannelIndex(-1) // Default to no input
{
  // Generate a unique ID for the track
  trackId = juce::Uuid().toString();

  // Initialize plugins
  updatePluginSnapshot();
}

Track::~Track() { releaseResources(); }

//==============================================================================
void Track::prepareToPlay(int samplesPerBlockExpected, double sampleRate) {
  // Store these for later
  lastSampleRate = sampleRate;
  lastBlockSize = samplesPerBlockExpected;

  // Prepare plugins
  for (auto &plugin : pluginsOwned_) {
    if (plugin != nullptr)
      plugin->prepareToPlay(sampleRate, samplesPerBlockExpected);
  }

  // Prepare mixer channel
  mixerChannel.prepareToPlay(samplesPerBlockExpected, sampleRate);

  // Buffer for PDC (2 seconds max)
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

//==============================================================================
void Track::getNextAudioBlock(juce::AudioSourceChannelInfo &bufferToFill,
                              juce::int64 playheadPosition,
                              juce::MidiBuffer *midiBuffer,
                              const AuxBufferList &auxBuffers,
                              const TempoMap *tempoMap) {
  // Mute logic
  if (isMuted() || isSilencedBySolo()) {
    bufferToFill.buffer->clear();
    return;
  }

  // Clear the buffer first
  bufferToFill.buffer->clear();

  // Process MIDI clips first (if any and if it's a MIDI/Instrument track)
  if (trackType == Type::MIDI || trackType == Type::Instrument) {
    if (midiBuffer != nullptr) {
      // Process MIDI clips
      for (const auto &clip : clips_) {
        if (clip->getType() == Clip::Type::MIDI && clip->isPlaying()) {
          clip->processMidiClip(*midiBuffer, playheadPosition, bufferToFill.numSamples);
        }
      }
    }
  }

  // Process audio clips
  if (trackType == Type::Audio || trackType == Type::Instrument) {
    for (const auto &clip : clips_) {
      if (clip->getType() == Clip::Type::Audio && clip->isPlaying()) {
        // Render audio clip into the buffer
        clip->processAudioClip(bufferToFill, playheadPosition);
      }
    }

    // If it's an instrument track, render instrument audio from MIDI input
    if (trackType == Type::Instrument && instrument_ != nullptr) {
       if (midiBuffer != nullptr) {
           instrument_->processBlock(*bufferToFill.buffer, *midiBuffer);
       } else {
           juce::MidiBuffer emptyMidi;
           instrument_->processBlock(*bufferToFill.buffer, emptyMidi);
       }
    }
  }

  // Apply automation (before plugins, typically)
  // For now, only volume and pan are automated via the MixerChannel
  // Future: apply parameter automation here

  // Get current plugin snapshot (audio thread safe)
  const auto *snapshot = activePluginSnapshot_.load(std::memory_order_acquire);
  if (snapshot != nullptr) {
    // Process plugins
    for (const auto &plugin : snapshot->plugins) {
      if (plugin != nullptr && !plugin->isSuspended()) {
        plugin->processBlock(*bufferToFill.buffer, *midiBuffer);
      }
    }
  }

  // Pass to mixer channel for volume, pan, sends, and metering
  juce::AudioSourceChannelInfo mixerInfo(bufferToFill.buffer, bufferToFill.startSample,
                                         bufferToFill.numSamples);
  mixerChannel.getNextAudioBlock(mixerInfo, auxBuffers);

  // Apply PDC (Plugin Delay Compensation)
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

// Legacy overload: uses default playhead of 0
void Track::getNextAudioBlock(juce::AudioSourceChannelInfo &bufferToFill) {
  juce::MidiBuffer midiBuffer; // Empty midi buffer
  AuxBufferList emptyAuxBuffers;
  getNextAudioBlock(bufferToFill, 0, &midiBuffer, emptyAuxBuffers,
                    nullptr); // Pass nullptr for tempoMap
}

//==============================================================================
void Track::addClip(std::unique_ptr<Clip> newClip) {
  clips_.push_back(std::move(newClip));
}

void Track::removeClip(const juce::String &clipId) {
  clips_.erase(std::remove_if(clips_.begin(), clips_.end(),
                               [&clipId](const std::unique_ptr<Clip> &clip) {
                                 return clip->getName() == clipId;
                               }),
               clips_.end());
}

Clip *Track::getClip(int index) const {
  if (index >= 0 && index < clips_.size())
    return clips_[index].get();
  return nullptr;
}

void Track::clearClips() { clips_.clear(); }

//==============================================================================
void Track::addPlugin(std::unique_ptr<juce::AudioPluginInstance> newPlugin) {
  pluginsOwned_.push_back(std::move(newPlugin));
  updatePluginSnapshot();
}

void Track::removePlugin(int index) {
  if (index >= 0 && index < pluginsOwned_.size()) {
    pluginsOwned_.erase(pluginsOwned_.begin() + index);
    updatePluginSnapshot();
  }
}

juce::AudioPluginInstance *Track::getPlugin(int index) const {
  if (index >= 0 && index < pluginsOwned_.size()) {
    return pluginsOwned_[index].get();
  }
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
// Phase 2A: Lock-free clip management (message thread only)
//==============================================================================

Track::ClipSnapshot::ClipSnapshot(const std::vector<std::unique_ptr<Clip>> &c) {
  for (const auto &clip : c) {
    clips.push_back(clip.get());
  }
}

void Track::updateClipSnapshot() {
  // Create new snapshot
  auto newSnapshot = std::make_shared<ClipSnapshot>(clips_);

  // Atomically swap (release semantics for the store)
  // The audio thread will see the new pointer immediately
  activeClipSnapshot_.store(newSnapshot.get());

  // Manage lifetime of old snapshots
  // We keep the previous snapshot alive in clipSnapshotTrash_
  // because the audio thread might still be reading it.
  clipSnapshotTrash_.push_back(currentClipSnapshotHolder_);

  // Update current holder to the new snapshot
  currentClipSnapshotHolder_ = newSnapshot;

  // Garbage collection: Keep last 5 snapshots
  // At 60Hz updates, this gives plenty of margin for the audio thread to finish
  if (clipSnapshotTrash_.size() > 5) {
    clipSnapshotTrash_.erase(clipSnapshotTrash_.begin());
  }
}

Track::PluginSnapshot::PluginSnapshot(
    const std::vector<std::unique_ptr<juce::AudioPluginInstance>> &p) {
  for (const auto &plugin : p) {
    plugins.push_back(plugin.get());
  }
}

void Track::updatePluginSnapshot() {
  // Create new snapshot
  auto newSnapshot = std::make_shared<PluginSnapshot>(pluginsOwned_);

  // Atomically swap (release semantics for the store)
  activePluginSnapshot_.store(newSnapshot.get());

  // Manage lifetime of old snapshots
  pluginSnapshotTrash_.push_back(currentPluginSnapshotHolder_);
  currentPluginSnapshotHolder_ = newSnapshot;

  if (pluginSnapshotTrash_.size() > 5) {
    pluginSnapshotTrash_.erase(pluginSnapshotTrash_.begin());
  }
}

Track::AutomationSnapshot::AutomationSnapshot(
    const std::map<juce::String, std::shared_ptr<AutomationLane>> &l) {
  for (const auto &pair : l) {
    lanes.emplace(pair.first, pair.second);
  }
}

void Track::updateAutomationSnapshot() {
  auto newSnapshot = std::make_shared<AutomationSnapshot>(automationLanes_);
  activeAutomationSnapshot_.store(newSnapshot.get());
  automationSnapshotTrash_.push_back(currentAutomationSnapshotHolder_);
  currentAutomationSnapshotHolder_ = newSnapshot;
  if (automationSnapshotTrash_.size() > 5) {
    automationSnapshotTrash_.erase(automationSnapshotTrash_.begin());
  }
}

//==============================================================================
void Track::setAutomationLane(const juce::String &paramId,
                              std::shared_ptr<AutomationLane> lane) {
  automationLanes_[paramId] = lane;
  updateAutomationSnapshot();
}

void Track::addAutomationLane(const juce::String& paramId, std::shared_ptr<AutomationLane> lane) {
    automationLanes_[paramId] = lane;
    updateAutomationSnapshot();
}

void Track::clearAutomationLanes() {
    automationLanes_.clear();
    updateAutomationSnapshot();
}


void Track::setLatencyCompensation(int samples) {
    latencyCompensationSamples.store(samples);
}

//==============================================================================
} // namespace zenith