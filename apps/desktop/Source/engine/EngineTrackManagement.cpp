/**
 * @file EngineTrackManagement.cpp
 * @brief Track creation, removal, and snapshot management
 * @note This is a modular component of Engine - declarations remain in Engine.h
 */

#include "../engine/AudioRenderer.h"
#include "../engine/AuxBus.h"
#include "../engine/Track.h"
#include "Engine.h"
#include "ProjectState.h"
#include "RealTimeGarbageCollector.h"

namespace zenith {

//==============================================================================
// Track Management
//==============================================================================

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

  // Reserve capacity to avoid reallocations
  tracks_.reserve(tracks_.size() + static_cast<size_t>(count));

  for (int i = 0; i < count; ++i) {
    // Create track via Factory
    auto track =
        zenith::Track::create("Track " + juce::String(tracks_.size() + 1),
                              zenith::Track::Type::Audio);

    // Prepare track for audio processing if engine is already running
    if (currentSampleRate.load() > 0) {
      track->prepareToPlay(currentBufferSize.load(), currentSampleRate.load());
    }

    track->setTrackIndex((int)tracks_.size());
    tracks_.push_back(std::move(
        track)); // Corrected: transfer ownership from unique_ptr to shared_ptr

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
    auto track =
        std::shared_ptr<zenith::Track>(zenith::Track::create(name, trackType));

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
  jassert(track != nullptr);

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

  if (index >= 0 && index < static_cast<int>(tracks_.size())) {
    juce::String name = tracks_[index]->getName();
    juce::String id = tracks_[index]->getTrackId();

    // Release resources
    tracks_[index]->releaseResources();

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

void Engine::updateTrackSnapshot() {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  // Create new snapshot
  // Include Aux Buses in snapshot for consistent audio thread access
  auto newSnapshot = std::make_shared<TrackSnapshot>(tracks_, auxBuses_);

  // [DSP Optimization] Update routing graph snapshot with direct pointers for
  // fast lookup
  {
    std::vector<Track *> trackPtrs;
    trackPtrs.reserve(tracks_.size());
    for (const auto &track : tracks_) {
      if (track)
        trackPtrs.push_back(track.get());
    }

    std::vector<AuxBus *> auxBusPtrs;
    auxBusPtrs.reserve(auxBuses_.size());
    for (const auto &bus : auxBuses_) {
      if (bus)
        auxBusPtrs.push_back(bus.get());
    }

    routingGraph_.updateSnapshotWithPointers(trackPtrs, auxBusPtrs);
  }

  // Atomic swap
  // The audio thread will see the new pointer immediately
  activeSnapshot_.store(newSnapshot.get(), std::memory_order_release);
  
  // Defer deletion of old snapshot
  if (currentSnapshot_) {
    RealTimeGarbageCollector::getInstance().push(currentSnapshot_);
  }
  
  currentSnapshot_ = newSnapshot;
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
    audioRenderer_->prepare(sampleRate, samplesPerBlockExpected, tracks_.size(),
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
      this->trackMap[track->getTrackId()] = track.get();
    }
  }

  this->auxBuses.reserve(ownedBuses.size());
  this->lifecycleAux.reserve(ownedBuses.size());
  for (const auto &bus : ownedBuses) {
    if (bus != nullptr) {
      this->auxBuses.push_back(bus.get());
      this->lifecycleAux.push_back(bus);
      this->auxBusMap[bus->getId()] = bus.get();
    }
  }
}

} // namespace zenith
