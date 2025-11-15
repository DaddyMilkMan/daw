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
     * @brief Add tempo point (Phase 15)
     * Params: { "timeBeats": 8.0, "bpm": 140.0 }
     * Returns: { "pointId": "tempopoint_123" }
     */
    juce::var cmd_addTempoPoint(const juce::var& params);

    /**
     * @brief Delete tempo point (Phase 15)
     * Params: { "pointId": "tempopoint_123" }
     * Returns: { "success": true }
     */
    juce::var cmd_deleteTempoPoint(const juce::var& params);

    /**
     * @brief Get tempo map (Phase 15)
     * Params: {}
     * Returns: { "points": [ { "id": "...", "timeBeats": 0.0, "bpm": 120.0 }, ... ] }
     */
    juce::var cmd_getTempoMap(const juce::var& params);

    /**
     * @brief Add marker (Phase 15)
     * Params: { "timeBeats": 16.0, "name": "Verse" }
     * Returns: { "markerId": "marker_123" }
     */
    juce::var cmd_addMarker(const juce::var& params);

    /**
     * @brief Move marker (Phase 15)
     * Params: { "markerId": "marker_123", "timeBeats": 20.0 }
     * Returns: { "success": true }
     */
    juce::var cmd_moveMarker(const juce::var& params);

    /**
     * @brief Rename marker (Phase 15)
     * Params: { "markerId": "marker_123", "name": "Chorus" }
     * Returns: { "success": true }
     */
    juce::var cmd_renameMarker(const juce::var& params);

    /**
     * @brief Delete marker (Phase 15)
     * Params: { "markerId": "marker_123" }
     * Returns: { "success": true }
     */
    juce::var cmd_deleteMarker(const juce::var& params);

    /**
     * @brief Get markers (Phase 15)
     * Params: {}
     * Returns: { "markers": [ { "id": "...", "timeBeats": 0.0, "name": "..." }, ... ] }
     */
    juce::var cmd_getMarkers(const juce::var& params);

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
