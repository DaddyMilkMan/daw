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
    registerCommand("add_automation_point", [this](const juce::var& p) { return cmd_addAutomationPoint(p); });
    registerCommand("clear_automation", [this](const juce::var& p) { return cmd_clearAutomation(p); });
    registerCommand("get_automation", [this](const juce::var& p) { return cmd_getAutomation(p); });
    registerCommand("add_track", [this](const juce::var& p) { return cmd_addTrack(p); });
    registerCommand("get_project_info", [this](const juce::var& p) { return cmd_getProjectInfo(p); });
    registerCommand("set_tempo", [this](const juce::var& p) { return cmd_setTempo(p); });

    // Phase 15: Tempo map and markers commands
    registerCommand("add_tempo_change", [this](const juce::var& p) { return cmd_addTempoChange(p); });
    registerCommand("get_tempo_map", [this](const juce::var& p) { return cmd_getTempoMap(p); });
    registerCommand("add_marker", [this](const juce::var& p) { return cmd_addMarker(p); });
    registerCommand("get_markers", [this](const juce::var& p) { return cmd_getMarkers(p); });
    registerCommand("delete_marker", [this](const juce::var& p) { return cmd_deleteMarker(p); });
    registerCommand("goto_marker", [this](const juce::var& p) { return cmd_gotoMarker(p); });
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
// Phase 15: Tempo Map and Markers Commands
//==============================================================================

juce::var CommandAPI::cmd_addTempoChange(const juce::var& params)
{
    juce::String error;

    if (!validateParam(params, "beatPosition", error)) throw std::runtime_error(error.toStdString());
    if (!validateParam(params, "bpm", error)) throw std::runtime_error(error.toStdString());

    double beatPosition = params["beatPosition"];
    double bpm = params["bpm"];
    int numerator = params.hasProperty("timeSigNumerator") ? (int)params["timeSigNumerator"] : 4;
    int denominator = params.hasProperty("timeSigDenominator") ? (int)params["timeSigDenominator"] : 4;

    auto tempoId = projectState.addTempoChange(beatPosition, bpm, numerator, denominator, "Add tempo change");

    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("tempoId", tempoId);
    return juce::var(result.get());
}

juce::var CommandAPI::cmd_getTempoMap(const juce::var& params)
{
    juce::ignoreUnused(params);

    auto tempoChanges = projectState.getTempoChanges();

    juce::Array<juce::var> tempoArray;
    for (const auto& tempo : tempoChanges)
    {
        juce::DynamicObject::Ptr tempoObj = new juce::DynamicObject();
        tempoObj->setProperty("id", tempo.id);
        tempoObj->setProperty("beatPosition", tempo.beatPosition);
        tempoObj->setProperty("bpm", tempo.bpm);
        tempoObj->setProperty("timeSigNumerator", tempo.timeSigNumerator);
        tempoObj->setProperty("timeSigDenominator", tempo.timeSigDenominator);
        tempoArray.add(juce::var(tempoObj.get()));
    }

    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("tempoChanges", tempoArray);
    return juce::var(result.get());
}

juce::var CommandAPI::cmd_addMarker(const juce::var& params)
{
    juce::String error;

    if (!validateParam(params, "beatPosition", error)) throw std::runtime_error(error.toStdString());

    double beatPosition = params["beatPosition"];
    juce::String name = params.hasProperty("name") ? params["name"].toString() : "Marker";
    juce::String color = params.hasProperty("color") ? params["color"].toString() : "#FFCC00";

    auto markerId = projectState.addMarker(beatPosition, name, color, "Add marker");

    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("markerId", markerId);
    return juce::var(result.get());
}

juce::var CommandAPI::cmd_getMarkers(const juce::var& params)
{
    juce::ignoreUnused(params);

    auto markers = projectState.getMarkers();

    juce::Array<juce::var> markerArray;
    for (const auto& marker : markers)
    {
        juce::DynamicObject::Ptr markerObj = new juce::DynamicObject();
        markerObj->setProperty("id", marker.id);
        markerObj->setProperty("name", marker.name);
        markerObj->setProperty("beatPosition", marker.beatPosition);
        markerObj->setProperty("color", marker.color);
        markerArray.add(juce::var(markerObj.get()));
    }

    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("markers", markerArray);
    return juce::var(result.get());
}

juce::var CommandAPI::cmd_deleteMarker(const juce::var& params)
{
    juce::String error;

    if (!validateParam(params, "markerId", error)) throw std::runtime_error(error.toStdString());

    juce::String markerId = params["markerId"].toString();
    bool success = projectState.deleteMarker(markerId, "Delete marker");

    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("success", success);
    return juce::var(result.get());
}

juce::var CommandAPI::cmd_gotoMarker(const juce::var& params)
{
    juce::String error;

    if (!validateParam(params, "markerId", error)) throw std::runtime_error(error.toStdString());

    juce::String markerId = params["markerId"].toString();

    // Find marker
    auto markers = projectState.getMarkers();
    for (const auto& marker : markers)
    {
        if (marker.id == markerId)
        {
            // Convert beat to samples and set playhead
            juce::int64 samplePos = engine.beatToSample(marker.beatPosition);
            engine.setPlayheadPosition(samplePos);

            juce::DynamicObject::Ptr result = new juce::DynamicObject();
            result->setProperty("success", true);
            result->setProperty("beatPosition", marker.beatPosition);
            return juce::var(result.get());
        }
    }

    throw std::runtime_error("Marker not found: " + markerId.toStdString());
}
