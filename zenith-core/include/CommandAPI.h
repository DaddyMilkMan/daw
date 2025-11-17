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
     * @brief Delete a track
     * Params: { "trackId": "track_1" }
     * Returns: { "success": true }
     */
    juce::var cmd_deleteTrack(const juce::var& params);

    /**
     * @brief Rename a track
     * Params: { "trackId": "track_1", "name": "New Name" }
     * Returns: { "success": true }
     */
    juce::var cmd_renameTrack(const juce::var& params);

    /**
     * @brief Get list of all tracks
     * Params: {}
     * Returns: { "tracks": [ { "id": "...", "name": "...", "type": "...", ... }, ... ] }
     */
    juce::var cmd_getTracks(const juce::var& params);

    /**
     * @brief Set track property
     * Params: { "trackId": "track_1", "property": "volume", "value": 0.8 }
     * Returns: { "success": true }
     */
    juce::var cmd_setTrackProperty(const juce::var& params);

    /**
     * @brief Create a clip
     * Params: { "trackId": "track_1", "startBeats": 0.0, "lengthBeats": 4.0, "type": "midi" }
     * Returns: { "clipId": "clip_123" }
     */
    juce::var cmd_createClip(const juce::var& params);

    /**
     * @brief Delete a clip
     * Params: { "trackId": "track_1", "clipId": "clip_1" }
     * Returns: { "success": true }
     */
    juce::var cmd_deleteClip(const juce::var& params);

    /**
     * @brief Move a clip
     * Params: { "trackId": "track_1", "clipId": "clip_1", "startBeats": 8.0 }
     * Returns: { "success": true }
     */
    juce::var cmd_moveClip(const juce::var& params);

    /**
     * @brief Resize a clip
     * Params: { "trackId": "track_1", "clipId": "clip_1", "lengthBeats": 8.0 }
     * Returns: { "success": true }
     */
    juce::var cmd_resizeClip(const juce::var& params);

    /**
     * @brief Get clips for a track
     * Params: { "trackId": "track_1" }
     * Returns: { "clips": [ { "id": "...", "type": "...", "start": 0.0, "length": 4.0 }, ... ] }
     */
    juce::var cmd_getClips(const juce::var& params);

    /**
     * @brief Create a MIDI note
     * Params: { "trackId": "track_1", "clipId": "clip_1", "startBeats": 0.0, "lengthBeats": 1.0, "pitch": 60, "velocity": 100 }
     * Returns: { "noteId": "note_123" }
     */
    juce::var cmd_createNote(const juce::var& params);

    /**
     * @brief Delete a MIDI note
     * Params: { "trackId": "track_1", "clipId": "clip_1", "noteId": "note_1" }
     * Returns: { "success": true }
     */
    juce::var cmd_deleteNote(const juce::var& params);

    /**
     * @brief Move a MIDI note
     * Params: { "trackId": "track_1", "clipId": "clip_1", "noteId": "note_1", "startBeats": 2.0, "pitch": 62 }
     * Returns: { "success": true }
     */
    juce::var cmd_moveNote(const juce::var& params);

    /**
     * @brief Resize a MIDI note
     * Params: { "trackId": "track_1", "clipId": "clip_1", "noteId": "note_1", "lengthBeats": 2.0 }
     * Returns: { "success": true }
     */
    juce::var cmd_resizeNote(const juce::var& params);

    /**
     * @brief Get notes for a clip
     * Params: { "trackId": "track_1", "clipId": "clip_1" }
     * Returns: { "notes": [ { "id": "...", "start": 0.0, "length": 1.0, "pitch": 60, "velocity": 100 }, ... ] }
     */
    juce::var cmd_getNotes(const juce::var& params);

    /**
     * @brief Delete automation point
     * Params: { "trackId": "track_1", "param": "volume", "pointId": "point_1" }
     * Returns: { "success": true }
     */
    juce::var cmd_deleteAutomationPoint(const juce::var& params);

    /**
     * @brief Move automation point
     * Params: { "trackId": "track_1", "param": "volume", "pointId": "point_1", "timeBeats": 4.0, "value": 0.5 }
     * Returns: { "success": true }
     */
    juce::var cmd_moveAutomationPoint(const juce::var& params);

    /**
     * @brief Play
     * Params: {}
     * Returns: { "success": true }
     */
    juce::var cmd_play(const juce::var& params);

    /**
     * @brief Stop
     * Params: {}
     * Returns: { "success": true }
     */
    juce::var cmd_stop(const juce::var& params);

    /**
     * @brief Undo
     * Params: {}
     * Returns: { "success": true }
     */
    juce::var cmd_undo(const juce::var& params);

    /**
     * @brief Redo
     * Params: {}
     * Returns: { "success": true }
     */
    juce::var cmd_redo(const juce::var& params);

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
