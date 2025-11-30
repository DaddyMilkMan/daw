/**
 * @file CommandAPI.cpp
 * @brief Command API implementation
 */

#include "../include/CommandAPI.h"
#include "instruments/InstrumentRegistry.h"
#include "instruments/InstrumentPreset.h"
#include "engine/Track.h"

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

    // MIDI note commands (merged from both branches)
    registerCommand("add_note", [this](const juce::var& p) { return cmd_addNote(p); });
    registerCommand("move_note", [this](const juce::var& p) { return cmd_moveNote(p); });
    registerCommand("create_note", [this](const juce::var& p) { return cmd_createNote(p); });
    registerCommand("delete_note", [this](const juce::var& p) { return cmd_deleteNote(p); });
    registerCommand("get_notes", [this](const juce::var& p) { return cmd_getNotes(p); });
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

    // Instrument commands
    registerCommand("list_instruments", [this](const juce::var& p) { return cmd_listInstruments(p); });
    registerCommand("list_presets", [this](const juce::var& p) { return cmd_listPresets(p); });
    registerCommand("load_preset", [this](const juce::var& p) { return cmd_loadPreset(p); });
    registerCommand("save_preset", [this](const juce::var& p) { return cmd_savePreset(p); });
    registerCommand("get_instrument_parameters", [this](const juce::var& p) { return cmd_getInstrumentParameters(p); });
    registerCommand("set_instrument_parameters", [this](const juce::var& p) { return cmd_setInstrumentParameters(p); });
    registerCommand("set_instrument_on_track", [this](const juce::var& p) { return cmd_setInstrumentOnTrack(p); });
    registerCommand("set_instrument_param", [this](const juce::var& p) { return cmd_setInstrumentParam(p); });
    registerCommand("get_instrument_param", [this](const juce::var& p) { return cmd_getInstrumentParam(p); });
    registerCommand("randomize_instrument_params", [this](const juce::var& p) { return cmd_randomizeInstrumentParams(p); });
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
    juce::ignoreUnused(params);
    // Automation not yet implemented in ProjectState
    throw std::runtime_error("Automation not yet implemented");
}

juce::var CommandAPI::cmd_clearAutomation(const juce::var& params)
{
    juce::ignoreUnused(params);
    // Automation not yet implemented in ProjectState
    throw std::runtime_error("Automation not yet implemented");
}

juce::var CommandAPI::cmd_getAutomation(const juce::var& params)
{
    juce::ignoreUnused(params);
    // Automation not yet implemented in ProjectState
    throw std::runtime_error("Automation not yet implemented");
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
    double durationSeconds = obj->hasProperty("durationSeconds") ? (double)obj->getProperty("durationSeconds") : 10.0;
    double sampleRate = obj->hasProperty("sampleRate") ? (double)obj->getProperty("sampleRate") : 0.0;  // 0 = use current engine rate
    int bitDepth = 24;  // Default bit depth

    // Export to WAV using unified render path
    bool success = engine.exportProjectToWav(juce::File(outputPath), sampleRate, bitDepth, durationSeconds);

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

juce::var CommandAPI::cmd_addNote(const juce::var& params)
{
    juce::String error;

    // Validate required params
    if (!validateParam(params, "clipId", error)) throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "startBeats", error)) throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "lengthBeats", error)) throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "pitch", error)) throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "velocity", error)) throw std::runtime_error(error.toStdString());

    auto* obj = params.getDynamicObject();
    juce::String clipId = obj->getProperty("clipId").toString();
    double startBeats = obj->getProperty("startBeats");
    double lengthBeats = obj->getProperty("lengthBeats");
    int pitch = obj->getProperty("pitch");
    int velocity = obj->getProperty("velocity");

    // Add note
    juce::String noteId = projectState.addNote(clipId, startBeats, lengthBeats, pitch, velocity, "Add Note");

    // Return result
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("noteId", noteId);
    return juce::var(result.get());
}

juce::var CommandAPI::cmd_moveNote(const juce::var& params)
{
    juce::String error;

    // Validate required params
    if (!validateParam(params, "clipId", error)) throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "noteId", error)) throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "startBeats", error)) throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "lengthBeats", error)) throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "pitch", error)) throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "velocity", error)) throw std::runtime_error(error.toStdString());

    auto* obj = params.getDynamicObject();
    juce::String clipId = obj->getProperty("clipId").toString();
    juce::String noteId = obj->getProperty("noteId").toString();
    double startBeats = obj->getProperty("startBeats");
    double lengthBeats = obj->getProperty("lengthBeats");
    int pitch = obj->getProperty("pitch");
    int velocity = obj->getProperty("velocity");

    // Move note
    bool success = projectState.moveNote(clipId, noteId, startBeats, lengthBeats, pitch, velocity, "Move Note");

    // Return result
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("success", success);
    return juce::var(result.get());
}

juce::var CommandAPI::cmd_deleteNote(const juce::var& params)
{
    juce::String error;

    // Validate required params
    if (!validateParam(params, "clipId", error)) throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "noteId", error)) throw std::runtime_error(error.toStdString());

    auto* obj = params.getDynamicObject();
    juce::String clipId = obj->getProperty("clipId").toString();
    juce::String noteId = obj->getProperty("noteId").toString();

    // Delete note
    bool success = projectState.deleteNote(clipId, noteId, "Delete Note");

    // Return result
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("success", success);
    return juce::var(result.get());
}

juce::var CommandAPI::cmd_getNotes(const juce::var& params)
{
    juce::String error;

    // Validate required params
    if (!validateParam(params, "clipId", error)) throw std::runtime_error(error.toStdString());

    auto* obj = params.getDynamicObject();
    juce::String clipId = obj->getProperty("clipId").toString();

    // Get notes
    auto notesTree = projectState.getNotes(clipId);

    // Build notes array
    juce::Array<juce::var> notesArray;
    if (notesTree.isValid())
    {
        for (const auto& note : notesTree)
        {
            juce::DynamicObject::Ptr noteObj = new juce::DynamicObject();
            noteObj->setProperty("id", note.getProperty("id", "").toString());
            noteObj->setProperty("startBeats", note.getProperty("startBeats", 0.0));
            noteObj->setProperty("lengthBeats", note.getProperty("lengthBeats", 0.0));
            noteObj->setProperty("pitch", note.getProperty("pitch", 60));
            noteObj->setProperty("velocity", note.getProperty("velocity", 100));
            notesArray.add(juce::var(noteObj.get()));
        }
    }

    // Return result
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("notes", notesArray);
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
    double durationSeconds = obj->hasProperty("durationSeconds") ? (double)obj->getProperty("durationSeconds") : 10.0;
    double sampleRate = obj->hasProperty("sampleRate") ? (double)obj->getProperty("sampleRate") : 0.0;  // 0 = use current engine rate
    int bitDepth = 24;  // Default bit depth

    // Export using Engine's exportProjectToWav
    bool success = engine.exportProjectToWav(juce::File(outputPath), sampleRate, bitDepth, durationSeconds);

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

//==============================================================================
// Instrument Commands
//==============================================================================

juce::var CommandAPI::cmd_listInstruments(const juce::var& params)
{
    juce::ignoreUnused(params);

    // Get list from InstrumentRegistry
    auto& registry = zenith::InstrumentRegistry::getInstance();
    auto instruments = registry.getInstrumentList();

    // Return result
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("instruments", instruments);
    return juce::var(result.get());
}

juce::var CommandAPI::cmd_listPresets(const juce::var& params)
{
    juce::String error;

    // Validate required params
    if (!validateParam(params, "instrumentId", error))
        throw std::runtime_error(error.toStdString());

    auto* obj = params.getDynamicObject();
    juce::String instrumentId = obj->getProperty("instrumentId").toString();

    // Optional filters
    juce::String categoryFilter = obj->hasProperty("category") ? obj->getProperty("category").toString() : juce::String();
    juce::String tagFilter = obj->hasProperty("tag") ? obj->getProperty("tag").toString() : juce::String();

    // Get presets from preset manager
    zenith::ZenithPresetManager presetManager;
    auto presets = presetManager.getPresetsForInstrument(instrumentId.toStdString());

    // Build response array with filtering
    juce::Array<juce::var> presetsArray;
    for (const auto& preset : presets)
    {
        // Apply category filter if specified
        if (categoryFilter.isNotEmpty() && preset.author != categoryFilter.toStdString())
            continue;

        // Apply tag filter if specified
        if (tagFilter.isNotEmpty())
        {
            bool hasTag = false;
            for (const auto& tag : preset.tags)
            {
                if (tag == tagFilter.toStdString())
                {
                    hasTag = true;
                    break;
                }
            }
            if (!hasTag)
                continue;
        }

        // Build preset object
        juce::DynamicObject::Ptr presetObj = new juce::DynamicObject();
        presetObj->setProperty("id", juce::String(preset.id));
        presetObj->setProperty("name", juce::String(preset.name));
        presetObj->setProperty("category", juce::String(preset.author));  // Using author as category for now

        // Convert tags to array
        juce::Array<juce::var> tagsArray;
        for (const auto& tag : preset.tags)
            tagsArray.add(juce::String(tag));
        presetObj->setProperty("tags", tagsArray);

        presetsArray.add(juce::var(presetObj.get()));
    }

    // Return result
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("presets", presetsArray);
    return juce::var(result.get());
}

juce::var CommandAPI::cmd_loadPreset(const juce::var& params)
{
    juce::String error;

    // Validate required params
    if (!validateParam(params, "trackId", error))
        throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "presetId", error))
        throw std::runtime_error(error.toStdString());

    auto* obj = params.getDynamicObject();
    juce::String trackId = obj->getProperty("trackId").toString();
    juce::String presetId = obj->getProperty("presetId").toString();
    juce::String instrumentIdOpt = obj->hasProperty("instrumentId") ? obj->getProperty("instrumentId").toString() : juce::String("");

    // Find track
    int trackIndex = trackId.getTrailingIntValue();
    if (trackIndex < 0 || trackIndex >= engine.getNumTracks())
    {
        throw std::runtime_error("Track not found: " + trackId.toStdString());
    }

    auto* track = const_cast<zenith::Track*>(engine.tracks()[trackIndex].get());
    auto* instrument = track->getInstrument();

    // If no instrument on track and instrumentId provided, create one
    if (instrument == nullptr && instrumentIdOpt.isNotEmpty())
    {
        auto& registry = zenith::InstrumentRegistry::getInstance();
        auto newInstrument = registry.createInstrument(instrumentIdOpt);
        if (newInstrument == nullptr)
        {
            throw std::runtime_error("Unknown instrument: " + instrumentIdOpt.toStdString());
        }
        track->setInstrument(std::move(newInstrument));
        instrument = track->getInstrument();
    }

    if (instrument == nullptr)
    {
        throw std::runtime_error("Track has no instrument and no instrumentId provided");
    }

    // Load preset on instrument
    bool success = instrument->loadPreset(presetId);
    if (!success)
    {
        throw std::runtime_error("Failed to load preset: " + presetId.toStdString());
    }

    // Note: UndoManager automatically groups actions into transactions

    // Return result
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("success", true);
    return juce::var(result.get());
}

juce::var CommandAPI::cmd_savePreset(const juce::var& params)
{
    juce::String error;

    // Validate required params
    if (!validateParam(params, "trackId", error))
        throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "name", error))
        throw std::runtime_error(error.toStdString());

    auto* obj = params.getDynamicObject();
    juce::String trackId = obj->getProperty("trackId").toString();
    juce::String name = obj->getProperty("name").toString();
    juce::String category = obj->hasProperty("category") ? obj->getProperty("category").toString() : juce::String("User");

    // Find track
    int trackIndex = trackId.getTrailingIntValue();
    if (trackIndex < 0 || trackIndex >= engine.getNumTracks())
    {
        throw std::runtime_error("Track not found: " + trackId.toStdString());
    }

    auto* track = const_cast<zenith::Track*>(engine.tracks()[trackIndex].get());
    auto* instrument = track->getInstrument();

    if (instrument == nullptr)
    {
        throw std::runtime_error("Track has no instrument");
    }

    // Create preset from current instrument state
    zenith::ZenithInstrumentPreset preset(
        name.toStdString(),
        instrument->getMetadata().instrumentId.toStdString(),
        category.toStdString()
    );

    // Get parameter values from instrument
    const auto& metadata = instrument->getMetadata();
    for (const auto& param : metadata.parameters)
    {
        float value = instrument->getParameter(param.id);
        preset.setParameter(param.id.toStdString(), value);
    }

    // Get macro values
    for (const auto& macro : metadata.macros)
    {
        float value = instrument->getMacro(macro.id);
        preset.setMacro(macro.id.toStdString(), value);
    }

    // Parse tags if provided
    auto tagsVar = obj->getProperty("tags");
    if (tagsVar.isArray())
    {
        auto* tagsArray = tagsVar.getArray();
        for (int i = 0; i < tagsArray->size(); ++i)
        {
            preset.tags.push_back((*tagsArray)[i].toString().toStdString());
        }
    }

    // Save preset
    zenith::ZenithPresetManager presetManager;
    bool success = presetManager.saveUserPreset(preset);

    if (!success)
    {
        throw std::runtime_error("Failed to save preset");
    }

    // Return result
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("success", true);
    result->setProperty("presetId", juce::String(preset.id));
    return juce::var(result.get());
}

juce::var CommandAPI::cmd_getInstrumentParameters(const juce::var& params)
{
    juce::String error;

    // Validate required params
    if (!validateParam(params, "trackId", error))
        throw std::runtime_error(error.toStdString());

    auto* obj = params.getDynamicObject();
    juce::String trackId = obj->getProperty("trackId").toString();

    // Find track
    int trackIndex = trackId.getTrailingIntValue();
    if (trackIndex < 0 || trackIndex >= engine.getNumTracks())
    {
        throw std::runtime_error("Track not found: " + trackId.toStdString());
    }

    auto* track = const_cast<zenith::Track*>(engine.tracks()[trackIndex].get());
    auto* instrument = track->getInstrument();

    if (instrument == nullptr)
    {
        throw std::runtime_error("Track has no instrument");
    }

    // Get metadata and build parameter array
    const auto& metadata = instrument->getMetadata();
    juce::Array<juce::var> paramsArray;

    for (const auto& param : metadata.parameters)
    {
        juce::DynamicObject::Ptr paramObj = new juce::DynamicObject();
        paramObj->setProperty("id", param.id);
        paramObj->setProperty("name", param.name);
        paramObj->setProperty("min", param.minValue);
        paramObj->setProperty("max", param.maxValue);
        paramObj->setProperty("default", param.defaultValue);
        paramObj->setProperty("value", instrument->getParameter(param.id));

        // Add tags (empty for now, could be extended)
        juce::Array<juce::var> tagsArray;
        paramObj->setProperty("tags", tagsArray);

        paramsArray.add(juce::var(paramObj.get()));
    }

    // Return result
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("parameters", paramsArray);
    return juce::var(result.get());
}

juce::var CommandAPI::cmd_setInstrumentParameters(const juce::var& params)
{
    juce::String error;

    // Validate required params
    if (!validateParam(params, "trackId", error))
        throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "params", error))
        throw std::runtime_error(error.toStdString());

    auto* obj = params.getDynamicObject();
    juce::String trackId = obj->getProperty("trackId").toString();
    auto paramsObj = obj->getProperty("params");

    if (!paramsObj.isObject())
    {
        throw std::runtime_error("'params' must be an object");
    }

    // Find track
    int trackIndex = trackId.getTrailingIntValue();
    if (trackIndex < 0 || trackIndex >= engine.getNumTracks())
    {
        throw std::runtime_error("Track not found: " + trackId.toStdString());
    }

    auto* track = const_cast<zenith::Track*>(engine.tracks()[trackIndex].get());
    auto* instrument = track->getInstrument();

    if (instrument == nullptr)
    {
        throw std::runtime_error("Track has no instrument");
    }

    // Note: UndoManager automatically groups actions into transactions

    // Set each parameter
    juce::Array<juce::var> updatedParams;
    auto* paramsDynObj = paramsObj.getDynamicObject();

    if (paramsDynObj != nullptr)
    {
        const auto& metadata = instrument->getMetadata();
        auto propNames = paramsDynObj->getProperties();

        for (int i = 0; i < propNames.size(); ++i)
        {
            juce::String paramId = propNames.getName(i).toString();
            float value = (float)propNames.getValueAt(i);

            // Validate parameter exists
            const auto* paramMeta = metadata.findParameter(paramId);
            if (paramMeta == nullptr)
            {
                DBG("Warning: Unknown parameter '" + paramId + "', skipping");
                continue;
            }

            // Clamp value to valid range
            value = juce::jlimit(paramMeta->minValue, paramMeta->maxValue, value);

            // Set parameter
            bool success = instrument->setParameter(paramId, value);
            if (success)
            {
                updatedParams.add(paramId);
            }
        }
    }

    // Return result
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("success", true);
    result->setProperty("updated", updatedParams);
    return juce::var(result.get());
}

//==============================================================================
// New Instrument Commands
//==============================================================================

juce::var CommandAPI::cmd_setInstrumentOnTrack(const juce::var& params)
{
    juce::String error;

    // Validate required params
    if (!validateParam(params, "trackId", error))
        throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "instrumentId", error))
        throw std::runtime_error(error.toStdString());

    auto* obj = params.getDynamicObject();
    juce::String trackId = obj->getProperty("trackId").toString();
    juce::String instrumentId = obj->getProperty("instrumentId").toString();

    // Find track
    int trackIndex = trackId.getTrailingIntValue();
    if (trackIndex < 0 || trackIndex >= engine.getNumTracks())
    {
        throw std::runtime_error("Track not found: " + trackId.toStdString());
    }

    auto* track = const_cast<zenith::Track*>(engine.tracks()[trackIndex].get());

    // Create instrument using registry
    auto& registry = zenith::InstrumentRegistry::getInstance();
    auto newInstrument = registry.createInstrument(instrumentId);

    if (newInstrument == nullptr)
    {
        throw std::runtime_error("Unknown instrument: " + instrumentId.toStdString());
    }

    // Set instrument on track
    track->setInstrument(std::move(newInstrument));

    // Note: UndoManager automatically groups actions into transactions

    // Return result
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("success", true);
    result->setProperty("instrumentId", instrumentId);
    return juce::var(result.get());
}

juce::var CommandAPI::cmd_setInstrumentParam(const juce::var& params)
{
    juce::String error;

    // Validate required params
    if (!validateParam(params, "trackId", error))
        throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "paramId", error))
        throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "value", error))
        throw std::runtime_error(error.toStdString());

    auto* obj = params.getDynamicObject();
    juce::String trackId = obj->getProperty("trackId").toString();
    juce::String paramId = obj->getProperty("paramId").toString();
    float value = (float)obj->getProperty("value");

    // Find track
    int trackIndex = trackId.getTrailingIntValue();
    if (trackIndex < 0 || trackIndex >= engine.getNumTracks())
    {
        throw std::runtime_error("Track not found: " + trackId.toStdString());
    }

    auto* track = const_cast<zenith::Track*>(engine.tracks()[trackIndex].get());
    auto* instrument = track->getInstrument();

    if (instrument == nullptr)
    {
        throw std::runtime_error("Track has no instrument");
    }

    // Get metadata to validate parameter
    const auto& metadata = instrument->getMetadata();
    const auto* paramMeta = metadata.findParameter(paramId);

    if (paramMeta == nullptr)
    {
        throw std::runtime_error("Unknown parameter: " + paramId.toStdString());
    }

    // Clamp value to valid range
    value = juce::jlimit(paramMeta->minValue, paramMeta->maxValue, value);

    // Set parameter
    bool success = instrument->setParameter(paramId, value);

    if (!success)
    {
        throw std::runtime_error("Failed to set parameter: " + paramId.toStdString());
    }

    // Note: UndoManager automatically groups actions into transactions

    // Return result
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("success", true);
    result->setProperty("paramId", paramId);
    result->setProperty("value", value);
    return juce::var(result.get());
}

juce::var CommandAPI::cmd_getInstrumentParam(const juce::var& params)
{
    juce::String error;

    // Validate required params
    if (!validateParam(params, "trackId", error))
        throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "paramId", error))
        throw std::runtime_error(error.toStdString());

    auto* obj = params.getDynamicObject();
    juce::String trackId = obj->getProperty("trackId").toString();
    juce::String paramId = obj->getProperty("paramId").toString();

    // Find track
    int trackIndex = trackId.getTrailingIntValue();
    if (trackIndex < 0 || trackIndex >= engine.getNumTracks())
    {
        throw std::runtime_error("Track not found: " + trackId.toStdString());
    }

    auto* track = const_cast<zenith::Track*>(engine.tracks()[trackIndex].get());
    auto* instrument = track->getInstrument();

    if (instrument == nullptr)
    {
        throw std::runtime_error("Track has no instrument");
    }

    // Get metadata to validate parameter
    const auto& metadata = instrument->getMetadata();
    const auto* paramMeta = metadata.findParameter(paramId);

    if (paramMeta == nullptr)
    {
        throw std::runtime_error("Unknown parameter: " + paramId.toStdString());
    }

    // Get parameter value
    float value = instrument->getParameter(paramId);

    // Return result
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("paramId", paramId);
    result->setProperty("name", paramMeta->name);
    result->setProperty("value", value);
    result->setProperty("min", paramMeta->minValue);
    result->setProperty("max", paramMeta->maxValue);
    result->setProperty("default", paramMeta->defaultValue);
    return juce::var(result.get());
}

juce::var CommandAPI::cmd_randomizeInstrumentParams(const juce::var& params)
{
    juce::String error;

    // Validate required params
    if (!validateParam(params, "trackId", error))
        throw std::runtime_error(error.toStdString());

    auto* obj = params.getDynamicObject();
    juce::String trackId = obj->getProperty("trackId").toString();

    // Optional intensity parameter (0.0 = no change, 1.0 = full random range)
    float intensity = obj->hasProperty("intensity") ? (float)obj->getProperty("intensity") : 0.7f;
    intensity = juce::jlimit(0.0f, 1.0f, intensity);

    // Find track
    int trackIndex = trackId.getTrailingIntValue();
    if (trackIndex < 0 || trackIndex >= engine.getNumTracks())
    {
        throw std::runtime_error("Track not found: " + trackId.toStdString());
    }

    auto* track = const_cast<zenith::Track*>(engine.tracks()[trackIndex].get());
    auto* instrument = track->getInstrument();

    if (instrument == nullptr)
    {
        throw std::runtime_error("Track has no instrument");
    }

    // Note: UndoManager automatically groups actions into transactions

    // Get metadata and randomize parameters
    const auto& metadata = instrument->getMetadata();
    juce::Array<juce::var> randomizedParams;
    juce::Random random;

    for (const auto& param : metadata.parameters)
    {
        // Get current value
        float currentValue = instrument->getParameter(param.id);

        // Calculate random offset based on intensity
        float range = param.maxValue - param.minValue;
        float maxOffset = range * intensity * 0.5f;  // Max 50% of range at full intensity

        // Generate random offset
        float offset = random.nextFloat() * maxOffset * 2.0f - maxOffset;

        // Calculate new value and clamp
        float newValue = currentValue + offset;
        newValue = juce::jlimit(param.minValue, param.maxValue, newValue);

        // Set parameter
        bool success = instrument->setParameter(param.id, newValue);

        if (success)
        {
            randomizedParams.add(param.id);
        }
    }

    // Return result
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("success", true);
    result->setProperty("randomized", randomizedParams);
    result->setProperty("intensity", intensity);
    return juce::var(result.get());
}

