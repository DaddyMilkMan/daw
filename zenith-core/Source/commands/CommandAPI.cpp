/*
  ==============================================================================

    CommandAPI.cpp
    Created: 2025-11-14
    Author:  Zenith DAW - Phase 5: Wingman v0

    JSON command processor implementation

  ==============================================================================
*/

#include "CommandAPI.h"
#include "SessionGraph.h"
#include "../Engine.h"
#include "../ProjectState.h"
#include "engine/Track.h"
#include "engine/Clip.h"
#include "../instruments/InstrumentRegistry.h"

namespace zenith {

//==============================================================================
CommandAPI::CommandAPI(Engine& eng, ProjectState& state)
    : engine(eng), projectState(state)
{
    DBG("CommandAPI: Initialized");
}

CommandAPI::~CommandAPI()
{
}

//==============================================================================
juce::var CommandAPI::executeCommand(const juce::var& request)
{
    // Validate request structure
    if (!request.isObject())
        return createErrorResponse("Invalid request: must be JSON object");

    if (!request.hasProperty("command"))
        return createErrorResponse("Missing 'command' field");

    juce::String command = request["command"].toString();
    juce::var params = request.getProperty("params", juce::var());

    DBG("CommandAPI: Executing command: " + command);

    // Route to appropriate handler
    // Track commands
    if (command == "list_tracks")
        return listTracks(params);
    else if (command == "create_track")
        return createTrack(params);
    else if (command == "delete_track")
        return deleteTrack(params);
    else if (command == "rename_track")
        return renameTrack(params);
    else if (command == "set_track_volume")
        return setTrackVolume(params);
    else if (command == "set_track_pan")
        return setTrackPan(params);

    // Clip commands
    else if (command == "list_clips")
        return listClips(params);
    else if (command == "create_clip")
        return createClip(params);
    else if (command == "delete_clip")
        return deleteClip(params);
    else if (command == "split_clip")
        return splitClip(params);
    else if (command == "move_clip")
        return moveClip(params);

    // Session/project commands
    else if (command == "get_session_graph")
        return getSessionGraph(params);

    // Undo/redo commands
    else if (command == "undo")
        return undo(params);
    else if (command == "redo")
        return redo(params);
    else if (command == "history")
        return history(params);

    // Instrument commands
    else if (command == "describe_instrument")
        return describeInstrument(params);

    else
        return createErrorResponse("Unknown command: " + command);
}

juce::String CommandAPI::executeCommandString(const juce::String& jsonRequest)
{
    // Parse JSON string to var
    juce::var parsedJson;
    auto result = juce::JSON::parse(jsonRequest, parsedJson);

    if (result.failed())
        return juce::JSON::toString(createErrorResponse("JSON parse error: " + result.getErrorMessage()));

    // Execute command
    juce::var response = executeCommand(parsedJson);

    // Convert back to string
    return juce::JSON::toString(response);
}

juce::var CommandAPI::executeBatch(const juce::Array<juce::var>& commands,
                                    const juce::String& batchName)
{
    if (commands.isEmpty())
    {
        return createErrorResponse("Empty command batch");
    }

    DBG("CommandAPI: Executing batch '" + batchName + "' with " + juce::String(commands.size()) + " commands");

    // Begin a single undo transaction for the entire batch
    projectState.getUndoManager().beginNewTransaction(batchName);

    int successCount = 0;

    for (int i = 0; i < commands.size(); ++i)
    {
        const auto& cmdVar = commands[i];

        // Validate command structure
        if (!cmdVar.isObject())
        {
            auto* errorObj = new juce::DynamicObject();
            errorObj->setProperty("success", false);
            errorObj->setProperty("error", "Command at index " + juce::String(i) + " is not a JSON object");
            errorObj->setProperty("failedIndex", i);
            errorObj->setProperty("failedCommand", cmdVar);
            errorObj->setProperty("successCount", successCount);

            return juce::var(errorObj);
        }

        if (!cmdVar.hasProperty("command"))
        {
            auto* errorObj = new juce::DynamicObject();
            errorObj->setProperty("success", false);
            errorObj->setProperty("error", "Command at index " + juce::String(i) + " missing 'command' field");
            errorObj->setProperty("failedIndex", i);
            errorObj->setProperty("failedCommand", cmdVar);
            errorObj->setProperty("successCount", successCount);

            return juce::var(errorObj);
        }

        // Execute command
        juce::var response = executeCommand(cmdVar);

        // Check for error
        bool success = response.getProperty("success", false);

        if (!success)
        {
            // Command failed - stop batch execution
            juce::String error = response.getProperty("error", "Unknown error").toString();

            auto* errorObj = new juce::DynamicObject();
            errorObj->setProperty("success", false);
            errorObj->setProperty("error", "Command failed: " + error);
            errorObj->setProperty("failedIndex", i);
            errorObj->setProperty("failedCommand", cmdVar);
            errorObj->setProperty("successCount", successCount);

            DBG("CommandAPI: Batch failed at command " + juce::String(i) + ": " + error);

            return juce::var(errorObj);
        }

        successCount++;
    }

    // All commands succeeded
    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("count", successCount);

    DBG("CommandAPI: Batch completed successfully (" + juce::String(successCount) + " commands)");

    return createSuccessResponse(juce::var(resultObj));
}

//==============================================================================
// Command Handlers
//==============================================================================

juce::var CommandAPI::listTracks(const juce::var& params)
{
    juce::ignoreUnused(params);

    juce::var tracksArray;
    auto* tracksArrayPtr = tracksArray.getArray();

    const auto& tracks = engine.tracks();

    for (size_t i = 0; i < tracks.size(); ++i)
    {
        const auto* track = tracks[i].get();
        if (track == nullptr)
            continue;

        auto* trackObj = new juce::DynamicObject();
        trackObj->setProperty("id", "track_" + juce::String((int)i));
        trackObj->setProperty("name", track->getName());
        trackObj->setProperty("type", track->getTypeString());
        trackObj->setProperty("volume", track->getVolume());
        trackObj->setProperty("pan", track->getPan());
        trackObj->setProperty("muted", track->isMuted());
        trackObj->setProperty("soloed", track->isSolo());
        trackObj->setProperty("numClips", track->getNumClips());
        trackObj->setProperty("numPlugins", track->getNumPlugins());

        tracksArrayPtr->add(juce::var(trackObj));
    }

    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("tracks", tracksArray);
    resultObj->setProperty("count", (int)tracks.size());

    return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::createTrack(const juce::var& params)
{
    // Validate params
    if (!params.hasProperty("type"))
        return createErrorResponse("Missing 'type' parameter (must be 'audio' or 'midi')");

    juce::String type = params["type"].toString().toLowerCase();
    juce::String name = params.getProperty("name", "New Track").toString();

    // Validate type
    if (type != "audio" && type != "midi")
        return createErrorResponse("Invalid type: must be 'audio' or 'midi'");

    // Create track via ProjectState (undoable)
    juce::String actionName = "Wingman: create_track '" + name + "'";
    juce::String trackId = projectState.addTrack(name, type);

    if (trackId.isEmpty())
        return createErrorResponse("Failed to create track");

    // Create result
    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("trackId", trackId);
    resultObj->setProperty("name", name);
    resultObj->setProperty("type", type);

    DBG("CommandAPI: Created track: " + trackId + " (" + name + ")");

    return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::deleteTrack(const juce::var& params)
{
    // Validate params
    if (!params.hasProperty("trackId"))
        return createErrorResponse("Missing 'trackId' parameter");

    juce::String trackId = params["trackId"].toString();

    // Check if track exists in ProjectState
    auto trackTree = projectState.getTrack(trackId);
    if (!trackTree.isValid())
        return createErrorResponse("Track not found: " + trackId);

    // Delete via ProjectState (undoable)
    juce::String actionName = "Wingman: delete_track " + trackId;
    projectState.removeTrack(trackId);

    DBG("CommandAPI: Deleted track: " + trackId);

    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("trackId", trackId);
    resultObj->setProperty("deleted", true);

    return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::renameTrack(const juce::var& params)
{
    // Validate params
    if (!params.hasProperty("trackId"))
        return createErrorResponse("Missing 'trackId' parameter");
    if (!params.hasProperty("name"))
        return createErrorResponse("Missing 'name' parameter");

    juce::String trackId = params["trackId"].toString();
    juce::String newName = params["name"].toString();

    // Check if track exists in ProjectState
    auto trackTree = projectState.getTrack(trackId);
    if (!trackTree.isValid())
        return createErrorResponse("Track not found: " + trackId);

    // Rename via ProjectState (undoable)
    juce::String actionName = "Wingman: rename_track " + trackId + " to '" + newName + "'";
    projectState.renameTrack(trackId, newName, actionName);

    DBG("CommandAPI: Renamed track: " + trackId + " to " + newName);

    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("trackId", trackId);
    resultObj->setProperty("name", newName);

    return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::listClips(const juce::var& params)
{
    // Validate params
    if (!params.hasProperty("trackId"))
        return createErrorResponse("Missing 'trackId' parameter");

    juce::String trackId = params["trackId"].toString();

    // Find track
    Track* track = findTrackById(trackId);
    if (track == nullptr)
        return createErrorResponse("Track not found: " + trackId);

    // Build clips array
    juce::var clipsArray;
    auto* clipsArrayPtr = clipsArray.getArray();

    for (int i = 0; i < track->getNumClips(); ++i)
    {
        Track::Clip* clip = track->getClip(i);
        if (clip == nullptr)
            continue;

        auto* clipObj = new juce::DynamicObject();
        clipObj->setProperty("id", "clip_" + juce::String(i));
        clipObj->setProperty("name", clip->getName());
        clipObj->setProperty("type", clip->getType() == Track::Clip::Type::Audio ? "audio" : "midi");
        clipObj->setProperty("startSamples", (juce::int64)clip->getStartPosition());
        clipObj->setProperty("lengthSamples", (juce::int64)clip->getLength());

        // Add type-specific info
        if (clip->getType() == Track::Clip::Type::Audio)
        {
            clipObj->setProperty("audioFile", clip->getAudioFile().getFullPathName());
        }
        else if (clip->getType() == Track::Clip::Type::MIDI)
        {
            const auto* midiSeq = clip->getMidiSequence();
            int noteCount = 0;
            if (midiSeq != nullptr)
            {
                for (int j = 0; j < midiSeq->getNumEvents(); ++j)
                {
                    if (midiSeq->getEventPointer(j)->message.isNoteOn())
                        noteCount++;
                }
            }
            clipObj->setProperty("midiNoteCount", noteCount);
        }

        clipsArrayPtr->add(juce::var(clipObj));
    }

    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("trackId", trackId);
    resultObj->setProperty("clips", clipsArray);
    resultObj->setProperty("count", track->getNumClips());

    return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::splitClip(const juce::var& params)
{
    // Validate params
    if (!params.hasProperty("trackId"))
        return createErrorResponse("Missing 'trackId' parameter");
    if (!params.hasProperty("clipId"))
        return createErrorResponse("Missing 'clipId' parameter");
    if (!params.hasProperty("splitSamples"))
        return createErrorResponse("Missing 'splitSamples' parameter");

    juce::String trackId = params["trackId"].toString();
    juce::String clipId = params["clipId"].toString();
    juce::int64 splitSamples = params["splitSamples"];

    // Check if clip exists in ProjectState
    auto clipTree = projectState.getClip(trackId, clipId);
    if (!clipTree.isValid())
        return createErrorResponse("Clip not found: " + clipId + " on track " + trackId);

    // Split via ProjectState (undoable)
    juce::String actionName = "Wingman: split_clip " + clipId + " at " + juce::String(splitSamples);
    auto newClipIds = projectState.splitClip(trackId, clipId, splitSamples, actionName);

    if (newClipIds.first.isEmpty() || newClipIds.second.isEmpty())
        return createErrorResponse("Split failed - invalid split position");

    DBG("CommandAPI: Split clip: " + clipId + " into " + newClipIds.first + " and " + newClipIds.second);

    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("originalClipId", clipId);
    resultObj->setProperty("splitSamples", splitSamples);
    resultObj->setProperty("leftClipId", newClipIds.first);
    resultObj->setProperty("rightClipId", newClipIds.second);

    return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::moveClip(const juce::var& params)
{
    // Validate params
    if (!params.hasProperty("trackId"))
        return createErrorResponse("Missing 'trackId' parameter");
    if (!params.hasProperty("clipId"))
        return createErrorResponse("Missing 'clipId' parameter");
    if (!params.hasProperty("newStartSamples"))
        return createErrorResponse("Missing 'newStartSamples' parameter");

    juce::String trackId = params["trackId"].toString();
    juce::String clipId = params["clipId"].toString();
    juce::int64 newStartSamples = params["newStartSamples"];

    // Check if clip exists in ProjectState
    auto clipTree = projectState.getClip(trackId, clipId);
    if (!clipTree.isValid())
        return createErrorResponse("Clip not found: " + clipId + " on track " + trackId);

    // Validate position (must be >= 0)
    if (newStartSamples < 0)
        return createErrorResponse("Clip position must be >= 0");

    // Move via ProjectState (undoable)
    juce::String actionName = "Wingman: move_clip " + clipId + " to " + juce::String(newStartSamples);
    projectState.moveClip(trackId, clipId, newStartSamples, actionName);

    DBG("CommandAPI: Moved clip: " + clipId + " to " + juce::String(newStartSamples));

    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("clipId", clipId);
    resultObj->setProperty("trackId", trackId);
    resultObj->setProperty("newStartSamples", newStartSamples);

    return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::setTrackVolume(const juce::var& params)
{
    // Validate params
    if (!params.hasProperty("trackId"))
        return createErrorResponse("Missing 'trackId' parameter");
    if (!params.hasProperty("volumeDb"))
        return createErrorResponse("Missing 'volumeDb' parameter");

    juce::String trackId = params["trackId"].toString();
    double volumeDb = params["volumeDb"];

    // Check if track exists in ProjectState
    auto trackTree = projectState.getTrack(trackId);
    if (!trackTree.isValid())
        return createErrorResponse("Track not found: " + trackId);

    // Convert dB to linear gain
    float gain = juce::Decibels::decibelsToGain((float)volumeDb);

    // Clamp to reasonable range (0.0 to 2.0 linear = -inf to +6dB)
    gain = juce::jlimit(0.0f, 2.0f, gain);

    // Set volume via ProjectState (undoable)
    juce::String actionName = "Wingman: set_track_volume " + trackId + " to " + juce::String(volumeDb, 1) + " dB";
    projectState.setTrackVolume(trackId, gain, actionName);

    DBG("CommandAPI: Set track volume: " + trackId + " to " + juce::String(volumeDb) + " dB");

    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("trackId", trackId);
    resultObj->setProperty("volumeDb", volumeDb);
    resultObj->setProperty("volumeLinear", gain);

    return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::setTrackPan(const juce::var& params)
{
    // Validate params
    if (!params.hasProperty("trackId"))
        return createErrorResponse("Missing 'trackId' parameter");
    if (!params.hasProperty("pan"))
        return createErrorResponse("Missing 'pan' parameter");

    juce::String trackId = params["trackId"].toString();
    double pan = params["pan"];

    // Check if track exists in ProjectState
    auto trackTree = projectState.getTrack(trackId);
    if (!trackTree.isValid())
        return createErrorResponse("Track not found: " + trackId);

    // Clamp to valid range (-1.0 to 1.0)
    float panValue = juce::jlimit(-1.0f, 1.0f, (float)pan);

    // Set pan via ProjectState (undoable)
    juce::String actionName = "Wingman: set_track_pan " + trackId + " to " + juce::String(panValue, 2);
    projectState.setTrackPan(trackId, panValue, actionName);

    DBG("CommandAPI: Set track pan: " + trackId + " to " + juce::String(panValue));

    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("trackId", trackId);
    resultObj->setProperty("pan", panValue);

    return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::createClip(const juce::var& params)
{
    // Validate params
    if (!params.hasProperty("trackId"))
        return createErrorResponse("Missing 'trackId' parameter");
    if (!params.hasProperty("type"))
        return createErrorResponse("Missing 'type' parameter (must be 'audio' or 'midi')");
    if (!params.hasProperty("start"))
        return createErrorResponse("Missing 'start' parameter (samples)");
    if (!params.hasProperty("length"))
        return createErrorResponse("Missing 'length' parameter (samples)");

    juce::String trackId = params["trackId"].toString();
    juce::String clipType = params["type"].toString().toLowerCase();
    juce::int64 startSamples = params["start"];
    juce::int64 lengthSamples = params["length"];
    juce::String clipName = params.getProperty("name", "New Clip").toString();

    // Validate type
    if (clipType != "audio" && clipType != "midi")
        return createErrorResponse("Invalid clip type: must be 'audio' or 'midi'");

    // Check if track exists
    auto trackTree = projectState.getTrack(trackId);
    if (!trackTree.isValid())
        return createErrorResponse("Track not found: " + trackId);

    // For audio clips, require audioFile parameter
    if (clipType == "audio" && !params.hasProperty("audioFile"))
        return createErrorResponse("Audio clips require 'audioFile' parameter");

    // Create clip via ProjectState (undoable)
    juce::String actionName = "Wingman: create_clip '" + clipName + "' on " + trackId;
    juce::String clipId = projectState.createClip(trackId, clipType, startSamples, lengthSamples,
                                                   clipName, actionName);

    if (clipId.isEmpty())
        return createErrorResponse("Failed to create clip");

    // If audio clip, set audio file property
    if (clipType == "audio" && params.hasProperty("audioFile"))
    {
        juce::String audioFile = params["audioFile"].toString();
        auto clip = projectState.getClip(trackId, clipId);
        if (clip.isValid())
            clip.setProperty(ProjectState::PROP_AUDIO_FILE, audioFile, &projectState.getUndoManager());
    }

    // Create result
    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("clipId", clipId);
    resultObj->setProperty("trackId", trackId);
    resultObj->setProperty("name", clipName);
    resultObj->setProperty("type", clipType);
    resultObj->setProperty("startSamples", startSamples);
    resultObj->setProperty("lengthSamples", lengthSamples);

    DBG("CommandAPI: Created clip: " + clipId + " on track " + trackId);

    return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::deleteClip(const juce::var& params)
{
    // Validate params
    if (!params.hasProperty("trackId"))
        return createErrorResponse("Missing 'trackId' parameter");
    if (!params.hasProperty("clipId"))
        return createErrorResponse("Missing 'clipId' parameter");

    juce::String trackId = params["trackId"].toString();
    juce::String clipId = params["clipId"].toString();

    // Check if clip exists
    auto clip = projectState.getClip(trackId, clipId);
    if (!clip.isValid())
        return createErrorResponse("Clip not found: " + clipId + " on track " + trackId);

    // Delete via ProjectState (undoable)
    juce::String actionName = "Wingman: delete_clip " + clipId + " from " + trackId;
    projectState.deleteClip(trackId, clipId, actionName);

    DBG("CommandAPI: Deleted clip: " + clipId + " from track " + trackId);

    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("clipId", clipId);
    resultObj->setProperty("trackId", trackId);
    resultObj->setProperty("deleted", true);

    return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::getSessionGraph(const juce::var& params)
{
    juce::ignoreUnused(params);

    // Use SessionGraph to generate full project state
    SessionGraph graph(engine, projectState);
    juce::var graphData = graph.generateGraph();

    DBG("CommandAPI: Generated session graph");

    return createSuccessResponse(graphData);
}

juce::var CommandAPI::undo(const juce::var& params)
{
    juce::ignoreUnused(params);

    if (!projectState.canUndo())
        return createErrorResponse("Nothing to undo");

    // Get current action name (if available) before undoing
    // Note: JUCE UndoManager doesn't expose action names easily,
    // so we'll just report success
    projectState.undo();

    DBG("CommandAPI: Undo executed");

    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("undone", true);
    resultObj->setProperty("message", "Undo successful");

    return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::redo(const juce::var& params)
{
    juce::ignoreUnused(params);

    if (!projectState.canRedo())
        return createErrorResponse("Nothing to redo");

    projectState.redo();

    DBG("CommandAPI: Redo executed");

    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("redone", true);
    resultObj->setProperty("message", "Redo successful");

    return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::history(const juce::var& params)
{
    juce::ignoreUnused(params);

    // JUCE UndoManager doesn't provide easy access to action history
    // For now, return basic undo/redo availability
    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("canUndo", projectState.canUndo());
    resultObj->setProperty("canRedo", projectState.canRedo());
    resultObj->setProperty("message", "Full history tracking not yet implemented");

    DBG("CommandAPI: History query");

    return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::describeInstrument(const juce::var& params)
{
    // Validate params
    if (!params.hasProperty("instrumentId"))
        return createErrorResponse("Missing 'instrumentId' parameter");

    juce::String instrumentId = params["instrumentId"].toString();

    // Get InstrumentRegistry
    auto& registry = InstrumentRegistry::getInstance();

    // Check if instrument exists
    const auto* metadata = registry.getMetadata(instrumentId);
    if (metadata == nullptr)
        return createErrorResponse("Instrument not found: " + instrumentId);

    // Convert metadata to JSON
    juce::var result = metadata->toVar();

    DBG("CommandAPI: Described instrument: " + instrumentId);

    return createSuccessResponse(result);
}

//==============================================================================
// Helper Methods
//==============================================================================

juce::var CommandAPI::createSuccessResponse(const juce::var& result)
{
    auto* response = new juce::DynamicObject();
    response->setProperty("success", true);
    response->setProperty("result", result);
    return juce::var(response);
}

juce::var CommandAPI::createErrorResponse(const juce::String& errorMessage)
{
    auto* response = new juce::DynamicObject();
    response->setProperty("success", false);
    response->setProperty("error", errorMessage);
    return juce::var(response);
}

Track* CommandAPI::findTrackById(const juce::String& trackId)
{
    // Track ID format: "track_N"
    if (!trackId.startsWith("track_"))
        return nullptr;

    int trackIndex = trackId.fromLastOccurrenceOf("_", false, false).getIntValue();

    if (trackIndex < 0 || trackIndex >= engine.getNumTracks())
        return nullptr;

    return const_cast<Track*>(engine.tracks()[trackIndex].get());
}

Track::Clip* CommandAPI::findClipById(Track* track, const juce::String& clipId)
{
    if (track == nullptr)
        return nullptr;

    // Clip ID format: "clip_N"
    if (!clipId.startsWith("clip_"))
        return nullptr;

    int clipIndex = clipId.fromLastOccurrenceOf("_", false, false).getIntValue();

    if (clipIndex < 0 || clipIndex >= track->getNumClips())
        return nullptr;

    return track->getClip(clipIndex);
}

} // namespace zenith
