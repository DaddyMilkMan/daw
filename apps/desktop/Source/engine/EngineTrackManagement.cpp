/**
 * @file EngineTrackManagement.cpp
 * @brief Track creation, removal, and snapshot management
 * @note This is a modular component of Engine - declarations remain in Engine.h
 */

#include "Engine.h"
#include "ProjectState.h"
#include "Track.h"
#include "Clip.h"
#include "AuxBus.h"
#include "AudioRenderer.h"
#include "TempoMap.h"
#include "RealTimeGarbageCollector.h"

namespace zenith {

//==============================================================================
// Track Management
//==============================================================================

void Engine::syncWithProjectState() {
  DBG("Engine: Syncing with project state");

  // Lock for exclusive access during sync
  const juce::ScopedWriteLock lock(tracksLock_);

  if (projectState_ == nullptr) {
    DBG("Engine: No project state, clearing tracks");
    tracks_.clear();
    return;
  }

  // Clear existing tracks
  tracks_.clear();

  // Get tracks from project state
  auto &state = projectState_->getState();
  auto tracksNode = state.getChildWithName(ProjectState::ID_TRACKS);

  if (!tracksNode.isValid()) {
    DBG("Engine: No tracks in project state");
    return;
  }

  const double sampleRate = currentSampleRate.load();
  const int bufferSize = currentBufferSize.load();

  // Helper: convert beats to samples using TempoMap
  auto beatsToSamples = [this, sampleRate](double beats) -> juce::int64 {
    if (tempoMap_) {
      return tempoMap_->beatsToSamples(beats, sampleRate);
    }
    // Fallback if no tempo map
    const double tempo = projectState_ ? projectState_->getTempo() : 120.0;
    const double secondsPerBeat = 60.0 / tempo;
    const double seconds = beats * secondsPerBeat;
    return static_cast<juce::int64>(seconds * sampleRate);
  };

  // Create engine tracks from project state
  for (auto trackNode : tracksNode) {
    juce::String trackName = trackNode[ProjectState::PROP_NAME].toString();
    juce::String trackType = trackNode[ProjectState::PROP_TYPE].toString();

    // Create track via Factory
    zenith::Track::Type actualType = zenith::Track::Type::Audio;
    if (trackType == "midi")
      actualType = zenith::Track::Type::MIDI;
    else if (trackType == "instrument")
      actualType = zenith::Track::Type::Instrument;
    else if (trackType == "bus")
      actualType = zenith::Track::Type::Bus;

    auto track = zenith::Track::create(trackName, actualType);

    // Set track ID
    track->setTrackId(trackNode[ProjectState::PROP_ID].toString());

    // Set mixer properties
    track->setVolume(trackNode[ProjectState::PROP_VOLUME]);
    track->setPan(trackNode[ProjectState::PROP_PAN]);
    track->setMuted(trackNode[ProjectState::PROP_MUTE]);
    track->setSolo(trackNode[ProjectState::PROP_SOLO]);

    // Prepare track for playback
    if (sampleRate > 0) {
      track->prepareToPlay(bufferSize, sampleRate);
    }

    // Load clips
    auto clipsNode = trackNode.getChildWithName(ProjectState::ID_CLIPS);
    if (clipsNode.isValid()) {
      for (auto clipNode : clipsNode) {
        // Create clip
        auto clip = std::make_unique<zenith::Clip>();

        // Set basic properties
        double startBeats = clipNode[ProjectState::PROP_START];
        double lengthBeats = clipNode[ProjectState::PROP_LENGTH];

        clip->setStartPosition(beatsToSamples(startBeats));
        clip->setLength(beatsToSamples(lengthBeats));

        // Load audio file if present
        juce::String audioFilePath =
            clipNode[ProjectState::PROP_AUDIO_FILE].toString();
        if (audioFilePath.isNotEmpty()) {
          juce::File audioFile(audioFilePath);
          if (audioFile.existsAsFile()) {
            clip->setAudioFile(audioFile);
            clip->setType(zenith::Clip::Type::Audio);
            DBG("Engine: Loaded audio file: " + audioFile.getFileName());
          } else {
            DBG("Engine: Warning - audio file not found: " + audioFilePath);
          }
        }

        // Prepare clip
        if (sampleRate > 0) {
          clip->prepareToPlay(bufferSize, sampleRate);
        }

        // Set clip as playing (so it's active during playback)
        clip->setPlaying(true);

        // Add clip to track
        track->addClip(std::move(clip));
      }
    }

    // Prepare track for audio processing if engine is already running
    if (sampleRate > 0) {
      track->prepareToPlay(bufferSize, sampleRate);
    }

    // Add track to engine
    track->setTrackIndex((int)tracks_.size());
    tracks_.push_back(std::move(track));

    // Register with RoutingGraph and connect to master bus
    RoutingGraph::Node node;
    node.id = tracks_.back()->getTrackId();
    node.name = tracks_.back()->getName();
    node.type = RoutingGraph::NodeType::Track;
    routingGraph_.addNode(node);

    // Automatically route track to master bus
    routingGraph_.connect(tracks_.back()->getTrackId(), "master", 1.0f);

    // Register Engine as listener for PDC updates (plugin changes, etc.)
    tracks_.back()->addChangeListener(this);
  }

  // Update snapshots
  updateTrackSnapshot();

  // Re-prepare AudioRenderer with new track/bus counts
  if (audioRenderer_) {
    // AudioRenderer is now stateless, so we prepare the context directly
    renderContext_.prepare(currentSampleRate.load(), currentBufferSize.load(),
                          tracks_.size(), auxBuses_.size());
  }

  DBG("Engine: Synced " + juce::String(tracks_.size()) + " tracks");
}

int Engine::getNumTracks() const noexcept {
  return static_cast<int>(tracks_.size());
}

const std::vector<std::shared_ptr<zenith::Track>> &
Engine::tracks() const noexcept {
  return tracks_;
}

void Engine::addTestTracks(int count) {
  if (count <= 0)
    return;

  DBG("Engine: Adding " + juce::String(count) + " test tracks");

  const juce::ScopedWriteLock lock(tracksLock_);

  // Reserve capacity to avoid reallocations
  tracks_.reserve(tracks_.size() + static_cast<size_t>(count));

  for (int i = 0; i < count; ++i) {
    // Create track via Factory
    auto track = zenith::Track::create(
        "Track " + juce::String(tracks_.size() + 1),
        zenith::Track::Type::Audio);

    // Prepare track for audio processing if engine is already running
    if (currentSampleRate.load() > 0) {
      track->prepareToPlay(currentBufferSize.load(), currentSampleRate.load());
    }

    track->setTrackIndex((int)tracks_.size());
    tracks_.push_back(std::move(track)); // Corrected: transfer ownership from unique_ptr to shared_ptr

    // Register track with RoutingGraph and connect to master bus
    RoutingGraph::Node node;
    node.id = track->getTrackId();
    node.name = track->getName();
    node.type = RoutingGraph::NodeType::Track;
    routingGraph_.addNode(node);

    // Automatically route track to master bus
    routingGraph_.connect(track->getTrackId(), "master", 1.0f);
  }

  DBG("Engine: Total tracks: " + juce::String(tracks_.size()));

  // Re-prepare tracks if audio device is already running
  auto *device = deviceManager.getCurrentAudioDevice();
  if (device != nullptr) {
    prepareTracks(device->getCurrentBufferSizeSamples(),
                  device->getCurrentSampleRate());
  }

  // Update snapshot for audio thread
  updateTrackSnapshot();
}

juce::String Engine::createTrack(const juce::String &name,
                                 const juce::String &type) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  if (projectState_) {
    // Add track to ProjectState
    // This will trigger TrackStateSynchronizer::valueTreeChildAdded,
    // which will call Engine::addTrack()
    return projectState_->addTrack(name, type);
  } else {
    DBG("Engine: Creating track without ProjectState (fallback)");

    // Fallback: Create track directly in Engine
    // Use shared_ptr to allow track references to outlive snapshot updates
    zenith::Track::Type trackType = (type == "midi")
                                        ? zenith::Track::Type::MIDI
                                        : zenith::Track::Type::Audio;
    auto track = std::shared_ptr<zenith::Track>(zenith::Track::create(name, trackType));

    // Use atomic counter for ID generation
    juce::String trackId = "track_" + juce::String(nextTrackId_++);
    track->setTrackId(trackId);

    addTrack(track); // No std::move for shared_ptr

    return trackId;
  }
}    

// Accept shared_ptr for RT-safe snapshot sharing across threads
void Engine::addTrack(std::shared_ptr<zenith::Track> track) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
  jassert(track != nullptr);

  const juce::ScopedWriteLock lock(tracksLock_);

  // Prepare the track if audio is already running
  if (currentSampleRate.load() > 0) {
    track->prepareToPlay(currentBufferSize.load(), currentSampleRate.load());
  }

  juce::String name = track->getName();
  juce::String id = track->getTrackId();

  track->setTrackIndex((int)tracks_.size());
  tracks_.push_back(track); // Shared_ptr, no move needed

  // Register with RoutingGraph
  RoutingGraph::Node node;
  node.id = id;
  node.name = name;
  node.type = RoutingGraph::NodeType::Track;

  // Determine proper node type based on track type
  if (track->getType() == Track::Type::Bus) {
    node.type = RoutingGraph::NodeType::Bus;
  } else if (track->getType() == Track::Type::Master) {
    node.type = RoutingGraph::NodeType::Master;
  }
  routingGraph_.addNode(node);

  // Automatically route regular tracks to master bus
  // Bus and Master tracks handle their own routing
  if (track->getType() != Track::Type::Bus &&
      track->getType() != Track::Type::Master) {
    routingGraph_.connect(id, "master", 1.0f);
  }

  // Register Engine as listener for PDC updates
  track->addChangeListener(this);

  DBG("Engine: Added track '" + name + "' (ID: " + id + ")");

  // Update Solo State (new track might need to be silenced if others are
  // soloed)
  updateSoloState();

  // Update snapshot for audio thread
  // Snapshot holds shared_ptr, extending track lifetime across thread
  // boundaries
  updateTrackSnapshot();
}

void Engine::removeTrack(int index) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  const juce::ScopedWriteLock lock(tracksLock_);

  if (index >= 0 && index < static_cast<int>(tracks_.size())) {
    juce::String name = tracks_[index]->getName();
    juce::String id = tracks_[index]->getTrackId();

    // Release resources
    tracks_[index]->releaseResources();
    
    // Unregister listener
    tracks_[index]->removeChangeListener(this);

    // Remove from vector
    tracks_.erase(tracks_.begin() + index);

    // Remove from RoutingGraph
    routingGraph_.removeNode(id);

    DBG("Engine: Removed track '" + name + "' at index " + juce::String(index));

    // Update Solo State (removed track might have been the only soloed one)
    updateSoloState();

    // Update snapshot for audio thread
    updateTrackSnapshot();
  }
}

Track* Engine::getTrackById(const juce::String& trackId) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
  
  const juce::ScopedReadLock lock(tracksLock_);
  for (const auto& track : tracks_) {
    if (track && track->getTrackId() == trackId) {
      return track.get();
    }
  }
  return nullptr;
}

void Engine::updateTrackSnapshot() {
  // Create new snapshot
  // Include Aux Buses in snapshot for consistent audio thread access
  auto newSnapshot = std::make_shared<TrackSnapshot>(tracks_, auxBuses_);

  // [DSP Optimization] Update routing graph snapshot with direct pointers for fast lookup
  {
      std::unordered_map<juce::String, std::shared_ptr<Track>> trackMap;
      for (const auto& track : tracks_) {
          if (track) trackMap[track->getTrackId()] = track;
      }
      
      std::unordered_map<juce::String, std::shared_ptr<AuxBus>> auxBusMap;
      for (const auto& bus : auxBuses_) {
          if (bus) auxBusMap[bus->getId()] = bus;
      }
      
      routingGraph_.updateSnapshotWithPointers(trackMap, auxBusMap);
  }

  // Atomic swap (release semantics for the store)
  // The audio thread will see the new pointer immediately
  activeSnapshot_.store(newSnapshot.get());

  // Manage lifetime of old snapshots
  // We defer deletion using RealTimeGarbageCollector to ensure audio thread safety
  RealTimeGarbageCollector::getInstance().deferDelete(currentSnapshotHolder_);

  // Update current holder to the new snapshot
  currentSnapshotHolder_ = newSnapshot;
}

void Engine::prepareTracks(int samplesPerBlockExpected, double sampleRate) {
  DBG("Engine: Preparing " + juce::String(tracks_.size()) + " tracks");

  // Prepare each track
  for (auto &track : tracks_) {
    if (track != nullptr) {
      track->prepareToPlay(samplesPerBlockExpected, sampleRate);
    }
  }

  if (audioRenderer_) {
    renderContext_.prepare(sampleRate, samplesPerBlockExpected, tracks_.size(),
                          auxBuses_.size());
  }
}

void Engine::updateSoloState() {
  bool anySolo = false;
  for (const auto &track : tracks_) {
    if (track && track->isSolo()) {
      anySolo = true;
      break;
    }
  }

  for (const auto &track : tracks_) {
    if (track) {
      if (anySolo) {
        // If any track is soloed, mute this track unless it is also soloed
        track->setSilencedBySolo(!track->isSolo());
      } else {
        // No solo active, unmute everyone (from solo perspective)
        track->setSilencedBySolo(false);
      }
    }
  }
}

//==============================================================================
// TrackSnapshot Implementation
//==============================================================================

Engine::TrackSnapshot::TrackSnapshot(
    const std::vector<std::shared_ptr<zenith::Track>> &ownedTracks,
    const std::vector<std::shared_ptr<zenith::AuxBus>> &ownedBuses) {
  this->tracks.reserve(ownedTracks.size());
  this->lifecycle.reserve(ownedTracks.size());
  for (const auto &track : ownedTracks) {
    if (track != nullptr) {
      this->tracks.push_back(track.get());
      this->lifecycle.push_back(track); // Increment refcount
      this->trackMap[track->getTrackId().toStdString()] = track.get();
    }
  }

  this->auxBuses.reserve(ownedBuses.size());
  this->lifecycleAux.reserve(ownedBuses.size());
  for (const auto &bus : ownedBuses) {
    if (bus != nullptr) {
      this->auxBuses.push_back(bus.get());
      this->lifecycleAux.push_back(bus);
      this->auxBusMap[bus->getId().toStdString()] = bus.get();
    }
  }
}

} // namespace zenith
