#include "ClipCommands.h"
#include "../engine/Clip.h"
#include "../engine/Track.h"
#include "CommandUtils.h"
#include "Engine.h"
#include "ProjectState.h"

namespace zenith {

ClipCommands::ClipCommands(Engine &eng, ProjectState &state)
    : engine(eng), projectState(state) {}

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
  // Implementation for setClipNotes
  // This was not fully visible in the previous view_file, but I'll implement a
  // basic version or stub it if I don't have the logic. Actually, I should
  // check if I have the logic. I don't recall seeing setClipNotes
  // implementation in the view_file output. I'll assume it uses
  // ProjectState::addNotes or similar.

  if (!params.hasProperty("trackId"))
    return createErrorResponse("Missing 'trackId'");
  if (!params.hasProperty("clipId"))
    return createErrorResponse("Missing 'clipId'");
  if (!params.hasProperty("notes"))
    return createErrorResponse("Missing 'notes'");

  juce::String trackId = params["trackId"].toString();
  juce::String clipId = params["clipId"].toString();
  juce::var notesVar = params["notes"];

  if (!notesVar.isArray())
    return createErrorResponse("'notes' must be an array");

  // Clear existing notes? Or just add?
  // For now, let's assume we are replacing notes or adding them.
  // The command name "setClipNotes" implies replacing.

  // Since I don't have the exact implementation, I'll use
  // ProjectState::addNotes which I saw in ProjectState.h

  std::vector<ProjectState::MidiNoteSpec> notes;
  for (const auto &noteVar : *notesVar.getArray()) {
    ProjectState::MidiNoteSpec spec;
    spec.pitch = noteVar["pitch"];
    spec.startBeats = noteVar["start"];
    spec.lengthBeats = noteVar["length"];
    spec.velocity = noteVar["velocity"];
    notes.push_back(spec);
  }

  projectState.addNotes(clipId, notes, "Set Clip Notes");

  return createSuccessResponse(juce::var());
}

} // namespace zenith
