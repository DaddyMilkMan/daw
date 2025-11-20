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
#include "../../include/Engine.h"
#include "../../include/ProjectState.h"
#include "../../include/TempoMap.h"
#include "../engine/Track.h"
#include "../engine/Clip.h"
#include "../engine/PluginHost.h"
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
    juce::var params = request.hasProperty("params") ? request["params"] : juce::var();

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

    // Plugin commands
    else if (command == "add_plugin")
        return addPlugin(params);
    else if (command == "remove_plugin")
        return removePlugin(params);
    else if (command == "list_plugins")
        return listPlugins(params);
    else if (command == "set_plugin_param")
        return setPluginParam(params);
    else if (command == "get_plugin_params")
        return getPluginParams(params);

    // Automation commands
    else if (command == "add_automation_point")
        return addAutomationPoint(params);
    else if (command == "clear_automation")
        return clearAutomation(params);
    else if (command == "get_automation")
        return getAutomation(params);

    // Tempo/Marker commands
    else if (command == "set_tempo")
        return setTempo(params);
    else if (command == "add_tempo_change")
        return addTempoChange(params);
    else if (command == "get_tempo_map")
        return getTempoMap(params);
    else if (command == "add_marker")
        return addMarker(params);
    else if (command == "get_markers")
        return getMarkers(params);
    else if (command == "delete_marker")
        return deleteMarker(params);
    else if (command == "goto_marker")
        return gotoMarker(params);

    // MIDI Note commands
    else if (command == "add_note")
        return addNote(params);
    else if (command == "move_note")
        return moveNote(params);
    else if (command == "delete_note")
        return deleteNote(params);
    else if (command == "get_notes")
        return getNotes(params);
    else if (command == "set_note_velocity")
        return setNoteVelocity(params);
    else if (command == "set_note_length")
        return setNoteLength(params);

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

    // Note: UndoManager will automatically group actions that happen in quick succession
    // No need to explicitly begin a transaction

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
        bool success = response.hasProperty("success") ? (bool)response["success"] : false;

        if (!success)
        {
            // Command failed - stop batch execution
            juce::String error = response.hasProperty("error") ? response["error"].toString() : "Unknown error";

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
    juce::String name = params.hasProperty("name") ? params["name"].toString() : juce::String("New Track");

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
    juce::String clipName = params.hasProperty("name") ? params["name"].toString() : juce::String("New Clip");

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

//==============================================================================
// Plugin Commands
//==============================================================================

juce::var CommandAPI::listPlugins(const juce::var& params)
{
    juce::ignoreUnused(params);

    juce::var pluginsArray;
    auto* pluginsArrayPtr = pluginsArray.getArray();

    const auto& knownPlugins = engine.getPluginHost().getKnownPlugins();

    for (int i = 0; i < knownPlugins.getNumTypes(); ++i)
    {
        auto desc = knownPlugins.getTypes()[i];
        auto* pluginObj = new juce::DynamicObject();
        pluginObj->setProperty("id", juce::var(desc.createIdentifierString()));
        pluginObj->setProperty("name", juce::var(desc.name));
        pluginObj->setProperty("manufacturer", juce::var(desc.manufacturerName));
        pluginObj->setProperty("format", juce::var(desc.pluginFormatName));
        pluginObj->setProperty("category", juce::var(desc.category));
        
        pluginsArrayPtr->add(juce::var(pluginObj));
    }

    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("plugins", pluginsArray);
    resultObj->setProperty("count", knownPlugins.getNumTypes());

    return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::addPlugin(const juce::var& params)
{
    if (!params.hasProperty("trackId"))
        return createErrorResponse("Missing 'trackId' parameter");
    if (!params.hasProperty("pluginId"))
        return createErrorResponse("Missing 'pluginId' parameter");

    juce::String trackId = params["trackId"].toString();
    juce::String pluginId = params["pluginId"].toString();

    Track* track = findTrackById(trackId);
    if (track == nullptr)
        return createErrorResponse("Track not found: " + trackId);

    // Create plugin instance
    juce::String errorMessage;
    auto instance = engine.getPluginHost().createInstance(
        pluginId,
        engine.getSampleRate(),
        engine.getBufferSize(),
        errorMessage
    );

    if (instance == nullptr)
        return createErrorResponse("Failed to create plugin: " + errorMessage);

    // Add to track
    juce::String pluginName = instance->getName();
    track->addPlugin(std::move(instance));

    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("trackId", trackId);
    resultObj->setProperty("pluginName", pluginName);
    resultObj->setProperty("success", true);

    DBG("CommandAPI: Added plugin " + pluginName + " to " + trackId);

    return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::removePlugin(const juce::var& params)
{
    if (!params.hasProperty("trackId"))
        return createErrorResponse("Missing 'trackId' parameter");
    if (!params.hasProperty("pluginIndex"))
        return createErrorResponse("Missing 'pluginIndex' parameter");

    juce::String trackId = params["trackId"].toString();
    int pluginIndex = params["pluginIndex"];

    Track* track = findTrackById(trackId);
    if (track == nullptr)
        return createErrorResponse("Track not found: " + trackId);

    if (pluginIndex < 0 || pluginIndex >= track->getNumPlugins())
        return createErrorResponse("Invalid plugin index");

    track->removePlugin(pluginIndex);

    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("trackId", trackId);
    resultObj->setProperty("pluginIndex", pluginIndex);
    resultObj->setProperty("success", true);

    DBG("CommandAPI: Removed plugin " + juce::String(pluginIndex) + " from " + trackId);

    return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::setPluginParam(const juce::var& params)
{
    if (!params.hasProperty("trackId"))
        return createErrorResponse("Missing 'trackId' parameter");
    if (!params.hasProperty("pluginIndex"))
        return createErrorResponse("Missing 'pluginIndex' parameter");
    if (!params.hasProperty("paramIndex"))
        return createErrorResponse("Missing 'paramIndex' parameter");
    if (!params.hasProperty("value"))
        return createErrorResponse("Missing 'value' parameter");

    juce::String trackId = params["trackId"].toString();
    int pluginIndex = params["pluginIndex"];
    int paramIndex = params["paramIndex"];
    float value = (float)params["value"];

    Track* track = findTrackById(trackId);
    if (track == nullptr)
        return createErrorResponse("Track not found: " + trackId);

    auto* plugin = track->getPlugin(pluginIndex);
    if (plugin == nullptr)
        return createErrorResponse("Plugin not found at index " + juce::String(pluginIndex));

    auto parameters = plugin->getParameters();
    if (paramIndex < 0 || paramIndex >= parameters.size())
        return createErrorResponse("Invalid parameter index");

    auto* param = parameters[paramIndex];
    if (param != nullptr)
    {
        param->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, value));
    }

    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("trackId", trackId);
    resultObj->setProperty("pluginIndex", pluginIndex);
    resultObj->setProperty("paramIndex", paramIndex);
    resultObj->setProperty("value", value);
    resultObj->setProperty("success", true);

    return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::getPluginParams(const juce::var& params)
{
    if (!params.hasProperty("trackId"))
        return createErrorResponse("Missing 'trackId' parameter");
    if (!params.hasProperty("pluginIndex"))
        return createErrorResponse("Missing 'pluginIndex' parameter");

    juce::String trackId = params["trackId"].toString();
    int pluginIndex = params["pluginIndex"];

    Track* track = findTrackById(trackId);
    if (track == nullptr)
        return createErrorResponse("Track not found: " + trackId);

    auto* plugin = track->getPlugin(pluginIndex);
    if (plugin == nullptr)
        return createErrorResponse("Plugin not found at index " + juce::String(pluginIndex));

    juce::var paramsArray;
    auto* paramsArrayPtr = paramsArray.getArray();

    auto parameters = plugin->getParameters();
    for (int i = 0; i < parameters.size(); ++i)
    {
        auto* param = parameters[i];
        if (param != nullptr)
        {
            auto* paramObj = new juce::DynamicObject();
            paramObj->setProperty("index", i);
            paramObj->setProperty("name", param->getName(100));
            paramObj->setProperty("value", param->getValue());
            paramObj->setProperty("label", juce::String(param->getLabel()));
            paramObj->setProperty("numSteps", param->getNumSteps());
            paramObj->setProperty("isDiscrete", param->isDiscrete());
            
            paramsArrayPtr->add(juce::var(paramObj));
        }
    }

    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("trackId", trackId);
    resultObj->setProperty("pluginIndex", pluginIndex);
    resultObj->setProperty("parameters", paramsArray);
    resultObj->setProperty("count", parameters.size());

    return createSuccessResponse(juce::var(resultObj));
}

//==============================================================================
// Automation Commands
//==============================================================================

juce::var CommandAPI::addAutomationPoint(const juce::var& params)
{
    if (!params.hasProperty("trackId")) return createErrorResponse("Missing 'trackId'");
    if (!params.hasProperty("paramId")) return createErrorResponse("Missing 'paramId'");
    if (!params.hasProperty("timeBeats")) return createErrorResponse("Missing 'timeBeats'");
    if (!params.hasProperty("value")) return createErrorResponse("Missing 'value'");

    juce::String trackId = params["trackId"].toString();
    juce::String paramId = params["paramId"].toString();
    double timeBeats = (double)params["timeBeats"];
    double value = (double)params["value"];

    juce::String pointId = projectState.addAutomationPoint(trackId, paramId, timeBeats, value, "Wingman: Add Point");

    if (pointId.isEmpty())
        return createErrorResponse("Failed to add automation point");

    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("pointId", pointId);
    resultObj->setProperty("success", true);

    return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::clearAutomation(const juce::var& params)
{
    if (!params.hasProperty("trackId")) return createErrorResponse("Missing 'trackId'");
    if (!params.hasProperty("paramId")) return createErrorResponse("Missing 'paramId'");

    juce::String trackId = params["trackId"].toString();
    juce::String paramId = params["paramId"].toString();

    bool success = projectState.clearAutomation(trackId, paramId, "Wingman: Clear Automation");

    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("success", success);

    return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::getAutomation(const juce::var& params)
{
    if (!params.hasProperty("trackId")) return createErrorResponse("Missing 'trackId'");
    if (!params.hasProperty("paramId")) return createErrorResponse("Missing 'paramId'");

    juce::String trackId = params["trackId"].toString();
    juce::String paramId = params["paramId"].toString();

    auto envelope = projectState.getAutomationEnvelope(trackId, paramId);
    
    juce::var pointsArray;
    auto* pointsArrayPtr = pointsArray.getArray();

    if (envelope.isValid())
    {
        for (const auto& point : envelope)
        {
            if (point.hasType(ProjectState::ID_POINT))
            {
                auto* pointObj = new juce::DynamicObject();
                pointObj->setProperty("id", point.getProperty(ProjectState::PROP_ID));
                pointObj->setProperty("timeBeats", point.getProperty(ProjectState::PROP_TIME_BEATS));
                pointObj->setProperty("value", point.getProperty(ProjectState::PROP_VALUE));
                pointsArrayPtr->add(juce::var(pointObj));
            }
        }
    }

    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("trackId", trackId);
    resultObj->setProperty("paramId", paramId);
    resultObj->setProperty("points", pointsArray);

    return createSuccessResponse(juce::var(resultObj));
}

//==============================================================================
// Tempo/Marker Commands
//==============================================================================

juce::var CommandAPI::setTempo(const juce::var& params)
{
    if (!params.hasProperty("bpm")) return createErrorResponse("Missing 'bpm'");

    double bpm = (double)params["bpm"];
    projectState.setTempo(bpm);
    
    // Sync tempo map with new global tempo
    engine.syncTempoMap();

    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("bpm", bpm);
    resultObj->setProperty("success", true);

    return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::addTempoChange(const juce::var& params)
{
    if (!params.hasProperty("timeBeats")) return createErrorResponse("Missing 'timeBeats'");
    if (!params.hasProperty("bpm")) return createErrorResponse("Missing 'bpm'");

    double timeBeats = (double)params["timeBeats"];
    double bpm = (double)params["bpm"];

    juce::String pointId = projectState.addTempoChange(timeBeats, bpm, "Wingman: Add Tempo Change");
    
    // Sync tempo map with new tempo change
    engine.syncTempoMap();

    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("pointId", pointId);
    resultObj->setProperty("success", true);

    return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::getTempoMap(const juce::var& params)
{
    juce::ignoreUnused(params);
    
    juce::var changesArray;
    auto* changesArrayPtr = changesArray.getArray();

    // Add global tempo as first point
    auto* globalObj = new juce::DynamicObject();
    globalObj->setProperty("timeBeats", 0.0);
    globalObj->setProperty("bpm", projectState.getTempo());
    changesArrayPtr->add(juce::var(globalObj));

    // Add tempo map points
    auto tempoMap = projectState.getTempoMap();
    if (tempoMap.isValid())
    {
        for (const auto& point : tempoMap)
        {
            if (point.hasType(ProjectState::ID_TEMPO_POINT))
            {
                auto* pointObj = new juce::DynamicObject();
                pointObj->setProperty("id", point.getProperty(ProjectState::PROP_ID));
                pointObj->setProperty("timeBeats", point.getProperty(ProjectState::PROP_TIME_BEATS));
                pointObj->setProperty("bpm", point.getProperty(ProjectState::PROP_BPM));
                changesArrayPtr->add(juce::var(pointObj));
            }
        }
    }

    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("tempoChanges", changesArray);

    return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::addMarker(const juce::var& params)
{
    if (!params.hasProperty("timeBeats")) return createErrorResponse("Missing 'timeBeats'");
    if (!params.hasProperty("name")) return createErrorResponse("Missing 'name'");

    double timeBeats = (double)params["timeBeats"];
    juce::String name = params["name"].toString();
    juce::String color = params.hasProperty("color") ? params["color"].toString() : juce::String("FF0000");

    juce::String markerId = projectState.addMarker(timeBeats, name, color, "Wingman: Add Marker");

    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("markerId", markerId);
    resultObj->setProperty("success", true);

    return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::getMarkers(const juce::var& params)
{
    juce::ignoreUnused(params);
    
    juce::var markersArray;
    auto* markersArrayPtr = markersArray.getArray();

    auto markers = projectState.getMarkers();
    if (markers.isValid())
    {
        for (const auto& marker : markers)
        {
            if (marker.hasType(ProjectState::ID_MARKER))
            {
                auto* markerObj = new juce::DynamicObject();
                markerObj->setProperty("id", marker.getProperty(ProjectState::PROP_ID));
                markerObj->setProperty("timeBeats", marker.getProperty(ProjectState::PROP_TIME_BEATS));
                markerObj->setProperty("name", marker.getProperty(ProjectState::PROP_NAME));
                markerObj->setProperty("color", marker.getProperty(ProjectState::PROP_COLOR));
                markersArrayPtr->add(juce::var(markerObj));
            }
        }
    }

    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("markers", markersArray);

    return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::deleteMarker(const juce::var& params)
{
    if (!params.hasProperty("markerId")) return createErrorResponse("Missing 'markerId'");

    juce::String markerId = params["markerId"].toString();
    bool success = projectState.deleteMarker(markerId, "Wingman: Delete Marker");

    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("success", success);

    return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::gotoMarker(const juce::var& params)
{
    if (!params.hasProperty("markerId")) return createErrorResponse("Missing 'markerId'");

    juce::String markerId = params["markerId"].toString();
    
    auto markers = projectState.getMarkers();
    if (markers.isValid())
    {
        for (const auto& marker : markers)
        {
            if (marker[ProjectState::PROP_ID].toString() == markerId)
            {
                double timeBeats = marker[ProjectState::PROP_TIME_BEATS];
                
                // Convert beats to samples using TempoMap
                double sampleRate = engine.getSampleRate();
                juce::int64 timeSamples = engine.getTempoMap().beatsToSamples(timeBeats, sampleRate);
                
                // Set playhead position
                engine.setPlayheadSamples(timeSamples);
                
                auto* resultObj = new juce::DynamicObject();
                resultObj->setProperty("timeBeats", timeBeats);
                resultObj->setProperty("success", true);
                return createSuccessResponse(juce::var(resultObj));
            }
        }
    }

    return createErrorResponse("Marker not found");
}

//==============================================================================
// MIDI Note Commands
//==============================================================================

juce::var CommandAPI::addNote(const juce::var& params)
{
    if (!params.hasProperty("clipId")) return createErrorResponse("Missing 'clipId'");
    if (!params.hasProperty("pitch")) return createErrorResponse("Missing 'pitch'");
    if (!params.hasProperty("startBeats")) return createErrorResponse("Missing 'startBeats'");
    if (!params.hasProperty("lengthBeats")) return createErrorResponse("Missing 'lengthBeats'");
    if (!params.hasProperty("velocity")) return createErrorResponse("Missing 'velocity'");

    juce::String clipId = params["clipId"].toString();
    
    ProjectState::MidiNoteSpec note;
    note.pitch = (int)params["pitch"];
    note.startBeats = (double)params["startBeats"];
    note.lengthBeats = (double)params["lengthBeats"];
    note.velocity = (int)params["velocity"];
    
    juce::String noteId = projectState.addMidiNote(clipId, note, "Wingman: Add Note");

    if (noteId.isEmpty())
        return createErrorResponse("Failed to add note");

    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("noteId", noteId);
    resultObj->setProperty("success", true);

    return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::moveNote(const juce::var& params)
{
    if (!params.hasProperty("clipId")) return createErrorResponse("Missing 'clipId'");
    if (!params.hasProperty("noteId")) return createErrorResponse("Missing 'noteId'");
    if (!params.hasProperty("newStartBeats")) return createErrorResponse("Missing 'newStartBeats'");
    if (!params.hasProperty("newPitch")) return createErrorResponse("Missing 'newPitch'");

    juce::String clipId = params["clipId"].toString();
    juce::String noteId = params["noteId"].toString();
    double newStartBeats = (double)params["newStartBeats"];
    int newPitch = (int)params["newPitch"];

    projectState.moveMidiNote(clipId, noteId, newStartBeats, newPitch, "Wingman: Move Note");

    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("success", true);

    return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::deleteNote(const juce::var& params)
{
    if (!params.hasProperty("clipId")) return createErrorResponse("Missing 'clipId'");
    if (!params.hasProperty("noteId")) return createErrorResponse("Missing 'noteId'");

    juce::String clipId = params["clipId"].toString();
    juce::String noteId = params["noteId"].toString();

    projectState.removeMidiNote(clipId, noteId, "Wingman: Delete Note");

    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("success", true);

    return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::getNotes(const juce::var& params)
{
    if (!params.hasProperty("clipId")) return createErrorResponse("Missing 'clipId'");

    juce::String clipId = params["clipId"].toString();
    auto notes = projectState.getMidiNotesForClip(clipId);

    juce::var notesArray;
    auto* notesArrayPtr = notesArray.getArray();

    for (const auto& note : notes)
    {
        auto* noteObj = new juce::DynamicObject();
        noteObj->setProperty("id", note.id);
        noteObj->setProperty("pitch", note.pitch);
        noteObj->setProperty("startBeats", note.startBeats);
        noteObj->setProperty("lengthBeats", note.lengthBeats);
        noteObj->setProperty("velocity", note.velocity);
        noteObj->setProperty("muted", note.muted);
        
        notesArrayPtr->add(juce::var(noteObj));
    }

    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("clipId", clipId);
    resultObj->setProperty("notes", notesArray);
    resultObj->setProperty("count", notes.size());

    return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::setNoteVelocity(const juce::var& params)
{
    if (!params.hasProperty("clipId")) return createErrorResponse("Missing 'clipId'");
    if (!params.hasProperty("noteId")) return createErrorResponse("Missing 'noteId'");
    if (!params.hasProperty("velocity")) return createErrorResponse("Missing 'velocity'");

    juce::String clipId = params["clipId"].toString();
    juce::String noteId = params["noteId"].toString();
    int velocity = (int)params["velocity"];

    projectState.setMidiNoteVelocity(clipId, noteId, velocity, "Wingman: Set Velocity");

    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("success", true);

    return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::setNoteLength(const juce::var& params)
{
    if (!params.hasProperty("clipId")) return createErrorResponse("Missing 'clipId'");
    if (!params.hasProperty("noteId")) return createErrorResponse("Missing 'noteId'");
    if (!params.hasProperty("lengthBeats")) return createErrorResponse("Missing 'lengthBeats'");

    juce::String clipId = params["clipId"].toString();
    juce::String noteId = params["noteId"].toString();
    double lengthBeats = (double)params["lengthBeats"];

    projectState.setMidiNoteLength(clipId, noteId, lengthBeats, "Wingman: Set Length");

    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("success", true);

    return createSuccessResponse(juce::var(resultObj));
}

} // namespace zenith
