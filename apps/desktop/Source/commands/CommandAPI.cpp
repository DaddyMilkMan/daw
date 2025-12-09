/*
  ==============================================================================

    CommandAPI.cpp
    Created: 2025-11-14
    Author:  Zenith DAW - Phase 5: Wingman v0

    JSON command processor implementation

  ==============================================================================
*/

#include "CommandAPI.h"
#include "../../include/Engine.h"
#include "../../include/ProjectState.h"
#include "../../include/TempoMap.h"
#include "../engine/Clip.h"
#include "../engine/PluginHost.h"
#include "../engine/Track.h"
#include "../instruments/InstrumentRegistry.h"
#include "ClipCommands.h"
#include "CommandUtils.h"
#include "SessionGraph.h"
#include "TrackCommands.h"
#include "TransportCommands.h"


#include "../ai/AIMasteringAgent.h"
#include "../dsp/ONNXStemSeparator.h"


namespace zenith {

//==============================================================================
CommandAPI::CommandAPI(ProjectState &state, Engine &eng)
    : projectState(state), engine(eng) {
  trackCommands = std::make_unique<TrackCommands>(engine, projectState);
  clipCommands = std::make_unique<ClipCommands>(engine, projectState);
  transportCommands = std::make_unique<TransportCommands>(engine, projectState);

  DBG("CommandAPI: Initialized");
  initializeCommandMap();
}

void CommandAPI::initializeCommandMap() {
  commandMap["list_tracks"] = CommandID::ListTracks;
  commandMap["create_track"] = CommandID::CreateTrack;
  commandMap["delete_track"] = CommandID::DeleteTrack;
  commandMap["rename_track"] = CommandID::RenameTrack;
  commandMap["set_track_volume"] = CommandID::SetTrackVolume;
  commandMap["set_track_pan"] = CommandID::SetTrackPan;
  commandMap["export_audio"] = CommandID::ExportAudio;
  commandMap["export_project_advanced"] = CommandID::ExportProjectAdvanced;
  commandMap["separate_track"] = CommandID::SeparateTrack;

  commandMap["list_clips"] = CommandID::ListClips;
  commandMap["create_clip"] =
      CommandID::CreateClip; // add_clip alias? original code checked add_clip
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
  commandMap["get_instrument_parameter_schema"] =
      CommandID::GetInstrumentParameterSchema;

  // Quick Wins: Mixer Control
  commandMap["set_track_send"] = CommandID::SetTrackSend;
  commandMap["set_track_eq"] = CommandID::SetTrackEQ;
  commandMap["set_track_compressor"] = CommandID::SetTrackCompressor;
}

CommandAPI::~CommandAPI() {}

//==============================================================================
juce::var CommandAPI::executeCommand(const juce::var &request) {
  // Validate request structure
  if (!request.isObject())
    return createErrorResponse("Invalid request: must be JSON object");

  if (!request.hasProperty("command"))
    return createErrorResponse("Missing 'command' field");

  juce::String commandStr = request["command"].toString();
  juce::var params =
      request.hasProperty("params") ? request["params"] : juce::var();

  DBG("CommandAPI: Executing command: " + commandStr);

  // Map Lookup
  auto it = commandMap.find(commandStr.toStdString());
  if (it == commandMap.end()) {
    return createErrorResponse("Unknown command: " + commandStr);
  }

  CommandID id = it->second;

  switch (id) {
  case CommandID::ListTracks:
    return trackCommands->listTracks(params);
  case CommandID::CreateTrack:
    return trackCommands->createTrack(params);
  case CommandID::DeleteTrack:
    return trackCommands->deleteTrack(params);
  case CommandID::RenameTrack:
    return trackCommands->renameTrack(params);
  case CommandID::SetTrackVolume:
    return trackCommands->setTrackVolume(params);
  case CommandID::SetTrackPan:
    return trackCommands->setTrackPan(params);
  case CommandID::ExportAudio:
    return exportAudio(params);
  case CommandID::ExportProjectAdvanced:
    return exportProjectAdvanced(params);
  case CommandID::SeparateTrack:
    return trackCommands->separateTrack(params);

  case CommandID::ListClips:
    return clipCommands->listClips(params);
  case CommandID::CreateClip:
    return clipCommands->createClip(params);
  case CommandID::DeleteClip:
    return clipCommands->deleteClip(params);
  case CommandID::SplitClip:
    return clipCommands->splitClip(params);
  case CommandID::MoveClip:
    return clipCommands->moveClip(params);
  case CommandID::ResizeClip:
    return clipCommands->resizeClip(params);

  case CommandID::Play:
    return transportCommands->play(params);
  case CommandID::Stop:
    return transportCommands->stop(params);
  case CommandID::Record:
    return transportCommands->record(params);
  case CommandID::Rewind:
    return transportCommands->rewind(params);
  case CommandID::SetLoop:
    return transportCommands->setLoop(params);
  case CommandID::SetTempo:
    return transportCommands->setTempo(params);
  case CommandID::SetTimeSignature:
    return transportCommands->setTimeSignature(params);

  case CommandID::GetSessionGraph:
    return getSessionGraph(params);
  case CommandID::Undo:
    return undo(params);
  case CommandID::Redo:
    return redo(params);
  case CommandID::History:
    return history(params);

  case CommandID::DescribeInstrument:
    return describeInstrument(params);

  case CommandID::AddPlugin:
    return addPlugin(params);
  case CommandID::RemovePlugin:
    return removePlugin(params);
  case CommandID::ListPlugins:
    return listPlugins(params);
  case CommandID::SetPluginParam:
    return setPluginParam(params);
  case CommandID::GetPluginParams:
    return getPluginParams(params);

  case CommandID::AddAutomationPoint:
    return addAutomationPoint(params);
  case CommandID::ClearAutomation:
    return clearAutomation(params);
  case CommandID::GetAutomation:
    return getAutomation(params);

  case CommandID::AddTempoChange:
    return transportCommands->addTempoChange(params);
  case CommandID::GetTempoMap:
    return transportCommands->getTempoMap(params);

  case CommandID::AddMarker:
    return addMarker(params);
  case CommandID::GetMarkers:
    return getMarkers(params);
  case CommandID::DeleteMarker:
    return deleteMarker(params);
  case CommandID::GotoMarker:
    return gotoMarker(params);

  case CommandID::AddNote:
    return addNote(params);
  case CommandID::DeleteNote:
    return deleteNote(params);
  case CommandID::MoveNote:
    return moveNote(params);
  case CommandID::GetNotes:
    return getNotes(params);
  case CommandID::SetNoteVelocity:
    return setNoteVelocity(params);
  case CommandID::SetNoteLength:
    return setNoteLength(params);
  case CommandID::GetMidiData:
    return getMidiData(params);
  case CommandID::SetClipNotes:
    return clipCommands->setClipNotes(params);

  case CommandID::ListPresets:
    return listPresets(params);
  case CommandID::LoadPreset:
    return loadPreset(params);
  case CommandID::SavePreset:
    return savePreset(params);
  case CommandID::CreatePreset:
    return createPreset(params);
  case CommandID::DeletePreset:
    return deletePreset(params);
  case CommandID::GeneratePreset:
    return generatePreset(params);

  case CommandID::GetInstrumentParameters:
    return getInstrumentParameters(params);
  case CommandID::SetInstrumentParameter:
    return setInstrumentParameter(params);
  case CommandID::GetInstrumentParameterSchema:
    return getInstrumentParameterSchema(params);

  // Quick Wins: Mixer Control
  case CommandID::SetTrackSend:
    return trackCommands->setTrackSend(params);
  case CommandID::SetTrackEQ:
    return trackCommands->setTrackEQ(params);
  case CommandID::SetTrackCompressor:
    return trackCommands->setTrackCompressor(params);

  default:
    return createErrorResponse("Command ID not implemented: " + commandStr);
  }
}

juce::String CommandAPI::executeCommandString(const juce::String &jsonRequest) {
  // Parse JSON string to var
  juce::var parsedJson;
  auto result = juce::JSON::parse(jsonRequest, parsedJson);

  if (result.failed())
    return juce::JSON::toString(
        createErrorResponse("JSON parse error: " + result.getErrorMessage()));

  // Execute command
  juce::var response = executeCommand(parsedJson);

  // Convert back to string
  return juce::JSON::toString(response);
}

juce::var CommandAPI::executeBatch(const juce::Array<juce::var> &commands,
                                   const juce::String &batchName) {
  if (commands.isEmpty()) {
    return createErrorResponse("Empty command batch");
  }

  DBG("CommandAPI: Executing batch '" + batchName + "' with " +
      juce::String(commands.size()) + " commands");

  // Start a new undo transaction for this batch
  projectState.getUndoManager().beginNewTransaction(batchName);

  int successCount = 0;

  for (int i = 0; i < commands.size(); ++i) {
    const auto &cmdVar = commands[i];

    // Validate command structure
    if (!cmdVar.isObject()) {
      auto *errorObj = new juce::DynamicObject();
      errorObj->setProperty("success", false);
      errorObj->setProperty("error", "Command at index " + juce::String(i) +
                                         " is not a JSON object");
      errorObj->setProperty("failedIndex", i);
      errorObj->setProperty("failedCommand", cmdVar);
      errorObj->setProperty("successCount", successCount);

      return juce::var(errorObj);
    }

    if (!cmdVar.hasProperty("command")) {
      auto *errorObj = new juce::DynamicObject();
      errorObj->setProperty("success", false);
      errorObj->setProperty("error", "Command at index " + juce::String(i) +
                                         " missing 'command' field");
      errorObj->setProperty("failedIndex", i);
      errorObj->setProperty("failedCommand", cmdVar);
      errorObj->setProperty("successCount", successCount);

      return juce::var(errorObj);
    }

    // Execute command
    juce::var response = executeCommand(cmdVar);

    // Check for error
    bool success =
        response.hasProperty("success") ? (bool)response["success"] : false;

    if (!success) {
      // Command failed - stop batch execution
      juce::String error = response.hasProperty("error")
                               ? response["error"].toString()
                               : "Unknown error";

      auto *errorObj = new juce::DynamicObject();
      errorObj->setProperty("success", false);
      errorObj->setProperty("error", "Command failed: " + error);
      errorObj->setProperty("failedIndex", i);
      errorObj->setProperty("failedCommand", cmdVar);
      errorObj->setProperty("successCount", successCount);

      DBG("CommandAPI: Batch failed at command " + juce::String(i) + ": " +
          error);

      return juce::var(errorObj);
    }

    successCount++;
  }

  // All commands succeeded
  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("count", successCount);

  DBG("CommandAPI: Batch completed successfully (" +
      juce::String(successCount) + " commands)");

  return createSuccessResponse(juce::var(resultObj));
}

//==============================================================================
// Command Handlers
//==============================================================================

// list_clips command

//==============================================================================
// Transport Commands
//==============================================================================

juce::var CommandAPI::exportAudio(const juce::var &params) {
  if (!params.hasProperty("outputPath"))
    return createErrorResponse("Missing 'outputPath' parameter");

  const juce::String outputPath = params["outputPath"].toString();
  const double sampleRate = params.hasProperty("sampleRate")
                                ? static_cast<double>(params["sampleRate"])
                                : 44100.0;
  const int bitDepth = params.hasProperty("bitDepth")
                           ? static_cast<int>(params["bitDepth"])
                           : 24;
  const double durationSeconds =
      params.hasProperty("durationSeconds")
          ? static_cast<double>(params["durationSeconds"])
          : 0.0;

  if (outputPath.isEmpty())
    return createErrorResponse("outputPath cannot be empty");

  const bool success = engine.exportProjectToWav(
      juce::File(outputPath), sampleRate, bitDepth, durationSeconds);

  if (!success)
    return createErrorResponse("Export failed. Check engine logs for details.");

  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("outputPath", outputPath);
  resultObj->setProperty("sampleRate", sampleRate);
  resultObj->setProperty("bitDepth", bitDepth);
  resultObj->setProperty("durationSeconds", durationSeconds);

  DBG("CommandAPI: Exported audio to " + outputPath);

  return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::getSessionGraph(const juce::var &params) {
  juce::ignoreUnused(params);

  // Use SessionGraph to generate full project state
  SessionGraph graph(engine, projectState);
  juce::var graphData = graph.generateGraph();

  DBG("CommandAPI: Generated session graph");

  return createSuccessResponse(graphData);
}

juce::var CommandAPI::undo(const juce::var &params) {
  juce::ignoreUnused(params);

  if (!projectState.canUndo())
    return createErrorResponse("Nothing to undo");

  // Get current action name (if available) before undoing
  // Note: JUCE UndoManager doesn't expose action names easily,
  // so we'll just report success
  projectState.undo();

  DBG("CommandAPI: Undo executed");

  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("undone", true);
  resultObj->setProperty("message", "Undo successful");

  return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::redo(const juce::var &params) {
  juce::ignoreUnused(params);

  if (!projectState.canRedo())
    return createErrorResponse("Nothing to redo");

  projectState.redo();

  // JUCE UndoManager doesn't provide easy access to action history
  // For now, return basic undo/redo availability
  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("canUndo", projectState.canUndo());
  resultObj->setProperty("canRedo", projectState.canRedo());
  resultObj->setProperty("message",
                         "Full history tracking not yet implemented");

  DBG("CommandAPI: History query");

  return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::history(const juce::var &params) {
  juce::ignoreUnused(params);

  // Return undo/redo history status
  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("canUndo", projectState.canUndo());
  resultObj->setProperty("canRedo", projectState.canRedo());

  // Get next undo/redo descriptions
  juce::var undoStack;
  juce::var redoStack;

  if (projectState.canUndo()) {
    undoStack.append(projectState.getUndoManager().getUndoDescription());
  }

  if (projectState.canRedo()) {
    redoStack.append(projectState.getUndoManager().getRedoDescription());
  }

  resultObj->setProperty("undoStack", undoStack);
  resultObj->setProperty("redoStack", redoStack);

  return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::describeInstrument(const juce::var &params) {
  // Validate params
  if (!params.hasProperty("instrumentId"))
    return createErrorResponse("Missing 'instrumentId' parameter");

  juce::String instrumentId = params["instrumentId"].toString();

  // Get InstrumentRegistry
  auto &registry = engine.getInstrumentRegistry();

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
// Helper methods moved to CommandUtils.h

//==============================================================================
// Plugin Commands
//==============================================================================

juce::var CommandAPI::listPlugins(const juce::var &params) {
  juce::ignoreUnused(params);

  juce::var pluginsArray;
  auto *pluginsArrayPtr = pluginsArray.getArray();

  const auto &knownPlugins = engine.getPluginHost().getKnownPlugins();

  for (int i = 0; i < knownPlugins.getNumTypes(); ++i) {
    auto desc = knownPlugins.getTypes()[i];
    auto *pluginObj = new juce::DynamicObject();
    pluginObj->setProperty("id", juce::var(desc.createIdentifierString()));
    pluginObj->setProperty("name", juce::var(desc.name));
    pluginObj->setProperty("manufacturer", juce::var(desc.manufacturerName));
    pluginObj->setProperty("format", juce::var(desc.pluginFormatName));
    pluginObj->setProperty("category", juce::var(desc.category));

    pluginsArrayPtr->add(juce::var(pluginObj));
  }

  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("plugins", pluginsArray);
  resultObj->setProperty("count", knownPlugins.getNumTypes());

  return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::addPlugin(const juce::var &params) {
  if (!params.hasProperty("trackId"))
    return createErrorResponse("Missing 'trackId' parameter");
  if (!params.hasProperty("pluginId"))
    return createErrorResponse("Missing 'pluginId' parameter");

  juce::String trackId = params["trackId"].toString();
  juce::String pluginId = params["pluginId"].toString();

  Track *track = findTrackById(engine, trackId);
  if (track == nullptr)
    return createErrorResponse("Track not found: " + trackId);

  // Create plugin instance
  juce::String errorMessage;
  auto instance = engine.getPluginHost().createInstance(
      pluginId, engine.getSampleRate(), engine.getBufferSize(), errorMessage);

  if (instance == nullptr)
    return createErrorResponse("Failed to create plugin: " + errorMessage);

  // Add to track
  juce::String pluginName = instance->getName();
  track->addPlugin(std::move(instance));

  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("trackId", trackId);
  resultObj->setProperty("pluginName", pluginName);
  resultObj->setProperty("success", true);

  DBG("CommandAPI: Added plugin " + pluginName + " to " + trackId);

  return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::removePlugin(const juce::var &params) {
  if (!params.hasProperty("trackId"))
    return createErrorResponse("Missing 'trackId' parameter");
  if (!params.hasProperty("pluginIndex"))
    return createErrorResponse("Missing 'pluginIndex' parameter");

  juce::String trackId = params["trackId"].toString();
  int pluginIndex = params["pluginIndex"];

  Track *track = findTrackById(engine, trackId);
  if (track == nullptr)
    return createErrorResponse("Track not found: " + trackId);

  if (pluginIndex < 0 || pluginIndex >= track->getNumPlugins())
    return createErrorResponse("Invalid plugin index");

  track->removePlugin(pluginIndex);

  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("trackId", trackId);
  resultObj->setProperty("pluginIndex", pluginIndex);
  resultObj->setProperty("success", true);

  DBG("CommandAPI: Removed plugin " + juce::String(pluginIndex) + " from " +
      trackId);

  return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::setPluginParam(const juce::var &params) {
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
  Track *track = findTrackById(engine, trackId);
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
      auto *resultObj = new juce::DynamicObject();
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
  auto *plugin = track->getPlugin(pluginIndex);
  if (plugin == nullptr)
    return createErrorResponse("Plugin not found at index " +
                               juce::String(pluginIndex));

  auto parameters = plugin->getParameters();
  if (paramIndex < 0 || paramIndex >= parameters.size())
    return createErrorResponse("Invalid parameter index");

  auto *param = parameters[paramIndex];
  if (param != nullptr) {
    param->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, value));
  }

  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("trackId", trackId);
  resultObj->setProperty("pluginIndex", pluginIndex);
  resultObj->setProperty("paramIndex", paramIndex);
  resultObj->setProperty("value", value);
  resultObj->setProperty("success", true);
  resultObj->setProperty("mode", "fallback");

  return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::getPluginParams(const juce::var &params) {
  if (!params.hasProperty("trackId"))
    return createErrorResponse("Missing 'trackId' parameter");
  if (!params.hasProperty("pluginIndex"))
    return createErrorResponse("Missing 'pluginIndex' parameter");

  juce::String trackId = params["trackId"].toString();
  int pluginIndex = params["pluginIndex"];

  Track *track = findTrackById(engine, trackId);
  if (track == nullptr)
    return createErrorResponse("Track not found: " + trackId);

  auto *plugin = track->getPlugin(pluginIndex);
  if (plugin == nullptr)
    return createErrorResponse("Plugin not found at index " +
                               juce::String(pluginIndex));

  juce::var paramsArray;
  auto *paramsArrayPtr = paramsArray.getArray();

  auto parameters = plugin->getParameters();
  for (int i = 0; i < parameters.size(); ++i) {
    auto *param = parameters[i];
    if (param != nullptr) {
      auto *paramObj = new juce::DynamicObject();
      paramObj->setProperty("index", i);
      paramObj->setProperty("name", param->getName(100));
      paramObj->setProperty("value", param->getValue());
      paramObj->setProperty("label", juce::String(param->getLabel()));
      paramObj->setProperty("numSteps", param->getNumSteps());
      paramObj->setProperty("isDiscrete", param->isDiscrete());

      paramsArrayPtr->add(juce::var(paramObj));
    }
  }

  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("trackId", trackId);
  resultObj->setProperty("pluginIndex", pluginIndex);
  resultObj->setProperty("parameters", paramsArray);
  resultObj->setProperty("count", parameters.size());

  return createSuccessResponse(juce::var(resultObj));
}

//==============================================================================
// Automation Commands
//==============================================================================

juce::var CommandAPI::addAutomationPoint(const juce::var &params) {
  if (!params.hasProperty("trackId"))
    return createErrorResponse("Missing 'trackId'");
  if (!params.hasProperty("paramId"))
    return createErrorResponse("Missing 'paramId'");
  if (!params.hasProperty("timeBeats"))
    return createErrorResponse("Missing 'timeBeats'");
  if (!params.hasProperty("value"))
    return createErrorResponse("Missing 'value'");

  juce::String trackId = params["trackId"].toString();
  juce::String paramId = params["paramId"].toString();
  double timeBeats = (double)params["timeBeats"];
  double value = (double)params["value"];

  // Validate paramId
  if (paramId != "volume" && paramId != "pan" && paramId != "mute")
    return createErrorResponse(
        "Invalid paramId: must be 'volume', 'pan', or 'mute'");

  juce::String pointId = projectState.addAutomationPoint(
      trackId, paramId, timeBeats, value, "Add Automation Point");

  if (pointId.isEmpty())
    return createErrorResponse("Failed to add automation point");

  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("pointId", pointId);
  resultObj->setProperty("success", true);

  return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::clearAutomation(const juce::var &params) {
  if (!params.hasProperty("trackId"))
    return createErrorResponse("Missing 'trackId'");
  if (!params.hasProperty("paramId"))
    return createErrorResponse("Missing 'paramId'");

  juce::String trackId = params["trackId"].toString();
  juce::String paramId = params["paramId"].toString();

  bool success =
      projectState.clearAutomation(trackId, paramId, "Clear Automation");

  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("success", success);

  return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::getAutomation(const juce::var &params) {
  if (!params.hasProperty("trackId"))
    return createErrorResponse("Missing 'trackId'");
  if (!params.hasProperty("paramId"))
    return createErrorResponse("Missing 'paramId'");

  juce::String trackId = params["trackId"].toString();
  juce::String paramId = params["paramId"].toString();

  auto envelope = projectState.getAutomationEnvelope(trackId, paramId);

  juce::var pointsArray;
  auto *pointsArrayPtr = pointsArray.getArray();

  if (envelope.isValid()) {
    // The envelope contains a POINTS container (ID_POINT) which contains the
    // actual points (ID_POINT) See ProjectState structure discussion
    auto pointsContainer = envelope.getChildWithName(ProjectState::ID_POINT);

    if (pointsContainer.isValid()) {
      for (const auto &point : pointsContainer) {
        if (point.hasType(ProjectState::ID_POINT)) {
          auto *pointObj = new juce::DynamicObject();
          pointObj->setProperty("id", point.getProperty(ProjectState::PROP_ID));
          pointObj->setProperty(
              "timeBeats", point.getProperty(ProjectState::PROP_TIME_BEATS));
          pointObj->setProperty("value",
                                point.getProperty(ProjectState::PROP_VALUE));

          pointsArrayPtr->add(juce::var(pointObj));
        }
      }
    }
  }

  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("trackId", trackId);
  resultObj->setProperty("paramId", paramId);
  resultObj->setProperty("points", pointsArray);
  resultObj->setProperty("count", pointsArray.size());

  return createSuccessResponse(juce::var(resultObj));
}

//==============================================================================
// Tempo/Marker Commands
//==============================================================================

juce::var CommandAPI::addMarker(const juce::var &params) {
  if (!params.hasProperty("timeBeats"))
    return createErrorResponse("Missing 'timeBeats'");
  if (!params.hasProperty("name"))
    return createErrorResponse("Missing 'name'");

  double timeBeats = (double)params["timeBeats"];
  juce::String name = params["name"].toString();
  juce::String color = params.hasProperty("color") ? params["color"].toString()
                                                   : juce::String("FF0000");

  juce::String markerId =
      projectState.addMarker(timeBeats, name, color, "Wingman: Add Marker");

  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("markerId", markerId);
  resultObj->setProperty("success", true);

  return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::getMarkers(const juce::var &params) {
  juce::ignoreUnused(params);

  juce::var markersArray;
  auto *markersArrayPtr = markersArray.getArray();

  auto markers = projectState.getMarkers();
  if (markers.isValid()) {
    for (const auto &marker : markers) {
      if (marker.hasType(ProjectState::ID_MARKER)) {
        auto *markerObj = new juce::DynamicObject();
        markerObj->setProperty("id", marker.getProperty(ProjectState::PROP_ID));
        markerObj->setProperty(
            "timeBeats", marker.getProperty(ProjectState::PROP_TIME_BEATS));
        markerObj->setProperty("name",
                               marker.getProperty(ProjectState::PROP_NAME));
        markerObj->setProperty("color",
                               marker.getProperty(ProjectState::PROP_COLOR));
        markersArrayPtr->add(juce::var(markerObj));
      }
    }
  }

  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("markers", markersArray);

  return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::deleteMarker(const juce::var &params) {
  if (!params.hasProperty("markerId"))
    return createErrorResponse("Missing 'markerId'");

  juce::String markerId = params["markerId"].toString();
  bool success = projectState.deleteMarker(markerId, "Wingman: Delete Marker");

  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("success", success);

  return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::gotoMarker(const juce::var &params) {
  if (!params.hasProperty("markerId"))
    return createErrorResponse("Missing 'markerId'");

  juce::String markerId = params["markerId"].toString();

  auto markers = projectState.getMarkers();
  if (markers.isValid()) {
    for (const auto &marker : markers) {
      if (marker[ProjectState::PROP_ID].toString() == markerId) {
        double timeBeats = marker[ProjectState::PROP_TIME_BEATS];

        // Convert beats to samples using TempoMap
        double sampleRate = engine.getSampleRate();
        juce::int64 timeSamples =
            engine.getTempoMap().beatsToSamples(timeBeats, sampleRate);

        // Set playhead position
        engine.setPlayheadSamples(timeSamples);

        auto *resultObj = new juce::DynamicObject();
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

juce::var CommandAPI::addNote(const juce::var &params) {
  if (!params.hasProperty("clipId"))
    return createErrorResponse("Missing 'clipId'");
  if (!params.hasProperty("pitch"))
    return createErrorResponse("Missing 'pitch'");
  if (!params.hasProperty("startBeats"))
    return createErrorResponse("Missing 'startBeats'");
  if (!params.hasProperty("lengthBeats"))
    return createErrorResponse("Missing 'lengthBeats'");
  if (!params.hasProperty("velocity"))
    return createErrorResponse("Missing 'velocity'");

  juce::String clipId = params["clipId"].toString();

  ProjectState::MidiNoteSpec note;
  note.pitch = (int)params["pitch"];
  note.startBeats = (double)params["startBeats"];
  note.lengthBeats = (double)params["lengthBeats"];
  note.velocity = (int)params["velocity"];

  juce::String noteId =
      projectState.addMidiNote(clipId, note, "Wingman: Add Note");

  if (noteId.isEmpty())
    return createErrorResponse("Failed to add note");

  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("noteId", noteId);
  resultObj->setProperty("success", true);

  return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::moveNote(const juce::var &params) {
  if (!params.hasProperty("clipId"))
    return createErrorResponse("Missing 'clipId'");
  if (!params.hasProperty("noteId"))
    return createErrorResponse("Missing 'noteId'");
  if (!params.hasProperty("newStartBeats"))
    return createErrorResponse("Missing 'newStartBeats'");
  if (!params.hasProperty("newPitch"))
    return createErrorResponse("Missing 'newPitch'");

  juce::String clipId = params["clipId"].toString();
  juce::String noteId = params["noteId"].toString();
  double newStartBeats = (double)params["newStartBeats"];
  int newPitch = (int)params["newPitch"];

  projectState.moveMidiNote(clipId, noteId, newStartBeats, newPitch,
                            "Wingman: Move Note");

  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("success", true);

  return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::deleteNote(const juce::var &params) {
  if (!params.hasProperty("clipId"))
    return createErrorResponse("Missing 'clipId'");
  if (!params.hasProperty("noteId"))
    return createErrorResponse("Missing 'noteId'");

  juce::String clipId = params["clipId"].toString();
  juce::String noteId = params["noteId"].toString();

  projectState.removeMidiNote(clipId, noteId, "Wingman: Delete Note");

  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("success", true);

  return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::getNotes(const juce::var &params) {
  if (!params.hasProperty("clipId"))
    return createErrorResponse("Missing 'clipId'");

  juce::String clipId = params["clipId"].toString();
  auto notes = projectState.getMidiNotesForClip(clipId);

  juce::var notesArray;
  auto *notesArrayPtr = notesArray.getArray();

  for (const auto &note : notes) {
    auto *noteObj = new juce::DynamicObject();
    noteObj->setProperty("id", note.id);
    noteObj->setProperty("pitch", note.pitch);
    noteObj->setProperty("startBeats", note.startBeats);
    noteObj->setProperty("lengthBeats", note.lengthBeats);
    noteObj->setProperty("velocity", note.velocity);
    noteObj->setProperty("muted", note.muted);

    notesArrayPtr->add(juce::var(noteObj));
  }

  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("clipId", clipId);
  resultObj->setProperty("notes", notesArray);
  resultObj->setProperty("count", notes.size());

  return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::setNoteVelocity(const juce::var &params) {
  if (!params.hasProperty("clipId"))
    return createErrorResponse("Missing 'clipId'");
  if (!params.hasProperty("noteId"))
    return createErrorResponse("Missing 'noteId'");
  if (!params.hasProperty("velocity"))
    return createErrorResponse("Missing 'velocity'");

  juce::String clipId = params["clipId"].toString();
  juce::String noteId = params["noteId"].toString();
  int velocity = (int)params["velocity"];

  projectState.setMidiNoteVelocity(clipId, noteId, velocity,
                                   "Wingman: Set Velocity");

  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("success", true);

  return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::setNoteLength(const juce::var &params) {
  if (!params.hasProperty("clipId"))
    return createErrorResponse("Missing 'clipId'");
  if (!params.hasProperty("noteId"))
    return createErrorResponse("Missing 'noteId'");
  if (!params.hasProperty("lengthBeats"))
    return createErrorResponse("Missing 'lengthBeats'");

  juce::String clipId = params["clipId"].toString();
  juce::String noteId = params["noteId"].toString();
  double lengthBeats = (double)params["lengthBeats"];

  projectState.setMidiNoteLength(clipId, noteId, lengthBeats,
                                 "Wingman: Set Length");

  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("success", true);

  return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::getMidiData(const juce::var &params) {
  const juce::String trackFilter = params.hasProperty("trackId")
                                       ? params["trackId"].toString()
                                       : juce::String();
  const juce::String clipFilter = params.hasProperty("clipId")
                                      ? params["clipId"].toString()
                                      : juce::String();

  juce::var tracksVar;
  auto *tracksArray = tracksVar.getArray();

  for (int i = 0; i < projectState.getNumTracks(); ++i) {
    auto trackTree = projectState.getTrackByIndex(i);
    if (!trackTree.isValid())
      continue;

    juce::String trackId =
        trackTree.getProperty(ProjectState::PROP_ID, "").toString();
    if (trackFilter.isNotEmpty() && trackFilter != trackId)
      continue;

    juce::String trackType =
        trackTree.getProperty(ProjectState::PROP_TYPE, "audio").toString();
    auto clipsNode = trackTree.getChildWithName(ProjectState::ID_CLIPS);
    if (!clipsNode.isValid())
      continue;

    juce::var clipsVar;
    auto *clipsArray = clipsVar.getArray();

    for (const auto &clip : clipsNode) {
      if (!clip.hasType(ProjectState::ID_CLIP))
        continue;

      const juce::String clipId =
          clip.getProperty(ProjectState::PROP_ID).toString();
      if (clipFilter.isNotEmpty() && clipFilter != clipId)
        continue;

      const juce::String clipType =
          clip.getProperty(ProjectState::PROP_TYPE).toString();
      if (clipType != "midi")
        continue;

      auto notes = projectState.getMidiNotesForClip(clipId);

      juce::var notesVar;
      auto *notesArray = notesVar.getArray();
      for (const auto &note : notes) {
        auto *noteObj = new juce::DynamicObject();
        noteObj->setProperty("id", note.id);
        noteObj->setProperty("pitch", note.pitch);
        noteObj->setProperty("startBeats", note.startBeats);
        noteObj->setProperty("lengthBeats", note.lengthBeats);
        noteObj->setProperty("velocity", note.velocity);
        noteObj->setProperty("muted", note.muted);
        notesArray->add(juce::var(noteObj));
      }

      auto *clipObj = new juce::DynamicObject();
      clipObj->setProperty("id", clipId);
      clipObj->setProperty("trackId", trackId);
      clipObj->setProperty("startBeats",
                           clip.getProperty(ProjectState::PROP_START_BEATS));
      clipObj->setProperty("lengthBeats",
                           clip.getProperty(ProjectState::PROP_LENGTH_BEATS));
      clipObj->setProperty("laneIndex",
                           clip.getProperty(ProjectState::PROP_LANE_INDEX));
      clipObj->setProperty("notes", notesVar);
      clipObj->setProperty("noteCount", notes.size());

      clipsArray->add(juce::var(clipObj));
    }

    if (clipsArray->isEmpty())
      continue;

    auto *trackObj = new juce::DynamicObject();
    trackObj->setProperty("id", trackId);
    trackObj->setProperty("type", trackType);
    trackObj->setProperty("clips", clipsVar);
    trackObj->setProperty("clipCount", clipsArray->size());

    tracksArray->add(juce::var(trackObj));
  }

  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("tracks", tracksVar);
  resultObj->setProperty("trackCount", tracksVar.getArray()->size());

  return createSuccessResponse(juce::var(resultObj));
}

//==============================================================================
// Preset Management Commands (NEW - for Grok AI)
//==============================================================================

juce::var CommandAPI::listPresets(const juce::var &params) {
  // Validate params
  if (!params.hasProperty("instrumentId"))
    return createErrorResponse("Missing 'instrumentId' parameter");

  juce::String instrumentId = params["instrumentId"].toString();

  // Get InstrumentRegistry
  auto &registry = engine.getInstrumentRegistry();

  // Check if instrument exists
  InstrumentMetadata metadata;
  if (!registry.getMetadata(instrumentId, metadata))
    return createErrorResponse("Instrument not found: " + instrumentId);

  // Get presets for this instrument
  auto presets = registry.getPresetsForInstrument(instrumentId);

  // Build presets array
  juce::var presetsArray;
  auto *presetsArrayPtr = presetsArray.getArray();

  for (const auto &preset : presets) {
    auto *presetObj = new juce::DynamicObject();
    presetObj->setProperty("name", preset.name);
    presetObj->setProperty("category", preset.category);
    presetObj->setProperty("description", preset.description);
    presetObj->setProperty("author", preset.author);
    presetObj->setProperty("tags", preset.tags);

    presetsArrayPtr->add(juce::var(presetObj));
  }

  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("instrumentId", instrumentId);
  resultObj->setProperty("presets", presetsArray);
  resultObj->setProperty("count", (int)presets.size());

  DBG("CommandAPI: Listed " + juce::String(presets.size()) + " presets for " +
      instrumentId);

  return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::loadPreset(const juce::var &params) {
  // Validate params
  if (!params.hasProperty("trackId"))
    return createErrorResponse("Missing 'trackId' parameter");
  if (!params.hasProperty("presetName"))
    return createErrorResponse("Missing 'presetName' parameter");

  juce::String trackId = params["trackId"].toString();
  juce::String presetName = params["presetName"].toString();

  // Find track
  Track *track = findTrackById(engine, trackId);
  if (track == nullptr)
    return createErrorResponse("Track not found: " + trackId);

  // Get instrument from track
  auto *instrument = track->getInstrument();
  if (instrument == nullptr)
    return createErrorResponse("Track has no instrument loaded");

  // Get instrument ID
  juce::String instrumentId = instrument->getMetadata().instrumentId;

  // Get InstrumentRegistry
  auto &registry = engine.getInstrumentRegistry();

  // Find preset
  InstrumentPreset preset;
  if (!registry.getPreset(instrumentId, presetName, preset))
    return createErrorResponse("Preset not found: " + presetName +
                               " for instrument " + instrumentId);

  // Convert to ZenithInstrumentPreset
  ZenithInstrumentPreset zenithPreset;
  zenithPreset.name = preset.name.toStdString();
  zenithPreset.category = preset.category.toStdString();
  zenithPreset.description = preset.description.toStdString();
  zenithPreset.author = preset.author.toStdString();

  if (preset.parameters.isObject()) {
    auto paramsObj = preset.parameters.getDynamicObject();
    for (auto &prop : paramsObj->getProperties()) {
      zenithPreset.setParameter(prop.name.toString().toStdString(),
                                (float)prop.value);
    }
  }

  // Load preset into instrument
  instrument->applyPreset(zenithPreset);

  DBG("CommandAPI: Loaded preset '" + presetName + "' on track " + trackId);

  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("trackId", trackId);
  resultObj->setProperty("instrumentId", instrumentId);
  resultObj->setProperty("presetName", presetName);
  resultObj->setProperty("loaded", true);

  return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::savePreset(const juce::var &params) {
  // Validate params
  if (!params.hasProperty("trackId"))
    return createErrorResponse("Missing 'trackId' parameter");
  if (!params.hasProperty("presetName"))
    return createErrorResponse("Missing 'presetName' parameter");

  juce::String trackId = params["trackId"].toString();
  juce::String presetName = params["presetName"].toString();
  juce::String category =
      params.hasProperty("category") ? params["category"].toString() : "User";
  juce::String description =
      params.hasProperty("description") ? params["description"].toString() : "";

  // Find track
  Track *track = findTrackById(engine, trackId);
  if (track == nullptr)
    return createErrorResponse("Track not found: " + trackId);

  // Get instrument from track
  auto *instrument = track->getInstrument();
  if (instrument == nullptr)
    return createErrorResponse("Track has no instrument loaded");

  // Get current instrument state as preset
  InstrumentPreset preset;
  preset.name = presetName;
  preset.category = category;
  preset.description = description;
  preset.author = "Grok AI";
  // Build parameters object
  auto *paramsObj = new juce::DynamicObject();
  const auto &metadata = instrument->getMetadata();
  for (const auto &param : metadata.parameters) {
    paramsObj->setProperty(param.id, instrument->getParameter(param.id));
  }
  preset.parameters = juce::var(paramsObj);

  // Get instrument ID
  juce::String instrumentId = metadata.instrumentId;

  // Save to registry
  auto &registry = engine.getInstrumentRegistry();
  registry.addPreset(instrumentId, preset);

  DBG("CommandAPI: Saved preset '" + presetName + "' from track " + trackId);

  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("trackId", trackId);
  resultObj->setProperty("instrumentId", instrumentId);
  resultObj->setProperty("presetName", presetName);
  resultObj->setProperty("saved", true);

  return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::createPreset(const juce::var &params) {
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
  juce::String category = params.hasProperty("category")
                              ? params["category"].toString()
                              : "AI Generated";
  juce::String description = params.hasProperty("description")
                                 ? params["description"].toString()
                                 : "Generated by Grok AI";

  // Validate parameters is an object
  if (!parameters.isObject())
    return createErrorResponse("'parameters' must be a JSON object");

  // Get InstrumentRegistry
  auto &registry = engine.getInstrumentRegistry();

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
  for (auto &prop : parameters.getDynamicObject()->getProperties()) {
    if (prop.value.isDouble() || prop.value.isInt()) {
      float val = static_cast<float>(prop.value);
      // Most params are normalized 0-1, but some (like detune) might not be.
      // For now, we just warn if values are extreme.
      if (std::abs(val) > 10000.0f) {
        return createErrorResponse("Parameter value out of reasonable range: " +
                                   prop.name.toString());
      }
    }
  }

  // Add to registry
  registry.addPreset(instrumentId, preset);

  DBG("CommandAPI: Created preset '" + presetName + "' for " + instrumentId);

  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("instrumentId", instrumentId);
  resultObj->setProperty("presetName", presetName);
  resultObj->setProperty("category", category);
  resultObj->setProperty("created", true);

  return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::deletePreset(const juce::var &params) {
  // Validate params
  if (!params.hasProperty("instrumentId"))
    return createErrorResponse("Missing 'instrumentId' parameter");
  if (!params.hasProperty("presetName"))
    return createErrorResponse("Missing 'presetName' parameter");

  juce::String instrumentId = params["instrumentId"].toString();
  juce::String presetName = params["presetName"].toString();

  // Get InstrumentRegistry
  auto &registry = engine.getInstrumentRegistry();

  // Delete preset
  bool deleted = registry.deletePreset(instrumentId, presetName);

  if (!deleted)
    return createErrorResponse("Preset not found or could not be deleted: " +
                               presetName);

  DBG("CommandAPI: Deleted preset '" + presetName + "' from " + instrumentId);

  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("instrumentId", instrumentId);
  resultObj->setProperty("presetName", presetName);
  resultObj->setProperty("deleted", true);

  return createSuccessResponse(juce::var(resultObj));
}

//==============================================================================
// Instrument Parameter Control (NEW - for Grok AI)
//==============================================================================

juce::var CommandAPI::getInstrumentParameters(const juce::var &params) {
  // Validate params
  if (!params.hasProperty("trackId"))
    return createErrorResponse("Missing 'trackId' parameter");

  juce::String trackId = params["trackId"].toString();

  // Find track
  Track *track = findTrackById(engine, trackId);
  if (track == nullptr)
    return createErrorResponse("Track not found: " + trackId);

  // Get instrument from track
  auto *instrument = track->getInstrument();
  if (instrument == nullptr)
    return createErrorResponse("Track has no instrument loaded");

  // Get current parameters
  auto *paramsObj = new juce::DynamicObject();
  const auto &metadata = instrument->getMetadata();
  for (const auto &param : metadata.parameters) {
    paramsObj->setProperty(param.id, instrument->getParameter(param.id));
  }
  juce::var parameters = juce::var(paramsObj);

  // Get instrument metadata
  juce::String instrumentId = metadata.instrumentId;

  DBG("CommandAPI: Retrieved parameters for instrument on track " + trackId);

  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("trackId", trackId);
  resultObj->setProperty("instrumentId", instrumentId);
  resultObj->setProperty("parameters", parameters);

  return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::setInstrumentParameter(const juce::var &params) {
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
  Track *track = findTrackById(engine, trackId);
  if (track == nullptr)
    return createErrorResponse("Track not found: " + trackId);

  // Get instrument from track
  auto *instrument = track->getInstrument();
  if (instrument == nullptr)
    return createErrorResponse("Track has no instrument loaded");

  // Set parameter
  bool success = instrument->setParameter(paramName, value);

  if (!success)
    return createErrorResponse("Failed to set parameter: " + paramName +
                               " (parameter may not exist)");

  DBG("CommandAPI: Set parameter '" + paramName + "' on track " + trackId);

  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("trackId", trackId);
  resultObj->setProperty("parameterName", paramName);
  resultObj->setProperty("value", value);
  resultObj->setProperty("set", true);

  return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::getInstrumentParameterSchema(const juce::var &params) {
  // Validate params
  if (!params.hasProperty("instrumentId"))
    return createErrorResponse("Missing 'instrumentId' parameter");

  juce::String instrumentId = params["instrumentId"].toString();

  // Get InstrumentRegistry
  auto &registry = engine.getInstrumentRegistry();

  // Check if instrument exists
  InstrumentMetadata metadata;
  if (!registry.getMetadata(instrumentId, metadata))
    return createErrorResponse("Instrument not found: " + instrumentId);

  // Get parameter schema
  juce::var schema = registry.getParameterSchema(instrumentId);

  DBG("CommandAPI: Retrieved parameter schema for " + instrumentId);

  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("instrumentId", instrumentId);
  resultObj->setProperty("schema", schema);

  return createSuccessResponse(juce::var(resultObj));
}

//==============================================================================
// AI Preset Generation (NEW - for Grok AI)
//==============================================================================

juce::var CommandAPI::generatePreset(const juce::var &params) {
  // Validate params
  if (!params.hasProperty("instrumentId"))
    return createErrorResponse("Missing 'instrumentId' parameter");
  if (!params.hasProperty("description"))
    return createErrorResponse("Missing 'description' parameter");

  juce::String instrumentId = params["instrumentId"].toString();
  juce::String description = params["description"].toString();
  juce::String genre =
      params.hasProperty("genre") ? params["genre"].toString() : "";

  // Get InstrumentRegistry
  auto &registry = engine.getInstrumentRegistry();

  // Check if instrument exists
  InstrumentMetadata metadata;
  if (!registry.getMetadata(instrumentId, metadata))
    return createErrorResponse("Instrument not found: " + instrumentId);

  // This function is called BY Grok after it generates parameters
  // The actual AI generation happens in GrokDAWController
  // This function validates and creates the preset

  // For now, return error - this should be called with generated parameters
  if (!params.hasProperty("generatedParameters")) {
    return createErrorResponse("This function should be called with "
                               "'generatedParameters' from Grok AI. "
                               "Use GrokDAWController.generatePreset() instead "
                               "for full AI generation.");
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

  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("instrumentId", instrumentId);
  resultObj->setProperty("presetName", presetName);
  resultObj->setProperty("description", description);
  resultObj->setProperty("parameters", generatedParams);
  resultObj->setProperty("generated", true);

  return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::exportProjectAdvanced(const juce::var &params) {
  if (!params.hasProperty("outputPath"))
    return createErrorResponse("Missing 'outputPath' parameter");

  Engine::ExportOptions opts;
  opts.outputFile = juce::File(params["outputPath"].toString());

  if (params.hasProperty("sampleRate"))
    opts.sampleRate = static_cast<double>(params["sampleRate"]);

  if (params.hasProperty("bitDepth"))
    opts.bitDepth = static_cast<int>(params["bitDepth"]);

  if (params.hasProperty("dither"))
    opts.enableDither = static_cast<bool>(params["dither"]);

  if (params.hasProperty("normalize"))
    opts.normalize = static_cast<bool>(params["normalize"]);

  if (params.hasProperty("normalizeDb"))
    opts.normalizeDb = static_cast<double>(params["normalizeDb"]);

  if (params.hasProperty("durationSeconds"))
    opts.duration = static_cast<double>(params["durationSeconds"]);

  if (params.hasProperty("format")) {
    juce::String fmt = params["format"].toString().toLowerCase();
    if (fmt == "flac")
      opts.format = Engine::ExportFormat::FLAC;
    else if (fmt == "ogg")
      opts.format = Engine::ExportFormat::OGG;
    else
      opts.format = Engine::ExportFormat::WAV;
  }

  // AI Enhance Trigger
  if (params.hasProperty("aiEnhance") &&
      static_cast<bool>(params["aiEnhance"])) {
    zenith::ai::AIMasteringAgent agent(engine);
    zenith::ai::AIMasteringAgent::MasteringOptions masterOpts;
    masterOpts.target8Bit = (opts.bitDepth == 8);
    agent.runMasteringPass(masterOpts);
  }

  bool success = engine.exportProject(opts);

  if (!success)
    return createErrorResponse("Export failed");

  return createSuccessResponse(juce::var());
}

juce::var CommandAPI::syncProjectToCloud(const juce::var& params) {
    // 1. Get Project File
    auto projectFile = projectState.getProjectFile();
    if (!projectFile.existsAsFile()) {
        return createErrorResponse("Project must be saved locally before syncing to cloud.");
    }

    // 2. Get Auth Token
    // In a production environment, retrieve this from a secure credential store or SessionManager
    juce::String token = ""; 
    if (params.hasProperty("token")) {
        token = params["token"].toString();
    } else {
        // Fallback for testing
        token = "test_token";
    }

    // 3. Prepare Request
    // Use localhost for the accompanying auth service
    juce::URL url("http://localhost:5000/api/projects/upload");
    
    url = url.withFileToUpload("projectFile", projectFile, "application/octet-stream");
    url = url.withParameter("name", projectState.getProjectName());
    url = url.withParameter("description", "Synced from Zenith DAW");
    url = url.withParameter("isPublic", "false");

    // 4. Execute Upload
    // Note: This is a blocking call. In a UI context, run this in a Thread or Task.
    int statusCode = 0;
    std::unique_ptr<juce::InputStream> stream = url.createInputStream(
        juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inPostData)
            .withExtraHeaders("Authorization: Bearer " + token)
            .withConnectionTimeoutMs(30000) // 30s timeout for uploads
            .withStatusCode(&statusCode)
    );

    if (stream != nullptr && (statusCode == 200 || statusCode == 201)) {
        juce::String responseText = stream->readEntireStreamAsString();
        auto jsonResponse = juce::JSON::parse(responseText);
        DBG("Cloud Sync Successful: " + responseText);
        return createSuccessResponse(jsonResponse);
    } else {
        juce::String errorMsg = "Upload failed. Status: " + juce::String(statusCode);
        if (stream) {
            errorMsg += " Response: " + stream->readEntireStreamAsString();
        }
        DBG(errorMsg);
        return createErrorResponse(errorMsg);
    }
}

//==============================================================================
// Helper Method Implementations
//==============================================================================

juce::String
CommandAPI::createErrorResponse(const juce::String &errorMessage) const {
  auto *response = new juce::DynamicObject();
  response->setProperty("success", false);
  response->setProperty("error", errorMessage);
  return juce::JSON::toString(juce::var(response));
}

juce::var CommandAPI::createSuccessResponse(const juce::var &result) const {
  auto *response = new juce::DynamicObject();
  response->setProperty("success", true);
  if (!result.isVoid())
    response->setProperty("result", result);
  return juce::var(response);
}

} // namespace zenith
