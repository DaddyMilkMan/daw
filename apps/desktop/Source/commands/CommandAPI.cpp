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

#include "../dsp/ONNXStemSeparator.h"

namespace zenith {

//==============================================================================
CommandAPI::CommandAPI(Engine& eng, ProjectState& state)
    : engine(eng), projectState(state)
{
    DBG("CommandAPI: Initialized");
    initializeCommandMap();
}

void CommandAPI::initializeCommandMap()
{
    commandMap["list_tracks"] = CommandID::ListTracks;
    commandMap["create_track"] = CommandID::CreateTrack;
    commandMap["delete_track"] = CommandID::DeleteTrack;
    commandMap["rename_track"] = CommandID::RenameTrack;
    commandMap["set_track_volume"] = CommandID::SetTrackVolume;
    commandMap["set_track_pan"] = CommandID::SetTrackPan;
    commandMap["export_audio"] = CommandID::ExportAudio;
    commandMap["separate_track"] = CommandID::SeparateTrack;
    
    commandMap["list_clips"] = CommandID::ListClips;
    commandMap["create_clip"] = CommandID::CreateClip; // add_clip alias? original code checked add_clip
    commandMap["add_clip"] = CommandID::CreateClip; 
    commandMap["delete_clip"] = CommandID::DeleteClip;
    commandMap["split_clip"] = CommandID::SplitClip;
    commandMap["move_clip"] = CommandID::MoveClip;
    commandMap["resize_clip"] = CommandID::ResizeClip;
    
    commandMap["play"] = CommandID::Play;
    commandMap["stop"] = CommandID::Stop;
    commandMap["record"] = CommandID::Record;
    commandMap["rewind"] = CommandID::Rewind;
    commandMap["set_loop"] = CommandID::SetLoop;
    commandMap["set_tempo"] = CommandID::SetTempo;
    commandMap["set_time_signature"] = CommandID::SetTimeSignature;
    
    commandMap["get_session_graph"] = CommandID::GetSessionGraph;
    commandMap["undo"] = CommandID::Undo;
    commandMap["redo"] = CommandID::Redo;
    commandMap["history"] = CommandID::History;
    
    commandMap["describe_instrument"] = CommandID::DescribeInstrument;
    
    commandMap["add_plugin"] = CommandID::AddPlugin;
    commandMap["remove_plugin"] = CommandID::RemovePlugin;
    commandMap["set_plugin_param"] = CommandID::SetPluginParam;
    commandMap["get_plugin_params"] = CommandID::GetPluginParams;
    commandMap["list_plugins"] = CommandID::ListPlugins;
    
    commandMap["add_automation_point"] = CommandID::AddAutomationPoint;
    commandMap["clear_automation"] = CommandID::ClearAutomation;
    commandMap["get_automation"] = CommandID::GetAutomation;
    
    commandMap["add_tempo_change"] = CommandID::AddTempoChange;
    commandMap["get_tempo_map"] = CommandID::GetTempoMap;
    
    commandMap["add_marker"] = CommandID::AddMarker;
    commandMap["get_markers"] = CommandID::GetMarkers;
    commandMap["delete_marker"] = CommandID::DeleteMarker;
    commandMap["goto_marker"] = CommandID::GotoMarker;
    
    commandMap["add_note"] = CommandID::AddNote;
    commandMap["delete_note"] = CommandID::DeleteNote;
    commandMap["move_note"] = CommandID::MoveNote;
    commandMap["get_notes"] = CommandID::GetNotes;
    commandMap["set_note_velocity"] = CommandID::SetNoteVelocity;
    commandMap["set_note_length"] = CommandID::SetNoteLength;
    commandMap["get_midi_data"] = CommandID::GetMidiData;
    commandMap["set_clip_notes"] = CommandID::SetClipNotes;
    
    commandMap["list_presets"] = CommandID::ListPresets;
    commandMap["load_preset"] = CommandID::LoadPreset;
    commandMap["save_preset"] = CommandID::SavePreset;
    commandMap["create_preset"] = CommandID::CreatePreset;
    commandMap["delete_preset"] = CommandID::DeletePreset;
    commandMap["generate_preset"] = CommandID::GeneratePreset;
    
    commandMap["get_instrument_parameters"] = CommandID::GetInstrumentParameters;
    commandMap["set_instrument_parameter"] = CommandID::SetInstrumentParameter;
    commandMap["get_instrument_parameter_schema"] = CommandID::GetInstrumentParameterSchema;
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

    juce::String commandStr = request["command"].toString();
    juce::var params = request.hasProperty("params") ? request["params"] : juce::var();

    DBG("CommandAPI: Executing command: " + commandStr);
    
    // Map Lookup
    auto it = commandMap.find(commandStr.toStdString());
    if (it == commandMap.end()) {
        return createErrorResponse("Unknown command: " + commandStr);
    }
    
    CommandID id = it->second;
    
    switch (id) {
        case CommandID::ListTracks: return listTracks(params);
        case CommandID::CreateTrack: return createTrack(params);
        case CommandID::DeleteTrack: return deleteTrack(params);
        case CommandID::RenameTrack: return renameTrack(params);
        case CommandID::SetTrackVolume: return setTrackVolume(params);
        case CommandID::SetTrackPan: return setTrackPan(params);
        case CommandID::ExportAudio: return exportAudio(params);
        case CommandID::SeparateTrack: return separateTrack(params);
        
        case CommandID::ListClips: return listClips(params);
        case CommandID::CreateClip: return createClip(params);
        case CommandID::DeleteClip: return deleteClip(params);
        case CommandID::SplitClip: return splitClip(params);
        case CommandID::MoveClip: return moveClip(params);
        case CommandID::ResizeClip: return resizeClip(params);
        
        case CommandID::Play: return play(params);
        case CommandID::Stop: return stop(params);
        case CommandID::Record: return record(params);
        case CommandID::Rewind: return rewind(params);
        case CommandID::SetLoop: return setLoop(params);
        case CommandID::SetTempo: return setTempo(params);
        case CommandID::SetTimeSignature: return setTimeSignature(params);
        
        case CommandID::GetSessionGraph: return getSessionGraph(params);
        case CommandID::Undo: return undo(params);
        case CommandID::Redo: return redo(params);
        case CommandID::History: return history(params);
        
        case CommandID::DescribeInstrument: return describeInstrument(params);
        
        case CommandID::AddPlugin: return addPlugin(params);
        case CommandID::RemovePlugin: return removePlugin(params);
        case CommandID::ListPlugins: return listPlugins(params);
        case CommandID::SetPluginParam: return setPluginParam(params);
        case CommandID::GetPluginParams: return getPluginParams(params);
        
        case CommandID::AddAutomationPoint: return addAutomationPoint(params);
        case CommandID::ClearAutomation: return clearAutomation(params);
        case CommandID::GetAutomation: return getAutomation(params);
        
        case CommandID::AddTempoChange: return addTempoChange(params);
        case CommandID::GetTempoMap: return getTempoMap(params);
        
        case CommandID::AddMarker: return addMarker(params);
        case CommandID::GetMarkers: return getMarkers(params);
        case CommandID::DeleteMarker: return deleteMarker(params);
        case CommandID::GotoMarker: return gotoMarker(params);
        
        case CommandID::AddNote: return addNote(params);
        case CommandID::DeleteNote: return deleteNote(params);
        case CommandID::MoveNote: return moveNote(params);
        case CommandID::GetNotes: return getNotes(params);
        case CommandID::SetNoteVelocity: return setNoteVelocity(params);
        case CommandID::SetNoteLength: return setNoteLength(params);
        case CommandID::GetMidiData: return getMidiData(params);
        case CommandID::SetClipNotes: return setClipNotes(params);
        
        case CommandID::ListPresets: return listPresets(params);
        case CommandID::LoadPreset: return loadPreset(params);
        case CommandID::SavePreset: return savePreset(params);
        case CommandID::CreatePreset: return createPreset(params);
        case CommandID::DeletePreset: return deletePreset(params);
        case CommandID::GeneratePreset: return generatePreset(params);
        
        case CommandID::GetInstrumentParameters: return getInstrumentParameters(params);
        case CommandID::SetInstrumentParameter: return setInstrumentParameter(params);
        case CommandID::GetInstrumentParameterSchema: return getInstrumentParameterSchema(params);
        
        default: return createErrorResponse("Command ID not implemented: " + commandStr);
    }
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

    // Start a new undo transaction for this batch
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
    resultObj->setProperty("newName", newName);
    resultObj->setProperty("success", true);

    return juce::var(resultObj);
}

// list_clips command
juce::var CommandAPI::listClips(const juce::var& params)
{
    if (!params.hasProperty("trackId"))
        return createErrorResponse("Missing 'trackId' parameter");

    juce::String trackId = params["trackId"].toString();
    Track* track = findTrackById(trackId);
    
    if (track == nullptr)
        return createErrorResponse("Track not found: " + trackId);

    juce::var clipsArray;
    auto* clipsArrayPtr = clipsArray.getArray();

    for (int i = 0; i < track->getNumClips(); ++i)
    {
        auto* clip = track->getClip(i);
        if (clip != nullptr)
        {
            auto* clipObj = new juce::DynamicObject();
            clipObj->setProperty("id", "clip_" + juce::String(i));
            clipObj->setProperty("name", clip->getName());
            clipObj->setProperty("start", clip->getStartPosition());
            clipObj->setProperty("length", clip->getLength());
            clipObj->setProperty("offset", clip->getOffset());
            
            clipsArrayPtr->add(juce::var(clipObj));
        }
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

juce::var CommandAPI::resizeClip(const juce::var& params)
{
    // Validate params
    if (!params.hasProperty("trackId"))
        return createErrorResponse("Missing 'trackId' parameter");
    if (!params.hasProperty("clipId"))
        return createErrorResponse("Missing 'clipId' parameter");
    if (!params.hasProperty("newLengthSamples"))
        return createErrorResponse("Missing 'newLengthSamples' parameter");

    juce::String trackId = params["trackId"].toString();
    juce::String clipId = params["clipId"].toString();
    juce::int64 newLengthSamples = params["newLengthSamples"];

    // Check if clip exists
    auto clipTree = projectState.getClip(trackId, clipId);
    if (!clipTree.isValid())
        return createErrorResponse("Clip not found: " + clipId + " on track " + trackId);

    if (newLengthSamples <= 0)
        return createErrorResponse("Clip length must be > 0");

    // Resize via ProjectState (undoable)
    juce::String actionName = "Wingman: resize_clip " + clipId + " to " + juce::String(newLengthSamples);
    projectState.resizeClip(trackId, clipId, newLengthSamples, actionName);

    DBG("CommandAPI: Resized clip: " + clipId + " to " + juce::String(newLengthSamples));

    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("clipId", clipId);
    resultObj->setProperty("trackId", trackId);
    resultObj->setProperty("newLengthSamples", newLengthSamples);

    return createSuccessResponse(juce::var(resultObj));
}

//==============================================================================
// Transport Commands
//==============================================================================

juce::var CommandAPI::play(const juce::var& params)
{
    juce::ignoreUnused(params);
    engine.play();
    return createSuccessResponse(juce::var());
}

juce::var CommandAPI::stop(const juce::var& params)
{
    juce::ignoreUnused(params);
    engine.stop();
    return createSuccessResponse(juce::var());
}

juce::var CommandAPI::record(const juce::var& params)
{
    juce::ignoreUnused(params);
    engine.record();
    return createSuccessResponse(juce::var());
}

juce::var CommandAPI::rewind(const juce::var& params)
{
    juce::ignoreUnused(params);
    engine.setPlayheadSamples(0);
    return createSuccessResponse(juce::var());
}

juce::var CommandAPI::setLoop(const juce::var& params)
{
    if (!params.hasProperty("enabled"))
        return createErrorResponse("Missing 'enabled' parameter");

    bool enabled = params["enabled"];
    engine.setLooping(enabled);

    if (params.hasProperty("start") && params.hasProperty("end"))
    {
        juce::int64 start = params["start"];
        juce::int64 end = params["end"];
        engine.setLoopRegion(start, end);
    }

    return createSuccessResponse(juce::var());
}



juce::var CommandAPI::setTimeSignature(const juce::var& params)
{
    if (!params.hasProperty("numerator") || !params.hasProperty("denominator"))
        return createErrorResponse("Missing numerator/denominator");

    int num = params["numerator"];
    int den = params["denominator"];
    
    projectState.setTimeSignature(num, den);
    return createSuccessResponse(juce::var());
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

    // Send RT event for immediate response
    Track* track = findTrackById(trackId);
    if (track) {
        int trackIndex = -1;
        for (int i = 0; i < engine.getNumTracks(); ++i) {
            if (engine.tracks()[i].get() == track) {
                trackIndex = i;
                break;
            }
        }
        if (trackIndex >= 0) {
            zenith::EngineEvent e(zenith::EngineEvent::Type::SetTrackVolume);
            e.trackIndex = trackIndex;
            e.value = gain;
            engine.queueEvent(e);
        }
    }

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

    // Send RT event
    Track* track = findTrackById(trackId);
    if (track) {
        int trackIndex = -1;
        for (int i = 0; i < engine.getNumTracks(); ++i) {
            if (engine.tracks()[i].get() == track) {
                trackIndex = i;
                break;
            }
        }
        if (trackIndex >= 0) {
            zenith::EngineEvent e(zenith::EngineEvent::Type::SetTrackPan);
            e.trackIndex = trackIndex;
            e.value = panValue;
            engine.queueEvent(e);
        }
    }

    // Set pan via ProjectState (undoable)
    juce::String actionName = "Wingman: set_track_pan " + trackId + " to " + juce::String(panValue, 2);
    projectState.setTrackPan(trackId, panValue, actionName);

    DBG("CommandAPI: Set track pan: " + trackId + " to " + juce::String(panValue));

    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("trackId", trackId);
    resultObj->setProperty("pan", panValue);

    return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::exportAudio(const juce::var& params)
{
    if (!params.hasProperty("outputPath"))
        return createErrorResponse("Missing 'outputPath' parameter");

    const juce::String outputPath = params["outputPath"].toString();
    const double sampleRate = params.hasProperty("sampleRate") ? static_cast<double>(params["sampleRate"]) : 44100.0;
    const int bitDepth = params.hasProperty("bitDepth") ? static_cast<int>(params["bitDepth"]) : 24;
    const double durationSeconds = params.hasProperty("durationSeconds")
        ? static_cast<double>(params["durationSeconds"])
        : 0.0;

    if (outputPath.isEmpty())
        return createErrorResponse("outputPath cannot be empty");

    const bool success = engine.exportProjectToWav(juce::File(outputPath), sampleRate, bitDepth, durationSeconds);

    if (!success)
        return createErrorResponse("Export failed. Check engine logs for details.");

    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("outputPath", outputPath);
    resultObj->setProperty("sampleRate", sampleRate);
    resultObj->setProperty("bitDepth", bitDepth);
    resultObj->setProperty("durationSeconds", durationSeconds);

    DBG("CommandAPI: Exported audio to " + outputPath);

    return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::separateTrack(const juce::var& params)
{
    if (!params.hasProperty("trackId"))
        return createErrorResponse("Missing 'trackId' parameter");

    juce::String trackId = params["trackId"].toString();
    Track* track = findTrackById(trackId);

    if (track == nullptr)
        return createErrorResponse("Track not found: " + trackId);

    // 1. Determine duration
    juce::int64 maxEnd = 0;
    for (int i = 0; i < track->getNumClips(); ++i)
    {
        auto* clip = track->getClip(i);
        if (clip)
        {
            maxEnd = juce::jmax(maxEnd, clip->getStartPosition() + clip->getLength());
        }
    }

    if (maxEnd == 0)
        return createErrorResponse("Track is empty");

    // 2. Render track to buffer
    double sampleRate = engine.getSampleRate();
    if (sampleRate <= 0) sampleRate = 44100.0;

    juce::AudioBuffer<float> trackBuffer(2, (int)maxEnd);
    trackBuffer.clear();

    // Render in blocks
    int blockSize = 1024;
    juce::int64 samplesRendered = 0;
    
    // Create a temporary buffer for block processing
    juce::AudioBuffer<float> blockBuffer(2, blockSize);

    while (samplesRendered < maxEnd)
    {
        int numSamples = (int)juce::jmin((juce::int64)blockSize, maxEnd - samplesRendered);
        
        blockBuffer.clear();
        juce::AudioSourceChannelInfo info(&blockBuffer, 0, numSamples);
        
        // Render track
        track->getNextAudioBlock(info, samplesRendered, nullptr);
        
        // Copy to main buffer
        for (int ch = 0; ch < 2; ++ch)
        {
            trackBuffer.copyFrom(ch, (int)samplesRendered, blockBuffer, ch, 0, numSamples);
        }
        
        samplesRendered += numSamples;
    }

    // 3. Perform Separation
    ONNXStemSeparator separator;
    // Initialize with dummy path (ONNXStemSeparator handles missing model gracefully via DSP fallback)
    separator.initialize(juce::File()); 
    
    auto result = separator.separate(trackBuffer, sampleRate);

    if (!result.success)
        return createErrorResponse("Separation failed: " + result.error);

    // 4. Save Stems and Create Tracks
    juce::File recordingsDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
        .getChildFile("ZenithDAW/Stems");
    
    if (!recordingsDir.exists())
        recordingsDir.createDirectory();

    juce::String timestamp = juce::Time::getCurrentTime().formatted("%Y%m%d_%H%M%S");
    juce::String baseName = track->getName() + "_" + timestamp;

    struct StemInfo {
        juce::String suffix;
        juce::AudioBuffer<float>& buffer;
    };

    StemInfo stems[] = {
        { "Vocals", result.vocals },
        { "Drums", result.drums },
        { "Bass", result.bass },
        { "Other", result.other }
    };

    juce::var createdTracks;
    juce::WavAudioFormat wavFormat;

    for (const auto& stem : stems)
    {
        // Save to file
        juce::File stemFile = recordingsDir.getChildFile(baseName + "_" + stem.suffix + ".wav");
        std::unique_ptr<juce::FileOutputStream> fileStream(new juce::FileOutputStream(stemFile));

        if (fileStream->openedOk())
        {
            std::unique_ptr<juce::AudioFormatWriter> writer(wavFormat.createWriterFor(
                fileStream.release(), sampleRate, 2, 24, {}, 0));

            if (writer)
            {
                writer->writeFromAudioSampleBuffer(stem.buffer, 0, stem.buffer.getNumSamples());
                writer.reset(); // Close file

                // Create Track
                juce::String newTrackName = track->getName() + " (" + stem.suffix + ")";
                juce::String newTrackId = projectState.addTrack(newTrackName, "audio");
                
                // Create Clip
                juce::String clipName = stem.suffix;
                juce::String actionName = "Wingman: create_stem_clip";
                juce::String clipId = projectState.createClip(newTrackId, "audio", 0, stem.buffer.getNumSamples(), clipName, actionName);
                
                // Set Audio File
                auto clipTree = projectState.getClip(newTrackId, clipId);
                if (clipTree.isValid())
                {
                    clipTree.setProperty(ProjectState::PROP_AUDIO_FILE, stemFile.getFullPathName(), &projectState.getUndoManager());
                }
                
                createdTracks.append(newTrackId);
            }
        }
    }

    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("originalTrackId", trackId);
    resultObj->setProperty("createdTracks", createdTracks);
    resultObj->setProperty("success", true);

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

    // JUCE UndoManager doesn't provide easy access to action history
    // For now, return basic undo/redo availability
    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("canUndo", projectState.canUndo());
    resultObj->setProperty("canRedo", projectState.canRedo());
    resultObj->setProperty("message", "Full history tracking not yet implemented");

    DBG("CommandAPI: History query");

    return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::history(const juce::var& params)
{
    juce::ignoreUnused(params);

    // Return undo/redo history status
    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("canUndo", projectState.canUndo());
    resultObj->setProperty("canRedo", projectState.canRedo());
    
    // In a real implementation, we would list the actual transactions
    juce::var undoStack;
    juce::var redoStack;
    
    resultObj->setProperty("undoStack", undoStack);
    resultObj->setProperty("redoStack", redoStack);

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
    InstrumentMetadata metadata;
    if (!registry.getMetadata(instrumentId, metadata))
        return createErrorResponse("Instrument not found: " + instrumentId);

    // Convert metadata to JSON
    juce::var result = metadata.toVar();

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
    
    // Get track index from ID
    Track* track = findTrackById(trackId);
    if (track == nullptr)
        return createErrorResponse("Track not found: " + trackId);
        
    // Find track index in engine (inefficient search, but safe for now)
    // Ideally trackId should encode index or we have a lookup
    int trackIndex = -1;
    for (int i = 0; i < engine.getNumTracks(); ++i) {
        if (engine.tracks()[i].get() == track) {
            trackIndex = i;
            break;
        }
    }

    if (trackIndex >= 0) {
        // Use fast path for real-time update
        zenith::EngineEvent e(zenith::EngineEvent::Type::SetPluginParam);
        e.trackIndex = trackIndex;
        e.pluginIndex = pluginIndex;
        e.paramIndex = paramIndex;
        e.value = value;
        
        if (engine.queueEvent(e)) {
             auto* resultObj = new juce::DynamicObject();
            resultObj->setProperty("trackId", trackId);
            resultObj->setProperty("pluginIndex", pluginIndex);
            resultObj->setProperty("paramIndex", paramIndex);
            resultObj->setProperty("value", value);
            resultObj->setProperty("success", true);
            resultObj->setProperty("mode", "realtime");
            return createSuccessResponse(juce::var(resultObj));
        }
    }

    // Fallback to old slow method if queue full or track not found
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
    resultObj->setProperty("mode", "fallback");

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

    // Validate paramId
    if (paramId != "volume" && paramId != "pan" && paramId != "mute")
        return createErrorResponse("Invalid paramId: must be 'volume', 'pan', or 'mute'");

    juce::String pointId = projectState.addAutomationPoint(trackId, paramId, timeBeats, value, "Wingman: Add Automation Point");

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
        // The envelope contains a POINTS container (ID_POINT) which contains the actual points (ID_POINT)
        // See ProjectState structure discussion
        auto pointsContainer = envelope.getChildWithName(ProjectState::ID_POINT);
        
        if (pointsContainer.isValid())
        {
            for (const auto& point : pointsContainer)
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
    }

    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("trackId", trackId);
    resultObj->setProperty("paramId", paramId);
    resultObj->setProperty("points", pointsArray);
    resultObj->setProperty("count", pointsArray.size());

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

juce::var CommandAPI::getMidiData(const juce::var& params)
{
    const juce::String trackFilter = params.hasProperty("trackId") ? params["trackId"].toString() : juce::String();
    const juce::String clipFilter = params.hasProperty("clipId") ? params["clipId"].toString() : juce::String();

    juce::var tracksVar;
    auto* tracksArray = tracksVar.getArray();

    for (int i = 0; i < projectState.getNumTracks(); ++i)
    {
        auto trackTree = projectState.getTrackByIndex(i);
        if (!trackTree.isValid())
            continue;

        juce::String trackId = trackTree.getProperty(ProjectState::PROP_ID, "").toString();
        if (trackFilter.isNotEmpty() && trackFilter != trackId)
            continue;

        juce::String trackType = trackTree.getProperty(ProjectState::PROP_TYPE, "audio").toString();
        auto clipsNode = trackTree.getChildWithName(ProjectState::ID_CLIPS);
        if (!clipsNode.isValid())
            continue;

        juce::var clipsVar;
        auto* clipsArray = clipsVar.getArray();

        for (const auto& clip : clipsNode)
        {
            if (!clip.hasType(ProjectState::ID_CLIP))
                continue;

            const juce::String clipId = clip.getProperty(ProjectState::PROP_ID).toString();
            if (clipFilter.isNotEmpty() && clipFilter != clipId)
                continue;

            const juce::String clipType = clip.getProperty(ProjectState::PROP_TYPE).toString();
            if (clipType != "midi")
                continue;

            auto notes = projectState.getMidiNotesForClip(clipId);

            juce::var notesVar;
            auto* notesArray = notesVar.getArray();
            for (const auto& note : notes)
            {
                auto* noteObj = new juce::DynamicObject();
                noteObj->setProperty("id", note.id);
                noteObj->setProperty("pitch", note.pitch);
                noteObj->setProperty("startBeats", note.startBeats);
                noteObj->setProperty("lengthBeats", note.lengthBeats);
                noteObj->setProperty("velocity", note.velocity);
                noteObj->setProperty("muted", note.muted);
                notesArray->add(juce::var(noteObj));
            }

            auto* clipObj = new juce::DynamicObject();
            clipObj->setProperty("id", clipId);
            clipObj->setProperty("trackId", trackId);
            clipObj->setProperty("startBeats", clip.getProperty(ProjectState::PROP_START_BEATS));
            clipObj->setProperty("lengthBeats", clip.getProperty(ProjectState::PROP_LENGTH_BEATS));
            clipObj->setProperty("laneIndex", clip.getProperty(ProjectState::PROP_LANE_INDEX));
            clipObj->setProperty("notes", notesVar);
            clipObj->setProperty("noteCount", notes.size());

            clipsArray->add(juce::var(clipObj));
        }

        if (clipsArray->isEmpty())
            continue;

        auto* trackObj = new juce::DynamicObject();
        trackObj->setProperty("id", trackId);
        trackObj->setProperty("type", trackType);
        trackObj->setProperty("clips", clipsVar);
        trackObj->setProperty("clipCount", clipsArray->size());

        tracksArray->add(juce::var(trackObj));
    }

    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("tracks", tracksVar);
    resultObj->setProperty("trackCount", tracksVar.getArray()->size());

    return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::setClipNotes(const juce::var& params)
{
    if (!params.hasProperty("trackId")) return createErrorResponse("Missing 'trackId'");
    if (!params.hasProperty("clipId")) return createErrorResponse("Missing 'clipId'");
    if (!params.hasProperty("notes")) return createErrorResponse("Missing 'notes'");

    const juce::String trackId = params["trackId"].toString();
    const juce::String clipId = params["clipId"].toString();

    if (!params["notes"].isArray())
        return createErrorResponse("'notes' must be an array");

    auto clipTree = projectState.getClip(trackId, clipId);
    if (!clipTree.isValid())
        return createErrorResponse("Clip not found: " + clipId);

    juce::String clipType = clipTree.getProperty(ProjectState::PROP_TYPE, "audio").toString();
    if (clipType != "midi")
        return createErrorResponse("Clip is not a MIDI clip: " + clipId);

    auto existingNotes = projectState.getMidiNotesForClip(clipId);
    for (const auto& note : existingNotes)
        projectState.removeMidiNote(clipId, note.id, "Wingman: set_clip_notes (clear)");

    int addedCount = 0;
    auto* notesArray = params["notes"].getArray();
    for (const auto& noteVar : *notesArray)
    {
        if (!noteVar.isObject())
            continue;

        ProjectState::MidiNoteSpec spec;
        spec.id = noteVar.hasProperty("id") ? noteVar["id"].toString() : juce::String();
        spec.pitch = noteVar.hasProperty("pitch") ? static_cast<int>(noteVar["pitch"]) : 60;
        spec.startBeats = noteVar.hasProperty("startBeats") ? static_cast<double>(noteVar["startBeats"]) : 0.0;
        spec.lengthBeats = noteVar.hasProperty("lengthBeats") ? static_cast<double>(noteVar["lengthBeats"]) : 1.0;
        spec.velocity = noteVar.hasProperty("velocity") ? static_cast<int>(noteVar["velocity"]) : 100;
        spec.muted = noteVar.hasProperty("muted") ? static_cast<bool>(noteVar["muted"]) : false;

        projectState.addMidiNote(clipId, spec, "Wingman: set_clip_notes (add)");
        ++addedCount;
    }

    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("trackId", trackId);
    resultObj->setProperty("clipId", clipId);
    resultObj->setProperty("cleared", existingNotes.size());
    resultObj->setProperty("added", addedCount);

    return createSuccessResponse(juce::var(resultObj));
}

//==============================================================================
// Preset Management Commands (NEW - for Grok AI)
//==============================================================================

juce::var CommandAPI::listPresets(const juce::var& params)
{
    // Validate params
    if (!params.hasProperty("instrumentId"))
        return createErrorResponse("Missing 'instrumentId' parameter");
    
    juce::String instrumentId = params["instrumentId"].toString();
    
    // Get InstrumentRegistry
    auto& registry = InstrumentRegistry::getInstance();
    
    // Check if instrument exists
    InstrumentMetadata metadata;
    if (!registry.getMetadata(instrumentId, metadata))
        return createErrorResponse("Instrument not found: " + instrumentId);
    
    // Get presets for this instrument
    auto presets = registry.getPresetsForInstrument(instrumentId);
    
    // Build presets array
    juce::var presetsArray;
    auto* presetsArrayPtr = presetsArray.getArray();
    
    for (const auto& preset : presets)
    {
        auto* presetObj = new juce::DynamicObject();
        presetObj->setProperty("name", preset.name);
        presetObj->setProperty("category", preset.category);
        presetObj->setProperty("description", preset.description);
        presetObj->setProperty("author", preset.author);
        presetObj->setProperty("tags", preset.tags);
        
        presetsArrayPtr->add(juce::var(presetObj));
    }
    
    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("instrumentId", instrumentId);
    resultObj->setProperty("presets", presetsArray);
    resultObj->setProperty("count", (int)presets.size());
    
    DBG("CommandAPI: Listed " + juce::String(presets.size()) + " presets for " + instrumentId);
    
    return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::loadPreset(const juce::var& params)
{
    // Validate params
    if (!params.hasProperty("trackId"))
        return createErrorResponse("Missing 'trackId' parameter");
    if (!params.hasProperty("presetName"))
        return createErrorResponse("Missing 'presetName' parameter");
    
    juce::String trackId = params["trackId"].toString();
    juce::String presetName = params["presetName"].toString();
    
    // Find track
    Track* track = findTrackById(trackId);
    if (track == nullptr)
        return createErrorResponse("Track not found: " + trackId);
    
    // Get instrument from track
    auto* instrument = track->getInstrument();
    if (instrument == nullptr)
        return createErrorResponse("Track has no instrument loaded");
    
    // Get instrument ID
    juce::String instrumentId = instrument->getMetadata().instrumentId;
    
    // Get InstrumentRegistry
    auto& registry = InstrumentRegistry::getInstance();
    
    // Find preset
    InstrumentPreset preset;
    if (!registry.getPreset(instrumentId, presetName, preset))
        return createErrorResponse("Preset not found: " + presetName + " for instrument " + instrumentId);
    
    // Convert to ZenithInstrumentPreset
    ZenithInstrumentPreset zenithPreset;
    zenithPreset.name = preset.name.toStdString();
    zenithPreset.category = preset.category.toStdString();
    zenithPreset.description = preset.description.toStdString();
    zenithPreset.author = preset.author.toStdString();
    
    if (preset.parameters.isObject()) {
        auto paramsObj = preset.parameters.getDynamicObject();
        for (auto& prop : paramsObj->getProperties()) {
            zenithPreset.setParameter(prop.name.toString().toStdString(), (float)prop.value);
        }
    }

    // Load preset into instrument
    instrument->applyPreset(zenithPreset);
    
    DBG("CommandAPI: Loaded preset '" + presetName + "' on track " + trackId);
    
    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("trackId", trackId);
    resultObj->setProperty("instrumentId", instrumentId);
    resultObj->setProperty("presetName", presetName);
    resultObj->setProperty("loaded", true);
    
    return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::savePreset(const juce::var& params)
{
    // Validate params
    if (!params.hasProperty("trackId"))
        return createErrorResponse("Missing 'trackId' parameter");
    if (!params.hasProperty("presetName"))
        return createErrorResponse("Missing 'presetName' parameter");
    
    juce::String trackId = params["trackId"].toString();
    juce::String presetName = params["presetName"].toString();
    juce::String category = params.hasProperty("category") ? params["category"].toString() : "User";
    juce::String description = params.hasProperty("description") ? params["description"].toString() : "";
    
    // Find track
    Track* track = findTrackById(trackId);
    if (track == nullptr)
        return createErrorResponse("Track not found: " + trackId);
    
    // Get instrument from track
    auto* instrument = track->getInstrument();
    if (instrument == nullptr)
        return createErrorResponse("Track has no instrument loaded");
    
    // Get current instrument state as preset
    InstrumentPreset preset;
    preset.name = presetName;
    preset.category = category;
    preset.description = description;
    preset.author = "Grok AI";
    // Build parameters object
    auto* paramsObj = new juce::DynamicObject();
    const auto& metadata = instrument->getMetadata();
    for (const auto& param : metadata.parameters) {
        paramsObj->setProperty(param.id, instrument->getParameter(param.id));
    }
    preset.parameters = juce::var(paramsObj);
    
    // Get instrument ID
    juce::String instrumentId = metadata.instrumentId;
    
    // Save to registry
    auto& registry = InstrumentRegistry::getInstance();
    registry.addPreset(instrumentId, preset);
    
    DBG("CommandAPI: Saved preset '" + presetName + "' from track " + trackId);
    
    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("trackId", trackId);
    resultObj->setProperty("instrumentId", instrumentId);
    resultObj->setProperty("presetName", presetName);
    resultObj->setProperty("saved", true);
    
    return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::createPreset(const juce::var& params)
{
    // Validate params
    if (!params.hasProperty("instrumentId"))
        return createErrorResponse("Missing 'instrumentId' parameter");
    if (!params.hasProperty("presetName"))
        return createErrorResponse("Missing 'presetName' parameter");
    if (!params.hasProperty("parameters"))
        return createErrorResponse("Missing 'parameters' object");
    
    juce::String instrumentId = params["instrumentId"].toString();
    juce::String presetName = params["presetName"].toString();
    juce::var parameters = params["parameters"];
    juce::String category = params.hasProperty("category") ? params["category"].toString() : "AI Generated";
    juce::String description = params.hasProperty("description") ? params["description"].toString() : "Generated by Grok AI";
    
    // Validate parameters is an object
    if (!parameters.isObject())
        return createErrorResponse("'parameters' must be a JSON object");
    
    // Get InstrumentRegistry
    auto& registry = InstrumentRegistry::getInstance();
    
    // Check if instrument exists
    InstrumentMetadata metadata;
    if (!registry.getMetadata(instrumentId, metadata))
        return createErrorResponse("Instrument not found: " + instrumentId);
    
    // Create preset
    InstrumentPreset preset;
    preset.name = presetName;
    preset.category = category;
    preset.description = description;
    preset.author = "Grok AI";
    preset.parameters = parameters;
    
    // Validate preset parameters (basic validation)
    // Validate preset parameters
    for (auto& prop : parameters.getDynamicObject()->getProperties())
    {
        if (prop.value.isDouble() || prop.value.isInt())
        {
            float val = static_cast<float>(prop.value);
            // Most params are normalized 0-1, but some (like detune) might not be.
            // For now, we just warn if values are extreme.
            if (std::abs(val) > 10000.0f)
            {
                return createErrorResponse("Parameter value out of reasonable range: " + prop.name.toString());
            }
        }
    }
    
    // Add to registry
    registry.addPreset(instrumentId, preset);
    
    DBG("CommandAPI: Created preset '" + presetName + "' for " + instrumentId);
    
    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("instrumentId", instrumentId);
    resultObj->setProperty("presetName", presetName);
    resultObj->setProperty("category", category);
    resultObj->setProperty("created", true);
    
    return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::deletePreset(const juce::var& params)
{
    // Validate params
    if (!params.hasProperty("instrumentId"))
        return createErrorResponse("Missing 'instrumentId' parameter");
    if (!params.hasProperty("presetName"))
        return createErrorResponse("Missing 'presetName' parameter");
    
    juce::String instrumentId = params["instrumentId"].toString();
    juce::String presetName = params["presetName"].toString();
    
    // Get InstrumentRegistry
    auto& registry = InstrumentRegistry::getInstance();
    
    // Delete preset
    bool deleted = registry.deletePreset(instrumentId, presetName);
    
    if (!deleted)
        return createErrorResponse("Preset not found or could not be deleted: " + presetName);
    
    DBG("CommandAPI: Deleted preset '" + presetName + "' from " + instrumentId);
    
    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("instrumentId", instrumentId);
    resultObj->setProperty("presetName", presetName);
    resultObj->setProperty("deleted", true);
    
    return createSuccessResponse(juce::var(resultObj));
}

//==============================================================================
// Instrument Parameter Control (NEW - for Grok AI)
//==============================================================================

juce::var CommandAPI::getInstrumentParameters(const juce::var& params)
{
    // Validate params
    if (!params.hasProperty("trackId"))
        return createErrorResponse("Missing 'trackId' parameter");
    
    juce::String trackId = params["trackId"].toString();
    
    // Find track
    Track* track = findTrackById(trackId);
    if (track == nullptr)
        return createErrorResponse("Track not found: " + trackId);
    
    // Get instrument from track
    auto* instrument = track->getInstrument();
    if (instrument == nullptr)
        return createErrorResponse("Track has no instrument loaded");
    
    // Get current parameters
    auto* paramsObj = new juce::DynamicObject();
    const auto& metadata = instrument->getMetadata();
    for (const auto& param : metadata.parameters) {
        paramsObj->setProperty(param.id, instrument->getParameter(param.id));
    }
    juce::var parameters = juce::var(paramsObj);
    
    // Get instrument metadata
    juce::String instrumentId = metadata.instrumentId;
    
    DBG("CommandAPI: Retrieved parameters for instrument on track " + trackId);
    
    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("trackId", trackId);
    resultObj->setProperty("instrumentId", instrumentId);
    resultObj->setProperty("parameters", parameters);
    
    return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::setInstrumentParameter(const juce::var& params)
{
    // Validate params
    if (!params.hasProperty("trackId"))
        return createErrorResponse("Missing 'trackId' parameter");
    if (!params.hasProperty("parameterName"))
        return createErrorResponse("Missing 'parameterName' parameter");
    if (!params.hasProperty("value"))
        return createErrorResponse("Missing 'value' parameter");
    
    juce::String trackId = params["trackId"].toString();
    juce::String paramName = params["parameterName"].toString();
    juce::var value = params["value"];
    
    // Find track
    Track* track = findTrackById(trackId);
    if (track == nullptr)
        return createErrorResponse("Track not found: " + trackId);
    
    // Get instrument from track
    auto* instrument = track->getInstrument();
    if (instrument == nullptr)
        return createErrorResponse("Track has no instrument loaded");
    
    // Set parameter
    bool success = instrument->setParameter(paramName, value);
    
    if (!success)
        return createErrorResponse("Failed to set parameter: " + paramName + " (parameter may not exist)");
    
    DBG("CommandAPI: Set parameter '" + paramName + "' on track " + trackId);
    
    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("trackId", trackId);
    resultObj->setProperty("parameterName", paramName);
    resultObj->setProperty("value", value);
    resultObj->setProperty("set", true);
    
    return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::getInstrumentParameterSchema(const juce::var& params)
{
    // Validate params
    if (!params.hasProperty("instrumentId"))
        return createErrorResponse("Missing 'instrumentId' parameter");
    
    juce::String instrumentId = params["instrumentId"].toString();
    
    // Get InstrumentRegistry
    auto& registry = InstrumentRegistry::getInstance();
    
    // Check if instrument exists
    InstrumentMetadata metadata;
    if (!registry.getMetadata(instrumentId, metadata))
        return createErrorResponse("Instrument not found: " + instrumentId);
    
    // Get parameter schema
    juce::var schema = registry.getParameterSchema(instrumentId);
    
    DBG("CommandAPI: Retrieved parameter schema for " + instrumentId);
    
    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("instrumentId", instrumentId);
    resultObj->setProperty("schema", schema);
    
    return createSuccessResponse(juce::var(resultObj));
}

//==============================================================================
// AI Preset Generation (NEW - for Grok AI)
//==============================================================================

juce::var CommandAPI::generatePreset(const juce::var& params)
{
    // Validate params
    if (!params.hasProperty("instrumentId"))
        return createErrorResponse("Missing 'instrumentId' parameter");
    if (!params.hasProperty("description"))
        return createErrorResponse("Missing 'description' parameter");
    
    juce::String instrumentId = params["instrumentId"].toString();
    juce::String description = params["description"].toString();
    juce::String genre = params.hasProperty("genre") ? params["genre"].toString() : "";
    
    // Get InstrumentRegistry
    auto& registry = InstrumentRegistry::getInstance();
    
    // Check if instrument exists
    InstrumentMetadata metadata;
    if (!registry.getMetadata(instrumentId, metadata))
        return createErrorResponse("Instrument not found: " + instrumentId);
    
    // This function is called BY Grok after it generates parameters
    // The actual AI generation happens in GrokDAWController
    // This function validates and creates the preset
    
    // For now, return error - this should be called with generated parameters
    if (!params.hasProperty("generatedParameters"))
    {
        return createErrorResponse("This function should be called with 'generatedParameters' from Grok AI. "
                                  "Use GrokDAWController.generatePreset() instead for full AI generation.");
    }
    
    juce::var generatedParams = params["generatedParameters"];
    
    // Validate generated parameters
    // TODO: Add instrument-specific validation
    
    // Create preset name from description
    juce::String presetName = description.substring(0, 50); // Limit length
    if (presetName.isEmpty())
        presetName = "AI Generated Preset";
    
    // Create preset
    InstrumentPreset preset;
    preset.name = presetName;
    preset.category = "AI Generated";
    preset.description = description;
    preset.author = "Grok AI";
    preset.tags = genre.isNotEmpty() ? genre : "ai-generated";
    preset.parameters = generatedParams;
    
    // Add to registry
    registry.addPreset(instrumentId, preset);
    
    DBG("CommandAPI: Generated preset '" + presetName + "' for " + instrumentId);
    
    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("instrumentId", instrumentId);
    resultObj->setProperty("presetName", presetName);
    resultObj->setProperty("description", description);
    resultObj->setProperty("parameters", generatedParams);
    resultObj->setProperty("generated", true);
    
    return createSuccessResponse(juce::var(resultObj));
}

} // namespace zenith

