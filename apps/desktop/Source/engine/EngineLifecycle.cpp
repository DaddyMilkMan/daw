/**
 * @file EngineLifecycle.cpp
 * @brief Engine initialization, shutdown, and state synchronization
 */

#include "../include/Engine.h"
#include "../include/ProjectState.h"
#include "TrackAutomationSynchronizer.h"
#include "RoutingGraph.h"
#include "AudioFilePool.h"
#include "PluginHost.h"
#include "Track.h"
#include "Clip.h"
#include "MixerChannel.h"
#include "AuxBus.h"
#include "../ui/PluginEditorWindow.h"
#include "../instruments/InstrumentRegistry.h"
#include "../instruments/RegisterBuiltInInstruments.h"
#include <algorithm>

namespace zenith {

//==============================================================================
// Initialization / Shutdown
//==============================================================================

void Engine::setProjectState(ProjectState* state)
{
    DBG("Engine: Setting project state");

    // Stop automation if running
    if (automationSynchronizer)
    {
        automationSynchronizer->stop();
        automationSynchronizer.reset();
    }

    projectState_ = state;

    // Create new automation synchronizer if we have a project state
    if (projectState_ != nullptr)
    {
        automationSynchronizer = std::make_unique<TrackAutomationSynchronizer>(*projectState_, *this);
        DBG("Engine: Created automation synchronizer");

        // Sync tracks with project state
        syncWithProjectState();

        // Sync tempo map
        syncTempoMap();
    }
}

void Engine::syncTempoMap()
{
    if (projectState_ && tempoMap_)
    {
        tempoMap_->updateFromValueTree(projectState_->getTempoMap());
    }
}

void Engine::syncWithProjectState()
{
    DBG("Engine: Syncing with project state");

  if (projectState_ == nullptr) {
    DBG("Engine: No project state, clearing tracks");
    // Remove all track nodes from routing graph
    for (const auto &track : tracks_) {
      routingGraph_.removeNode(track->getTrackId());
    }
    tracks_.clear();
    updateTrackSnapshot();
    return;
  }

  // Get tracks from project state
  auto &state = projectState_->getState();
  auto tracksNode = state.getChildWithName(ProjectState::ID_TRACKS);

  if (!tracksNode.isValid()) {
    DBG("Engine: No tracks in project state");
    // Remove all track nodes from routing graph
    for (const auto &track : tracks_) {
      routingGraph_.removeNode(track->getTrackId());
    }
    tracks_.clear();
    updateTrackSnapshot();
    return;
  }

  const double sampleRate = currentSampleRate.load();
  const int bufferSize = currentBufferSize.load();
  const double tempo = projectState_->getTempo();

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

  std::vector<std::shared_ptr<zenith::Track>> newTracks;
  std::vector<juce::String> processedTrackIds;

  // Sync tracks (Create or Update)
  for (auto trackNode : tracksNode) {
    juce::String trackId = trackNode[ProjectState::PROP_ID].toString();
    processedTrackIds.push_back(trackId);

    // Try to find existing track
    auto it = std::find_if(
        tracks_.begin(), tracks_.end(),
        [&trackId](const auto &t) { return t->getTrackId() == trackId; });

    std::shared_ptr<zenith::Track> track;
    bool isNewTrack = false;

    if (it != tracks_.end()) {
      // Reuse existing track (CRITICAL: Preserves Plugins and Audio State)
      track = *it;
      // DBG("Engine: Updating existing track " + trackId);
    } else {
      // Create new track
      isNewTrack = true;
      juce::String trackName = trackNode[ProjectState::PROP_NAME].toString();
      juce::String trackType = trackNode[ProjectState::PROP_TYPE].toString();

      track = std::make_shared<zenith::Track>(
          trackName, trackType == "midi" ? zenith::Track::Type::MIDI
                                         : zenith::Track::Type::Audio);
      track->setTrackId(trackId);
      
      // Register with RoutingGraph
      RoutingGraph::Node node;
      node.id = trackId;
      node.name = trackName;
      node.type = RoutingGraph::NodeType::Track;
      routingGraph_.addNode(node);
      
      DBG("Engine: Created new track " + trackId);
    }

    // Update properties (for both new and existing)
    track->setName(trackNode[ProjectState::PROP_NAME].toString());
    track->setVolume(trackNode[ProjectState::PROP_VOLUME]);
    track->setPan(trackNode[ProjectState::PROP_PAN]);
    track->setMuted(trackNode[ProjectState::PROP_MUTE]);
    track->setSolo(trackNode[ProjectState::PROP_SOLO]);

    // Prepare if needed (new tracks or if engine started)
    if (isNewTrack && sampleRate > 0) {
      track->prepareToPlay(bufferSize, sampleRate);
    }

    // Sync Clips (Diffing clips is harder without IDs, so we rebuild them for now)
    // NOTE: While this replaces Clip objects, it keeps the Track (and Plugins) alive.
    // Ideally we would diff clips too, but this solves the "Catastrophic Plugin Reload" issue.
    track->clearClips();

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
            // Use AudioFilePool if available
            if (audioFilePool_) {
               clip->setAudioFileFromPool(audioFile, *audioFilePool_);
            } else {
               clip->setAudioFile(audioFile);
            }
            clip->setType(zenith::Clip::Type::Audio);
            // DBG("Engine: Loaded audio file: " + audioFile.getFileName());
          }
        }

        // Prepare clip
        if (sampleRate > 0) {
          clip->prepareToPlay(bufferSize, sampleRate);
        }

        // Set clip as playing (default)
        clip->setPlaying(true);

        // Add clip to track
        track->addClip(std::move(clip));
      }
    }
    
    newTracks.push_back(track);
  }

  // Remove deleted tracks from RoutingGraph
  for (const auto &oldTrack : tracks_) {
    bool stillExists = std::find(processedTrackIds.begin(), processedTrackIds.end(), 
                               oldTrack->getTrackId()) != processedTrackIds.end();
    if (!stillExists) {
       routingGraph_.removeNode(oldTrack->getTrackId());
       DBG("Engine: Removed track " + oldTrack->getTrackId());
    }
  }

  // Atomically swap the track list
  tracks_ = std::move(newTracks);

  DBG("Engine: Synced " + juce::String(tracks_.size()) + " tracks");

  // Update snapshot for audio thread
  updateTrackSnapshot();
}

bool Engine::initialize()
{
    DBG("Engine: Initializing...");

    // Initialize audio device manager
    auto error = deviceManager.initialiseWithDefaultDevices(2, 2);  // 2 in, 2 out

    if (error.isNotEmpty())
    {
        DBG("Engine: Failed to initialize audio device: " + error);
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Audio Device Error",
            "Failed to initialize audio device:\n" + error,
            "OK");
        return false;
    }

    // Get current device setup
    auto setup = deviceManager.getAudioDeviceSetup();

    DBG("Engine: Audio device initialized");
    DBG("  Device: " + setup.outputDeviceName);
    DBG("  Sample Rate: " + juce::String(setup.sampleRate) + " Hz");
    DBG("  Buffer Size: " + juce::String(setup.bufferSize) + " samples");

    // Store settings
    currentSampleRate.store(setup.sampleRate);
    currentBufferSize.store(setup.bufferSize);

    // Add this engine as the audio callback
    deviceManager.addAudioCallback(this);

    // Phase 2A: Enable MIDI input
    enableMidiInput();

    // C3: Optional debug seed (disabled by default; enable with -DZENITH_ENGINE_SEED_DEBUG_TRACKS=ON)
#if defined(JUCE_DEBUG) && defined(ZENITH_ENGINE_SEED_DEBUG_TRACKS)
    DBG("Engine: Seeding debug tracks (ZENITH_ENGINE_SEED_DEBUG_TRACKS enabled)");
    addTestTracks(8);
#endif

    DBG("Engine: Initialization complete!");
    return true;
}

void Engine::shutdown()
{
    DBG("Engine: Shutting down...");

    // Stop playback
    stop();

    // Remove audio callback
    deviceManager.removeAudioCallback(this);

    // Close audio device
    deviceManager.closeAudioDevice();

    // Clear audio file pool
    if (audioFilePool_)
    {
        audioFilePool_->clear();
    }

    DBG("Engine: Shutdown complete");
}

} // namespace zenith
