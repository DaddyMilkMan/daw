/**
 * @file CommandAPI.h
 * @brief Command API for Wingman AI integration (Phase 7/8)
 *
 * Provides a JSON-based command interface for:
 * - Track operations (create, delete, modify)
 * - Clip operations (create, delete, move, split)
 * - MIDI note operations (add, delete, move, quantize) - Phase 8
 * - Project queries (session graph, state inspection)
 * - Undo/redo
 *
 * All commands are undoable via ProjectState's UndoManager.
 * Commands execute on the message thread only (never RT thread).
 */

#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include "ProjectState.h"

//==============================================================================
/**
 * @class CommandAPI
 * @brief Executes JSON commands on the project state
 *
 * Example command format:
 * {
 *   "command": "add_note",
 *   "params": {
 *     "clipId": "clip_3",
 *     "pitch": 60,
 *     "startBeats": 4.0,
 *     "lengthBeats": 1.0,
 *     "velocity": 100
 *   }
 * }
 *
 * Response format:
 * {
 *   "status": "ok",
 *   "data": { ... }
 * }
 * or
 * {
 *   "status": "error",
 *   "error": "Error message"
 * }
 */
class CommandAPI
{
public:
    //==========================================================================
    explicit CommandAPI(ProjectState& state);
    ~CommandAPI();

    //==========================================================================
    /**
     * @brief Execute a command from JSON string
     * @param commandJson JSON command string
     * @return JSON response string
     */
    juce::String executeCommand(const juce::String& commandJson);

    //==========================================================================
    // Phase 8: MIDI Note Commands
    //==========================================================================

    /**
     * @brief Add a MIDI note to a clip
     * Command: "add_note"
     * Params: clipId, pitch, startBeats, lengthBeats, velocity (optional)
     */
    juce::var handleAddNote(const juce::var& params);

    /**
     * @brief Delete a MIDI note from a clip
     * Command: "delete_note"
     * Params: clipId, noteId
     */
    juce::var handleDeleteNote(const juce::var& params);

    /**
     * @brief Move/modify a MIDI note
     * Command: "move_note"
     * Params: clipId, noteId, newPitch, newStartBeats
     */
    juce::var handleMoveNote(const juce::var& params);

    /**
     * @brief Quantize all notes in a clip to a grid
     * Command: "quantize_clip"
     * Params: clipId, grid (e.g., "1/16", "1/8", "1/4")
     */
    juce::var handleQuantizeClip(const juce::var& params);

    /**
     * @brief Get all notes in a clip
     * Command: "get_notes"
     * Params: clipId
     */
    juce::var handleGetNotes(const juce::var& params);

    //==========================================================================
    // Previous phase commands (Phase 6/7)
    //==========================================================================

    juce::var handleUndo(const juce::var& params);
    juce::var handleRedo(const juce::var& params);
    juce::var handleGetSessionGraph(const juce::var& params);

private:
    //==========================================================================
    /**
     * @brief Parse grid string to beat value
     * @param grid Grid string (e.g., "1/16" = 0.25 beats at 4/4)
     * @return Beat value, or -1.0 if invalid
     */
    double parseGrid(const juce::String& grid) const;

    /**
     * @brief Create success response
     */
    juce::var createSuccessResponse(const juce::var& data = juce::var()) const;

    /**
     * @brief Create error response
     */
    juce::var createErrorResponse(const juce::String& error) const;

    //==========================================================================
    ProjectState& projectState;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CommandAPI)
};
