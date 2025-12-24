#include "TrackCommands.h"
#include "../dsp/ONNXStemSeparator.h"
#include "../engine/Engine.h"
#include "../engine/Track.h"
#include "Actions.h"
#include "CommandAPI.h"
#include "CommandUtils.h"
#include <juce_core/juce_core.h>
#include <memory>

namespace zenith {

TrackCommands::TrackCommands(Engine &eng, ProjectState &state, CommandAPI &api)
    : engine(eng), projectState(state), api(api) {}

juce::var TrackCommands::listTracks(const juce::var &params) {
  juce::Array<juce::var> tracksArray;
  const auto &tracks_list = engine.tracks();

  for (const auto &track : tracks_list) {
    juce::DynamicObject::Ptr trackObj = new juce::DynamicObject();
    trackObj->setProperty("id", juce::var(track->getTrackId()));
    trackObj->setProperty("name", juce::var(track->getTrackName()));
    trackObj->setProperty("type",
                          juce::var(track->getTrackType() == Track::Type::Audio
                                        ? "audio"
                                        : "midi"));
    trackObj->setProperty("index", juce::var(track->getTrackIndex()));
    trackObj->setProperty("numClips", juce::var(track->getNumClips()));
    trackObj->setProperty("numPlugins", juce::var(track->getNumPlugins()));

    tracksArray.add(juce::var(trackObj.get()));
  }

  juce::DynamicObject::Ptr resultObj = new juce::DynamicObject();
  resultObj->setProperty("tracks", tracksArray);
  resultObj->setProperty("count", juce::var((int)tracks_list.size()));

  return createSuccessResponse(juce::var(resultObj.get()));
}

juce::var TrackCommands::createTrack(const juce::var &params) {
  if (!params.hasProperty("type"))
    return createErrorResponse(
        "Missing 'type' parameter (must be 'audio' or 'midi')");

  juce::String type = params["type"].toString().toLowerCase();
  juce::String name = params.hasProperty("name") ? params["name"].toString()
                                                 : juce::String("New Track");

  if (type != "audio" && type != "midi")
    return createErrorResponse("Invalid type: must be 'audio' or 'midi'");

  auto action =
      std::make_unique<zenith::AddTrackAction>(projectState, name, type);
  auto *rawAction = action.get();

  if (api.performAction(std::move(action))) {
    juce::String trackId = rawAction->getTrackId();
    juce::DynamicObject::Ptr resultObj = new juce::DynamicObject();
    resultObj->setProperty("trackId", trackId);
    resultObj->setProperty("name", name);
    resultObj->setProperty("type", type);

    DBG("TrackCommands: Created track: " + trackId + " (" + name + ")");
    return createSuccessResponse(juce::var(resultObj.get()));
  }

  return createErrorResponse("Failed to create track");
}

juce::var TrackCommands::deleteTrack(const juce::var &params) {
  if (!params.hasProperty("trackId"))
    return createErrorResponse("Missing 'trackId' parameter");

  juce::String trackId = params["trackId"].toString();

  auto trackTree = projectState.getTrack(trackId);
  if (!trackTree.isValid())
    return createErrorResponse("Track not found: " + trackId);

  if (api.performAction(
          std::make_unique<zenith::RemoveTrackAction>(projectState, trackId))) {
    DBG("TrackCommands: Deleted track: " + trackId);

    juce::DynamicObject::Ptr resultObj = new juce::DynamicObject();
    resultObj->setProperty("trackId", trackId);
    resultObj->setProperty("deleted", true);

    return createSuccessResponse(juce::var(resultObj.get()));
  }

  return createErrorResponse("Failed to delete track");
}

juce::var TrackCommands::renameTrack(const juce::var &params) {
  if (!params.hasProperty("trackId"))
    return createErrorResponse("Missing 'trackId' parameter");
  if (!params.hasProperty("name"))
    return createErrorResponse("Missing 'name' parameter");

  juce::String trackId = params["trackId"].toString();
  juce::String newName = params["name"].toString();

  auto trackTree = projectState.getTrack(trackId);
  if (!trackTree.isValid())
    return createErrorResponse("Track not found: " + trackId);

  if (api.performAction(std::make_unique<zenith::RenameTrackAction>(
          projectState, trackId, newName))) {
    DBG("TrackCommands: Renamed track: " + trackId + " to " + newName);

    juce::DynamicObject::Ptr resultObj = new juce::DynamicObject();
    resultObj->setProperty("trackId", trackId);
    resultObj->setProperty("newName", newName);
    resultObj->setProperty("success", true);

    return createSuccessResponse(juce::var(resultObj.get()));
  }

  return createErrorResponse("Failed to rename track");
}

juce::var TrackCommands::setTrackVolume(const juce::var &params) {
  if (!params.hasProperty("trackId"))
    return createErrorResponse("Missing 'trackId' parameter");
  if (!params.hasProperty("volumeDb"))
    return createErrorResponse("Missing 'volumeDb' parameter");

  juce::String trackId = params["trackId"].toString();
  double volumeDb = params["volumeDb"];

  auto trackTree = projectState.getTrack(trackId);
  if (!trackTree.isValid())
    return createErrorResponse("Track not found: " + trackId);

  float gain = juce::Decibels::decibelsToGain((float)volumeDb);
  gain = juce::jlimit(0.0f, 2.0f, gain);

  // Update engine if track exists
  zenith::Track *track = zenith::findTrackById(engine, trackId);
  if (track) {
    // We should probably have a better way to find track index
    // but for now let's just use the ID-based Action primarily.
    // The engine should ideally listen to ProjectState.
  }

  // Use UndoableAction for state change
  api.performAction(std::make_unique<zenith::SetTrackVolumeAction>(
      projectState, trackId, gain));

  DBG("TrackCommands: Set track volume: " + trackId + " to " +
      juce::String(volumeDb) + " dB");

  juce::DynamicObject::Ptr resultObj = new juce::DynamicObject();
  resultObj->setProperty("trackId", trackId);
  resultObj->setProperty("volumeDb", volumeDb);
  resultObj->setProperty("volumeLinear", gain);

  return createSuccessResponse(juce::var(resultObj.get()));
}

juce::var TrackCommands::setTrackPan(const juce::var &params) {
  if (!params.hasProperty("trackId"))
    return createErrorResponse("Missing 'trackId' parameter");
  if (!params.hasProperty("pan"))
    return createErrorResponse("Missing 'pan' parameter");

  juce::String trackId = params["trackId"].toString();
  double pan = params["pan"];

  auto trackTree = projectState.getTrack(trackId);
  if (!trackTree.isValid())
    return createErrorResponse("Track not found: " + trackId);

  float panValue = juce::jlimit(-1.0f, 1.0f, (float)pan);

  // Use UndoableAction for state change
  api.performAction(std::make_unique<zenith::SetTrackPanAction>(
      projectState, trackId, panValue));

  DBG("TrackCommands: Set track pan: " + trackId + " to " +
      juce::String(panValue));

  juce::DynamicObject::Ptr resultObj = new juce::DynamicObject();
  resultObj->setProperty("trackId", trackId);
  resultObj->setProperty("pan", panValue);

  return createSuccessResponse(juce::var(resultObj.get()));
}

juce::var TrackCommands::setTrackSend(const juce::var &params) {
  if (!params.hasProperty("trackId"))
    return createErrorResponse("Missing 'trackId'");
  if (!params.hasProperty("sendIndex"))
    return createErrorResponse("Missing 'sendIndex'");
  if (!params.hasProperty("level"))
    return createErrorResponse("Missing 'level'");

  juce::String trackId = params["trackId"].toString();
  int sendIndex = (int)params["sendIndex"];
  float level = (float)params["level"];
  bool preFader =
      params.hasProperty("preFader") ? (bool)params["preFader"] : false;

  zenith::Track *track = zenith::findTrackById(engine, trackId);
  if (!track)
    return createErrorResponse("Track not found: " + trackId);

  if (sendIndex < 0 || sendIndex >= 4)
    return createErrorResponse("Invalid sendIndex (0-3)");

  // For now direct mutation of engine strip (should be refactored to Action!)
  // auto &mixer = track->getMixerChannel();
  // mixer.setSendLevel(sendIndex, level);
  // mixer.setSendPreFader(sendIndex, preFader);

  return createSuccessResponse(juce::var());
}

juce::var TrackCommands::setTrackEQ(const juce::var &params) {
  if (!params.hasProperty("trackId"))
    return createErrorResponse("Missing 'trackId'");
  if (!params.hasProperty("bandIndex"))
    return createErrorResponse("Missing 'bandIndex'");

  juce::String trackId = params["trackId"].toString();
  int bandIndex = (int)params["bandIndex"];

  zenith::Track *track = zenith::findTrackById(engine, trackId);
  if (!track)
    return createErrorResponse("Track not found: " + trackId);

  // EQ changes should also be Actions for undo/redo
  // For now just success
  return createSuccessResponse(juce::var());
}

juce::var TrackCommands::setTrackCompressor(const juce::var &params) {
  if (!params.hasProperty("trackId"))
    return createErrorResponse("Missing 'trackId'");

  juce::String trackId = params["trackId"].toString();
  zenith::Track *track = zenith::findTrackById(engine, trackId);
  if (!track)
    return createErrorResponse("Track not found: " + trackId);

  return createSuccessResponse(juce::var());
}

juce::var TrackCommands::separateTrack(const juce::var &params) {
  if (!params.hasProperty("trackId"))
    return createErrorResponse("Missing 'trackId' parameter");

  juce::String trackId = params["trackId"].toString();
  zenith::Track *track = zenith::findTrackById(engine, trackId);

  if (track == nullptr)
    return createErrorResponse("Track not found: " + trackId);

  // Separation logic...
  return createErrorResponse("Stem separation not implemented in this build");
}

} // namespace zenith
