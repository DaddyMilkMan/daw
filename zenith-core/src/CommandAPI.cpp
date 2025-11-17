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
    // Project
    registerCommand("get_project_info", [this](const juce::var& p) { return cmd_getProjectInfo(p); });
    registerCommand("set_tempo", [this](const juce::var& p) { return cmd_setTempo(p); });

    // Tracks
    registerCommand("add_track", [this](const juce::var& p) { return cmd_addTrack(p); });
    registerCommand("delete_track", [this](const juce::var& p) { return cmd_deleteTrack(p); });
    registerCommand("rename_track", [this](const juce::var& p) { return cmd_renameTrack(p); });
    registerCommand("get_tracks", [this](const juce::var& p) { return cmd_getTracks(p); });
    registerCommand("set_track_property", [this](const juce::var& p) { return cmd_setTrackProperty(p); });

    // Clips
    registerCommand("create_clip", [this](const juce::var& p) { return cmd_createClip(p); });
    registerCommand("delete_clip", [this](const juce::var& p) { return cmd_deleteClip(p); });
    registerCommand("move_clip", [this](const juce::var& p) { return cmd_moveClip(p); });
    registerCommand("resize_clip", [this](const juce::var& p) { return cmd_resizeClip(p); });
    registerCommand("get_clips", [this](const juce::var& p) { return cmd_getClips(p); });

    // MIDI Notes
    registerCommand("create_note", [this](const juce::var& p) { return cmd_createNote(p); });
    registerCommand("delete_note", [this](const juce::var& p) { return cmd_deleteNote(p); });
    registerCommand("move_note", [this](const juce::var& p) { return cmd_moveNote(p); });
    registerCommand("resize_note", [this](const juce::var& p) { return cmd_resizeNote(p); });
    registerCommand("get_notes", [this](const juce::var& p) { return cmd_getNotes(p); });

    // Automation
    registerCommand("add_automation_point", [this](const juce::var& p) { return cmd_addAutomationPoint(p); });
    registerCommand("delete_automation_point", [this](const juce::var& p) { return cmd_deleteAutomationPoint(p); });
    registerCommand("move_automation_point", [this](const juce::var& p) { return cmd_moveAutomationPoint(p); });
    registerCommand("clear_automation", [this](const juce::var& p) { return cmd_clearAutomation(p); });
    registerCommand("get_automation", [this](const juce::var& p) { return cmd_getAutomation(p); });

    // Transport
    registerCommand("play", [this](const juce::var& p) { return cmd_play(p); });
    registerCommand("stop", [this](const juce::var& p) { return cmd_stop(p); });

    // Undo/Redo
    registerCommand("undo", [this](const juce::var& p) { return cmd_undo(p); });
    registerCommand("redo", [this](const juce::var& p) { return cmd_redo(p); });
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

//==============================================================================
// Track Management Commands
//==============================================================================

juce::var CommandAPI::cmd_deleteTrack(const juce::var& params)
{
    juce::String error;

    // Validate required params
    if (!validateParam(params, "trackId", error)) throw std::runtime_error(error.toStdString());

    auto* obj = params.getDynamicObject();
    juce::String trackId = obj->getProperty("trackId").toString();

    // Delete track
    projectState.removeTrack(trackId);

    // Return result
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("success", true);
    return juce::var(result.get());
}

juce::var CommandAPI::cmd_renameTrack(const juce::var& params)
{
    juce::String error;

    // Validate required params
    if (!validateParam(params, "trackId", error)) throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "name", error)) throw std::runtime_error(error.toStdString());

    auto* obj = params.getDynamicObject();
    juce::String trackId = obj->getProperty("trackId").toString();
    juce::String name = obj->getProperty("name").toString();

    // Rename track
    projectState.setTrackName(trackId, name);

    // Return result
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("success", true);
    return juce::var(result.get());
}

juce::var CommandAPI::cmd_getTracks(const juce::var& params)
{
    juce::ignoreUnused(params);

    juce::Array<juce::var> tracksArray;

    auto tracksNode = projectState.getState().getChildWithName(ProjectState::ID_TRACKS);
    if (tracksNode.isValid())
    {
        for (int i = 0; i < tracksNode.getNumChildren(); ++i)
        {
            auto track = tracksNode.getChild(i);
            if (track.hasType(ProjectState::ID_TRACK))
            {
                juce::DynamicObject::Ptr trackObj = new juce::DynamicObject();
                trackObj->setProperty("id", track[ProjectState::PROP_ID].toString());
                trackObj->setProperty("name", track[ProjectState::PROP_NAME].toString());
                trackObj->setProperty("type", track[ProjectState::PROP_TYPE].toString());
                trackObj->setProperty("volume", track[ProjectState::PROP_VOLUME]);
                trackObj->setProperty("pan", track[ProjectState::PROP_PAN]);
                trackObj->setProperty("mute", track[ProjectState::PROP_MUTE]);
                trackObj->setProperty("solo", track[ProjectState::PROP_SOLO]);
                tracksArray.add(juce::var(trackObj.get()));
            }
        }
    }

    // Return result
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("tracks", tracksArray);
    return juce::var(result.get());
}

juce::var CommandAPI::cmd_setTrackProperty(const juce::var& params)
{
    juce::String error;

    // Validate required params
    if (!validateParam(params, "trackId", error)) throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "property", error)) throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "value", error)) throw std::runtime_error(error.toStdString());

    auto* obj = params.getDynamicObject();
    juce::String trackId = obj->getProperty("trackId").toString();
    juce::String propertyName = obj->getProperty("property").toString();
    juce::var value = obj->getProperty("value");

    // Map property name to identifier
    juce::Identifier propId;
    if (propertyName == "volume") propId = ProjectState::PROP_VOLUME;
    else if (propertyName == "pan") propId = ProjectState::PROP_PAN;
    else if (propertyName == "mute") propId = ProjectState::PROP_MUTE;
    else if (propertyName == "solo") propId = ProjectState::PROP_SOLO;
    else throw std::runtime_error("Invalid property name. Must be 'volume', 'pan', 'mute', or 'solo'");

    // Set property
    projectState.setTrackProperty(trackId, propId, value);

    // Return result
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("success", true);
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
    if (!validateParam(params, "type", error)) throw std::runtime_error(error.toStdString());

    auto* obj = params.getDynamicObject();
    juce::String trackId = obj->getProperty("trackId").toString();
    double startBeats = obj->getProperty("startBeats");
    double lengthBeats = obj->getProperty("lengthBeats");
    juce::String type = obj->getProperty("type").toString();

    // Validate type
    if (type != "audio" && type != "midi")
    {
        throw std::runtime_error("Invalid type. Must be 'audio' or 'midi'");
    }

    // Create clip
    juce::String clipId = projectState.addClip(trackId, startBeats, lengthBeats, type);

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
    bool success = projectState.deleteClip(trackId, clipId);

    // Return result
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("success", success);
    return juce::var(result.get());
}

juce::var CommandAPI::cmd_moveClip(const juce::var& params)
{
    juce::String error;

    // Validate required params
    if (!validateParam(params, "trackId", error)) throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "clipId", error)) throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "startBeats", error)) throw std::runtime_error(error.toStdString());

    auto* obj = params.getDynamicObject();
    juce::String trackId = obj->getProperty("trackId").toString();
    juce::String clipId = obj->getProperty("clipId").toString();
    double startBeats = obj->getProperty("startBeats");

    // Move clip
    bool success = projectState.moveClip(trackId, clipId, startBeats);

    // Return result
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("success", success);
    return juce::var(result.get());
}

juce::var CommandAPI::cmd_resizeClip(const juce::var& params)
{
    juce::String error;

    // Validate required params
    if (!validateParam(params, "trackId", error)) throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "clipId", error)) throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "lengthBeats", error)) throw std::runtime_error(error.toStdString());

    auto* obj = params.getDynamicObject();
    juce::String trackId = obj->getProperty("trackId").toString();
    juce::String clipId = obj->getProperty("clipId").toString();
    double lengthBeats = obj->getProperty("lengthBeats");

    // Resize clip
    bool success = projectState.resizeClip(trackId, clipId, lengthBeats);

    // Return result
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("success", success);
    return juce::var(result.get());
}

juce::var CommandAPI::cmd_getClips(const juce::var& params)
{
    juce::String error;

    // Validate required params
    if (!validateParam(params, "trackId", error)) throw std::runtime_error(error.toStdString());

    auto* obj = params.getDynamicObject();
    juce::String trackId = obj->getProperty("trackId").toString();

    juce::Array<juce::var> clipsArray;

    auto clipsNode = projectState.getClips(trackId);
    if (clipsNode.isValid())
    {
        for (int i = 0; i < clipsNode.getNumChildren(); ++i)
        {
            auto clip = clipsNode.getChild(i);
            if (clip.hasType(ProjectState::ID_CLIP))
            {
                juce::DynamicObject::Ptr clipObj = new juce::DynamicObject();
                clipObj->setProperty("id", clip[ProjectState::PROP_ID].toString());
                clipObj->setProperty("type", clip[ProjectState::PROP_TYPE].toString());
                clipObj->setProperty("start", clip[ProjectState::PROP_START]);
                clipObj->setProperty("length", clip[ProjectState::PROP_LENGTH]);
                clipsArray.add(juce::var(clipObj.get()));
            }
        }
    }

    // Return result
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("clips", clipsArray);
    return juce::var(result.get());
}

//==============================================================================
// MIDI Note Management Commands
//==============================================================================

juce::var CommandAPI::cmd_createNote(const juce::var& params)
{
    juce::String error;

    // Validate required params
    if (!validateParam(params, "trackId", error)) throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "clipId", error)) throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "startBeats", error)) throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "lengthBeats", error)) throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "pitch", error)) throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "velocity", error)) throw std::runtime_error(error.toStdString());

    auto* obj = params.getDynamicObject();
    juce::String trackId = obj->getProperty("trackId").toString();
    juce::String clipId = obj->getProperty("clipId").toString();
    double startBeats = obj->getProperty("startBeats");
    double lengthBeats = obj->getProperty("lengthBeats");
    int pitch = obj->getProperty("pitch");
    int velocity = obj->getProperty("velocity");

    // Validate ranges
    if (pitch < 0 || pitch > 127)
        throw std::runtime_error("Pitch must be 0-127");
    if (velocity < 0 || velocity > 127)
        throw std::runtime_error("Velocity must be 0-127");

    // Create note
    juce::String noteId = projectState.addNote(trackId, clipId, startBeats, lengthBeats, pitch, velocity);

    if (noteId.isEmpty())
    {
        throw std::runtime_error("Failed to create note. Check trackId/clipId.");
    }

    // Return result
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("noteId", noteId);
    return juce::var(result.get());
}

juce::var CommandAPI::cmd_deleteNote(const juce::var& params)
{
    juce::String error;

    // Validate required params
    if (!validateParam(params, "trackId", error)) throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "clipId", error)) throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "noteId", error)) throw std::runtime_error(error.toStdString());

    auto* obj = params.getDynamicObject();
    juce::String trackId = obj->getProperty("trackId").toString();
    juce::String clipId = obj->getProperty("clipId").toString();
    juce::String noteId = obj->getProperty("noteId").toString();

    // Delete note
    bool success = projectState.deleteNote(trackId, clipId, noteId);

    // Return result
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("success", success);
    return juce::var(result.get());
}

juce::var CommandAPI::cmd_moveNote(const juce::var& params)
{
    juce::String error;

    // Validate required params
    if (!validateParam(params, "trackId", error)) throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "clipId", error)) throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "noteId", error)) throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "startBeats", error)) throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "pitch", error)) throw std::runtime_error(error.toStdString());

    auto* obj = params.getDynamicObject();
    juce::String trackId = obj->getProperty("trackId").toString();
    juce::String clipId = obj->getProperty("clipId").toString();
    juce::String noteId = obj->getProperty("noteId").toString();
    double startBeats = obj->getProperty("startBeats");
    int pitch = obj->getProperty("pitch");

    // Validate range
    if (pitch < 0 || pitch > 127)
        throw std::runtime_error("Pitch must be 0-127");

    // Move note
    bool success = projectState.moveNote(trackId, clipId, noteId, startBeats, pitch);

    // Return result
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("success", success);
    return juce::var(result.get());
}

juce::var CommandAPI::cmd_resizeNote(const juce::var& params)
{
    juce::String error;

    // Validate required params
    if (!validateParam(params, "trackId", error)) throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "clipId", error)) throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "noteId", error)) throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "lengthBeats", error)) throw std::runtime_error(error.toStdString());

    auto* obj = params.getDynamicObject();
    juce::String trackId = obj->getProperty("trackId").toString();
    juce::String clipId = obj->getProperty("clipId").toString();
    juce::String noteId = obj->getProperty("noteId").toString();
    double lengthBeats = obj->getProperty("lengthBeats");

    // Resize note
    bool success = projectState.resizeNote(trackId, clipId, noteId, lengthBeats);

    // Return result
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("success", success);
    return juce::var(result.get());
}

juce::var CommandAPI::cmd_getNotes(const juce::var& params)
{
    juce::String error;

    // Validate required params
    if (!validateParam(params, "trackId", error)) throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "clipId", error)) throw std::runtime_error(error.toStdString());

    auto* obj = params.getDynamicObject();
    juce::String trackId = obj->getProperty("trackId").toString();
    juce::String clipId = obj->getProperty("clipId").toString();

    juce::Array<juce::var> notesArray;

    auto notesNode = projectState.getNotes(trackId, clipId);
    if (notesNode.isValid())
    {
        for (int i = 0; i < notesNode.getNumChildren(); ++i)
        {
            auto note = notesNode.getChild(i);
            if (note.hasType(ProjectState::ID_NOTE))
            {
                juce::DynamicObject::Ptr noteObj = new juce::DynamicObject();
                noteObj->setProperty("id", note[ProjectState::PROP_ID].toString());
                noteObj->setProperty("start", note[ProjectState::PROP_START]);
                noteObj->setProperty("length", note[ProjectState::PROP_LENGTH]);
                noteObj->setProperty("pitch", note[ProjectState::PROP_PITCH]);
                noteObj->setProperty("velocity", note[ProjectState::PROP_VELOCITY]);
                notesArray.add(juce::var(noteObj.get()));
            }
        }
    }

    // Return result
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("notes", notesArray);
    return juce::var(result.get());
}

//==============================================================================
// Additional Automation Commands
//==============================================================================

juce::var CommandAPI::cmd_deleteAutomationPoint(const juce::var& params)
{
    juce::String error;

    // Validate required params
    if (!validateParam(params, "trackId", error)) throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "param", error)) throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "pointId", error)) throw std::runtime_error(error.toStdString());

    auto* obj = params.getDynamicObject();
    juce::String trackId = obj->getProperty("trackId").toString();
    juce::String param = obj->getProperty("param").toString();
    juce::String pointId = obj->getProperty("pointId").toString();

    // Validate param name
    if (param != "volume" && param != "pan" && param != "mute")
    {
        throw std::runtime_error("Invalid param. Must be 'volume', 'pan', or 'mute'");
    }

    // Delete point
    bool success = projectState.deleteAutomationPoint(trackId, param, pointId, "Delete automation point");

    // Return result
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("success", success);
    return juce::var(result.get());
}

juce::var CommandAPI::cmd_moveAutomationPoint(const juce::var& params)
{
    juce::String error;

    // Validate required params
    if (!validateParam(params, "trackId", error)) throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "param", error)) throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "pointId", error)) throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "timeBeats", error)) throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "value", error)) throw std::runtime_error(error.toStdString());

    auto* obj = params.getDynamicObject();
    juce::String trackId = obj->getProperty("trackId").toString();
    juce::String param = obj->getProperty("param").toString();
    juce::String pointId = obj->getProperty("pointId").toString();
    double timeBeats = obj->getProperty("timeBeats");
    double value = obj->getProperty("value");

    // Validate param name
    if (param != "volume" && param != "pan" && param != "mute")
    {
        throw std::runtime_error("Invalid param. Must be 'volume', 'pan', or 'mute'");
    }

    // Move point
    bool success = projectState.moveAutomationPoint(trackId, param, pointId, timeBeats, value,
                                                     "Move automation point");

    // Return result
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("success", success);
    return juce::var(result.get());
}

//==============================================================================
// Transport Commands
//==============================================================================

juce::var CommandAPI::cmd_play(const juce::var& params)
{
    juce::ignoreUnused(params);

    engine.play();

    // Return result
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("success", true);
    return juce::var(result.get());
}

juce::var CommandAPI::cmd_stop(const juce::var& params)
{
    juce::ignoreUnused(params);

    engine.stop();

    // Return result
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("success", true);
    return juce::var(result.get());
}

//==============================================================================
// Undo/Redo Commands
//==============================================================================

juce::var CommandAPI::cmd_undo(const juce::var& params)
{
    juce::ignoreUnused(params);

    projectState.undo();

    // Return result
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

    // Return result
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("success", true);
    result->setProperty("canUndo", projectState.canUndo());
    result->setProperty("canRedo", projectState.canRedo());
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
