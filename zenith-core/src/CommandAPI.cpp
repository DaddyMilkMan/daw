/**
 * @file CommandAPI.cpp
 * @brief Command API implementation
 */

#include "../include/CommandAPI.h"
#include "../include/InstrumentCommandHelpers.h"
#include "../Source/instruments/InstrumentRegistry.h"
#include "../Source/engine/Track.h"

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
    registerCommand("export_wav", [this](const juce::var& p) { return cmd_exportWav(p); });

    // Instrument commands
    registerCommand("list_instruments", [this](const juce::var& p) { return cmd_listInstruments(p); });
    registerCommand("describe_instrument", [this](const juce::var& p) { return cmd_describeInstrument(p); });
    registerCommand("list_instrument_presets", [this](const juce::var& p) { return cmd_listInstrumentPresets(p); });
    registerCommand("set_track_instrument", [this](const juce::var& p) { return cmd_setTrackInstrument(p); });
    registerCommand("set_instrument_param", [this](const juce::var& p) { return cmd_setInstrumentParam(p); });
    registerCommand("set_instrument_macro", [this](const juce::var& p) { return cmd_setInstrumentMacro(p); });
    registerCommand("load_instrument_preset", [this](const juce::var& p) { return cmd_loadInstrumentPreset(p); });
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
// Instrument Commands
//==============================================================================

juce::var CommandAPI::cmd_listInstruments(const juce::var& params)
{
    juce::ignoreUnused(params);

    auto& registry = zenith::InstrumentRegistry::getInstance();
    auto instrumentList = registry.getInstrumentList();

    auto* result = new juce::DynamicObject();
    result->setProperty("instruments", instrumentList);

    return InstrumentCommandHelpers::createSuccess(juce::var(result));
}

juce::var CommandAPI::cmd_describeInstrument(const juce::var& params)
{
    juce::String error;

    // Validate required params
    if (!validateParam(params, "instrumentId", error))
        return InstrumentCommandHelpers::createError(
            InstrumentCommandHelpers::ErrorCode::InvalidValue, error);

    auto* obj = params.getDynamicObject();
    juce::String instrumentId = obj->getProperty("instrumentId").toString();

    // Get metadata from registry
    auto& registry = zenith::InstrumentRegistry::getInstance();
    const auto* metadata = registry.getMetadata(instrumentId);

    if (metadata == nullptr)
    {
        return InstrumentCommandHelpers::createError(
            InstrumentCommandHelpers::ErrorCode::UnknownInstrument,
            "Instrument '" + instrumentId + "' not found");
    }

    // Convert to var
    auto metadataVar = metadata->toVar();

    auto* result = new juce::DynamicObject();
    result->setProperty("instrument", metadataVar);

    return InstrumentCommandHelpers::createSuccess(juce::var(result));
}

juce::var CommandAPI::cmd_listInstrumentPresets(const juce::var& params)
{
    juce::String error;

    // Validate required params
    if (!validateParam(params, "instrumentId", error))
        return InstrumentCommandHelpers::createError(
            InstrumentCommandHelpers::ErrorCode::InvalidValue, error);

    auto* obj = params.getDynamicObject();
    juce::String instrumentId = obj->getProperty("instrumentId").toString();

    // Create a temporary instance to get presets
    auto& registry = zenith::InstrumentRegistry::getInstance();
    auto instrument = registry.createInstrument(instrumentId);

    if (instrument == nullptr)
    {
        return InstrumentCommandHelpers::createError(
            InstrumentCommandHelpers::ErrorCode::UnknownInstrument,
            "Instrument '" + instrumentId + "' not found");
    }

    // Get preset IDs
    auto presetIds = instrument->getPresetIds();

    juce::Array<juce::var> presetsArray;
    for (const auto& presetId : presetIds)
        presetsArray.add(presetId);

    auto* result = new juce::DynamicObject();
    result->setProperty("presets", presetsArray);

    return InstrumentCommandHelpers::createSuccess(juce::var(result));
}

juce::var CommandAPI::cmd_setTrackInstrument(const juce::var& params)
{
    juce::String error;

    // Validate required params
    if (!validateParam(params, "trackId", error))
        return InstrumentCommandHelpers::createError(
            InstrumentCommandHelpers::ErrorCode::InvalidValue, error);
    if (!validateParam(params, "instrumentId", error))
        return InstrumentCommandHelpers::createError(
            InstrumentCommandHelpers::ErrorCode::InvalidValue, error);

    auto* obj = params.getDynamicObject();
    juce::String trackId = obj->getProperty("trackId").toString();
    juce::String instrumentId = obj->getProperty("instrumentId").toString();
    juce::String presetId = obj->getProperty("presetId", "").toString();

    // Find track
    auto* track = InstrumentCommandHelpers::findTrack(engine, trackId);
    if (track == nullptr)
    {
        return InstrumentCommandHelpers::createError(
            InstrumentCommandHelpers::ErrorCode::UnknownTrack,
            "Track '" + trackId + "' not found");
    }

    // Create instrument
    auto& registry = zenith::InstrumentRegistry::getInstance();
    auto instrument = registry.createInstrument(instrumentId);

    if (instrument == nullptr)
    {
        return InstrumentCommandHelpers::createError(
            InstrumentCommandHelpers::ErrorCode::UnknownInstrument,
            "Instrument '" + instrumentId + "' not found");
    }

    // Load preset if specified
    if (presetId.isNotEmpty())
    {
        if (!instrument->loadPreset(presetId))
        {
            return InstrumentCommandHelpers::createError(
                InstrumentCommandHelpers::ErrorCode::UnknownPreset,
                "Preset '" + presetId + "' not found for instrument '" + instrumentId + "'");
        }
    }

    // Get metadata before moving instrument
    auto metadata = instrument->getMetadata().toVar();

    // Set instrument on track
    track->setInstrument(std::move(instrument));

    auto* result = new juce::DynamicObject();
    result->setProperty("instrument", metadata);
    if (presetId.isNotEmpty())
        result->setProperty("presetId", presetId);

    return InstrumentCommandHelpers::createSuccess(juce::var(result));
}

juce::var CommandAPI::cmd_setInstrumentParam(const juce::var& params)
{
    juce::String error;

    // Validate required params
    if (!validateParam(params, "trackId", error))
        return InstrumentCommandHelpers::createError(
            InstrumentCommandHelpers::ErrorCode::InvalidValue, error);
    if (!validateParam(params, "parameterId", error))
        return InstrumentCommandHelpers::createError(
            InstrumentCommandHelpers::ErrorCode::InvalidValue, error);
    if (!validateParam(params, "value", error))
        return InstrumentCommandHelpers::createError(
            InstrumentCommandHelpers::ErrorCode::InvalidValue, error);

    auto* obj = params.getDynamicObject();
    juce::String trackId = obj->getProperty("trackId").toString();
    juce::String parameterId = obj->getProperty("parameterId").toString();
    float value = static_cast<float>(obj->getProperty("value"));

    // Find track and instrument
    auto* track = InstrumentCommandHelpers::findTrack(engine, trackId);
    juce::var errorVar;
    auto* instrument = InstrumentCommandHelpers::getTrackInstrument(track, errorVar);
    if (instrument == nullptr)
        return errorVar;

    // Validate parameter exists
    const auto& metadata = instrument->getMetadata();
    if (metadata.findParameter(parameterId) == nullptr)
    {
        return InstrumentCommandHelpers::createError(
            InstrumentCommandHelpers::ErrorCode::UnknownParameter,
            "Parameter '" + parameterId + "' not found on instrument '" + metadata.instrumentId + "'");
    }

    // Clamp and set value
    value = InstrumentCommandHelpers::clampNormalizedValue(value);
    if (!instrument->setParameter(parameterId, value))
    {
        return InstrumentCommandHelpers::createError(
            InstrumentCommandHelpers::ErrorCode::InternalError,
            "Failed to set parameter '" + parameterId + "'");
    }

    auto* result = new juce::DynamicObject();
    result->setProperty("parameterId", parameterId);
    result->setProperty("value", value);

    return InstrumentCommandHelpers::createSuccess(juce::var(result));
}

juce::var CommandAPI::cmd_setInstrumentMacro(const juce::var& params)
{
    juce::String error;

    // Validate required params
    if (!validateParam(params, "trackId", error))
        return InstrumentCommandHelpers::createError(
            InstrumentCommandHelpers::ErrorCode::InvalidValue, error);
    if (!validateParam(params, "macroId", error))
        return InstrumentCommandHelpers::createError(
            InstrumentCommandHelpers::ErrorCode::InvalidValue, error);
    if (!validateParam(params, "value", error))
        return InstrumentCommandHelpers::createError(
            InstrumentCommandHelpers::ErrorCode::InvalidValue, error);

    auto* obj = params.getDynamicObject();
    juce::String trackId = obj->getProperty("trackId").toString();
    juce::String macroId = obj->getProperty("macroId").toString();
    float value = static_cast<float>(obj->getProperty("value"));

    // Find track and instrument
    auto* track = InstrumentCommandHelpers::findTrack(engine, trackId);
    juce::var errorVar;
    auto* instrument = InstrumentCommandHelpers::getTrackInstrument(track, errorVar);
    if (instrument == nullptr)
        return errorVar;

    // Validate macro exists
    const auto& metadata = instrument->getMetadata();
    if (metadata.findMacro(macroId) == nullptr)
    {
        return InstrumentCommandHelpers::createError(
            InstrumentCommandHelpers::ErrorCode::UnknownMacro,
            "Macro '" + macroId + "' not found on instrument '" + metadata.instrumentId + "'");
    }

    // Clamp and set value
    value = InstrumentCommandHelpers::clampNormalizedValue(value);
    if (!instrument->setMacro(macroId, value))
    {
        return InstrumentCommandHelpers::createError(
            InstrumentCommandHelpers::ErrorCode::InternalError,
            "Failed to set macro '" + macroId + "'");
    }

    auto* result = new juce::DynamicObject();
    result->setProperty("macroId", macroId);
    result->setProperty("value", value);

    return InstrumentCommandHelpers::createSuccess(juce::var(result));
}

juce::var CommandAPI::cmd_loadInstrumentPreset(const juce::var& params)
{
    juce::String error;

    // Validate required params
    if (!validateParam(params, "trackId", error))
        return InstrumentCommandHelpers::createError(
            InstrumentCommandHelpers::ErrorCode::InvalidValue, error);
    if (!validateParam(params, "presetId", error))
        return InstrumentCommandHelpers::createError(
            InstrumentCommandHelpers::ErrorCode::InvalidValue, error);

    auto* obj = params.getDynamicObject();
    juce::String trackId = obj->getProperty("trackId").toString();
    juce::String presetId = obj->getProperty("presetId").toString();

    // Find track and instrument
    auto* track = InstrumentCommandHelpers::findTrack(engine, trackId);
    juce::var errorVar;
    auto* instrument = InstrumentCommandHelpers::getTrackInstrument(track, errorVar);
    if (instrument == nullptr)
        return errorVar;

    // Load preset
    if (!instrument->loadPreset(presetId))
    {
        const auto& metadata = instrument->getMetadata();
        return InstrumentCommandHelpers::createError(
            InstrumentCommandHelpers::ErrorCode::UnknownPreset,
            "Preset '" + presetId + "' not found for instrument '" + metadata.instrumentId + "'");
    }

    auto* result = new juce::DynamicObject();
    result->setProperty("presetId", presetId);

    return InstrumentCommandHelpers::createSuccess(juce::var(result));
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
