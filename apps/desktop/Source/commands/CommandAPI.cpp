/*
  ==============================================================================

    CommandAPI.cpp
    Created: 2025-11-14
    Author:  Zenith DAW - Phase 5: Wingman v0

    JSON command processor implementation

  ==============================================================================
*/

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "../engine/AuxBus.h"
#include "../engine/Clip.h"
#include "../engine/PluginHost.h"
#include "../engine/Track.h"
#include "../instruments/InstrumentRegistry.h"
#include "Actions.h"
#include "ClipCommands.h"
#include "CommandAPI.h"
#include "CommandUtils.h"
#include "Engine.h"
#include "ProjectState.h"
#include "SessionGraph.h"
#include "SkiaComponent.h"
#include "TempoMap.h"
#include "TrackCommands.h"
#include "TransportCommands.h"

#include "../ai/AIMasteringAgent.h"
#include "../ai/PresetGeneticistAgent.h"
#include "../ai/UXDirectorAgent.h"
#include "../dsp/ONNXStemSeparator.h"

namespace zenith {

//==============================================================================
CommandAPI::CommandAPI(ProjectState &state, Engine &eng)
    : projectState(state), engine(eng) {
  trackCommands = std::make_unique<TrackCommands>(engine, projectState, *this);
  clipCommands = std::make_unique<ClipCommands>(engine, projectState, *this);
  transportCommands =
      std::make_unique<TransportCommands>(engine, projectState, *this);

  DBG("CommandAPI: Initialized");
#pragma message("C++ standard: " JUCE_STRINGIFY(__cplusplus))
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

  // Presets & Instruments
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

  // Aux Bus Commands
  commandMap["create_aux_bus"] = CommandID::CreateAuxBus;
  commandMap["remove_aux_bus"] = CommandID::RemoveAuxBus;
  commandMap["set_aux_bus_volume"] = CommandID::SetAuxBusVolume;
  commandMap["set_aux_bus_pan"] = CommandID::SetAuxBusPan;
  commandMap["set_aux_bus_mute"] = CommandID::SetAuxBusMute;
  commandMap["get_aux_buses"] = CommandID::GetAuxBuses;

  // Vision Command
  commandMap["get_ui_state"] = CommandID::GetUIState;

  // Evolution Commands
  commandMap["start_evolution"] = CommandID::StartEvolution;
  commandMap["stop_evolution"] = CommandID::StopEvolution;
  commandMap["get_evolution_stats"] = CommandID::GetEvolutionStats;

  // Routing Graph Commands
  commandMap["get_routing_graph"] = CommandID::GetRoutingGraph;
  commandMap["connect_nodes"] = CommandID::ConnectNodes;
  commandMap["disconnect_nodes"] = CommandID::DisconnectNodes;

  commandMap["search_plugins"] = CommandID::SearchPlugins;

  // Quick Wins: Mixer Control
  commandMap["set_track_send"] = CommandID::SetTrackSend;
  commandMap["set_track_eq"] = CommandID::SetTrackEQ;
  commandMap["set_track_compressor"] = CommandID::SetTrackCompressor;

  // Register Handlers
  registerCommand("list_tracks", [this](const juce::var &p) {
    return trackCommands->listTracks(p);
  });
  registerCommand("create_track", [this](const juce::var &p) {
    return trackCommands->createTrack(p);
  });
  registerCommand("delete_track", [this](const juce::var &p) {
    return trackCommands->deleteTrack(p);
  });
  registerCommand("rename_track", [this](const juce::var &p) {
    return trackCommands->renameTrack(p);
  });
  registerCommand("set_track_volume", [this](const juce::var &p) {
    return trackCommands->setTrackVolume(p);
  });
  registerCommand("set_track_pan", [this](const juce::var &p) {
    return trackCommands->setTrackPan(p);
  });

  registerCommand("export_audio",
                  [this](const juce::var &p) { return exportAudio(p); });
  registerCommand("export_project_advanced", [this](const juce::var &p) {
    return exportProjectAdvanced(p);
  });
  registerCommand("get_session_graph",
                  [this](const juce::var &p) { return getSessionGraph(p); });

  registerCommand("undo", [this](const juce::var &p) { return undo(p); });
  registerCommand("redo", [this](const juce::var &p) { return redo(p); });
  registerCommand("history", [this](const juce::var &p) { return history(p); });

  registerCommand("create_clip", [this](const juce::var &p) {
    return clipCommands->createClip(p);
  });
  registerCommand("add_clip", [this](const juce::var &p) {
    return clipCommands->createClip(p);
  });
  registerCommand("delete_clip", [this](const juce::var &p) {
    return clipCommands->deleteClip(p);
  });
  registerCommand("list_clips", [this](const juce::var &p) {
    return clipCommands->listClips(p);
  });
  registerCommand("move_clip", [this](const juce::var &p) {
    return clipCommands->moveClip(p);
  });
  registerCommand("resize_clip", [this](const juce::var &p) {
    return clipCommands->resizeClip(p);
  });
  registerCommand("split_clip", [this](const juce::var &p) {
    return clipCommands->splitClip(p);
  });

  registerCommand("play", [this](const juce::var &p) {
    return transportCommands->play(p);
  });
  registerCommand("stop", [this](const juce::var &p) {
    return transportCommands->stop(p);
  });
  registerCommand("record", [this](const juce::var &p) {
    return transportCommands->record(p);
  });
  registerCommand("rewind", [this](const juce::var &p) {
    return transportCommands->rewind(p);
  });
  registerCommand("set_loop", [this](const juce::var &p) {
    return transportCommands->setLoop(p);
  });
  registerCommand("set_tempo", [this](const juce::var &p) {
    return transportCommands->setTempo(p);
  });
  registerCommand("set_time_signature", [this](const juce::var &p) {
    return transportCommands->setTimeSignature(p);
  });

  registerCommand("add_plugin",
                  [this](const juce::var &p) { return addPlugin(p); });
  registerCommand("remove_plugin",
                  [this](const juce::var &p) { return removePlugin(p); });
  registerCommand("list_plugins",
                  [this](const juce::var &p) { return listPlugins(p); });
  registerCommand("search_plugins",
                  [this](const juce::var &p) { return searchPlugins(p); });
  registerCommand("set_plugin_param",
                  [this](const juce::var &p) { return setPluginParam(p); });
  registerCommand("get_plugin_params",
                  [this](const juce::var &p) { return getPluginParams(p); });

  registerCommand("add_automation_point",
                  [this](const juce::var &p) { return addAutomationPoint(p); });
  registerCommand("clear_automation",
                  [this](const juce::var &p) { return clearAutomation(p); });
  registerCommand("get_automation",
                  [this](const juce::var &p) { return getAutomation(p); });

  registerCommand("add_marker",
                  [this](const juce::var &p) { return addMarker(p); });
  registerCommand("get_markers",
                  [this](const juce::var &p) { return getMarkers(p); });
  registerCommand("delete_marker",
                  [this](const juce::var &p) { return deleteMarker(p); });
  registerCommand("goto_marker",
                  [this](const juce::var &p) { return gotoMarker(p); });

  registerCommand("add_note",
                  [this](const juce::var &p) { return addNote(p); });
  registerCommand("delete_note",
                  [this](const juce::var &p) { return deleteNote(p); });
  registerCommand("move_note",
                  [this](const juce::var &p) { return moveNote(p); });
  registerCommand("get_notes",
                  [this](const juce::var &p) { return getNotes(p); });

  registerCommand("start_evolution",
                  [this](const juce::var &p) { return startEvolution(p); });
  registerCommand("stop_evolution",
                  [this](const juce::var &p) { return stopEvolution(p); });
  registerCommand("get_evolution_stats",
                  [this](const juce::var &p) { return getEvolutionStats(p); });
  registerCommand("set_note_velocity",
                  [this](const juce::var &p) { return setNoteVelocity(p); });
  registerCommand("set_note_length",
                  [this](const juce::var &p) { return setNoteLength(p); });
  registerCommand("get_midi_data",
                  [this](const juce::var &p) { return getMidiData(p); });

  registerCommand("describe_instrument",
                  [this](const juce::var &p) { return describeInstrument(p); });

  // Aux Bus Handlers
  registerCommand("create_aux_bus",
                  [this](const juce::var &p) { return createAuxBus(p); });
  registerCommand("remove_aux_bus",
                  [this](const juce::var &p) { return removeAuxBus(p); });
  registerCommand("set_aux_bus_volume",
                  [this](const juce::var &p) { return setAuxBusVolume(p); });
  registerCommand("set_aux_bus_pan",
                  [this](const juce::var &p) { return setAuxBusPan(p); });
  registerCommand("set_aux_bus_mute",
                  [this](const juce::var &p) { return setAuxBusMute(p); });
  registerCommand("get_aux_buses",
                  [this](const juce::var &p) { return getAuxBuses(p); });

  // Vision Handler
  registerCommand("get_ui_health",
                  [this](const juce::var &p) { return getUIHealth(p); });
  registerCommand("get_ui_state",
                  [this](const juce::var &p) { return getUIState(p); });

  // Routing Graph Handlers
  registerCommand("get_routing_graph",
                  [this](const juce::var &p) { return getRoutingGraph(p); });
  registerCommand("connect_nodes",
                  [this](const juce::var &p) { return connectNodes(p); });
  registerCommand("disconnect_nodes",
                  [this](const juce::var &p) { return disconnectNodes(p); });
}

void CommandAPI::registerCommand(const juce::String &commandName,
                                 CommandAPI::CommandHandler handler) {
  commandHandlers[commandName] = handler;
}

//==============================================================================
// Track Commands
//==============================================================================

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

  // Use Handler Map lookup (BRAIN Architecture)
  if (commandHandlers.count(commandStr) > 0) {
    return commandHandlers[commandStr](params);
  }

  return createErrorResponse("Unknown command or handler not registered: " +
                             commandStr);
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
      errorObj->setProperty("failedIndex", (int)i);
      errorObj->setProperty("failedCommand", cmdVar);
      errorObj->setProperty("successCount", (int)successCount);

      return juce::var(errorObj);
    }

    if (!cmdVar.hasProperty("command")) {
      auto *errorObj = new juce::DynamicObject();
      errorObj->setProperty("success", false);
      errorObj->setProperty("error", "Command at index " + juce::String(i) +
                                         " missing 'command' field");
      errorObj->setProperty("failedIndex", (int)i);
      errorObj->setProperty("failedCommand", cmdVar);
      errorObj->setProperty("successCount", (int)successCount);

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
      errorObj->setProperty("failedIndex", (int)i);
      errorObj->setProperty("failedCommand", cmdVar);
      errorObj->setProperty("successCount", (int)successCount);

      DBG("CommandAPI: Batch failed at command " + juce::String(i) + ": " +
          error);

      return juce::var(errorObj);
    }

    successCount++;
  }

  // All commands succeeded
  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("count", (int)successCount);

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

juce::var CommandAPI::getRoutingGraph(const juce::var &params) {
  juce::ignoreUnused(params);
  return createSuccessResponse(engine.getRoutingGraph().toVar());
}

juce::var CommandAPI::connectNodes(const juce::var &params) {
  if (!params.hasProperty("sourceId"))
    return createErrorResponse("Missing 'sourceId' parameter");
  if (!params.hasProperty("destId"))
    return createErrorResponse("Missing 'destId' parameter");

  juce::String sourceId = params["sourceId"].toString();
  juce::String destId = params["destId"].toString();
  float gain = params.hasProperty("gain") ? (float)params["gain"] : 1.0f;

  if (engine.getRoutingGraph().connect(sourceId, destId, gain)) {
    return createSuccessResponse();
  }

  return createErrorResponse("Failed to connect nodes: check if IDs exist");
}

juce::var CommandAPI::disconnectNodes(const juce::var &params) {
  if (!params.hasProperty("sourceId"))
    return createErrorResponse("Missing 'sourceId' parameter");
  if (!params.hasProperty("destId"))
    return createErrorResponse("Missing 'destId' parameter");

  juce::String sourceId = params["sourceId"].toString();
  juce::String destId = params["destId"].toString();

  if (engine.getRoutingGraph().disconnect(sourceId, destId)) {
    return createSuccessResponse();
  }

  return createErrorResponse("Failed to disconnect nodes");
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
  return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::getUIState(const juce::var &params) {
  juce::ignoreUnused(params);

  auto *resultObj = new juce::DynamicObject();

  if (uxDirector_) {
    resultObj->setProperty("healthScore", uxDirector_->getUIHealthScore());
    resultObj->setProperty("summary", uxDirector_->getIssueSummary());

    juce::var issuesArray;
    for (const auto &issue : uxDirector_->getIssues()) {
      if (!issue.isFixed) {
        auto *issueObj = new juce::DynamicObject();
        issueObj->setProperty("type", (int)issue.type);
        issueObj->setProperty("severity", (int)issue.severity);
        issueObj->setProperty("description", issue.description);
        issueObj->setProperty("componentName", issue.componentName);
        issueObj->setProperty("componentType", issue.componentType);
        issueObj->setProperty("suggestedFix", issue.suggestedFix);
        issuesArray.append(juce::var(issueObj));
      }
    }
    resultObj->setProperty("unresolvedIssues", issuesArray);
    resultObj->setProperty("issueCount",
                           uxDirector_->getUnresolvedIssueCount());

    // Add health breakdown
    auto breakdown = uxDirector_->getHealthBreakdown();
    auto *breakdownObj = new juce::DynamicObject();
    breakdownObj->setProperty("styleConsistency", breakdown.styleConsistency);
    breakdownObj->setProperty("dataBindingHealth", breakdown.dataBindingHealth);
    breakdownObj->setProperty("layoutHealth", breakdown.layoutHealth);
    breakdownObj->setProperty("dataFreshness", breakdown.dataFreshness);
    resultObj->setProperty("healthBreakdown", juce::var(breakdownObj));
  } else {
    resultObj->setProperty("error", "UXDirectorAgent not available");
  }

  return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::getUIHealth(const juce::var &params) {
  juce::ignoreUnused(params);

  auto *resultObj = new juce::DynamicObject();

  if (uxDirector_) {
    resultObj->setProperty("healthScore", uxDirector_->getUIHealthScore());
    resultObj->setProperty("summary", uxDirector_->getIssueSummary());
    resultObj->setProperty("unresolvedIssues",
                           uxDirector_->getUnresolvedIssueCount());
    resultObj->setProperty("status", "ok");
  } else {
    resultObj->setProperty("error", "UXDirectorAgent not available");
  }

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

juce::var CommandAPI::searchPlugins(const juce::var &params) {
  if (!params.hasProperty("query"))
    return createErrorResponse("Missing 'query' parameter");

  juce::String query = params["query"].toString().toLowerCase();
  juce::var resultsArray;
  auto *resultsArrayPtr = resultsArray.getArray();

  const auto &knownPlugins = engine.getPluginHost().getKnownPlugins();

  for (int i = 0; i < knownPlugins.getNumTypes(); ++i) {
    auto desc = knownPlugins.getTypes()[i];

    bool match = desc.name.toLowerCase().contains(query) ||
                 desc.manufacturerName.toLowerCase().contains(query) ||
                 desc.category.toLowerCase().contains(query);

    if (match) {
      auto *pluginObj = new juce::DynamicObject();
      pluginObj->setProperty("id", juce::var(desc.createIdentifierString()));
      pluginObj->setProperty("name", juce::var(desc.name));
      pluginObj->setProperty("manufacturer", juce::var(desc.manufacturerName));
      pluginObj->setProperty("format", juce::var(desc.pluginFormatName));
      pluginObj->setProperty("category", juce::var(desc.category));
      resultsArrayPtr->add(juce::var(pluginObj));
    }
  }

  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("results", resultsArray);
  resultObj->setProperty("count", resultsArray.size());

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

  // Validate generated parameters based on instrument parameter schema
  if (generatedParams.isObject()) {
    auto paramSchema = registry.getParameterSchema(instrumentId);

    for (auto &prop : generatedParams.getDynamicObject()->getProperties()) {
      juce::String paramId = prop.name.toString();

      // Check if parameter exists in schema
      bool paramExists = false;
      if (paramSchema.isArray()) {
        for (const auto &schemaParam : *paramSchema.getArray()) {
          if (schemaParam.hasProperty("id") &&
              schemaParam["id"].toString() == paramId) {
            paramExists = true;

            // Validate range if specified
            if (schemaParam.hasProperty("min") &&
                schemaParam.hasProperty("max")) {
              float minVal = static_cast<float>(schemaParam["min"]);
              float maxVal = static_cast<float>(schemaParam["max"]);
              float currentVal = static_cast<float>(prop.value);

              if (currentVal < minVal || currentVal > maxVal) {
                // Clamp to valid range
                float clampedVal = juce::jlimit(minVal, maxVal, currentVal);
                generatedParams.getDynamicObject()->setProperty(prop.name,
                                                                clampedVal);
                DBG("CommandAPI: Clamped parameter " + paramId +
                    " to valid range");
              }
            }
            break;
          }
        }
      }

      // Log unknown parameters but don't fail
      if (!paramExists && paramSchema.isArray() &&
          paramSchema.getArray()->size() > 0) {
        DBG("CommandAPI: Warning - unknown parameter in preset: " + paramId);
      }
    }
  }

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
    // zenith::ai::AIMasteringAgent agent(engine);
    // zenith::ai::AIMasteringAgent::MasteringOptions masterOpts;
    // masterOpts.target8Bit = (opts.bitDepth == 8);
    // agent.runMasteringPass(masterOpts);
  }

  bool success = engine.exportProject(opts);

  if (!success)
    return createErrorResponse("Export failed");

  return createSuccessResponse(juce::var());
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

//==============================================================================
// Aux Bus Commands
//==============================================================================

juce::var CommandAPI::createAuxBus(const juce::var &params) {
  if (!params.hasProperty("name"))
    return createErrorResponse("Missing 'name'");

  juce::String name = params["name"].toString();

  // Check for message thread
  // CommandAPI is usually called from message thread, but Engine methods assert
  // it.

  int index = engine.createAuxBus(name);
  auto *bus = engine.getAuxBus(index);

  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("index", index);
  if (bus) {
    resultObj->setProperty("id", bus->getId());
  }
  resultObj->setProperty("success", true);

  return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::removeAuxBus(const juce::var &params) {
  if (!params.hasProperty("index"))
    return createErrorResponse("Missing 'index'");

  int index = static_cast<int>(params["index"]);

  engine.removeAuxBus(index);

  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("success", true);
  return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::setAuxBusVolume(const juce::var &params) {
  if (!params.hasProperty("index"))
    return createErrorResponse("Missing 'index'");
  if (!params.hasProperty("volume"))
    return createErrorResponse("Missing 'volume'");

  int index = static_cast<int>(params["index"]);
  float volume = static_cast<float>(params["volume"]);

  auto *bus = engine.getAuxBus(index);
  if (bus) {
    bus->setVolume(volume);
    return createSuccessResponse();
  }
  return createErrorResponse("Aux Bus not found");
}

juce::var CommandAPI::setAuxBusPan(const juce::var &params) {
  if (!params.hasProperty("index"))
    return createErrorResponse("Missing 'index'");
  if (!params.hasProperty("pan"))
    return createErrorResponse("Missing 'pan'");

  int index = static_cast<int>(params["index"]);
  float pan = static_cast<float>(params["pan"]);

  auto *bus = engine.getAuxBus(index);
  if (bus) {
    bus->setPan(pan);
    return createSuccessResponse();
  }
  return createErrorResponse("Aux Bus not found");
}

juce::var CommandAPI::setAuxBusMute(const juce::var &params) {
  if (!params.hasProperty("index"))
    return createErrorResponse("Missing 'index'");
  if (!params.hasProperty("mute"))
    return createErrorResponse("Missing 'mute'");

  int index = static_cast<int>(params["index"]);
  bool mute = static_cast<bool>(params["mute"]);

  auto *bus = engine.getAuxBus(index);
  if (bus) {
    bus->setMuted(mute);
    return createSuccessResponse();
  }
  return createErrorResponse("Aux Bus not found");
}

juce::var CommandAPI::getAuxBuses(const juce::var &params) {
  juce::var buses;

  int count = engine.getNumAuxBuses();
  for (int i = 0; i < count; ++i) {
    auto *bus = engine.getAuxBus(i);
    if (bus) {
      auto *obj = new juce::DynamicObject();
      obj->setProperty("index", i);
      obj->setProperty("name", bus->getName());
      obj->setProperty("id", bus->getId());
      obj->setProperty("volume", bus->getVolume());
      obj->setProperty("pan", bus->getPan());
      obj->setProperty("mute", bus->isMuted());
      buses.append(juce::var(obj));
    }
  }

  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("buses", buses);
  return createSuccessResponse(juce::var(resultObj));
}

//==============================================================================
// Vision Command
//==============================================================================

// Recursive helper to traverse component tree
static void traverseComponentTree(juce::Component *comp,
                                  juce::Array<juce::var> &elements) {
  if (!comp)
    return;

  // Check if it's a SkiaComponent
  if (auto *skiaComp = dynamic_cast<SkiaComponent *>(comp)) {
    auto inspectables = skiaComp->getInspectableElements();
    for (const auto &info : inspectables) {
      auto *obj = new juce::DynamicObject();
      obj->setProperty("type", info.type);
      obj->setProperty("parameterId", info.parameterId);
      obj->setProperty("value", info.currentValue);

      // Screen coordinates
      auto screenBounds = comp->localAreaToGlobal(
          juce::Rectangle<int>(info.bounds.left(), info.bounds.top(),
                               info.bounds.width(), info.bounds.height()));

      obj->setProperty("x", screenBounds.getX());
      obj->setProperty("y", screenBounds.getY());
      obj->setProperty("width", screenBounds.getWidth());
      obj->setProperty("height", screenBounds.getHeight());

      elements.add(juce::var(obj));
    }
  }

  // Recurse children
  for (auto *child : comp->getChildren()) {
    traverseComponentTree(child, elements);
  }
}

juce::var CommandAPI::startEvolution(const juce::var &params) {
  if (presetGeneticist_ == nullptr)
    return createErrorResponse("PresetGeneticistAgent not available");

  int maxGens = params.hasProperty("maxGenerations")
                    ? static_cast<int>(params["maxGenerations"])
                    : 100;

  presetGeneticist_->startEvolution(maxGens);

  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("success", true);
  resultObj->setProperty("message", "Evolution started");
  return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::stopEvolution(const juce::var &params) {
  juce::ignoreUnused(params);
  if (presetGeneticist_ == nullptr)
    return createErrorResponse("PresetGeneticistAgent not available");

  presetGeneticist_->stopEvolution();

  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("success", true);
  resultObj->setProperty("message", "Evolution stopped");
  return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::getEvolutionStats(const juce::var &params) {
  juce::ignoreUnused(params);
  if (presetGeneticist_ == nullptr)
    return createErrorResponse("PresetGeneticistAgent not available");

  auto stats = presetGeneticist_->getStats();

  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("generation", stats.generation);
  resultObj->setProperty("bestFitness", stats.bestFitness);
  resultObj->setProperty("averageFitness", stats.averageFitness);
  resultObj->setProperty("totalEvaluated", stats.totalEvaluated);
  resultObj->setProperty("bestPresetName", stats.bestPresetName);
  resultObj->setProperty("isRunning", presetGeneticist_->isRunning());
  resultObj->setProperty("isPaused", presetGeneticist_->isPaused());

  return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::executeCommand(CommandID id, const juce::var &params) {
  // Find the command string for this ID (reverse lookup or switch)
  // For efficiency, we should probably have a map ID -> Handler
  // But since we register by string, let's reverse lookup or assume the caller
  // knows what they are doing.

  // Better approach: Since we have commandHandlers map which is string ->
  // handler, we need ID -> handler. Let's iterate commandMap to find the string
  // for this ID. This is slow O(N), but safe for now.

  juce::String commandName;
  for (const auto &pair : commandMap) {
    if (pair.second == id) {
      commandName = pair.first;
      break;
    }
  }

  if (commandName.isNotEmpty()) {
    auto it = commandHandlers.find(commandName);
    if (it != commandHandlers.end()) {
      return it->second(params);
    }
  }

  return createErrorResponse("Unknown command ID");
}

//==============================================================================
juce::var createSuccessResponse(const juce::var &result) {
  juce::DynamicObject::Ptr response = new juce::DynamicObject();
  response->setProperty("success", true);
  if (!result.isVoid())
    response->setProperty("result", result);
  return juce::var(response.get());
}

juce::var createErrorResponse(const juce::String &errorMessage) {
  juce::DynamicObject::Ptr response = new juce::DynamicObject();
  response->setProperty("success", false);
  response->setProperty("error", errorMessage);
  return juce::var(response.get());
}
} // namespace zenith
