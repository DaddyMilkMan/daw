/**
 * @file CommandAPI.h
 * @brief JSON command API for Wingman/AI integration
 *
 * Phase 13: Track Automation MVP
 *
 * Provides a JSON-based command interface for external tools (Wingman AI) to:
 * - Query project state
 * - Modify automation
 * - Control playback
 * - Manage tracks
 *
 * All commands follow this format:
 * Request:
 * {
 *   "command": "command_name",
 *   "params": { ... }
 * }
 *
 * Response:
 * {
 *   "status": "ok" | "error",
 *   "data": { ... },      // if status == "ok"
 *   "error": "message"    // if status == "error"
 * }
 */

#pragma once

#include <JuceHeader.h>
#include "ProjectState.h"
#include "Engine.h"
#include <functional>
#include <map>
#include <memory>

//==============================================================================
/**
 * @class CommandAPI
 * @brief JSON command interface for external automation
 *
 * Thread Safety:
 * - All methods run on MESSAGE THREAD
 * - Uses ProjectState which is message-thread only
 */
class CommandAPI
{
public:
    //==========================================================================
    /**
     * @brief Constructor
     * @param projectState Reference to project state
     * @param engine Reference to engine
     */
    CommandAPI(ProjectState& projectState, Engine& engine);

    /**
     * @brief Destructor
     */
    ~CommandAPI();

    //==========================================================================
    /**
     * @brief Execute a JSON command
     * @param commandJson JSON string containing command and params
     * @return JSON string containing response (status, data/error)
     * @note Message thread only
     */
    juce::String executeCommand(const juce::String& commandJson);

    //==========================================================================
    /**
     * @brief Register custom command handler
     * @param commandName Command name
     * @param handler Function that takes params JSON and returns response JSON
     */
    using CommandHandler = std::function<juce::var(const juce::var& params)>;
    void registerCommand(const juce::String& commandName, CommandHandler handler);

private:
    //==========================================================================
    // Built-in command handlers
    //==========================================================================

    /**
     * @brief Add automation point
     * Params: { "trackId": "track_1", "param": "volume", "timeBeats": 8.0, "value": 0.5 }
     * Returns: { "pointId": "point_123" }
     */
    juce::var cmd_addAutomationPoint(const juce::var& params);

    /**
     * @brief Clear automation for a parameter
     * Params: { "trackId": "track_1", "param": "volume" }
     * Returns: { "success": true }
     */
    juce::var cmd_clearAutomation(const juce::var& params);

    /**
     * @brief Get automation data
     * Params: { "trackId": "track_1", "param": "volume" }
     * Returns: { "points": [ { "id": "...", "timeBeats": 0.0, "value": 0.8 }, ... ] }
     */
    juce::var cmd_getAutomation(const juce::var& params);

    /**
     * @brief Add a track
     * Params: { "name": "Audio 1", "type": "audio" }
     * Returns: { "trackId": "track_123" }
     */
    juce::var cmd_addTrack(const juce::var& params);

    /**
     * @brief Get project info
     * Params: {}
     * Returns: { "name": "...", "tempo": 120.0, "numTracks": 3, ... }
     */
    juce::var cmd_getProjectInfo(const juce::var& params);

    /**
     * @brief Set tempo
     * Params: { "tempo": 140.0 }
     * Returns: { "success": true }
     */
    juce::var cmd_setTempo(const juce::var& params);

    /**
     * @brief Export project to WAV file
     * Params: { "outputPath": "/path/to/output.wav", "durationSeconds": 10.0, "sampleRate": 44100.0 }
     * Returns: { "success": true }
     */
    juce::var cmd_exportWav(const juce::var& params);

    //==========================================================================
    // Transport commands
    //==========================================================================

    /**
     * @brief Start playback
     * Params: {}
     * Returns: { "success": true }
     */
    juce::var cmd_play(const juce::var& params);

    /**
     * @brief Stop playback
     * Params: {}
     * Returns: { "success": true }
     */
    juce::var cmd_stop(const juce::var& params);

    /**
     * @brief Toggle recording state
     * Params: { "enabled": true }
     * Returns: { "success": true, "recording": true }
     */
    juce::var cmd_record(const juce::var& params);

    /**
     * @brief Toggle loop state
     * Params: { "enabled": true }
     * Returns: { "success": true, "looping": true }
     */
    juce::var cmd_loop(const juce::var& params);

    /**
     * @brief Set playback position
     * Params: { "positionBeats": 8.0 }
     * Returns: { "success": true }
     */
    juce::var cmd_setPosition(const juce::var& params);

    //==========================================================================
    // Undo/Redo commands
    //==========================================================================

    /**
     * @brief Undo last action
     * Params: {}
     * Returns: { "success": true }
     */
    juce::var cmd_undo(const juce::var& params);

    /**
     * @brief Redo last undone action
     * Params: {}
     * Returns: { "success": true }
     */
    juce::var cmd_redo(const juce::var& params);

    //==========================================================================
    // Clip management commands
    //==========================================================================

    /**
     * @brief Create a clip on a track
     * Params: { "trackId": "track_0", "startBeats": 0.0, "lengthBeats": 4.0 }
     * Returns: { "clipId": "clip_123" }
     */
    juce::var cmd_createClip(const juce::var& params);

    /**
     * @brief Delete a clip
     * Params: { "trackId": "track_0", "clipId": "clip_123" }
     * Returns: { "success": true }
     */
    juce::var cmd_deleteClip(const juce::var& params);

    /**
     * @brief Get clips on a track
     * Params: { "trackId": "track_0" }
     * Returns: { "clips": [ { "id": "...", "startBeats": 0.0, "lengthBeats": 4.0 }, ... ] }
     */
    juce::var cmd_getTrackClips(const juce::var& params);

    /**
     * @brief Set audio file for a clip
     * Params: { "trackId": "track_0", "clipId": "clip_123", "path": "/path/to/audio.wav" }
     * Returns: { "success": true }
     */
    juce::var cmd_setClipAudioFile(const juce::var& params);

    /**
     * @brief Create audio clip with file in one step
     * Params: { "trackId": "track_0", "startBeats": 0.0, "lengthBeats": 4.0, "path": "/path/to/audio.wav" }
     * Returns: { "clipId": "clip_123" }
     */
    juce::var cmd_createAudioClip(const juce::var& params);

    //==========================================================================
    // MIDI note commands (merged from both branches)
    //==========================================================================

    /**
     * @brief Add MIDI note
     * Params: { "clipId": "clip_1", "startBeats": 0.0, "lengthBeats": 1.0, "pitch": 60, "velocity": 100 }
     * Returns: { "noteId": "note_123" }
     */
    juce::var cmd_addNote(const juce::var& params);

    /**
     * @brief Move/edit MIDI note
     * Params: { "clipId": "clip_1", "noteId": "note_1", "startBeats": 0.0, "lengthBeats": 1.0, "pitch": 60, "velocity": 100 }
     * Returns: { "success": true }
     */
    juce::var cmd_moveNote(const juce::var& params);

    /**
     * @brief Create MIDI note in a clip
     * Params: { "trackId": "track_0", "clipId": "clip_123", "note": 60, "velocity": 100, "startBeats": 0.0, "lengthBeats": 1.0 }
     * Returns: { "success": false, "message": "MIDI note API not yet implemented in ProjectState" }
     */
    juce::var cmd_createNote(const juce::var& params);

    /**
     * @brief Delete MIDI note
     * Params: { "clipId": "clip_1", "noteId": "note_1" } OR { "trackId": "track_0", "clipId": "clip_123", "noteId": "note_123" }
     * Returns: { "success": true }
     */
    juce::var cmd_deleteNote(const juce::var& params);

    /**
     * @brief Get all notes for a clip
     * Params: { "clipId": "clip_1" }
     * Returns: { "notes": [ { "id": "...", "startBeats": 0.0, "lengthBeats": 1.0, "pitch": 60, "velocity": 100 }, ... ] }
     */
    juce::var cmd_getNotes(const juce::var& params);

    /**
     * @brief Get MIDI notes in a clip
     * Params: { "trackId": "track_0", "clipId": "clip_123" }
     * Returns: { "notes": [] }
     */
    juce::var cmd_getClipNotes(const juce::var& params);

    //==========================================================================
    // Plugin commands (stubbed - plugins not yet implemented)
    //==========================================================================


    /**
     * @brief Scan for plugins
     * Params: { "force": true } (optional)
     * Returns: { "count": 0, "message": "Plugin system not yet implemented" }
     */
    juce::var cmd_scanPlugins(const juce::var& params);

    /**
     * @brief Get available plugins
     * Params: { "type": "instrument"|"effect", "searchTerm": "..." } (all optional)
     * Returns: { "plugins": [] }
     */
    juce::var cmd_getPlugins(const juce::var& params);

    /**
     * @brief Add plugin to track
     * Params: { "trackId": "track_0", "pluginId": "some-plugin-id" }
     * Returns: { "success": false, "message": "Plugin system not yet implemented" }
     */
    juce::var cmd_addTrackPlugin(const juce::var& params);

    /**
     * @brief Remove plugin from track
     * Params: { "trackId": "track_0", "pluginIndex": 0 }
     * Returns: { "success": false, "message": "Plugin system not yet implemented" }
     */
    juce::var cmd_removeTrackPlugin(const juce::var& params);

    /**
     * @brief Set plugin bypassed state
     * Params: { "trackId": "track_0", "pluginIndex": 0, "bypassed": true }
     * Returns: { "success": false, "message": "Plugin system not yet implemented" }
     */
    juce::var cmd_setTrackPluginBypassed(const juce::var& params);

    /**
     * @brief Get track's plugin chain
     * Params: { "trackId": "track_0" }
     * Returns: { "plugins": [] }
     */
    juce::var cmd_getTrackPlugins(const juce::var& params);

    /**
     * @brief Export project to WAV
     * Params: { "path": "/path/to/output.wav", "startBeats": 0.0, "endBeats": 16.0 } (start/end optional)
     * Returns: { "success": true, "outputPath": "..." }
     */
    juce::var cmd_exportProject(const juce::var& params);

    //==========================================================================
    // Instrument commands
    //==========================================================================

    /**
     * @brief List all available instruments
     * Params: {}
     * Returns: { "instruments": [{ "id": "...", "name": "...", "category": "..." }, ...] }
     */
    juce::var cmd_listInstruments(const juce::var& params);

    /**
     * @brief List presets for an instrument
     * Params: { "instrumentId": "zenith_poly_synth", "category": "...", "tag": "..." } (category/tag optional)
     * Returns: { "presets": [{ "id": "...", "name": "...", "category": "...", "tags": [...] }, ...] }
     */
    juce::var cmd_listPresets(const juce::var& params);

    /**
     * @brief Load preset on a track's instrument
     * Params: { "trackId": "track_0", "instrumentId": "...", "presetId": "..." } (instrumentId optional if track has instrument)
     * Returns: { "success": true }
     */
    juce::var cmd_loadPreset(const juce::var& params);

    /**
     * @brief Save current instrument state as preset
     * Params: { "trackId": "track_0", "name": "My Preset", "category": "...", "tags": [...] } (category/tags optional)
     * Returns: { "success": true, "presetId": "..." }
     */
    juce::var cmd_savePreset(const juce::var& params);

    /**
     * @brief Get all instrument parameters for a track
     * Params: { "trackId": "track_0" }
     * Returns: { "parameters": [{ "id": "...", "name": "...", "min": 0, "max": 1, "default": 0.5, "value": 0.7, "tags": [...] }, ...] }
     */
    juce::var cmd_getInstrumentParameters(const juce::var& params);

    /**
     * @brief Set multiple instrument parameters at once
     * Params: { "trackId": "track_0", "params": { "filter_cutoff": 0.8, "filter_resonance": 0.3 } }
     * Returns: { "success": true, "updated": ["filter_cutoff", "filter_resonance"] }
     */
    juce::var cmd_setInstrumentParameters(const juce::var& params);

    /**
     * @brief Set instrument on a track
     * Params: { "trackId": "track_0", "instrumentId": "zenith_poly_synth" }
     * Returns: { "success": true }
     */
    juce::var cmd_setInstrumentOnTrack(const juce::var& params);

    /**
     * @brief Set a single instrument parameter
     * Params: { "trackId": "track_0", "paramId": "filter_cutoff", "value": 0.8 }
     * Returns: { "success": true, "value": 0.8 }
     */
    juce::var cmd_setInstrumentParam(const juce::var& params);

    /**
     * @brief Get a single instrument parameter value
     * Params: { "trackId": "track_0", "paramId": "filter_cutoff" }
     * Returns: { "paramId": "filter_cutoff", "value": 0.65, "min": 0.0, "max": 1.0 }
     */
    juce::var cmd_getInstrumentParam(const juce::var& params);

    /**
     * @brief Randomize instrument parameters with safe ranges
     * Params: { "trackId": "track_0", "intensity": 0.5 } (intensity optional, 0.0-1.0)
     * Returns: { "success": true, "randomized": ["filter_cutoff", "filter_resonance", ...] }
     */
    juce::var cmd_randomizeInstrumentParams(const juce::var& params);

    //==========================================================================
    // Phase 15: Tempo map and markers commands
    //==========================================================================

    /**
     * @brief Add tempo change
     * Params: { "beatPosition": 8.0, "bpm": 140.0, "timeSigNumerator": 4, "timeSigDenominator": 4 }
     * Returns: { "tempoId": "tempo_123" }
     */
    juce::var cmd_addTempoChange(const juce::var& params);

    /**
     * @brief Get tempo map
     * Params: {}
     * Returns: { "tempoChanges": [ { "id": "...", "beatPosition": 0.0, "bpm": 120.0, ... }, ... ] }
     */
    juce::var cmd_getTempoMap(const juce::var& params);

    /**
     * @brief Add marker
     * Params: { "beatPosition": 4.0, "name": "Verse", "color": "#FFCC00" }
     * Returns: { "markerId": "marker_123" }
     */
    juce::var cmd_addMarker(const juce::var& params);

    /**
     * @brief Get markers
     * Params: {}
     * Returns: { "markers": [ { "id": "...", "name": "...", "beatPosition": 0.0, "color": "..." }, ... ] }
     */
    juce::var cmd_getMarkers(const juce::var& params);

    /**
     * @brief Delete marker
     * Params: { "markerId": "marker_123" }
     * Returns: { "success": true }
     */
    juce::var cmd_deleteMarker(const juce::var& params);

    /**
     * @brief Go to marker (set playhead to marker position)
     * Params: { "markerId": "marker_123" }
     * Returns: { "success": true, "beatPosition": 4.0 }
     */
    juce::var cmd_gotoMarker(const juce::var& params);

    //==========================================================================
    // Helper methods
    //==========================================================================

    /**
     * @brief Create success response
     */
    juce::String createResponse(const juce::var& data) const;

    /**
     * @brief Create error response
     */
    juce::String createErrorResponse(const juce::String& errorMessage) const;

    /**
     * @brief Validate parameter exists
     */
    bool validateParam(const juce::var& params, const juce::String& paramName, juce::String& errorOut) const;

    //==========================================================================
    // Member Variables
    //==========================================================================

    ProjectState& projectState;
    Engine& engine;

    // Custom command handlers
    std::map<juce::String, CommandHandler> commandHandlers;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CommandAPI)
};
