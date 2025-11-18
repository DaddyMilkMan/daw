/*
  ==============================================================================

    CommandAPI.h
    Created: 2025-11-14
    Author:  Zenith DAW - Phase 5: Wingman v0

    JSON command processor for AI-driven DAW control

    Responsibilities:
    - Parse JSON command requests
    - Execute commands on Engine/ProjectState
    - Return JSON responses with results or errors
    - Provide get_session_graph for full project state snapshot

    Commands:
    - list_tracks: Get all tracks with basic info
    - create_track: Create new audio or MIDI track
    - delete_track: Remove track by ID
    - rename_track: Change track name
    - list_clips: Get clips for a specific track
    - split_clip: Split clip at specified time
    - move_clip: Move clip to new position
    - set_track_volume: Adjust track volume (dB)
    - set_track_pan: Adjust track pan (-1.0 to 1.0)
    - get_session_graph: Export full project state as JSON

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <memory>

// Forward declarations
class Engine;
class ProjectState;

namespace zenith {

//==============================================================================
/**
    JSON-based command API for DAW control

    All operations execute on the message thread (RT-safe).
    Uses juce::var for JSON parsing/serialization.
*/
class CommandAPI
{
public:
    //==============================================================================
    CommandAPI(Engine& eng, ProjectState& state);
    ~CommandAPI();

    //==============================================================================
    /**
        Execute a JSON command and return JSON response

        Request format:
        {
            "command": "command_name",
            "params": { ... }
        }

        Response format (success):
        {
            "success": true,
            "result": { ... }
        }

        Response format (error):
        {
            "success": false,
            "error": "error message"
        }
    */
    juce::var executeCommand(const juce::var& request);

    //==============================================================================
    /**
        Parse and execute command from JSON string
        Convenience wrapper around executeCommand()
    */
    juce::String executeCommandString(const juce::String& jsonRequest);

    /**
        Execute a batch of commands in a single undo transaction

        @param commands Array of command objects {command, params}
        @param batchName Name for the undo transaction
        @return Response with status and results

        Response format (success):
        {
            "success": true,
            "result": { "count": N }
        }

        Response format (error):
        {
            "success": false,
            "error": "error message",
            "failedIndex": N,
            "failedCommand": {...},
            "successCount": N
        }
    */
    juce::var executeBatch(const juce::Array<juce::var>& commands,
                           const juce::String& batchName = "Wingman AI batch");

private:
    //==============================================================================
    // Command handlers (all return juce::var response)

    // Track commands
    juce::var listTracks(const juce::var& params);
    juce::var createTrack(const juce::var& params);
    juce::var deleteTrack(const juce::var& params);
    juce::var renameTrack(const juce::var& params);
    juce::var setTrackVolume(const juce::var& params);
    juce::var setTrackPan(const juce::var& params);

    // Clip commands
    juce::var listClips(const juce::var& params);
    juce::var createClip(const juce::var& params);
    juce::var deleteClip(const juce::var& params);
    juce::var splitClip(const juce::var& params);
    juce::var moveClip(const juce::var& params);

    // Session/project commands
    juce::var getSessionGraph(const juce::var& params);

    // Undo/redo commands
    juce::var undo(const juce::var& params);
    juce::var redo(const juce::var& params);
    juce::var history(const juce::var& params);

    //==============================================================================
    // Helper methods

    juce::var createSuccessResponse(const juce::var& result);
    juce::var createErrorResponse(const juce::String& errorMessage);

    Track* findTrackById(const juce::String& trackId);
    Track::Clip* findClipById(Track* track, const juce::String& clipId);

    //==============================================================================
    // Member variables

    Engine& engine;
    ProjectState& projectState;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CommandAPI)
};

} // namespace zenith
