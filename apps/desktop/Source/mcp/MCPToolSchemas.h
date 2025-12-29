/*
  ==============================================================================

    MCPToolSchemas.h
    Created: 2025-12-23
    Author:  Zenith DAW Team

    JSON Schema definitions for all MCP tools exposed by Zenith DAW.
    Each tool maps to a CommandAPI command.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>

namespace zenith {
namespace mcp {
namespace schemas {

//==============================================================================
// Schema Builder Helpers
//==============================================================================

inline void setStringProp(juce::DynamicObject *obj, const char *key,
                          const juce::String &value) {
  obj->setProperty(juce::Identifier(key), juce::var(value));
}

inline void setIntProp(juce::DynamicObject *obj, const char *key, int value) {
  obj->setProperty(juce::Identifier(key), juce::var(value));
}

inline void setBoolProp(juce::DynamicObject *obj, const char *key, bool value) {
  obj->setProperty(juce::Identifier(key), juce::var(value));
}

inline void setObjProp(juce::DynamicObject *parent, const char *key,
                       juce::DynamicObject *child) {
  parent->setProperty(juce::Identifier(key), juce::var(child));
}

inline void setArrProp(juce::DynamicObject *obj, const char *key,
                       const juce::var &arr) {
  obj->setProperty(juce::Identifier(key), arr);
}

inline juce::var makeStringProperty(const juce::String &description) {
  auto prop = std::make_unique<juce::DynamicObject>();
  setStringProp(prop.get(), "type", "string");
  setStringProp(prop.get(), "description", description);
  return juce::var(prop.release());
}

inline juce::var makeNumberProperty(const juce::String &description,
                                    bool isInteger = false) {
  auto prop = std::make_unique<juce::DynamicObject>();
  setStringProp(prop.get(), "type", isInteger ? "integer" : "number");
  setStringProp(prop.get(), "description", description);
  return juce::var(prop.release());
}

inline juce::var makeBoolProperty(const juce::String &description) {
  auto prop = std::make_unique<juce::DynamicObject>();
  setStringProp(prop.get(), "type", "boolean");
  setStringProp(prop.get(), "description", description);
  return juce::var(prop.release());
}

inline juce::var makeEnumProperty(const juce::String &description,
                                  std::initializer_list<const char *> values) {
  auto prop = std::make_unique<juce::DynamicObject>();
  setStringProp(prop.get(), "type", "string");
  setStringProp(prop.get(), "description", description);

  juce::var enumArray;
  for (const char *v : values) {
    enumArray.append(juce::var(juce::String(v)));
  }
  setArrProp(prop.get(), "enum", enumArray);
  return juce::var(prop.release());
}

//==============================================================================
// Tool Schema Definitions
//==============================================================================

/**
 * @brief Build a complete tool schema for MCP
 */
inline juce::var
buildToolSchema(const juce::String &name, const juce::String &description,
                juce::DynamicObject *properties,
                std::initializer_list<const char *> required = {}) {
  auto tool = std::make_unique<juce::DynamicObject>();
  setStringProp(tool.get(), "name", name);
  setStringProp(tool.get(), "description", description);

  auto inputSchema = std::make_unique<juce::DynamicObject>();
  setStringProp(inputSchema.get(), "type", "object");
  setObjProp(inputSchema.get(), "properties", properties);

  if (required.size() > 0) {
    juce::var reqArray;
    for (const char *r : required) {
      reqArray.append(juce::var(juce::String(r)));
    }
    setArrProp(inputSchema.get(), "required", reqArray);
  }

  setObjProp(tool.get(), "inputSchema", inputSchema.release());
  return juce::var(tool.release());
}

//==============================================================================
// Transport Tools
//==============================================================================

inline juce::var playSchema() {
  auto props = std::make_unique<juce::DynamicObject>();
  return buildToolSchema("play", "Start playback from the current position",
                         props.release());
}

inline juce::var stopSchema() {
  auto props = std::make_unique<juce::DynamicObject>();
  return buildToolSchema("stop", "Stop playback and recording", props.release());
}

inline juce::var recordSchema() {
  auto props = std::make_unique<juce::DynamicObject>();
  return buildToolSchema("record", "Toggle recording on armed tracks", props.release());
}

inline juce::var rewindSchema() {
  auto props = std::make_unique<juce::DynamicObject>();
  return buildToolSchema(
      "rewind", "Return playhead to the beginning (position 0)", props.release());
}

inline juce::var setTempoSchema() {
  auto props = std::make_unique<juce::DynamicObject>();
  setArrProp(props.get(), "bpm",
             makeNumberProperty("Tempo in beats per minute (20-999)"));
  return buildToolSchema("set_tempo", "Set the project tempo", props.release(), {"bpm"});
}

inline juce::var setTimeSignatureSchema() {
  auto props = std::make_unique<juce::DynamicObject>();
  setArrProp(props.get(), "numerator",
             makeNumberProperty("Beats per bar (e.g., 4)", true));
  setArrProp(props.get(), "denominator",
             makeNumberProperty("Beat unit (e.g., 4 for quarter note)", true));
  return buildToolSchema("set_time_signature", "Set the project time signature",
                         props.release(), {"numerator", "denominator"});
}

inline juce::var setLoopSchema() {
  auto props = std::make_unique<juce::DynamicObject>();
  setArrProp(props.get(), "enabled", makeBoolProperty("Enable or disable loop mode"));
  setArrProp(props.get(), "startBar",
             makeNumberProperty("Loop start position in bars", true));
  setArrProp(props.get(), "endBar",
             makeNumberProperty("Loop end position in bars", true));
  return buildToolSchema("set_loop",
                         "Configure loop region and enable/disable looping",
                         props.release(), {"enabled"});
}

//==============================================================================
// Track Tools
//==============================================================================

inline juce::var listTracksSchema() {
  auto props = std::make_unique<juce::DynamicObject>();
  return buildToolSchema(
      "list_tracks",
      "Get a list of all tracks in the project with their properties", props.release());
}

inline juce::var createTrackSchema() {
  auto props = std::make_unique<juce::DynamicObject>();
  setArrProp(props.get(), "name", makeStringProperty("Name for the new track"));
  setArrProp(props.get(), "type",
             makeEnumProperty("Track type", {"audio", "midi", "instrument",
                                             "aux", "master"}));
  return buildToolSchema("create_track", "Create a new track in the project",
                         props.release(), {"name", "type"});
}

inline juce::var deleteTrackSchema() {
  auto props = std::make_unique<juce::DynamicObject>();
  setArrProp(props.get(), "trackId", makeStringProperty("ID of the track to delete"));
  return buildToolSchema("delete_track", "Delete a track from the project",
                         props.release(), {"trackId"});
}

inline juce::var renameTrackSchema() {
  auto props = std::make_unique<juce::DynamicObject>();
  setArrProp(props.get(), "trackId", makeStringProperty("ID of the track to rename"));
  setArrProp(props.get(), "name", makeStringProperty("New name for the track"));
  return buildToolSchema("rename_track", "Rename an existing track", props.release(),
                         {"trackId", "name"});
}

inline juce::var setTrackVolumeSchema() {
  auto props = std::make_unique<juce::DynamicObject>();
  setArrProp(props.get(), "trackId", makeStringProperty("ID of the track"));
  setArrProp(
      props.get(), "volume",
      makeNumberProperty("Volume level (0.0 to 1.0, where 1.0 is unity gain)"));
  return buildToolSchema("set_track_volume", "Set the volume fader for a track",
                         props.release(), {"trackId", "volume"});
}

inline juce::var setTrackPanSchema() {
  auto props = std::make_unique<juce::DynamicObject>();
  setArrProp(props.get(), "trackId", makeStringProperty("ID of the track"));
  setArrProp(
      props.get(), "pan",
      makeNumberProperty(
          "Pan position (-1.0 = full left, 0.0 = center, 1.0 = full right)"));
  return buildToolSchema("set_track_pan", "Set the pan position for a track",
                         props.release(), {"trackId", "pan"});
}

//==============================================================================
// Clip Tools
//==============================================================================

inline juce::var listClipsSchema() {
  auto props = std::make_unique<juce::DynamicObject>();
  setArrProp(props.get(), "trackId",
             makeStringProperty(
                 "ID of the track (optional, lists all clips if omitted)"));
  return buildToolSchema(
      "list_clips", "Get a list of clips, optionally filtered by track", props.release());
}

inline juce::var createClipSchema() {
  auto props = std::make_unique<juce::DynamicObject>();
  setArrProp(props.get(), "trackId",
             makeStringProperty("ID of the track to add the clip to"));
  setArrProp(props.get(), "name", makeStringProperty("Name for the new clip"));
  setArrProp(props.get(), "startBeat", makeNumberProperty("Start position in beats"));
  setArrProp(props.get(), "lengthBeats", makeNumberProperty("Length in beats"));
  setArrProp(props.get(), "type", makeEnumProperty("Clip type", {"audio", "midi"}));
  return buildToolSchema("create_clip", "Create a new clip on a track", props.release(),
                         {"trackId", "startBeat", "lengthBeats"});
}

inline juce::var deleteClipSchema() {
  auto props = std::make_unique<juce::DynamicObject>();
  setArrProp(props.get(), "clipId", makeStringProperty("ID of the clip to delete"));
  return buildToolSchema("delete_clip", "Delete a clip from the project", props.release(),
                         {"clipId"});
}

inline juce::var moveClipSchema() {
  auto props = std::make_unique<juce::DynamicObject>();
  setArrProp(props.get(), "clipId", makeStringProperty("ID of the clip to move"));
  setArrProp(props.get(), "trackId",
             makeStringProperty(
                 "Target track ID (optional, keeps same track if omitted)"));
  setArrProp(props.get(), "startBeat",
             makeNumberProperty("New start position in beats"));
  return buildToolSchema("move_clip", "Move a clip to a new position or track",
                         props.release(), {"clipId", "startBeat"});
}

inline juce::var splitClipSchema() {
  auto props = std::make_unique<juce::DynamicObject>();
  setArrProp(props.get(), "clipId", makeStringProperty("ID of the clip to split"));
  setArrProp(props.get(), "splitBeat",
             makeNumberProperty("Position in beats where to split"));
  return buildToolSchema("split_clip", "Split a clip at a specific position",
                         props.release(), {"clipId", "splitBeat"});
}

//==============================================================================
// MIDI Tools
//==============================================================================

inline juce::var addNoteSchema() {
  auto props = std::make_unique<juce::DynamicObject>();
  setArrProp(props.get(), "clipId", makeStringProperty("ID of the MIDI clip"));
  setArrProp(
      props.get(), "pitch",
      makeNumberProperty("MIDI note number (0-127, 60 = middle C)", true));
  setArrProp(
      props.get(), "startBeat",
      makeNumberProperty("Start position in beats relative to clip start"));
  setArrProp(props.get(), "lengthBeats",
             makeNumberProperty("Note duration in beats"));
  setArrProp(props.get(), "velocity",
             makeNumberProperty("Note velocity (1-127, default 100)", true));
  return buildToolSchema("add_note", "Add a MIDI note to a clip", props.release(),
                         {"clipId", "pitch", "startBeat", "lengthBeats"});
}

inline juce::var deleteNoteSchema() {
  auto props = std::make_unique<juce::DynamicObject>();
  setArrProp(props.get(), "clipId", makeStringProperty("ID of the MIDI clip"));
  setArrProp(props.get(), "noteIndex",
             makeNumberProperty("Index of the note to delete", true));
  return buildToolSchema("delete_note", "Delete a MIDI note from a clip", props.release(),
                         {"clipId", "noteIndex"});
}

inline juce::var getNotesSchema() {
  auto props = std::make_unique<juce::DynamicObject>();
  setArrProp(props.get(), "clipId", makeStringProperty("ID of the MIDI clip"));
  return buildToolSchema("get_notes", "Get all MIDI notes in a clip", props.release(),
                         {"clipId"});
}

inline juce::var setClipNotesSchema() {
  auto props = std::make_unique<juce::DynamicObject>();
  setArrProp(props.get(), "clipId", makeStringProperty("ID of the MIDI clip"));
  setArrProp(
      props.get(), "notes",
      makeStringProperty(
          "Array of notes with pitch, startBeat, lengthBeats, velocity"));
  return buildToolSchema("set_clip_notes", "Replace all notes in a MIDI clip",
                         props.release(), {"clipId", "notes"});
}

//==============================================================================
// Plugin Tools
//==============================================================================

inline juce::var listPluginsSchema() {
  auto props = std::make_unique<juce::DynamicObject>();
  return buildToolSchema("list_plugins", "Get a list of all available plugins",
                         props.release());
}

inline juce::var searchPluginsSchema() {
  auto props = std::make_unique<juce::DynamicObject>();
  setArrProp(props.get(), "query",
             makeStringProperty(
                 "Search term for plugin name, manufacturer, or category"));
  return buildToolSchema(
      "search_plugins", "Search for plugins by name, manufacturer, or category",
      props.release(), {"query"});
}

inline juce::var addPluginSchema() {
  auto props = std::make_unique<juce::DynamicObject>();
  setArrProp(props.get(), "trackId",
             makeStringProperty("ID of the track to add the plugin to"));
  setArrProp(props.get(), "pluginId", makeStringProperty("Plugin identifier string"));
  return buildToolSchema("add_plugin", "Add a plugin to a track's insert chain",
                         props.release(), {"trackId", "pluginId"});
}

inline juce::var removePluginSchema() {
  auto props = std::make_unique<juce::DynamicObject>();
  setArrProp(props.get(), "trackId", makeStringProperty("ID of the track"));
  setArrProp(
      props.get(), "pluginIndex",
      makeNumberProperty("Index of the plugin in the insert chain", true));
  return buildToolSchema("remove_plugin", "Remove a plugin from a track", props.release(),
                         {"trackId", "pluginIndex"});
}

inline juce::var getPluginParamsSchema() {
  auto props = std::make_unique<juce::DynamicObject>();
  setArrProp(props.get(), "trackId", makeStringProperty("ID of the track"));
  setArrProp(props.get(), "pluginIndex",
             makeNumberProperty("Index of the plugin", true));
  return buildToolSchema("get_plugin_params", "Get all parameters of a plugin",
                         props.release(), {"trackId", "pluginIndex"});
}

inline juce::var setPluginParamSchema() {
  auto props = std::make_unique<juce::DynamicObject>();
  setArrProp(props.get(), "trackId", makeStringProperty("ID of the track"));
  setArrProp(props.get(), "pluginIndex",
             makeNumberProperty("Index of the plugin", true));
  setArrProp(props.get(), "paramIndex",
             makeNumberProperty("Index of the parameter", true));
  setArrProp(props.get(), "value",
             makeNumberProperty("New parameter value (0.0 to 1.0)"));
  return buildToolSchema("set_plugin_param", "Set a plugin parameter value",
                         props.release(),
                         {"trackId", "pluginIndex", "paramIndex", "value"});
}

//==============================================================================
// Session/Project Tools
//==============================================================================

inline juce::var getSessionGraphSchema() {
  auto props = std::make_unique<juce::DynamicObject>();
  return buildToolSchema(
      "get_session_graph",
      "Get the full session graph including all tracks, clips, and routing",
      props.release());
}

inline juce::var undoSchema() {
  auto props = std::make_unique<juce::DynamicObject>();
  return buildToolSchema("undo", "Undo the last action", props.release());
}

inline juce::var redoSchema() {
  auto props = std::make_unique<juce::DynamicObject>();
  return buildToolSchema("redo", "Redo the last undone action", props.release());
}

inline juce::var historySchema() {
  auto props = std::make_unique<juce::DynamicObject>();
  return buildToolSchema("history", "Get the undo/redo history state", props.release());
}

//==============================================================================
// Export Tools
//==============================================================================

inline juce::var exportAudioSchema() {
  auto props = std::make_unique<juce::DynamicObject>();
  setArrProp(props.get(), "outputPath",
             makeStringProperty("Full path for the output file"));
  setArrProp(props.get(), "sampleRate",
             makeNumberProperty("Sample rate in Hz (default 44100)", true));
  setArrProp(props.get(), "bitDepth",
             makeNumberProperty("Bit depth (16 or 24, default 24)", true));
  setArrProp(props.get(), "durationSeconds",
             makeNumberProperty("Duration to export in seconds"));
  return buildToolSchema("export_audio", "Export the project to an audio file",
                         props.release(), {"outputPath"});
}

//==============================================================================
// Aux Bus Tools
//==============================================================================

inline juce::var createAuxBusSchema() {
  auto props = std::make_unique<juce::DynamicObject>();
  setArrProp(props.get(), "name", makeStringProperty("Name for the aux bus"));
  return buildToolSchema("create_aux_bus",
                         "Create a new auxiliary bus for parallel processing",
                         props.release(), {"name"});
}

inline juce::var getAuxBusesSchema() {
  auto props = std::make_unique<juce::DynamicObject>();
  return buildToolSchema("get_aux_buses", "Get a list of all auxiliary buses",
                         props.release());
}

//==============================================================================
// Routing Tools
//==============================================================================

inline juce::var getRoutingGraphSchema() {
  auto props = std::make_unique<juce::DynamicObject>();
  return buildToolSchema("get_routing_graph",
                         "Get the audio routing graph showing all connections",
                         props.release());
}

inline juce::var connectNodesSchema() {
  auto props = std::make_unique<juce::DynamicObject>();
  setArrProp(props.get(), "sourceId",
             makeStringProperty("ID of the source node (track or aux)"));
  setArrProp(props.get(), "destId", makeStringProperty("ID of the destination node"));
  setArrProp(props.get(), "gain",
             makeNumberProperty("Connection gain (0.0 to 1.0, default 1.0)"));
  return buildToolSchema("connect_nodes", "Create an audio routing connection",
                         props.release(), {"sourceId", "destId"});
}

inline juce::var disconnectNodesSchema() {
  auto props = std::make_unique<juce::DynamicObject>();
  setArrProp(props.get(), "sourceId", makeStringProperty("ID of the source node"));
  setArrProp(props.get(), "destId", makeStringProperty("ID of the destination node"));
  return buildToolSchema("disconnect_nodes",
                         "Remove an audio routing connection", props.release(),
                         {"sourceId", "destId"});
}

//==============================================================================
// UI State Tools
//==============================================================================

inline juce::var getUIStateSchema() {
  auto props = std::make_unique<juce::DynamicObject>();
  return buildToolSchema(
      "get_ui_state",
      "Get the current UI state including health metrics and any issues",
      props.release());
}

//==============================================================================
// Master Schema Builder
//==============================================================================

/**
 * @brief Get all tool schemas as an array
 */
inline juce::var getAllToolSchemas() {
  juce::var tools;

  // Transport
  tools.append(playSchema());
  tools.append(stopSchema());
  tools.append(recordSchema());
  tools.append(rewindSchema());
  tools.append(setTempoSchema());
  tools.append(setTimeSignatureSchema());
  tools.append(setLoopSchema());

  // Tracks
  tools.append(listTracksSchema());
  tools.append(createTrackSchema());
  tools.append(deleteTrackSchema());
  tools.append(renameTrackSchema());
  tools.append(setTrackVolumeSchema());
  tools.append(setTrackPanSchema());

  // Clips
  tools.append(listClipsSchema());
  tools.append(createClipSchema());
  tools.append(deleteClipSchema());
  tools.append(moveClipSchema());
  tools.append(splitClipSchema());

  // MIDI
  tools.append(addNoteSchema());
  tools.append(deleteNoteSchema());
  tools.append(getNotesSchema());
  tools.append(setClipNotesSchema());

  // Plugins
  tools.append(listPluginsSchema());
  tools.append(searchPluginsSchema());
  tools.append(addPluginSchema());
  tools.append(removePluginSchema());
  tools.append(getPluginParamsSchema());
  tools.append(setPluginParamSchema());

  // Session
  tools.append(getSessionGraphSchema());
  tools.append(undoSchema());
  tools.append(redoSchema());
  tools.append(historySchema());

  // Export
  tools.append(exportAudioSchema());

  // Aux Buses
  tools.append(createAuxBusSchema());
  tools.append(getAuxBusesSchema());

  // Routing
  tools.append(getRoutingGraphSchema());
  tools.append(connectNodesSchema());
  tools.append(disconnectNodesSchema());

  // UI State
  tools.append(getUIStateSchema());

  return tools;
}

} // namespace schemas
} // namespace mcp
} // namespace zenith
