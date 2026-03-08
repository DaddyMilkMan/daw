/**
 * @file Actions.h
 * @brief UndoableAction subclasses for ProjectState mutations
 */

#pragma once

#include "ProjectState.h"
#include <juce_data_structures/juce_data_structures.h>

namespace zenith {

class ProjectAction : public juce::UndoableAction {
public:
  ProjectAction(ProjectState &state, const juce::String &actionName)
      : projectState(state), actionName(actionName) {}

protected:
  ProjectState &projectState;
  juce::String actionName;

  // Protected wrappers to allow subclasses to access private ProjectState
  // members since friendship is not inherited.
  juce::ValueTree &getStateInternal() {
    return projectState.getState();
  }
  juce::String generateUniqueId(const juce::String &prefix) {
    // Since generateUniqueId is private, we'll create a simple ID
    return prefix + "_" + juce::String(juce::Time::currentTimeMillis());
  }
};

//==============================================================================

/**
 * @brief Action to change a property of a ValueTree
 */
class SetPropertyAction : public ProjectAction {
public:
  SetPropertyAction(ProjectState &state, juce::ValueTree tree,
                    const juce::Identifier &property, const juce::var &newValue,
                    const juce::String &actionName)
      : ProjectAction(state, actionName), targetTree(tree), prop(property),
        value(newValue) {
    oldValue = targetTree.getProperty(prop);
  }

  bool perform() override {
    targetTree.setProperty(prop, value,
                           nullptr); // UndoManager handles this via the Action
    return true;
  }

  bool undo() override {
    targetTree.setProperty(prop, oldValue, nullptr);
    return true;
  }

  int getSizeInUnits() override { return (int)sizeof(*this); }

private:
  juce::ValueTree targetTree;
  juce::Identifier prop;
  juce::var value;
  juce::var oldValue;
};

//==============================================================================

/**
 * @brief Action to set track volume
 */
class SetTrackVolumeAction : public ProjectAction {
public:
  SetTrackVolumeAction(ProjectState &state, const juce::String &trackId,
                       float newValue)
      : ProjectAction(state, "Set Track Volume"), trackId(trackId),
        value(newValue) {
    auto tree = projectState.getTrack(trackId);
    if (tree.isValid())
      oldValue = tree.getProperty(ProjectState::PROP_VOLUME);
  }

  bool perform() override {
    auto tree = projectState.getTrack(trackId);
    if (tree.isValid()) {
      tree.setProperty(ProjectState::PROP_VOLUME, value, nullptr);
      return true;
    }
    return false;
  }

  bool undo() override {
    auto tree = projectState.getTrack(trackId);
    if (tree.isValid()) {
      tree.setProperty(ProjectState::PROP_VOLUME, oldValue, nullptr);
      return true;
    }
    return false;
  }

  int getSizeInUnits() override { return (int)sizeof(*this); }

private:
  juce::String trackId;
  float value;
  float oldValue = 1.0f;
};

//==============================================================================

/**
 * @brief Action to set track pan
 */
class SetTrackPanAction : public ProjectAction {
public:
  SetTrackPanAction(ProjectState &state, const juce::String &trackId,
                    float newValue)
      : ProjectAction(state, "Set Track Pan"), trackId(trackId),
        value(newValue) {
    auto tree = projectState.getTrack(trackId);
    if (tree.isValid())
      oldValue = tree.getProperty(ProjectState::PROP_PAN);
  }

  bool perform() override {
    auto tree = projectState.getTrack(trackId);
    if (tree.isValid()) {
      tree.setProperty(ProjectState::PROP_PAN, value, nullptr);
      return true;
    }
    return false;
  }

  bool undo() override {
    auto tree = projectState.getTrack(trackId);
    if (tree.isValid()) {
      tree.setProperty(ProjectState::PROP_PAN, oldValue, nullptr);
      return true;
    }
    return false;
  }

  int getSizeInUnits() override { return (int)sizeof(*this); }

private:
  juce::String trackId;
  float value;
  float oldValue = 0.0f;
};

//==============================================================================

/**
 * @brief Action to rename a track
 */
class RenameTrackAction : public ProjectAction {
public:
  RenameTrackAction(ProjectState &state, const juce::String &trackId,
                    const juce::String &newName)
      : ProjectAction(state, "Rename Track"), trackId(trackId),
        newName(newName) {
    auto tree = projectState.getTrack(trackId);
    if (tree.isValid())
      oldName = tree.getProperty(ProjectState::PROP_NAME).toString();
  }

  bool perform() override {
    auto tree = projectState.getTrack(trackId);
    if (tree.isValid()) {
      tree.setProperty(ProjectState::PROP_NAME, juce::var(newName), nullptr);
      return true;
    }
    return false;
  }

  bool undo() override {
    auto tree = projectState.getTrack(trackId);
    if (tree.isValid()) {
      tree.setProperty(ProjectState::PROP_NAME, juce::var(oldName), nullptr);
      return true;
    }
    return false;
  }

  int getSizeInUnits() override { return (int)sizeof(*this); }

private:
  juce::String trackId;
  juce::String newName;
  juce::String oldName;
};

//==============================================================================

/**
 * @brief Action to move a clip
 */
class MoveClipAction : public ProjectAction {
public:
  MoveClipAction(ProjectState &state, const juce::String &trackId,
                 const juce::String &clipId, double newStartBeats)
      : ProjectAction(state, "Move Clip"), trackId(trackId), clipId(clipId),
        newStart(newStartBeats) {
    auto clip = projectState.getClip(trackId, clipId);
    if (clip.isValid())
      oldStart = clip.getProperty(ProjectState::PROP_START_BEATS);
  }

  // Overload for sample-based move (legacy support or if still used)
  MoveClipAction(ProjectState &state, const juce::String &trackId,
                 const juce::String &clipId, juce::int64 newStartSamples)
      : ProjectAction(state, "Move Clip"), trackId(trackId), clipId(clipId) {
    // Convert samples to beats if possible, or just support PROP_START for
    // samples Current ProjectState seems to support both beat-based and
    // sample-based clips? Let's assume beat-based for modern clips, but check
    // if we need sample support. ProjectState.h had PROJ_START (sample) and
    // PROJ_START_BEATS.

    // For this action, we'll try to detect which property is used or just set
    // both/either. But to be safe, let's look at ProjectState::moveClip
    // implementation. It had two overloads. We will implement perform/undo
    // based on what properties exist.

    isSampleBased = true;
    newStartSamples_ = newStartSamples;

    auto clip = projectState.getClip(trackId, clipId);
    if (clip.isValid())
      oldStartSamples_ = clip.getProperty(ProjectState::PROP_START);
  }

  bool perform() override {
    auto clip = projectState.getClip(trackId, clipId);
    if (!clip.isValid())
      return false;

    if (isSampleBased) {
      clip.setProperty(ProjectState::PROP_START, newStartSamples_, nullptr);
    } else {
      clip.setProperty(ProjectState::PROP_START_BEATS, newStart, nullptr);
    }
    return true;
  }

  bool undo() override {
    auto clip = projectState.getClip(trackId, clipId);
    if (!clip.isValid())
      return false;

    if (isSampleBased) {
      clip.setProperty(ProjectState::PROP_START, oldStartSamples_, nullptr);
    } else {
      clip.setProperty(ProjectState::PROP_START_BEATS, oldStart, nullptr);
    }
    return true;
  }

  int getSizeInUnits() override { return (int)sizeof(*this); }

private:
  juce::String trackId;
  juce::String clipId;
  double newStart = 0.0;
  double oldStart = 0.0;

  bool isSampleBased = false;
  juce::int64 newStartSamples_ = 0;
  juce::int64 oldStartSamples_ = 0;
};

//==============================================================================

/**
 * @brief Action to resize a clip
 */
class ResizeClipAction : public ProjectAction {
public:
  ResizeClipAction(ProjectState &state, const juce::String &trackId,
                   const juce::String &clipId, double newLengthBeats)
      : ProjectAction(state, "Resize Clip"), trackId(trackId), clipId(clipId),
        newLength(newLengthBeats) {
    auto clip = projectState.getClip(trackId, clipId);
    if (clip.isValid())
      oldLength = clip.getProperty(ProjectState::PROP_LENGTH_BEATS);
  }

  // Sample-based overload
  ResizeClipAction(ProjectState &state, const juce::String &trackId,
                   const juce::String &clipId, juce::int64 newLengthSamples)
      : ProjectAction(state, "Resize Clip"), trackId(trackId), clipId(clipId) {
    isSampleBased = true;
    newLengthSamples_ = newLengthSamples;

    auto clip = projectState.getClip(trackId, clipId);
    if (clip.isValid())
      oldLengthSamples_ = clip.getProperty(ProjectState::PROP_LENGTH);
  }

  bool perform() override {
    auto clip = projectState.getClip(trackId, clipId);
    if (!clip.isValid())
      return false;

    if (isSampleBased) {
      clip.setProperty(ProjectState::PROP_LENGTH, newLengthSamples_, nullptr);
    } else {
      clip.setProperty(ProjectState::PROP_LENGTH_BEATS, newLength, nullptr);
    }
    return true;
  }

  bool undo() override {
    auto clip = projectState.getClip(trackId, clipId);
    if (!clip.isValid())
      return false;

    if (isSampleBased) {
      clip.setProperty(ProjectState::PROP_LENGTH, oldLengthSamples_, nullptr);
    } else {
      clip.setProperty(ProjectState::PROP_LENGTH_BEATS, oldLength, nullptr);
    }
    return true;
  }

  int getSizeInUnits() override { return (int)sizeof(*this); }

private:
  juce::String trackId;
  juce::String clipId;
  double newLength = 1.0;
  double oldLength = 1.0;

  bool isSampleBased = false;
  juce::int64 newLengthSamples_ = 0;
  juce::int64 oldLengthSamples_ = 0;
};

//==============================================================================

/**
 * @brief Action to add a new track
 */
class AddTrackAction : public ProjectAction {
public:
  AddTrackAction(ProjectState &state, const juce::String &trackName,
                 const juce::String &trackType)
      : ProjectAction(state, "Add Track"), name(trackName), type(trackType) {}

  bool perform() override {
    // We'll use the internal state access to bypass the UndoManager on the
    // ValueTree directly, as this Action itself is being managed by the
    // project's UndoManager.
    auto &state = getStateInternal();
    auto tracks =
        state.getOrCreateChildWithName(ProjectState::ID_TRACKS, nullptr);

    juce::ValueTree newTrack(ProjectState::ID_TRACK);
    trackId = generateUniqueId("track");
    newTrack.setProperty(ProjectState::PROP_ID, juce::var(trackId), nullptr);
    newTrack.setProperty(ProjectState::PROP_NAME, juce::var(name), nullptr);
    newTrack.setProperty(ProjectState::PROP_TYPE, juce::var(type), nullptr);
    newTrack.setProperty(ProjectState::PROP_VOLUME, 0.75f, nullptr);
    newTrack.setProperty(ProjectState::PROP_ID, juce::var(trackId), nullptr);
    newTrack.setProperty(ProjectState::PROP_MUTE, false, nullptr);
    newTrack.setProperty(ProjectState::PROP_SOLO, false, nullptr);
    newTrack.setProperty(ProjectState::PROP_ARMED, false, nullptr);

    tracks.addChild(newTrack, -1, nullptr);
    return true;
  }

  bool undo() override {
    auto &state = getStateInternal();
    auto tracks = state.getChildWithName(ProjectState::ID_TRACKS);
    if (!tracks.isValid())
      return false;

    for (int i = 0; i < tracks.getNumChildren(); ++i) {
      if (tracks.getChild(i).getProperty(ProjectState::PROP_ID).toString() ==
          trackId) {
        tracks.removeChild(i, nullptr);
        return true;
      }
    }
    return false;
  }

  juce::String getTrackId() const { return trackId; }

  int getSizeInUnits() override { return (int)sizeof(*this); }

private:
  juce::String name;
  juce::String type;
  juce::String trackId;
};

//==============================================================================

/**
 * @brief Action to remove a track
 */
class RemoveTrackAction : public ProjectAction {
public:
  RemoveTrackAction(ProjectState &state, const juce::String &trackId)
      : ProjectAction(state, "Remove Track"), trackId(trackId) {
    auto &stateInternal = getStateInternal();
    auto tracks = stateInternal.getChildWithName(ProjectState::ID_TRACKS);
    if (tracks.isValid()) {
      for (int i = 0; i < tracks.getNumChildren(); ++i) {
        if (tracks.getChild(i).getProperty(ProjectState::PROP_ID).toString() ==
            trackId) {
          trackTree = tracks.getChild(i).createCopy();
          index = i;
          break;
        }
      }
    }
  }

  bool perform() override {
    auto &state = getStateInternal();
    auto tracks = state.getChildWithName(ProjectState::ID_TRACKS);
    if (!tracks.isValid())
      return false;

    for (int i = 0; i < tracks.getNumChildren(); ++i) {
      if (tracks.getChild(i).getProperty(ProjectState::PROP_ID).toString() ==
          trackId) {
        tracks.removeChild(i, nullptr);
        return true;
      }
    }
    return false;
  }

  bool undo() override {
    if (!trackTree.isValid())
      return false;

    auto &state = getStateInternal();
    auto tracks =
        state.getOrCreateChildWithName(ProjectState::ID_TRACKS, nullptr);
    tracks.addChild(trackTree.createCopy(), index, nullptr);
    return true;
  }

  int getSizeInUnits() override { return (int)sizeof(*this); }

private:
  juce::String trackId;
  juce::ValueTree trackTree;
  int index = -1;
};

//==============================================================================

/**
 * @brief Action to create a clip
 */
class CreateClipAction : public ProjectAction {
public:
  CreateClipAction(ProjectState &state, const juce::String &trackId,
                   const juce::String &type, juce::int64 start,
                   juce::int64 length, const juce::String &name)
      : ProjectAction(state, "Create Clip"), trackId(trackId), type(type),
        startSamples(start), lengthSamples(length), clipName(name) {
    isSampleBased = true;
  }

  CreateClipAction(ProjectState &state, const juce::String &trackId,
                   const juce::String &type, double startBeats,
                   double lengthBeats, const juce::String &name)
      : ProjectAction(state, "Create Clip"), trackId(trackId), type(type),
        startBeats(startBeats), lengthBeats(lengthBeats), clipName(name) {
    isSampleBased = false;
  }

  bool perform() override {
    auto trackTree = projectState.getTrack(trackId);
    if (!trackTree.isValid())
      return false;

    auto clips =
        trackTree.getOrCreateChildWithName(ProjectState::ID_CLIPS, nullptr);
    juce::ValueTree clip(ProjectState::ID_CLIP);

    clipId = generateUniqueId("clip");
    clip.setProperty(ProjectState::PROP_ID, juce::var(clipId), nullptr);
    clip.setProperty(ProjectState::PROP_TYPE, juce::var(type), nullptr);
    clip.setProperty(ProjectState::PROP_NAME, juce::var(clipName), nullptr);

    if (isSampleBased) {
      clip.setProperty(ProjectState::PROP_START, startSamples, nullptr);
      clip.setProperty(ProjectState::PROP_LENGTH, lengthSamples, nullptr);
    } else {
      clip.setProperty(ProjectState::PROP_START_BEATS, startBeats, nullptr);
      clip.setProperty(ProjectState::PROP_LENGTH_BEATS, lengthBeats, nullptr);
    }

    clips.addChild(clip, -1, nullptr);
    return true;
  }

  bool undo() override {
    auto trackTree = projectState.getTrack(trackId);
    if (!trackTree.isValid())
      return false;

    auto clips = trackTree.getChildWithName(ProjectState::ID_CLIPS);
    if (!clips.isValid())
      return false;

    for (int i = 0; i < clips.getNumChildren(); ++i) {
      if (clips.getChild(i).getProperty(ProjectState::PROP_ID).toString() ==
          clipId) {
        clips.removeChild(i, nullptr);
        return true;
      }
    }
    return false;
  }

  juce::String getClipId() const { return clipId; }

  int getSizeInUnits() override { return (int)sizeof(*this); }

private:
  juce::String trackId;
  juce::String clipId;
  juce::String type;
  juce::String clipName;
  bool isSampleBased = false;
  juce::int64 startSamples = 0;
  juce::int64 lengthSamples = 0;
  double startBeats = 0.0;
  double lengthBeats = 0.0;
};

//==============================================================================

/**
 * @brief Action to delete a clip
 */
class DeleteClipAction : public ProjectAction {
public:
  DeleteClipAction(ProjectState &state, const juce::String &trackId,
                   const juce::String &clipId)
      : ProjectAction(state, "Delete Clip"), trackId(trackId), clipId(clipId) {
    auto trackTree = projectState.getTrack(trackId);
    if (trackTree.isValid()) {
      auto clips = trackTree.getChildWithName(ProjectState::ID_CLIPS);
      if (clips.isValid()) {
        for (int i = 0; i < clips.getNumChildren(); ++i) {
          if (clips.getChild(i).getProperty(ProjectState::PROP_ID).toString() ==
              clipId) {
            clipTree = clips.getChild(i).createCopy();
            index = i;
            break;
          }
        }
      }
    }
  }

  bool perform() override {
    auto trackTree = projectState.getTrack(trackId);
    if (!trackTree.isValid())
      return false;

    auto clips = trackTree.getChildWithName(ProjectState::ID_CLIPS);
    if (!clips.isValid())
      return false;

    for (int i = 0; i < clips.getNumChildren(); ++i) {
      if (clips.getChild(i).getProperty(ProjectState::PROP_ID).toString() ==
          clipId) {
        clips.removeChild(i, nullptr);
        return true;
      }
    }
    return false;
  }

  bool undo() override {
    if (!clipTree.isValid())
      return false;

    auto trackTree = projectState.getTrack(trackId);
    if (!trackTree.isValid())
      return false;

    auto clips =
        trackTree.getOrCreateChildWithName(ProjectState::ID_CLIPS, nullptr);
    clips.addChild(clipTree.createCopy(), index, nullptr);
    return true;
  }

  int getSizeInUnits() override { return (int)sizeof(*this); }

private:
  juce::String trackId;
  juce::String clipId;
  juce::ValueTree clipTree;
  int index = -1;
};

/**
 * @brief Action to set global tempo
 */
class SetTempoAction : public ProjectAction {
public:
  SetTempoAction(ProjectState &state, double newBpm)
      : ProjectAction(state, "Set Tempo"), newBpm(newBpm) {
    oldBpm = projectState.getTempo();
  }

  bool perform() override {
    auto &state = getStateInternal();
    state.setProperty(ProjectState::PROP_TEMPO, newBpm, nullptr);
    return true;
  }

  bool undo() override {
    auto &state = getStateInternal();
    state.setProperty(ProjectState::PROP_TEMPO, oldBpm, nullptr);
    return true;
  }

  int getSizeInUnits() override { return (int)sizeof(*this); }

private:
  double newBpm;
  double oldBpm;
};

//==============================================================================

/**
 * @brief Action to set time signature
 */
class SetTimeSignatureAction : public ProjectAction {
public:
  SetTimeSignatureAction(ProjectState &state, int num, int den)
      : ProjectAction(state, "Set Time Signature"), newNum(num), newDen(den) {
    oldNum = projectState.getTimeSignatureNumerator();
    oldDen = projectState.getTimeSignatureDenominator();
  }

  bool perform() override {
    auto &state = getStateInternal();
    state.setProperty(ProjectState::PROP_TIME_SIG_NUM, newNum, nullptr);
    state.setProperty(ProjectState::PROP_TIME_SIG_DEN, newDen, nullptr);
    return true;
  }

  bool undo() override {
    auto &state = getStateInternal();
    state.setProperty(ProjectState::PROP_TIME_SIG_NUM, oldNum, nullptr);
    state.setProperty(ProjectState::PROP_TIME_SIG_DEN, oldDen, nullptr);
    return true;
  }

  int getSizeInUnits() override { return (int)sizeof(*this); }

private:
  int newNum, newDen;
  int oldNum, oldDen;
};

//==============================================================================

/**
 * @brief Action to add a tempo point
 */
class AddTempoPointAction : public ProjectAction {
public:
  AddTempoPointAction(ProjectState &state, double timeBeats, double bpm)
      : ProjectAction(state, "Add Tempo Point"), timeBeats(timeBeats),
        bpm(bpm) {}

  bool perform() override {
    auto &state = getStateInternal();
    auto tempoMap =
        state.getOrCreateChildWithName(ProjectState::ID_TEMPO_MAP, nullptr);

    juce::ValueTree point(ProjectState::ID_TEMPO_POINT);
    pointId = generateUniqueId("tempo_point");
    point.setProperty(ProjectState::PROP_ID, juce::var(pointId), nullptr);
    point.setProperty(ProjectState::PROP_TIME_BEATS, timeBeats, nullptr);
    point.setProperty(ProjectState::PROP_BPM, bpm, nullptr);

    tempoMap.addChild(point, -1, nullptr);
    return true;
  }

  bool undo() override {
    auto &state = getStateInternal();
    auto tempoMap = state.getChildWithName(ProjectState::ID_TEMPO_MAP);
    if (!tempoMap.isValid())
      return false;

    for (int i = 0; i < tempoMap.getNumChildren(); ++i) {
      if (tempoMap.getChild(i).getProperty(ProjectState::PROP_ID).toString() ==
          pointId) {
        tempoMap.removeChild(i, nullptr);
        return true;
      }
    }
    return false;
  }

  juce::String getPointId() const { return pointId; }

  int getSizeInUnits() override { return (int)sizeof(*this); }

private:
  double timeBeats;
  double bpm;
  juce::String pointId;
};

} // namespace zenith
