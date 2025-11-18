/**
 * @file CommandAPI.cpp
 * @brief Command API implementation
 */

#include "../include/CommandAPI.h"

//==============================================================================
CommandAPI::CommandAPI(ProjectState& ps, Engine& eng)
    : projectState(ps), engine(eng)
{
    DBG("CommandAPI: Constructor");

    // Register built-in commands

    // Automation commands
    registerCommand("add_automation_point", [this](const juce::var& p) { return cmd_addAutomationPoint(p); });
    registerCommand("clear_automation", [this](const juce::var& p) { return cmd_clearAutomation(p); });
    registerCommand("get_automation", [this](const juce::var& p) { return cmd_getAutomation(p); });

    // Track management
    registerCommand("add_track", [this](const juce::var& p) { return cmd_addTrack(p); });

    // Project state
    registerCommand("get_project_info", [this](const juce::var& p) { return cmd_getProjectInfo(p); });
    registerCommand("set_tempo", [this](const juce::var& p) { return cmd_setTempo(p); });

    // Transport commands
    registerCommand("play", [this](const juce::var& p) { return cmd_play(p); });
    registerCommand("stop", [this](const juce::var& p) { return cmd_stop(p); });
    registerCommand("record", [this](const juce::var& p) { return cmd_record(p); });
    registerCommand("loop", [this](const juce::var& p) { return cmd_loop(p); });
    registerCommand("set_position", [this](const juce::var& p) { return cmd_setPosition(p); });

    // Undo/Redo
    registerCommand("undo", [this](const juce::var& p) { return cmd_undo(p); });
    registerCommand("redo", [this](const juce::var& p) { return cmd_redo(p); });

    // Clip management
    registerCommand("create_clip", [this](const juce::var& p) { return cmd_createClip(p); });
    registerCommand("delete_clip", [this](const juce::var& p) { return cmd_deleteClip(p); });
    registerCommand("get_track_clips", [this](const juce::var& p) { return cmd_getTrackClips(p); });
    registerCommand("set_clip_audio_file", [this](const juce::var& p) { return cmd_setClipAudioFile(p); });
    registerCommand("create_audio_clip", [this](const juce::var& p) { return cmd_createAudioClip(p); });

    // MIDI note commands (stubbed)
    registerCommand("create_note", [this](const juce::var& p) { return cmd_createNote(p); });
    registerCommand("delete_note", [this](const juce::var& p) { return cmd_deleteNote(p); });
    registerCommand("get_clip_notes", [this](const juce::var& p) { return cmd_getClipNotes(p); });

    // Plugin commands (stubbed)
    registerCommand("scan_plugins", [this](const juce::var& p) { return cmd_scanPlugins(p); });
    registerCommand("get_plugins", [this](const juce::var& p) { return cmd_getPlugins(p); });
    registerCommand("add_track_plugin", [this](const juce::var& p) { return cmd_addTrackPlugin(p); });
    registerCommand("remove_track_plugin", [this](const juce::var& p) { return cmd_removeTrackPlugin(p); });
    registerCommand("set_track_plugin_bypassed", [this](const juce::var& p) { return cmd_setTrackPluginBypassed(p); });
    registerCommand("get_track_plugins", [this](const juce::var& p) { return cmd_getTrackPlugins(p); });

    // Export
    registerCommand("export_wav", [this](const juce::var& p) { return cmd_exportWav(p); });
    registerCommand("export_project", [this](const juce::var& p) { return cmd_exportProject(p); });
}

CommandAPI::~CommandAPI()
{
    DBG("CommandAPI: Destructor");
}

//==============================================================================
juce::String CommandAPI::executeCommand(const juce::String& commandJson)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    // Parse JSON
    juce::var parsedJson;
    auto parseResult = juce::JSON::parse(commandJson, parsedJson);

    if (parseResult.failed())
    {
        return createErrorResponse("Invalid JSON: " + parseResult.getErrorMessage());
    }

    if (!parsedJson.isObject())
    {
        return createErrorResponse("JSON root must be an object");
    }

    // Extract command name
    auto* obj = parsedJson.getDynamicObject();
    if (obj == nullptr)
    {
        return createErrorResponse("Invalid JSON object");
    }

    if (!obj->hasProperty("command"))
    {
        return createErrorResponse("Missing 'command' field");
    }

    juce::String commandName = obj->getProperty("command").toString();
    juce::var params = obj->getProperty("params");

    // Find and execute handler
    auto it = commandHandlers.find(commandName);
    if (it == commandHandlers.end())
    {
        return createErrorResponse("Unknown command: " + commandName);
    }

    try
    {
        juce::var result = it->second(params);
        return createResponse(result);
    }
    catch (const std::exception& e)
    {
        return createErrorResponse("Exception: " + juce::String(e.what()));
    }
    catch (...)
    {
        return createErrorResponse("Unknown exception occurred");
    }
}

void CommandAPI::registerCommand(const juce::String& commandName, CommandHandler handler)
{
    commandHandlers[commandName] = std::move(handler);
    DBG("CommandAPI: Registered command '" + commandName + "'");
}

//==============================================================================
// Built-in Command Handlers
//==============================================================================

juce::var CommandAPI::cmd_addAutomationPoint(const juce::var& params)
{
    juce::String error;

    // Validate required params
    if (!validateParam(params, "trackId", error)) throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "param", error)) throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "timeBeats", error)) throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "value", error)) throw std::runtime_error(error.toStdString());

    auto* obj = params.getDynamicObject();
    juce::String trackId = obj->getProperty("trackId").toString();
    juce::String param = obj->getProperty("param").toString();
    double timeBeats = obj->getProperty("timeBeats");
    double value = obj->getProperty("value");

    // Validate param name
    if (param != "volume" && param != "pan" && param != "mute")
    {
        throw std::runtime_error("Invalid param. Must be 'volume', 'pan', or 'mute'");
    }

    // Add point
    juce::String pointId = projectState.addAutomationPoint(trackId, param, timeBeats, value,
                                                            "Add automation point");

    if (pointId.isEmpty())
    {
        throw std::runtime_error("Failed to add automation point. Check trackId.");
    }

    // Return result
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("pointId", pointId);
    return juce::var(result.get());
}

juce::var CommandAPI::cmd_clearAutomation(const juce::var& params)
{
    juce::String error;

    // Validate required params
    if (!validateParam(params, "trackId", error)) throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "param", error)) throw std::runtime_error(error.toStdString());

    auto* obj = params.getDynamicObject();
    juce::String trackId = obj->getProperty("trackId").toString();
    juce::String param = obj->getProperty("param").toString();

    // Validate param name
    if (param != "volume" && param != "pan" && param != "mute")
    {
        throw std::runtime_error("Invalid param. Must be 'volume', 'pan', or 'mute'");
    }

    // Clear automation
    bool success = projectState.clearAutomation(trackId, param, "Clear automation");

    // Return result
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("success", success);
    return juce::var(result.get());
}

juce::var CommandAPI::cmd_getAutomation(const juce::var& params)
{
    juce::String error;

    // Validate required params
    if (!validateParam(params, "trackId", error)) throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "param", error)) throw std::runtime_error(error.toStdString());

    auto* obj = params.getDynamicObject();
    juce::String trackId = obj->getProperty("trackId").toString();
    juce::String param = obj->getProperty("param").toString();

    // Get envelope
    auto envelope = projectState.getAutomationEnvelope(trackId, param);

    juce::Array<juce::var> pointsArray;

    if (envelope.isValid())
    {
        for (int i = 0; i < envelope.getNumChildren(); ++i)
        {
            auto point = envelope.getChild(i);
            if (point.hasType(ProjectState::ID_POINT))
            {
                juce::DynamicObject::Ptr pointObj = new juce::DynamicObject();
                pointObj->setProperty("id", point[ProjectState::PROP_ID].toString());
                pointObj->setProperty("timeBeats", point[ProjectState::PROP_TIME_BEATS]);
                pointObj->setProperty("value", point[ProjectState::PROP_VALUE]);
                pointsArray.add(juce::var(pointObj.get()));
            }
        }
    }

    // Return result
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("points", pointsArray);
    return juce::var(result.get());
}

juce::var CommandAPI::cmd_addTrack(const juce::var& params)
{
    juce::String error;

    // Validate required params
    if (!validateParam(params, "name", error)) throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "type", error)) throw std::runtime_error(error.toStdString());

    auto* obj = params.getDynamicObject();
    juce::String name = obj->getProperty("name").toString();
    juce::String type = obj->getProperty("type").toString();

    // Validate type
    if (type != "audio" && type != "midi")
    {
        throw std::runtime_error("Invalid type. Must be 'audio' or 'midi'");
    }

    // Add track
    juce::String trackId = projectState.addTrack(name, type);

    // Return result
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("trackId", trackId);
    return juce::var(result.get());
}

juce::var CommandAPI::cmd_getProjectInfo(const juce::var& params)
{
    juce::ignoreUnused(params);

    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("name", projectState.getProjectName());
    result->setProperty("tempo", projectState.getTempo());
    result->setProperty("timeSignatureNumerator", projectState.getTimeSignatureNumerator());
    result->setProperty("timeSignatureDenominator", projectState.getTimeSignatureDenominator());
    result->setProperty("numTracks", projectState.getNumTracks());
    result->setProperty("isPlaying", engine.isPlaying());

    return juce::var(result.get());
}

juce::var CommandAPI::cmd_setTempo(const juce::var& params)
{
    juce::String error;

    // Validate required params
    if (!validateParam(params, "tempo", error)) throw std::runtime_error(error.toStdString());

    auto* obj = params.getDynamicObject();
    double tempo = obj->getProperty("tempo");

    // Set tempo
    projectState.setTempo(tempo);

    // Return result
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("success", true);
    return juce::var(result.get());
}

juce::var CommandAPI::cmd_exportWav(const juce::var& params)
{
    juce::String error;

    // Validate required params
    if (!validateParam(params, "outputPath", error)) throw std::runtime_error(error.toStdString());

    auto* obj = params.getDynamicObject();
    juce::String outputPath = obj->getProperty("outputPath").toString();

    // Optional params
    double durationSeconds = obj->getProperty("durationSeconds", 10.0);
    double sampleRate = obj->getProperty("sampleRate", 0.0);  // 0 = use current engine rate

    // Export to WAV using unified render path
    bool success = engine.exportProjectToWav(outputPath, durationSeconds, sampleRate);

    if (!success)
    {
        throw std::runtime_error("Failed to export WAV file");
    }

    // Return result
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("success", true);
    result->setProperty("outputPath", outputPath);
    result->setProperty("durationSeconds", durationSeconds);
    return juce::var(result.get());
}

//==============================================================================
// Helper Methods
//==============================================================================

juce::String CommandAPI::createResponse(const juce::var& data) const
{
    juce::DynamicObject::Ptr response = new juce::DynamicObject();
    response->setProperty("status", "ok");
    response->setProperty("data", data);
    return juce::JSON::toString(juce::var(response.get()), true);
}

juce::String CommandAPI::createErrorResponse(const juce::String& errorMessage) const
{
    juce::DynamicObject::Ptr response = new juce::DynamicObject();
    response->setProperty("status", "error");
    response->setProperty("error", errorMessage);
    return juce::JSON::toString(juce::var(response.get()), true);
}

bool CommandAPI::validateParam(const juce::var& params, const juce::String& paramName, juce::String& errorOut) const
{
    if (!params.isObject())
    {
        errorOut = "Params must be an object";
        return false;
    }

    auto* obj = params.getDynamicObject();
    if (obj == nullptr || !obj->hasProperty(paramName))
    {
        errorOut = "Missing required parameter: " + paramName;
        return false;
    }

    return true;
}

//==============================================================================
// Transport Commands
//==============================================================================

juce::var CommandAPI::cmd_play(const juce::var& params)
{
    juce::ignoreUnused(params);

    engine.play();

    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("success", true);
    return juce::var(result.get());
}

juce::var CommandAPI::cmd_stop(const juce::var& params)
{
    juce::ignoreUnused(params);

    engine.stop();

    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("success", true);
    return juce::var(result.get());
}

juce::var CommandAPI::cmd_record(const juce::var& params)
{
    juce::ignoreUnused(params);

    // Recording state not yet implemented in Engine
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("success", false);
    result->setProperty("message", "Recording not yet implemented in Engine");
    result->setProperty("recording", false);
    return juce::var(result.get());
}

juce::var CommandAPI::cmd_loop(const juce::var& params)
{
    juce::ignoreUnused(params);

    // Loop state not yet implemented in Engine
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("success", false);
    result->setProperty("message", "Loop not yet implemented in Engine");
    result->setProperty("looping", false);
    return juce::var(result.get());
}

juce::var CommandAPI::cmd_setPosition(const juce::var& params)
{
    juce::ignoreUnused(params);

    // Playback position setting not yet implemented in Engine
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("success", false);
    result->setProperty("message", "Set position not yet implemented in Engine");
    return juce::var(result.get());
}

//==============================================================================
// Undo/Redo Commands
//==============================================================================

juce::var CommandAPI::cmd_undo(const juce::var& params)
{
    juce::ignoreUnused(params);

    projectState.undo();

    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("success", true);
    result->setProperty("canUndo", projectState.canUndo());
    result->setProperty("canRedo", projectState.canRedo());
    return juce::var(result.get());
}

juce::var CommandAPI::cmd_redo(const juce::var& params)
{
    juce::ignoreUnused(params);

    projectState.redo();

    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("success", true);
    result->setProperty("canUndo", projectState.canUndo());
    result->setProperty("canRedo", projectState.canRedo());
    return juce::var(result.get());
}

//==============================================================================
// Clip Management Commands
//==============================================================================

juce::var CommandAPI::cmd_createClip(const juce::var& params)
{
    juce::String error;

    // Validate required params
    if (!validateParam(params, "trackId", error)) throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "startBeats", error)) throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "lengthBeats", error)) throw std::runtime_error(error.toStdString());

    auto* obj = params.getDynamicObject();
    juce::String trackId = obj->getProperty("trackId").toString();
    double startBeats = obj->getProperty("startBeats");
    double lengthBeats = obj->getProperty("lengthBeats");

    // Create clip
    juce::String clipId = projectState.addClip(trackId, startBeats, lengthBeats, "Create clip");

    if (clipId.isEmpty())
    {
        throw std::runtime_error("Failed to create clip. Check trackId.");
    }

    // Return result
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("clipId", clipId);
    return juce::var(result.get());
}

juce::var CommandAPI::cmd_deleteClip(const juce::var& params)
{
    juce::String error;

    // Validate required params
    if (!validateParam(params, "trackId", error)) throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "clipId", error)) throw std::runtime_error(error.toStdString());

    auto* obj = params.getDynamicObject();
    juce::String trackId = obj->getProperty("trackId").toString();
    juce::String clipId = obj->getProperty("clipId").toString();

    // Delete clip
    bool success = projectState.removeClip(trackId, clipId, "Delete clip");

    // Return result
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("success", success);
    return juce::var(result.get());
}

juce::var CommandAPI::cmd_getTrackClips(const juce::var& params)
{
    juce::String error;

    // Validate required params
    if (!validateParam(params, "trackId", error)) throw std::runtime_error(error.toStdString());

    auto* obj = params.getDynamicObject();
    juce::String trackId = obj->getProperty("trackId").toString();

    // Get track
    auto track = projectState.getTrack(trackId);

    juce::Array<juce::var> clipsArray;

    if (track.isValid())
    {
        // Find clips child node
        auto clipsNode = track.getChildWithName(ProjectState::ID_CLIPS);
        if (clipsNode.isValid())
        {
            for (int i = 0; i < clipsNode.getNumChildren(); ++i)
            {
                auto clip = clipsNode.getChild(i);
                if (clip.hasType(ProjectState::ID_CLIP))
                {
                    juce::DynamicObject::Ptr clipObj = new juce::DynamicObject();
                    clipObj->setProperty("id", clip[ProjectState::PROP_ID].toString());
                    clipObj->setProperty("startBeats", clip[ProjectState::PROP_START]);
                    clipObj->setProperty("lengthBeats", clip[ProjectState::PROP_LENGTH]);

                    // Include audio file path if present
                    if (clip.hasProperty(ProjectState::PROP_AUDIO_FILE))
                    {
                        clipObj->setProperty("audioFile", clip[ProjectState::PROP_AUDIO_FILE].toString());
                    }

                    clipsArray.add(juce::var(clipObj.get()));
                }
            }
        }
    }

    // Return result
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("clips", clipsArray);
    return juce::var(result.get());
}

juce::var CommandAPI::cmd_setClipAudioFile(const juce::var& params)
{
    juce::String error;

    // Validate required params
    if (!validateParam(params, "trackId", error)) throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "clipId", error)) throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "path", error)) throw std::runtime_error(error.toStdString());

    auto* obj = params.getDynamicObject();
    juce::String trackId = obj->getProperty("trackId").toString();
    juce::String clipId = obj->getProperty("clipId").toString();
    juce::String path = obj->getProperty("path").toString();

    // Set audio file
    juce::File audioFile(path);
    bool success = projectState.setClipAudioFile(trackId, clipId, audioFile, "Set clip audio file");

    // Return result
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("success", success);
    return juce::var(result.get());
}

juce::var CommandAPI::cmd_createAudioClip(const juce::var& params)
{
    juce::String error;

    // Validate required params
    if (!validateParam(params, "trackId", error)) throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "startBeats", error)) throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "lengthBeats", error)) throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "path", error)) throw std::runtime_error(error.toStdString());

    auto* obj = params.getDynamicObject();
    juce::String trackId = obj->getProperty("trackId").toString();
    double startBeats = obj->getProperty("startBeats");
    double lengthBeats = obj->getProperty("lengthBeats");
    juce::String path = obj->getProperty("path").toString();

    // Create clip
    juce::String clipId = projectState.addClip(trackId, startBeats, lengthBeats, "Create audio clip");

    if (clipId.isEmpty())
    {
        throw std::runtime_error("Failed to create clip. Check trackId.");
    }

    // Set audio file
    juce::File audioFile(path);
    bool fileSet = projectState.setClipAudioFile(trackId, clipId, audioFile, "Set clip audio file");

    if (!fileSet)
    {
        // Clip was created but audio file wasn't set - still return the clipId
        DBG("CommandAPI: Created clip but failed to set audio file");
    }

    // Return result
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("clipId", clipId);
    result->setProperty("success", true);
    return juce::var(result.get());
}

//==============================================================================
// MIDI Note Commands (Stubbed)
//==============================================================================

juce::var CommandAPI::cmd_createNote(const juce::var& params)
{
    juce::ignoreUnused(params);

    // MIDI note API not yet implemented in ProjectState
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("success", false);
    result->setProperty("message", "MIDI note API not yet implemented in ProjectState");
    return juce::var(result.get());
}

juce::var CommandAPI::cmd_deleteNote(const juce::var& params)
{
    juce::ignoreUnused(params);

    // MIDI note API not yet implemented in ProjectState
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("success", false);
    result->setProperty("message", "MIDI note API not yet implemented in ProjectState");
    return juce::var(result.get());
}

juce::var CommandAPI::cmd_getClipNotes(const juce::var& params)
{
    juce::ignoreUnused(params);

    // MIDI note API not yet implemented in ProjectState
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("notes", juce::Array<juce::var>());
    result->setProperty("message", "MIDI note API not yet implemented in ProjectState");
    return juce::var(result.get());
}

//==============================================================================
// Plugin Commands (Stubbed)
//==============================================================================

juce::var CommandAPI::cmd_scanPlugins(const juce::var& params)
{
    juce::ignoreUnused(params);

    // Plugin system not yet implemented
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("count", 0);
    result->setProperty("message", "Plugin system not yet implemented");
    return juce::var(result.get());
}

juce::var CommandAPI::cmd_getPlugins(const juce::var& params)
{
    juce::ignoreUnused(params);

    // Plugin system not yet implemented
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("plugins", juce::Array<juce::var>());
    result->setProperty("message", "Plugin system not yet implemented");
    return juce::var(result.get());
}

juce::var CommandAPI::cmd_addTrackPlugin(const juce::var& params)
{
    juce::ignoreUnused(params);

    // Plugin system not yet implemented
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("success", false);
    result->setProperty("message", "Plugin system not yet implemented");
    return juce::var(result.get());
}

juce::var CommandAPI::cmd_removeTrackPlugin(const juce::var& params)
{
    juce::ignoreUnused(params);

    // Plugin system not yet implemented
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("success", false);
    result->setProperty("message", "Plugin system not yet implemented");
    return juce::var(result.get());
}

juce::var CommandAPI::cmd_setTrackPluginBypassed(const juce::var& params)
{
    juce::ignoreUnused(params);

    // Plugin system not yet implemented
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("success", false);
    result->setProperty("message", "Plugin system not yet implemented");
    return juce::var(result.get());
}

juce::var CommandAPI::cmd_getTrackPlugins(const juce::var& params)
{
    juce::ignoreUnused(params);

    // Plugin system not yet implemented
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("plugins", juce::Array<juce::var>());
    result->setProperty("message", "Plugin system not yet implemented");
    return juce::var(result.get());
}

//==============================================================================
// Export Commands
//==============================================================================

juce::var CommandAPI::cmd_exportProject(const juce::var& params)
{
    juce::String error;

    // Validate required params
    if (!validateParam(params, "path", error)) throw std::runtime_error(error.toStdString());

    auto* obj = params.getDynamicObject();
    juce::String outputPath = obj->getProperty("path").toString();

    // Optional params - default to full project length
    // For now, use a fixed duration since we don't have project length calculation yet
    double durationSeconds = obj->getProperty("durationSeconds", 10.0);
    double sampleRate = obj->getProperty("sampleRate", 0.0);  // 0 = use current engine rate

    // Export using Engine's exportProjectToWav
    bool success = engine.exportProjectToWav(outputPath, durationSeconds, sampleRate);

    if (!success)
    {
        throw std::runtime_error("Failed to export project to WAV");
    }

    // Return result
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("success", true);
    result->setProperty("outputPath", outputPath);
    result->setProperty("durationSeconds", durationSeconds);
    return juce::var(result.get());
}
