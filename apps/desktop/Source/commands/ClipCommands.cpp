#include "ClipCommands.h"
#include "../engine/Clip.h"
#include "../engine/Engine.h"
#include "../engine/ProjectState.h"
#include "../engine/Track.h"
#include "Actions.h"
#include "CommandAPI.h"
#include "CommandUtils.h"
#include <juce_core/juce_core.h>
#include <memory>

namespace zenith {

ClipCommands::ClipCommands(Engine &eng, ProjectState &state, CommandAPI &api)
    : engine(eng), projectState(state), api(api) {}

juce::var ClipCommands::listClips(const juce::var &params) {
  if (!params.hasProperty("trackId"))
    return createErrorResponse("Missing 'trackId' parameter");

  juce::String trackId = params["trackId"].toString();
  zenith::Track *track = zenith::findTrackById(engine, trackId);

  if (track == nullptr)
    return createErrorResponse("Track not found: " + trackId);

  juce::Array<juce::var> clipsArray;

  for (int i = 0; i < track->getNumClips(); ++i) {
    auto *clip = track->getClip(i);
    if (clip != nullptr) {
      juce::DynamicObject::Ptr clipData = new juce::DynamicObject();
      clipData->setProperty("id", "clip_" + juce::String(i));
      clipData->setProperty("name", clip->getName());
      clipData->setProperty("start", (juce::int64)clip->getStartPosition());
      clipData->setProperty("length", (juce::int64)clip->getLength());
      clipData->setProperty("offset", (juce::int64)clip->getOffset());

      clipsArray.add(juce::var(clipData.get()));
    }
  }

  juce::DynamicObject::Ptr resultObj = new juce::DynamicObject();
  resultObj->setProperty("trackId", trackId);
  resultObj->setProperty("clips", clipsArray);
  resultObj->setProperty("count", (int)clipsArray.size());

  return createSuccessResponse(juce::var(resultObj.get()));
}

juce::var ClipCommands::createClip(const juce::var &params) {
  if (!params.hasProperty("trackId"))
    return createErrorResponse("Missing 'trackId' parameter");
  if (!params.hasProperty("type"))
    return createErrorResponse("Missing 'type' parameter");
  if (!params.hasProperty("start"))
    return createErrorResponse("Missing 'start' parameter");
  if (!params.hasProperty("length"))
    return createErrorResponse("Missing 'length' parameter");

  juce::String trackId = params["trackId"].toString();
  juce::String clipType = params["type"].toString().toLowerCase();
  juce::int64 startSamples = params["start"];
  juce::int64 lengthSamples = params["length"];
  juce::String clipName = params.hasProperty("name") ? params["name"].toString()
                                                     : juce::String("New Clip");

  auto trackTree = projectState.getTrack(trackId);
  if (!trackTree.isValid())
    return createErrorResponse("Track not found: " + trackId);

  auto action = std::make_unique<zenith::CreateClipAction>(
      projectState, trackId, clipType, startSamples, lengthSamples, clipName);
  auto *rawAction = action.get();

  if (api.performAction(std::move(action))) {
    juce::String clipId = rawAction->getClipId();

    if (clipType == "audio" && params.hasProperty("audioFile")) {
      juce::String audioFile = params["audioFile"].toString();
      auto clip = projectState.getClip(trackId, clipId);
      if (clip.isValid()) {
        api.performAction(std::make_unique<zenith::SetPropertyAction>(
            projectState, clip, ProjectState::PROP_AUDIO_FILE,
            juce::var(audioFile), "Set Clip Audio File"));
      }
    }

    juce::DynamicObject::Ptr resultObj = new juce::DynamicObject();
    resultObj->setProperty("clipId", clipId);
    resultObj->setProperty("trackId", trackId);
    resultObj->setProperty("name", clipName);
    resultObj->setProperty("type", clipType);

    return createSuccessResponse(juce::var(resultObj.get()));
  }

  return createErrorResponse("Failed to create clip");
}

juce::var ClipCommands::deleteClip(const juce::var &params) {
  if (!params.hasProperty("trackId"))
    return createErrorResponse("Missing 'trackId' parameter");
  if (!params.hasProperty("clipId"))
    return createErrorResponse("Missing 'clipId' parameter");

  juce::String trackId = params["trackId"].toString();
  juce::String clipId = params["clipId"].toString();

  if (api.performAction(std::make_unique<zenith::DeleteClipAction>(
          projectState, trackId, clipId))) {
    juce::DynamicObject::Ptr resultObj = new juce::DynamicObject();
    resultObj->setProperty("clipId", clipId);
    resultObj->setProperty("trackId", trackId);
    resultObj->setProperty("deleted", true);

    return createSuccessResponse(juce::var(resultObj.get()));
  }

  return createErrorResponse("Failed to delete clip");
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

  if (api.performAction(std::make_unique<zenith::MoveClipAction>(
          projectState, trackId, clipId, newStartSamples))) {
    juce::DynamicObject::Ptr resultObj = new juce::DynamicObject();
    resultObj->setProperty("clipId", clipId);
    resultObj->setProperty("trackId", trackId);
    resultObj->setProperty("newStartSamples", newStartSamples);

    return createSuccessResponse(juce::var(resultObj.get()));
  }

  return createErrorResponse("Failed to move clip");
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

  if (api.performAction(std::make_unique<zenith::ResizeClipAction>(
          projectState, trackId, clipId, newLengthSamples))) {
    juce::DynamicObject::Ptr resultObj = new juce::DynamicObject();
    resultObj->setProperty("clipId", clipId);
    resultObj->setProperty("trackId", trackId);
    resultObj->setProperty("newLengthSamples", newLengthSamples);

    return createSuccessResponse(juce::var(resultObj.get()));
  }

  return createErrorResponse("Failed to resize clip");
}

juce::var ClipCommands::splitClip(const juce::var &params) {
  // Use projectState split helper
  if (!params.hasProperty("trackId") || !params.hasProperty("clipId") ||
      !params.hasProperty("splitSamples"))
    return createErrorResponse("Missing parameters for split");

  juce::String trackId = params["trackId"];
  juce::String clipId = params["clipId"];
  juce::int64 splitSamples = params["splitSamples"];

  auto ids =
      projectState.splitClip(trackId, clipId, splitSamples, "Split Clip");

  if (ids.first.isNotEmpty()) {
    juce::DynamicObject::Ptr resultObj = new juce::DynamicObject();
    resultObj->setProperty("leftClipId", ids.first);
    resultObj->setProperty("rightClipId", ids.second);
    return createSuccessResponse(juce::var(resultObj.get()));
  }

  return createErrorResponse("Split failed");
}

juce::var ClipCommands::setClipNotes(const juce::var &params) {
  if (!params.hasProperty("clipId") || !params.hasProperty("notes"))
    return createErrorResponse("Missing parameters for setClipNotes");

  juce::String clipId = params["clipId"];
  juce::var notesVar = params["notes"];

  std::vector<ProjectState::MidiNoteSpec> notes;
  if (auto *notesArray = notesVar.getArray()) {
    for (const auto &n : *notesArray) {
      ProjectState::MidiNoteSpec s;
      s.pitch = (int)n["pitch"];
      s.startBeats = (double)n["start"];
      s.lengthBeats = (double)n["length"];
      s.velocity = (int)n["velocity"];
      notes.push_back(s);
    }
  }

  projectState.addNotes(clipId, notes, "Set Clip Notes");
  return createSuccessResponse(juce::var());
}

} // namespace zenith
