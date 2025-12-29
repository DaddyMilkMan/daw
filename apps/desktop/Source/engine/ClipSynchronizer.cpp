/**
 * @file ClipSynchronizer.cpp
 * @brief ClipSynchronizer implementation - bidirectional sync between ProjectState and Engine clips
 */

#include "ClipSynchronizer.h"
#include "../../Source/engine/Clip.h"
#include "../../Source/engine/Track.h"
#include "ClipTrack.h"
#include "ZenithLogger.h"

namespace zenith {

//==============================================================================
ClipSynchronizer::ClipSynchronizer(ProjectState &ps, Engine &eng)
    : projectState(ps), engine(eng) {
  DBG("ClipSynchronizer: Constructor");
}

ClipSynchronizer::~ClipSynchronizer() {
  stop();
  DBG("ClipSynchronizer: Destructor");
}

//==============================================================================
void ClipSynchronizer::start(int updateRateHz) {
  if (updateRateHz <= 0)
    updateRateHz = 30;

  projectState.getState().addListener(this);
  if (juce::MessageManager::getInstanceWithoutCreating() != nullptr)
      if (juce::MessageManager::getInstanceWithoutCreating() != nullptr) startTimer(1000 / updateRateHz);
  DBG("ClipSynchronizer: Started at " + juce::String(updateRateHz) + " Hz");
}

void ClipSynchronizer::stop() {
  projectState.getState().removeListener(this);
  stopTimer();
  DBG("ClipSynchronizer: Stopped");
}

//==============================================================================
juce::String ClipSynchronizer::createClip(const juce::String &trackId,
                                          double startBeats, double lengthBeats,
                                          const juce::String &clipType) {
  DBG("ClipSynchronizer: createClip(" + trackId + ", " +
      juce::String(startBeats) + ", " + juce::String(lengthBeats) + ", " +
      clipType + ")");
      
  // Consistency checks
  jassert(startBeats >= 0.0);
  jassert(lengthBeats > 0.0);
  jassert(clipType == "audio" || clipType == "midi");

  // 1. Create in zenith::ProjectState first to generate ID
  auto &state = projectState.getState();
  auto tracksNode = state.getChildWithName(zenith::ProjectState::ID_TRACKS);

  if (!tracksNode.isValid())
    return {};

  juce::String newClipId;

  // Find track (need mutable reference to appendChild)
  for (auto track : tracksNode) {
    if (track[zenith::ProjectState::PROP_ID].toString() == trackId) {
      auto clipsNode = track.getChildWithName(zenith::ProjectState::ID_CLIPS);
      if (!clipsNode.isValid()) {
        clipsNode = juce::ValueTree(zenith::ProjectState::ID_CLIPS);
        track.appendChild(clipsNode, nullptr);
      }

      // Create clip
      juce::ValueTree clip(zenith::ProjectState::ID_CLIP);
      newClipId = "clip_" + juce::Uuid().toString().substring(0, 8);
      clip.setProperty(zenith::ProjectState::PROP_ID, newClipId, nullptr);
      clip.setProperty(zenith::ProjectState::PROP_TYPE, clipType, nullptr);
      clip.setProperty(zenith::ProjectState::PROP_START, startBeats, nullptr);
      clip.setProperty(zenith::ProjectState::PROP_LENGTH, lengthBeats, nullptr);

      // This will trigger valueTreeChildAdded, which will update the Engine
      clipsNode.appendChild(clip, &projectState.getUndoManager());
      break;
    }
  }

  if (newClipId.isEmpty()) {
    DBG("ClipSynchronizer: Failed to find track in zenith::ProjectState: " +
        trackId);
    return {};
  }

  DBG("ClipSynchronizer: Created clip " + newClipId +
      " (Engine sync via listener)");
  return newClipId;
}

//==============================================================================
void ClipSynchronizer::timerCallback() {
  // Sync Engine clips to zenith::ProjectState
  syncEngineToProjectState();
}

//==============================================================================
//==============================================================================
void ClipSynchronizer::syncEngineToProjectState() {
  // Guard against re-entrant checks
  if (isModifyingState)
    return;
    
  // Safety check: Engine must be initialized with valid sample rate
  if (engine.getSampleRate() <= 0.0)
     return;
     
  // Check buffer size consistency (sanity check)
  jassert(engine.getBufferSize() > 0);

  isModifyingState = true;

  // This method implements Engine→ProjectState sync for recorded clips.
  // Called from timer (Message Thread), so safe to modify ProjectState.

  // Get all Engine tracks (read-only access, should be lock-free)
  const auto &engineTracks = engine.tracks();

  // Iterate Engine tracks to find new/modified clips
  for (const auto &trackPtr : engineTracks) {
    if (!trackPtr)
      continue;

    juce::String trackId = trackPtr->getTrackId();

    // Get corresponding zenith::ProjectState track
    auto projectTrack = projectState.getTrack(trackId);
    if (!projectTrack.isValid()) {
      // Typically skips if track not in UI, but Engine might have temp tracks
      continue;
    }

    // Get zenith::ProjectState clips container
    auto clipsNode =
        projectTrack.getChildWithName(zenith::ProjectState::ID_CLIPS);
    if (!clipsNode.isValid()) {
      clipsNode = juce::ValueTree(zenith::ProjectState::ID_CLIPS);
      projectTrack.appendChild(clipsNode, nullptr);
    }

    // Sync each Engine clip to zenith::ProjectState
    for (int i = 0; i < trackPtr->getNumClips(); ++i) {
      auto* engineClip = trackPtr->getClip(i);
      
      if (engineClip == nullptr)
          continue;

      // Check if this clip exists in zenith::ProjectState
      juce::String clipId = engineClip->getName(); // Assuming Name == ID
      bool foundInProjectState = false;

      for (auto clipNode : clipsNode) {
        if (clipNode[zenith::ProjectState::PROP_ID].toString() == clipId) {
          foundInProjectState = true;

          // Update clip properties if changed
          int64_t engineStart = engineClip->getStartPosition();
          int64_t engineLength = engineClip->getLength();

          // Convert samples to beats
          double tempo = projectState.getTempo();
          double sampleRate = engine.getSampleRate();

          double startBeats = samplesToBeats(engineStart, tempo, sampleRate);
          double lengthBeats = samplesToBeats(engineLength, tempo, sampleRate);

          // Update if different (with small tolerance for float precision)
          double currentStart = clipNode[zenith::ProjectState::PROP_START];
          double currentLength = clipNode[zenith::ProjectState::PROP_LENGTH];

          const double tolerance = 0.001; // ~1ms at 120bpm
          if (std::abs(currentStart - startBeats) > tolerance ||
              std::abs(currentLength - lengthBeats) > tolerance) {

            clipNode.setProperty(zenith::ProjectState::PROP_START, startBeats,
                                 &projectState.getUndoManager());
            clipNode.setProperty(zenith::ProjectState::PROP_LENGTH, lengthBeats,
                                 &projectState.getUndoManager());
          }
          break;
        }
      }

      // If clip not found in zenith::ProjectState, it was just recorded - add
      // it
      if (!foundInProjectState) {
        juce::ValueTree newClip(zenith::ProjectState::ID_CLIP);

        // If Name matches an ID format, use it, otherwise generate new
        if (clipId.isEmpty()) {
          clipId = "clip_" + juce::Uuid().toString().substring(0, 8);
          engineClip->setName(clipId);
        }

        newClip.setProperty(zenith::ProjectState::PROP_ID, clipId, nullptr);
        newClip.setProperty(zenith::ProjectState::PROP_NAME, clipId, nullptr);
        newClip.setProperty(
            zenith::ProjectState::PROP_TYPE,
            (engineClip->getType() == zenith::Clip::Type::MIDI
                 ? "midi"
                 : "audio"),
            nullptr);

        // Convert samples to beats
        double tempo = projectState.getTempo();
        double sampleRate = engine.getSampleRate();

        double startBeats =
            samplesToBeats(engineClip->getStartPosition(), tempo, sampleRate);
        double lengthBeats =
            samplesToBeats(engineClip->getLength(), tempo, sampleRate);

        newClip.setProperty(zenith::ProjectState::PROP_START, startBeats,
                            nullptr);
        newClip.setProperty(zenith::ProjectState::PROP_LENGTH, lengthBeats,
                            nullptr);

        clipsNode.appendChild(newClip, &projectState.getUndoManager());

        DBG("ClipSynchronizer: Added new recorded clip " + clipId +
            " to track " + trackId);
      }
    }
  }

  isModifyingState = false;
}

//==============================================================================
int64_t ClipSynchronizer::beatsToSamples(double beats, double tempo,
                                         double sampleRate) const {
  // beats * (60 / tempo) * sampleRate = samples
  if (tempo <= 0.0)
    tempo = 120.0;
  double seconds = beats * (60.0 / tempo);
  return static_cast<int64_t>(seconds * sampleRate);
}

double ClipSynchronizer::samplesToBeats(int64_t samples, double tempo,
                                        double sampleRate) const {
  // samples / sampleRate / (60 / tempo) = beats
  if (sampleRate <= 0.0)
    return 0.0;
  if (tempo <= 0.0)
    tempo = 120.0;
  double seconds = static_cast<double>(samples) / sampleRate;
  return seconds / (60.0 / tempo);
}

//==============================================================================
// juce::ValueTree::Listener overrides
void ClipSynchronizer::valueTreePropertyChanged(
    juce::ValueTree &treeWhosePropertyHasChanged,
    const juce::Identifier &property) {

  if (isModifyingState)
    return;
  if (!treeWhosePropertyHasChanged.hasType(zenith::ProjectState::ID_CLIP))
    return;

  // Only interested in start/length/start_offset/mute
  if (property != zenith::ProjectState::PROP_START &&
      property != zenith::ProjectState::PROP_LENGTH) {
    return;
  }

  // Find Track
  auto clipsNode = treeWhosePropertyHasChanged.getParent();
  if (!clipsNode.isValid() ||
      clipsNode.getType() != zenith::ProjectState::ID_CLIPS)
    return;

  auto trackNode = clipsNode.getParent();
  if (!trackNode.isValid())
    return;

  juce::String trackId = trackNode[zenith::ProjectState::PROP_ID].toString();
  juce::String clipId =
      treeWhosePropertyHasChanged[zenith::ProjectState::PROP_ID].toString();

  // Find in Engine
  for (const auto &trackPtr : engine.tracks()) {
    if (trackPtr->getTrackId() == trackId) {
      for (int i = 0; i < trackPtr->getNumClips(); ++i) {
        auto* clipPtr = trackPtr->getClip(i);
        if (clipPtr != nullptr && clipPtr->getName() == clipId) {
          // Found it, sync properties
          double tempo = projectState.getTempo();
          double sampleRate = engine.getSampleRate();
          double startBeats =
              treeWhosePropertyHasChanged[zenith::ProjectState::PROP_START];
          double lenBeats =
              treeWhosePropertyHasChanged[zenith::ProjectState::PROP_LENGTH];

          clipPtr->setStartPosition(
              beatsToSamples(startBeats, tempo, sampleRate));
          clipPtr->setLength(beatsToSamples(lenBeats, tempo, sampleRate));
          DBG("ClipSynchronizer: Synced prop change for " + clipId);
          return;
        }
      }
    }
  }
}

void ClipSynchronizer::valueTreeChildAdded(
    juce::ValueTree &parentTree, juce::ValueTree &childWhichHasBeenAdded) {

  if (isModifyingState)
    return;

  if (childWhichHasBeenAdded.hasType(zenith::ProjectState::ID_CLIP) &&
      parentTree.hasType(zenith::ProjectState::ID_CLIPS)) {

    auto trackNode = parentTree.getParent();
    if (!trackNode.isValid())
      return;
    juce::String trackId = trackNode[zenith::ProjectState::PROP_ID].toString();

    // Check if already in Engine (to avoid duplication if logic elsewhere)
    // But assuming we trust this flow

    juce::String clipId =
        childWhichHasBeenAdded[zenith::ProjectState::PROP_ID].toString();
    juce::String clipType =
        childWhichHasBeenAdded[zenith::ProjectState::PROP_TYPE].toString();
    double startBeats =
        childWhichHasBeenAdded[zenith::ProjectState::PROP_START];
    double lengthBeats =
        childWhichHasBeenAdded[zenith::ProjectState::PROP_LENGTH];

    // Add to Engine
    for (const auto &trackPtr : engine.tracks()) {
      if (trackPtr->getTrackId() == trackId) {
        auto newClip = std::make_unique<zenith::Clip>();

        double tempo = projectState.getTempo();
        double sampleRate = engine.getSampleRate();

        newClip->setStartPosition(
            beatsToSamples(startBeats, tempo, sampleRate));
        newClip->setLength(beatsToSamples(lengthBeats, tempo, sampleRate));
        newClip->setName(clipId);
        newClip->setType(clipType == "midi" ? zenith::Clip::Type::MIDI
                                            : zenith::Clip::Type::Audio);

        trackPtr->addClip(std::move(newClip));
        DBG("ClipSynchronizer: Added new clip via Listener " + clipId);
        return;
      }
    }
  }
}

void ClipSynchronizer::valueTreeChildRemoved(
    juce::ValueTree &parentTree, juce::ValueTree &childWhichHasBeenRemoved,
    int indexFromWhichChildWasRemoved) {

  juce::ignoreUnused(indexFromWhichChildWasRemoved);
  if (isModifyingState)
    return;

  if (childWhichHasBeenRemoved.hasType(zenith::ProjectState::ID_CLIP) &&
      parentTree.hasType(zenith::ProjectState::ID_CLIPS)) {

    auto trackNode = parentTree.getParent();
    if (!trackNode.isValid())
      return;
    juce::String trackId = trackNode[zenith::ProjectState::PROP_ID].toString();
    juce::String clipId =
        childWhichHasBeenRemoved[zenith::ProjectState::PROP_ID].toString();

    // Remove from Engine
    for (const auto &trackPtr : engine.tracks()) {
      if (trackPtr->getTrackId() == trackId) {
        zenith::ClipTrack* clipTrack = dynamic_cast<zenith::ClipTrack*>(trackPtr.get());
        if (clipTrack == nullptr) {
            // This is a logic error - track should be a ClipTrack to have clips.
            // Log and continue rather than asserting to avoid crash in production.
            ZENITH_LOG_ERROR("ClipSynchronizer: trackPtr is not a ClipTrack during clip removal for track: " + trackId);
            return;
        }

        const int numClips = clipTrack->getNumClips();
        for (int i = 0; i < numClips; ++i) {
          zenith::Clip* clipPtr = clipTrack->getClip(i);
          if (clipPtr != nullptr && clipPtr->getName() == clipId) {
            clipTrack->removeClip(clipPtr);
            DBG("ClipSynchronizer: Removed clip " + clipId);
            return;
          }
        }
      }
    }
  }
}

} // namespace zenith
