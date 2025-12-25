/*
  ==============================================================================

    TrackStateManager.cpp
    Created: 2025-12-11
    Author:  Zenith DAW

    Track management implementation for ProjectState.

  ==============================================================================
*/

#include "TrackStateManager.h"
#include "ProjectState.h"

namespace zenith {

//==============================================================================
// Auto-color palette for tracks
//==============================================================================

static const juce::Colour kTrackColors[] = {
    juce::Colour(0xFFE57373), // Red
    juce::Colour(0xFF81C784), // Green
    juce::Colour(0xFF64B5F6), // Blue
    juce::Colour(0xFFFFD54F), // Yellow
    juce::Colour(0xFFBA68C8), // Purple
    juce::Colour(0xFF4DD0E1), // Cyan
    juce::Colour(0xFFFFB74D), // Orange
    juce::Colour(0xFF90A4AE), // Gray
    juce::Colour(0xFFF06292), // Pink
    juce::Colour(0xFFAED581), // Light Green
    juce::Colour(0xFF7986CB), // Indigo
    juce::Colour(0xFFDCE775), // Lime
};

static constexpr int kNumAutoColors =
    sizeof(kTrackColors) / sizeof(kTrackColors[0]);

//==============================================================================
// Constructor
//==============================================================================

TrackStateManager::TrackStateManager(ProjectState &projectState)
    : projectState_(projectState) {}

//==============================================================================
// Track Creation/Deletion
//==============================================================================

juce::String TrackStateManager::addTrack(const juce::String &name,
                                         const juce::String &type) {
  auto tracksNode = getTracksContainer();
  if (!tracksNode.isValid())
    return {};

  // Generate unique ID
  juce::String trackId = generateTrackId();
  int numTracks = tracksNode.getNumChildren();

  // Create track node
  juce::ValueTree track(ProjectState::ID_TRACK);
  track.setProperty(ProjectState::PROP_ID, trackId, nullptr);
  track.setProperty(ProjectState::PROP_NAME, name, nullptr);
  track.setProperty(ProjectState::PROP_TYPE, type, nullptr);
  track.setProperty(ProjectState::PROP_VOLUME, 1.0f, nullptr);
  track.setProperty(ProjectState::PROP_PAN, 0.0f, nullptr);
  track.setProperty(ProjectState::PROP_MUTE, false, nullptr);
  track.setProperty(ProjectState::PROP_SOLO, false, nullptr);
  track.setProperty(ProjectState::PROP_ARMED, false, nullptr);

  // Auto-assign color
  juce::Colour autoColor = getAutoColor(numTracks);
  track.setProperty(ProjectState::PROP_COLOR, autoColor.toString(), nullptr);
  track.setProperty(ProjectState::PROP_MANUALLY_COLORED, false, nullptr);

  // Create empty clips container with ID for CRDT sync
  juce::ValueTree clipsNode(ProjectState::ID_CLIPS);
  clipsNode.setProperty(ProjectState::PROP_ID, trackId + "_clips", nullptr);
  track.addChild(clipsNode, -1, nullptr);

  // Create empty automation container with ID for CRDT sync
  juce::ValueTree automationNode(ProjectState::ID_AUTOMATION);
  automationNode.setProperty(ProjectState::PROP_ID, trackId + "_automation", nullptr);
  track.addChild(automationNode, -1, nullptr);

  // Add with undo
  tracksNode.addChild(track, -1, &projectState_.getUndoManager());

  DBG("TrackStateManager: Added track '" + name + "' (ID: " + trackId + ")");

  return trackId;
}

void TrackStateManager::removeTrack(const juce::String &trackId) {
  auto tracksNode = getTracksContainer();
  if (!tracksNode.isValid())
    return;

  auto track = findTrack(trackId);
  if (track.isValid()) {
    // FIX: Pass address
    tracksNode.removeChild(track, &projectState_.getUndoManager());
    DBG("TrackStateManager: Removed track ID: " + trackId);
  }
}

void TrackStateManager::removeTrackByIndex(int index) {
  auto tracksNode = getTracksContainer();
  if (!tracksNode.isValid())
    return;

  if (index >= 0 && index < tracksNode.getNumChildren()) {
    // FIX: Pass address
    tracksNode.removeChild(index, &projectState_.getUndoManager());
    DBG("TrackStateManager: Removed track at index: " + juce::String(index));
  }
}

//==============================================================================
// Track Queries
//==============================================================================

int TrackStateManager::getNumTracks() const {
  auto tracksNode = getTracksContainer();
  if (!tracksNode.isValid())
    return 0;

  int count = 0;
  for (auto child : tracksNode) {
    if (child.hasType(ProjectState::ID_TRACK))
      ++count;
  }
  return count;
}

juce::ValueTree TrackStateManager::getTrack(const juce::String &trackId) const {
  return findTrack(trackId);
}

juce::ValueTree TrackStateManager::getTrackByIndex(int index) const {
  auto tracksNode = getTracksContainer();
  if (!tracksNode.isValid())
    return {};

  int currentIndex = 0;
  for (auto child : tracksNode) {
    if (child.hasType(ProjectState::ID_TRACK)) {
      if (currentIndex == index)
        return child;
      ++currentIndex;
    }
  }
  return {};
}

int TrackStateManager::getTrackIndex(const juce::String &trackId) const {
  auto tracksNode = getTracksContainer();
  if (!tracksNode.isValid())
    return -1;

  int index = 0;
  for (auto child : tracksNode) {
    if (child.hasType(ProjectState::ID_TRACK)) {
      if (child[ProjectState::PROP_ID].toString() == trackId)
        return index;
      ++index;
    }
  }
  return -1;
}

//==============================================================================
// Track Properties - Getters
//==============================================================================

juce::String
TrackStateManager::getTrackName(const juce::String &trackId) const {
  auto track = findTrack(trackId);
  return track.isValid() ? track[ProjectState::PROP_NAME].toString()
                         : juce::String();
}

juce::String
TrackStateManager::getTrackType(const juce::String &trackId) const {
  auto track = findTrack(trackId);
  return track.isValid() ? track[ProjectState::PROP_TYPE].toString()
                         : juce::String();
}

float TrackStateManager::getTrackVolume(const juce::String &trackId) const {
  auto track = findTrack(trackId);
  return track.isValid() ? static_cast<float>(track[ProjectState::PROP_VOLUME])
                         : 1.0f;
}

float TrackStateManager::getTrackPan(const juce::String &trackId) const {
  auto track = findTrack(trackId);
  return track.isValid() ? static_cast<float>(track[ProjectState::PROP_PAN])
                         : 0.0f;
}

bool TrackStateManager::isTrackMuted(const juce::String &trackId) const {
  auto track = findTrack(trackId);
  return track.isValid() ? static_cast<bool>(track[ProjectState::PROP_MUTE])
                         : false;
}

bool TrackStateManager::isTrackSolo(const juce::String &trackId) const {
  auto track = findTrack(trackId);
  return track.isValid() ? static_cast<bool>(track[ProjectState::PROP_SOLO])
                         : false;
}

bool TrackStateManager::isTrackArmed(const juce::String &trackId) const {
  auto track = findTrack(trackId);
  return track.isValid() ? static_cast<bool>(track[ProjectState::PROP_ARMED])
                         : false;
}

bool TrackStateManager::isTrackInputMonitoring(
    const juce::String &trackId) const {
  auto track = findTrack(trackId);
  return track.isValid()
             ? static_cast<bool>(track[ProjectState::PROP_INPUT_MONITOR])
             : false;
}

juce::Colour
TrackStateManager::getTrackColor(const juce::String &trackId) const {
  auto track = findTrack(trackId);
  if (!track.isValid())
    return juce::Colours::grey;

  juce::String colorStr = track[ProjectState::PROP_COLOR].toString();
  if (colorStr.isNotEmpty())
    return juce::Colour::fromString(colorStr);

  return juce::Colours::grey;
}

//==============================================================================
// Track Properties - Setters
//==============================================================================

void TrackStateManager::setTrackName(const juce::String &trackId,
                                     const juce::String &name,
                                     const juce::String &actionName) {
  auto track = findTrack(trackId);
  if (track.isValid()) {
    // FIX: Dot operator
    projectState_.getUndoManager().beginNewTransaction(actionName);
    // FIX: Pass address
    track.setProperty(ProjectState::PROP_NAME, name,
                      &projectState_.getUndoManager());
  }
}

void TrackStateManager::setTrackVolume(const juce::String &trackId,
                                       float volume,
                                       const juce::String &actionName) {
  auto track = findTrack(trackId);
  if (track.isValid()) {
    projectState_.getUndoManager().beginNewTransaction(actionName);
    track.setProperty(ProjectState::PROP_VOLUME, volume,
                      &projectState_.getUndoManager());
  }
}

void TrackStateManager::setTrackPan(const juce::String &trackId, float pan,
                                    const juce::String &actionName) {
  auto track = findTrack(trackId);
  if (track.isValid()) {
    projectState_.getUndoManager().beginNewTransaction(actionName);
    track.setProperty(ProjectState::PROP_PAN, pan,
                      &projectState_.getUndoManager());
  }
}

void TrackStateManager::setTrackMute(const juce::String &trackId, bool muted,
                                     const juce::String &actionName) {
  auto track = findTrack(trackId);
  if (track.isValid()) {
    projectState_.getUndoManager().beginNewTransaction(actionName);
    track.setProperty(ProjectState::PROP_MUTE, muted,
                      &projectState_.getUndoManager());
  }
}

void TrackStateManager::setTrackSolo(const juce::String &trackId, bool solo,
                                     const juce::String &actionName) {
  auto track = findTrack(trackId);
  if (track.isValid()) {
    projectState_.getUndoManager().beginNewTransaction(actionName);
    track.setProperty(ProjectState::PROP_SOLO, solo,
                      &projectState_.getUndoManager());
  }
}

void TrackStateManager::setTrackArmed(const juce::String &trackId, bool armed,
                                      const juce::String &actionName) {
  auto track = findTrack(trackId);
  if (track.isValid()) {
    projectState_.getUndoManager().beginNewTransaction(actionName);
    track.setProperty(ProjectState::PROP_ARMED, armed,
                      &projectState_.getUndoManager());
  }
}

void TrackStateManager::setTrackInputMonitor(const juce::String &trackId,
                                             bool monitoring,
                                             const juce::String &actionName) {
  auto track = findTrack(trackId);
  if (track.isValid()) {
    projectState_.getUndoManager().beginNewTransaction(actionName);
    track.setProperty(ProjectState::PROP_INPUT_MONITOR, monitoring,
                      &projectState_.getUndoManager());
  }
}

void TrackStateManager::setTrackColor(const juce::String &trackId,
                                      const juce::Colour &color,
                                      bool manuallySet,
                                      const juce::String &actionName) {
  auto track = findTrack(trackId);
  if (track.isValid()) {
    projectState_.getUndoManager().beginNewTransaction(actionName);
    track.setProperty(ProjectState::PROP_COLOR, color.toString(),
                      &projectState_.getUndoManager());
    track.setProperty(ProjectState::PROP_MANUALLY_COLORED, manuallySet,
                      &projectState_.getUndoManager());
  }
}

//==============================================================================
// Track Ordering
//==============================================================================

void TrackStateManager::moveTrack(const juce::String &trackId, int newIndex,
                                  const juce::String &actionName) {
  auto tracksNode = getTracksContainer();
  if (!tracksNode.isValid())
    return;

  auto track = findTrack(trackId);
  if (!track.isValid())
    return;

  projectState_.getUndoManager().beginNewTransaction(actionName);

  int oldIndex = tracksNode.indexOf(track);
  if (oldIndex >= 0 && oldIndex != newIndex) {
    // FIX: Pass address
    tracksNode.moveChild(oldIndex, newIndex, &projectState_.getUndoManager());
  }
}

juce::String TrackStateManager::duplicateTrack(const juce::String &trackId,
                                               const juce::String &actionName) {
  auto tracksNode = getTracksContainer();
  if (!tracksNode.isValid())
    return {};

  auto original = findTrack(trackId);
  if (!original.isValid())
    return {};

  projectState_.getUndoManager().beginNewTransaction(actionName);

  // Deep copy the track
  juce::ValueTree copy = original.createCopy();

  // Generate new ID
  juce::String newId = generateTrackId();
  copy.setProperty(ProjectState::PROP_ID, newId, nullptr);

  // Append "(copy)" to name
  juce::String name = copy[ProjectState::PROP_NAME].toString();
  copy.setProperty(ProjectState::PROP_NAME, name + " (copy)", nullptr);

  // Generate new IDs for all clips
  auto clipsNode = copy.getChildWithName(ProjectState::ID_CLIPS);
  if (clipsNode.isValid()) {
    for (auto clip : clipsNode) {
      if (clip.hasType(ProjectState::ID_CLIP)) {
        juce::String newClipId =
            "clip_" + juce::Uuid().toString().substring(0, 8);
        clip.setProperty(ProjectState::PROP_ID, newClipId, nullptr);
      }
    }
  }

  // Add after original
  int originalIndex = tracksNode.indexOf(original);
  // FIX: Pass address
  tracksNode.addChild(copy, originalIndex + 1, &projectState_.getUndoManager());

  return newId;
}

//==============================================================================
// Internal Helpers
//==============================================================================

juce::ValueTree
TrackStateManager::findTrack(const juce::String &trackId) const {
  auto tracksNode = getTracksContainer();
  if (!tracksNode.isValid())
    return {};

  for (auto child : tracksNode) {
    if (child.hasType(ProjectState::ID_TRACK) &&
        child[ProjectState::PROP_ID].toString() == trackId) {
      return child;
    }
  }
  return {};
}

juce::ValueTree TrackStateManager::getTracksContainer() const {
  return projectState_.getState().getChildWithName(ProjectState::ID_TRACKS);
}

juce::String TrackStateManager::generateTrackId() const {
  return "track_" + juce::Uuid().toString().substring(0, 8);
}

juce::Colour TrackStateManager::getAutoColor(int trackIndex) const {
  return kTrackColors[trackIndex % kNumAutoColors];
}

} // namespace zenith
