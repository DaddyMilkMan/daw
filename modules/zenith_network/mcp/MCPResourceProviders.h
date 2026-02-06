/*
  ==============================================================================

    MCPResourceProviders.h
    Created: 2025-12-23
    Author:  Zenith DAW Team

    Resource providers for MCP read-only state access.
    Resources allow AI models to inspect DAW state without using tools.

  ==============================================================================
*/

#pragma once

#include "zenith_commands/commands/CommandAPI.h"
#include "../engine/Engine.h"
#include "../engine/ProjectState.h"
#include "../engine/TempoMap.h"
#include <juce_core/juce_core.h>

namespace zenith {
namespace mcp {
namespace resources {

//==============================================================================
// Helper for setting properties with correct JUCE 8 API
//==============================================================================

inline void setProp(juce::DynamicObject *obj, const char *key,
                    const juce::var &value) {
  obj->setProperty(juce::Identifier(key), value);
}

//==============================================================================
// Resource Definitions
//==============================================================================

/**
 * @brief Get a list of all available resources with their URIs
 */
inline juce::var getResourcesList() {
  juce::var resources;

  // Session Graph
  {
    auto *resource = new juce::DynamicObject();
    setProp(resource, "uri", juce::var("zenith://session/graph"));
    setProp(resource, "name", juce::var("Session Graph"));
    setProp(resource, "description",
            juce::var(
                "Complete session state including tracks, clips, and routing"));
    setProp(resource, "mimeType", juce::var("application/json"));
    resources.append(juce::var(resource));
  }

  // Tracks
  {
    auto *resource = new juce::DynamicObject();
    setProp(resource, "uri", juce::var("zenith://tracks"));
    setProp(resource, "name", juce::var("Track List"));
    setProp(resource, "description",
            juce::var("List of all tracks with their properties"));
    setProp(resource, "mimeType", juce::var("application/json"));
    resources.append(juce::var(resource));
  }

  // Transport State
  {
    auto *resource = new juce::DynamicObject();
    setProp(resource, "uri", juce::var("zenith://transport/state"));
    setProp(resource, "name", juce::var("Transport State"));
    setProp(resource, "description",
            juce::var("Current transport state (playing, position, tempo, time "
                      "signature)"));
    setProp(resource, "mimeType", juce::var("application/json"));
    resources.append(juce::var(resource));
  }

  // Markers
  {
    auto *resource = new juce::DynamicObject();
    setProp(resource, "uri", juce::var("zenith://markers"));
    setProp(resource, "name", juce::var("Timeline Markers"));
    setProp(resource, "description",
            juce::var("All timeline markers in the project"));
    setProp(resource, "mimeType", juce::var("application/json"));
    resources.append(juce::var(resource));
  }

  // Available Plugins
  {
    auto *resource = new juce::DynamicObject();
    setProp(resource, "uri", juce::var("zenith://plugins/available"));
    setProp(resource, "name", juce::var("Available Plugins"));
    setProp(resource, "description",
            juce::var("List of all plugins available for insertion"));
    setProp(resource, "mimeType", juce::var("application/json"));
    resources.append(juce::var(resource));
  }

  // Undo History
  {
    auto *resource = new juce::DynamicObject();
    setProp(resource, "uri", juce::var("zenith://history"));
    setProp(resource, "name", juce::var("Undo History"));
    setProp(resource, "description", juce::var("Current undo/redo state"));
    setProp(resource, "mimeType", juce::var("application/json"));
    resources.append(juce::var(resource));
  }

  // UI Health
  {
    auto *resource = new juce::DynamicObject();
    setProp(resource, "uri", juce::var("zenith://ui/health"));
    setProp(resource, "name", juce::var("UI Health"));
    setProp(resource, "description",
            juce::var("UI health metrics and diagnostics"));
    setProp(resource, "mimeType", juce::var("application/json"));
    resources.append(juce::var(resource));
  }

  return resources;
}

//==============================================================================
// Resource Readers
//==============================================================================

/**
 * @brief Read the transport state resource
 */
inline juce::var readTransportState(Engine &engine) {
  auto *state = new juce::DynamicObject();

  setProp(state, "playing", juce::var(engine.isPlaying()));
  setProp(state, "recording", juce::var(engine.isRecording()));
  setProp(state, "positionSamples",
          juce::var((juce::int64)engine.getPlaybackPosition()));
  setProp(state, "positionBeats", juce::var(engine.getPlaybackPositionBeats()));
  setProp(state, "tempo", juce::var(engine.getTempoMap().getTempoAt(0)));
  setProp(state, "sampleRate", juce::var(engine.getSampleRate()));
  setProp(state, "bufferSize", juce::var(engine.getBufferSize()));
  setProp(state, "cpuUsage", juce::var(engine.getCpuUsage()));

  // Get time signature from tempo map if available
  setProp(state, "timeSignatureNumerator", juce::var(4));
  setProp(state, "timeSignatureDenominator", juce::var(4));

  return juce::var(state);
}

/**
 * @brief Read the tracks resource
 */
inline juce::var readTracks(Engine &engine, ProjectState &projectState) {
  juce::ignoreUnused(engine);
  juce::var tracks;

  auto tracksNode =
      projectState.getState().getChildWithName(ProjectState::ID_TRACKS);

  for (int i = 0; i < tracksNode.getNumChildren(); ++i) {
    auto trackNode = tracksNode.getChild(i);
    auto *trackObj = new juce::DynamicObject();

    setProp(trackObj, "id",
            trackNode.getProperty(ProjectState::PROP_ID, juce::var("")));
    setProp(
        trackObj, "name",
        trackNode.getProperty(ProjectState::PROP_NAME, juce::var("Untitled")));
    setProp(trackObj, "type",
            trackNode.getProperty(ProjectState::PROP_TYPE, juce::var("audio")));
    setProp(trackObj, "volume",
            trackNode.getProperty(ProjectState::PROP_VOLUME, juce::var(1.0)));
    setProp(trackObj, "pan",
            trackNode.getProperty(ProjectState::PROP_PAN, juce::var(0.0)));
    setProp(trackObj, "mute",
            trackNode.getProperty(ProjectState::PROP_MUTE, juce::var(false)));
    setProp(trackObj, "solo",
            trackNode.getProperty(ProjectState::PROP_SOLO, juce::var(false)));
    setProp(trackObj, "armed",
            trackNode.getProperty(ProjectState::PROP_ARMED, juce::var(false)));

    // Count clips
    auto clipsNode = trackNode.getChildWithName(ProjectState::ID_CLIPS);
    setProp(trackObj, "clipCount", juce::var(clipsNode.getNumChildren()));

    tracks.append(juce::var(trackObj));
  }

  auto *result = new juce::DynamicObject();
  setProp(result, "tracks", tracks);
  setProp(result, "count", juce::var((int)tracks.size()));

  return juce::var(result);
}

/**
 * @brief Read the history resource
 */
inline juce::var readHistory(ProjectState &projectState) {
  auto *result = new juce::DynamicObject();

  setProp(result, "canUndo", juce::var(projectState.canUndo()));
  setProp(result, "canRedo", juce::var(projectState.canRedo()));

  if (projectState.canUndo()) {
    setProp(result, "nextUndoDescription",
            juce::var(projectState.getUndoManager().getUndoDescription()));
  }

  if (projectState.canRedo()) {
    setProp(result, "nextRedoDescription",
            juce::var(projectState.getUndoManager().getRedoDescription()));
  }

  return juce::var(result);
}

/**
 * @brief Read a resource by URI
 */
inline juce::var readResource(const juce::String &uri, CommandAPI &commandAPI,
                              Engine &engine, ProjectState &projectState) {
  // Parse URI
  if (uri == "zenith://session/graph") {
    auto *paramsObj = new juce::DynamicObject();
    auto *reqObj = new juce::DynamicObject();
    setProp(reqObj, "command", juce::var("get_session_graph"));
    setProp(reqObj, "params", juce::var(paramsObj));
    return commandAPI.executeCommand(juce::var(reqObj));
  }

  if (uri == "zenith://tracks") {
    return readTracks(engine, projectState);
  }

  if (uri == "zenith://transport/state") {
    return readTransportState(engine);
  }

  if (uri == "zenith://markers") {
    auto *paramsObj = new juce::DynamicObject();
    auto *reqObj = new juce::DynamicObject();
    setProp(reqObj, "command", juce::var("get_markers"));
    setProp(reqObj, "params", juce::var(paramsObj));
    return commandAPI.executeCommand(juce::var(reqObj));
  }

  if (uri == "zenith://plugins/available") {
    auto *paramsObj = new juce::DynamicObject();
    auto *reqObj = new juce::DynamicObject();
    setProp(reqObj, "command", juce::var("list_plugins"));
    setProp(reqObj, "params", juce::var(paramsObj));
    return commandAPI.executeCommand(juce::var(reqObj));
  }

  if (uri == "zenith://history") {
    return readHistory(projectState);
  }

  if (uri == "zenith://ui/health") {
    auto *paramsObj = new juce::DynamicObject();
    auto *reqObj = new juce::DynamicObject();
    setProp(reqObj, "command", juce::var("get_ui_state"));
    setProp(reqObj, "params", juce::var(paramsObj));
    return commandAPI.executeCommand(juce::var(reqObj));
  }

  // Track-specific clips: zenith://tracks/{trackId}/clips
  if (uri.startsWith("zenith://tracks/") && uri.endsWith("/clips")) {
    juce::String trackId = uri.substring(16); // After "zenith://tracks/"
    trackId = trackId.upToLastOccurrenceOf("/clips", false, false);

    auto *paramsObj = new juce::DynamicObject();
    setProp(paramsObj, "trackId", juce::var(trackId));

    auto *reqObj = new juce::DynamicObject();
    setProp(reqObj, "command", juce::var("list_clips"));
    setProp(reqObj, "params", juce::var(paramsObj));
    return commandAPI.executeCommand(juce::var(reqObj));
  }

  // Unknown resource
  auto *error = new juce::DynamicObject();
  setProp(error, "error", juce::var("Resource not found: " + uri));
  return juce::var(error);
}

} // namespace resources
} // namespace mcp
} // namespace zenith
