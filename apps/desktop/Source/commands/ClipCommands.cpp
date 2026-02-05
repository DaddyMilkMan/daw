#include "ClipCommands.h"
#include "../engine/Clip.h"
#include "../engine/Track.h"
#include "CommandUtils.h"
#include "Engine.h"
#include "ProjectState.h"
#include <algorithm>

namespace zenith {

ClipCommands::ClipCommands(Engine &eng, ProjectState &state, CommandAPI &apiRef)
    : engine(eng), projectState(state), api(apiRef) {}

juce::var ClipCommands::listClips(const juce::var &params) {
  if (!params.hasProperty("trackId"))
    return createErrorResponse("Missing 'trackId' parameter");

  juce::String trackId = params["trackId"].toString();
  Track *track = findTrackById(engine, trackId);

  if (track == nullptr)
    return createErrorResponse("Track not found: " + trackId);

  juce::var clipsArray;
  auto *clipsArrayPtr = clipsArray.getArray();

  for (int i = 0; i < track->getNumClips(); ++i) {
    auto *clip = track->getClip(i);
    if (clip != nullptr) {
      auto *clipObj = new juce::DynamicObject();
      clipObj->setProperty("id", "clip_" + juce::String(i));
      clipObj->setProperty("name", clip->getName());
      clipObj->setProperty("start",
                           static_cast<juce::int64>(clip->getStartPosition()));
      clipObj->setProperty("length",
                           static_cast<juce::int64>(clip->getLength()));
      clipObj->setProperty("offset",
                           static_cast<juce::int64>(clip->getOffset()));

      clipsArrayPtr->add(juce::var(clipObj));
    }
  }

  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("trackId", trackId);
  resultObj->setProperty("clips", clipsArray);
  resultObj->setProperty("count", track->getNumClips());

  return createSuccessResponse(juce::var(resultObj));
}

juce::var ClipCommands::createClip(const juce::var &params) {
  if (!params.hasProperty("trackId"))
    return createErrorResponse("Missing 'trackId' parameter");
  if (!params.hasProperty("type"))
    return createErrorResponse(
        "Missing 'type' parameter (must be 'audio' or 'midi')");
  if (!params.hasProperty("start"))
    return createErrorResponse("Missing 'start' parameter (samples)");
  if (!params.hasProperty("length"))
    return createErrorResponse("Missing 'length' parameter (samples)");

  juce::String trackId = params["trackId"].toString();
  juce::String clipType = params["type"].toString().toLowerCase();
  juce::int64 startSamples = params["start"];
  juce::int64 lengthSamples = params["length"];
  juce::String clipName = params.hasProperty("name") ? params["name"].toString()
                                                     : juce::String("New Clip");

  if (clipType != "audio" && clipType != "midi")
    return createErrorResponse("Invalid clip type: must be 'audio' or 'midi'");

  auto trackTree = projectState.getTrack(trackId);
  if (!trackTree.isValid())
    return createErrorResponse("Track not found: " + trackId);

  if (clipType == "audio" && !params.hasProperty("audioFile"))
    return createErrorResponse("Audio clips require 'audioFile' parameter");

  juce::String actionName = "create_clip '" + clipName + "' on " + trackId;
  juce::String clipId = projectState.createClip(
      trackId, clipType, startSamples, lengthSamples, clipName, actionName);

  if (clipId.isEmpty())
    return createErrorResponse("Failed to create clip");

  if (clipType == "audio" && params.hasProperty("audioFile")) {
    juce::String audioFile = params["audioFile"].toString();
    auto clip = projectState.getClip(trackId, clipId);
    if (clip.isValid())
      clip.setProperty(ProjectState::PROP_AUDIO_FILE, audioFile,
                       &projectState.getUndoManager());
  }

  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("clipId", clipId);
  resultObj->setProperty("trackId", trackId);
  resultObj->setProperty("name", clipName);
  resultObj->setProperty("type", clipType);
  resultObj->setProperty("startSamples", startSamples);
  resultObj->setProperty("lengthSamples", lengthSamples);

  DBG("ClipCommands: Created clip: " + clipId + " on track " + trackId);

  return createSuccessResponse(juce::var(resultObj));
}

juce::var ClipCommands::deleteClip(const juce::var &params) {
  if (!params.hasProperty("trackId"))
    return createErrorResponse("Missing 'trackId' parameter");
  if (!params.hasProperty("clipId"))
    return createErrorResponse("Missing 'clipId' parameter");

  juce::String trackId = params["trackId"].toString();
  juce::String clipId = params["clipId"].toString();

  auto clip = projectState.getClip(trackId, clipId);
  if (!clip.isValid())
    return createErrorResponse("Clip not found: " + clipId + " on track " +
                               trackId);

  juce::String actionName = "delete_clip " + clipId + " from " + trackId;
  projectState.deleteClip(trackId, clipId, actionName);

  DBG("ClipCommands: Deleted clip: " + clipId + " from track " + trackId);

  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("clipId", clipId);
  resultObj->setProperty("trackId", trackId);
  resultObj->setProperty("deleted", true);

  return createSuccessResponse(juce::var(resultObj));
}

juce::var ClipCommands::splitClip(const juce::var &params) {
  if (!params.hasProperty("trackId"))
    return createErrorResponse("Missing 'trackId' parameter");
  if (!params.hasProperty("clipId"))
    return createErrorResponse("Missing 'clipId' parameter");
  if (!params.hasProperty("splitSamples"))
    return createErrorResponse("Missing 'splitSamples' parameter");

  juce::String trackId = params["trackId"].toString();
  juce::String clipId = params["clipId"].toString();
  juce::int64 splitSamples = params["splitSamples"];

  auto clipTree = projectState.getClip(trackId, clipId);
  if (!clipTree.isValid())
    return createErrorResponse("Clip not found: " + clipId + " on track " +
                               trackId);

  juce::String actionName =
      "split_clip " + clipId + " at " + juce::String(splitSamples);
  auto newClipIds =
      projectState.splitClip(trackId, clipId, splitSamples, actionName);

  if (newClipIds.first.isEmpty() || newClipIds.second.isEmpty())
    return createErrorResponse("Split failed - invalid split position");

  DBG("ClipCommands: Split clip: " + clipId + " into " + newClipIds.first +
      " and " + newClipIds.second);

  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("originalClipId", clipId);
  resultObj->setProperty("splitSamples", splitSamples);
  resultObj->setProperty("leftClipId", newClipIds.first);
  resultObj->setProperty("rightClipId", newClipIds.second);

  return createSuccessResponse(juce::var(resultObj));
}

juce::var ClipCommands::moveClip(const juce::var &params) {
  if (!params.hasProperty("trackId"))
    return createErrorResponse("Missing 'trackId' parameter");
  if (!params.hasProperty("clipId"))
    return createErrorResponse("Missing 'clipId' parameter");
  if (!params.hasProperty("newStartSamples"))
    return createErrorResponse("Missing 'newStartSamples' parameter");

  juce::String trackId = params["trackId"].toString();
  juce::String clipId = params["clipId"].toString();
  juce::int64 newStartSamples = params["newStartSamples"];

  auto clipTree = projectState.getClip(trackId, clipId);
  if (!clipTree.isValid())
    return createErrorResponse("Clip not found: " + clipId + " on track " +
                               trackId);

  if (newStartSamples < 0)
    return createErrorResponse("Clip position must be >= 0");

  juce::String actionName =
      "move_clip " + clipId + " to " + juce::String(newStartSamples);
  projectState.moveClip(trackId, clipId, newStartSamples, actionName);

  DBG("ClipCommands: Moved clip: " + clipId + " to " +
      juce::String(newStartSamples));

  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("clipId", clipId);
  resultObj->setProperty("trackId", trackId);
  resultObj->setProperty("newStartSamples", newStartSamples);

  return createSuccessResponse(juce::var(resultObj));
}

juce::var ClipCommands::resizeClip(const juce::var &params) {
  if (!params.hasProperty("trackId"))
    return createErrorResponse("Missing 'trackId' parameter");
  if (!params.hasProperty("clipId"))
    return createErrorResponse("Missing 'clipId' parameter");
  if (!params.hasProperty("newLengthSamples"))
    return createErrorResponse("Missing 'newLengthSamples' parameter");

  juce::String trackId = params["trackId"].toString();
  juce::String clipId = params["clipId"].toString();
  juce::int64 newLengthSamples = params["newLengthSamples"];

  auto clipTree = projectState.getClip(trackId, clipId);
  if (!clipTree.isValid())
    return createErrorResponse("Clip not found: " + clipId + " on track " +
                               trackId);

  if (newLengthSamples <= 0)
    return createErrorResponse("Clip length must be > 0");

  juce::String actionName =
      "resize_clip " + clipId + " to " + juce::String(newLengthSamples);
  projectState.resizeClip(trackId, clipId, newLengthSamples, actionName);

  DBG("ClipCommands: Resized clip: " + clipId + " to " +
      juce::String(newLengthSamples));

  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("clipId", clipId);
  resultObj->setProperty("trackId", trackId);
  resultObj->setProperty("newLengthSamples", newLengthSamples);

  return createSuccessResponse(juce::var(resultObj));
}

juce::var ClipCommands::setClipNotes(const juce::var &params) {
  if (!params.hasProperty("clipId"))
    return createErrorResponse("Missing 'clipId'");
  if (!params.hasProperty("notes"))
    return createErrorResponse("Missing 'notes'");

  juce::String clipId = params["clipId"].toString();
  juce::String trackId;
  juce::var notesVar = params["notes"];

  if (!notesVar.isArray())
    return createErrorResponse("'notes' must be an array");

  juce::ValueTree clipTree;
  if (params.hasProperty("trackId")) {
    trackId = params["trackId"].toString();
    clipTree = projectState.getClip(trackId, clipId);
  }

  if (!clipTree.isValid()) {
    auto [trackTree, foundClip] = projectState.findClip(clipId);
    clipTree = foundClip;
    if (trackTree.isValid()) {
      trackId = trackTree[ProjectState::PROP_ID].toString();
    }
  }

  if (!clipTree.isValid())
    return createErrorResponse("Clip not found: " + clipId);

  auto clipType =
      clipTree.getProperty(ProjectState::PROP_TYPE, juce::String())
          .toString()
          .toLowerCase();
  if (clipType != "midi")
    return createErrorResponse("Clip is not MIDI: " + clipId);

  struct ParsedNote {
    juce::ValueTree tree;
  };
  std::vector<ParsedNote> parsedNotes;
  parsedNotes.reserve(notesVar.getArray()->size());

  for (const auto &noteVar : *notesVar.getArray()) {
    if (!noteVar.isObject())
      return createErrorResponse("Note entries must be objects");

    const bool hasStart = noteVar.hasProperty("start") ||
                          noteVar.hasProperty("startBeats");
    const bool hasLength = noteVar.hasProperty("length") ||
                           noteVar.hasProperty("lengthBeats");
    if (!noteVar.hasProperty("pitch") || !hasStart || !hasLength ||
        !noteVar.hasProperty("velocity")) {
      return createErrorResponse(
          "Each note requires pitch, start/startBeats, length/lengthBeats, velocity");
    }

    int pitch = static_cast<int>(noteVar["pitch"]);
    double startBeats = noteVar.hasProperty("startBeats")
                            ? static_cast<double>(noteVar["startBeats"])
                            : static_cast<double>(noteVar["start"]);
    double lengthBeats = noteVar.hasProperty("lengthBeats")
                             ? static_cast<double>(noteVar["lengthBeats"])
                             : static_cast<double>(noteVar["length"]);
    double velocityVal = static_cast<double>(noteVar["velocity"]);

    pitch = juce::jlimit(0, 127, pitch);
    startBeats = std::max(0.0, startBeats);
    lengthBeats = std::max(0.001, lengthBeats);

    int midiVelocity = 0;
    if (velocityVal <= 1.0) {
      midiVelocity = juce::jlimit(
          0, 127, static_cast<int>(velocityVal * 127.0 + 0.5));
    } else {
      midiVelocity = juce::jlimit(0, 127, static_cast<int>(velocityVal));
    }

    juce::ValueTree noteTree(ProjectState::ID_NOTE);
    if (noteVar.hasProperty("id")) {
      noteTree.setProperty(ProjectState::PROP_ID,
                           noteVar["id"].toString(), nullptr);
    } else {
      noteTree.setProperty(
          ProjectState::PROP_ID,
          "note_" + juce::Uuid().toString().substring(0, 8), nullptr);
    }
    noteTree.setProperty(ProjectState::PROP_PITCH, pitch, nullptr);
    noteTree.setProperty(ProjectState::PROP_START_BEATS, startBeats, nullptr);
    noteTree.setProperty(ProjectState::PROP_LENGTH_BEATS, lengthBeats, nullptr);
    noteTree.setProperty(ProjectState::PROP_VELOCITY, midiVelocity, nullptr);

    if (noteVar.hasProperty("muted") &&
        static_cast<bool>(noteVar["muted"])) {
      noteTree.setProperty(ProjectState::PROP_MUTE, true, nullptr);
    }
    if (noteVar.hasProperty("probability")) {
      noteTree.setProperty(ProjectState::PROP_PROBABILITY,
                           static_cast<double>(noteVar["probability"]),
                           nullptr);
    }
    if (noteVar.hasProperty("condition")) {
      noteTree.setProperty(ProjectState::PROP_CONDITION,
                           noteVar["condition"].toString(), nullptr);
    }
    if (noteVar.hasProperty("recurrence")) {
      noteTree.setProperty(ProjectState::PROP_RECURRENCE,
                           noteVar["recurrence"].toString(), nullptr);
    }
    if (noteVar.hasProperty("articulationId")) {
      noteTree.setProperty(ProjectState::PROP_ARTICULATION_ID,
                           static_cast<int>(noteVar["articulationId"]),
                           nullptr);
    }
    if (noteVar.hasProperty("tension")) {
      noteTree.setProperty(ProjectState::PROP_NOTE_TENSION,
                           static_cast<double>(noteVar["tension"]),
                           nullptr);
    }

    parsedNotes.push_back({noteTree});
  }

  auto &undo = projectState.getUndoManager();
  undo.beginNewTransaction("Set Clip Notes");

  auto notesNode = clipTree.getChildWithName(ProjectState::ID_NOTES);
  if (!notesNode.isValid()) {
    notesNode = juce::ValueTree(ProjectState::ID_NOTES);
    clipTree.appendChild(notesNode, &undo);
  } else {
    notesNode.removeAllChildren(&undo);
  }

  for (auto &note : parsedNotes) {
    notesNode.appendChild(note.tree, &undo);
  }

  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("clipId", clipId);
  if (trackId.isNotEmpty())
    resultObj->setProperty("trackId", trackId);
  resultObj->setProperty("noteCount",
                         static_cast<int>(parsedNotes.size()));

  return createSuccessResponse(juce::var(resultObj));
}

} // namespace zenith
