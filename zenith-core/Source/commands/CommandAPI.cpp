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
#include "../export/NFTMintingService.h"

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
    
    // Preset management commands (NEW)
    else if (command == "list_presets")
        return listPresets(params);
    else if (command == "load_preset")
        return loadPreset(params);
    else if (command == "save_preset")
        return savePreset(params);
    else if (command == "create_preset")
        return createPreset(params);
    else if (command == "delete_preset")
        return deletePreset(params);
    
    // Instrument parameter control (NEW)
    else if (command == "get_instrument_parameters")
        return getInstrumentParameters(params);
    else if (command == "set_instrument_parameter")
        return setInstrumentParameter(params);
    else if (command == "get_instrument_parameter_schema")
        return getInstrumentParameterSchema(params);
    
    // AI preset generation (NEW)
    else if (command == "generate_preset")
        return generatePreset(params);

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
        return deleteMarker(params);
    else if (command == "goto_marker")
        return gotoMarker(params);
        return getMidiData(params);
    else if (command == "set_clip_notes")
        return setClipNotes(params);

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
    juce::ignoreUnused(params);
    // Automation not yet implemented in ProjectState
    return createErrorResponse("Automation not yet implemented");
}

juce::var CommandAPI::clearAutomation(const juce::var& params)
{
    juce::ignoreUnused(params);
    // Automation not yet implemented in ProjectState
    return createErrorResponse("Automation not yet implemented");
}

juce::var CommandAPI::getAutomation(const juce::var& params)
{
    juce::ignoreUnused(params);
    // Automation not yet implemented in ProjectState
    return createErrorResponse("Automation not yet implemented");
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
    resultObj->setProperty("count", presets.size());
    
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
    juce::String instrumentId = instrument->getMetadata().id;
    
    // Get InstrumentRegistry
    auto& registry = InstrumentRegistry::getInstance();
    
    // Find preset
    InstrumentPreset preset;
    if (!registry.getPreset(instrumentId, presetName, preset))
        return createErrorResponse("Preset not found: " + presetName + " for instrument " + instrumentId);
    
    // Load preset into instrument
    instrument->loadPreset(preset);
    
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
    preset.parameters = instrument->getCurrentParameters();
    
    // Get instrument ID
    juce::String instrumentId = instrument->getMetadata().id;
    
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
    juce::var parameters = instrument->getCurrentParameters();
    
    // Get instrument metadata
    juce::String instrumentId = instrument->getMetadata().id;
    
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

