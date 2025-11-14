/**
 * @file CommandAPI.cpp
 * @brief Command API implementation
 */

#include "../include/CommandAPI.h"
#include <juce_core/juce_core.h>

//==============================================================================
CommandAPI::CommandAPI(ProjectState& state)
    : projectState(state)
{
    DBG("CommandAPI: Initialized");
}

CommandAPI::~CommandAPI()
{
    DBG("CommandAPI: Destroyed");
}

//==============================================================================
juce::String CommandAPI::executeCommand(const juce::String& commandJson)
{
    // Parse JSON
    auto json = juce::JSON::parse(commandJson);

    if (!json.isObject())
    {
        auto response = createErrorResponse("Invalid JSON format");
        return juce::JSON::toString(response);
    }

    auto obj = json.getDynamicObject();
    if (obj == nullptr)
    {
        auto response = createErrorResponse("Invalid command object");
        return juce::JSON::toString(response);
    }

    juce::String command = obj->getProperty("command").toString();
    juce::var params = obj->getProperty("params");

    DBG("CommandAPI: Executing command: " + command);

    // Route to appropriate handler
    juce::var response;

    // Phase 8: MIDI commands
    if (command == "add_note")
        response = handleAddNote(params);
    else if (command == "delete_note")
        response = handleDeleteNote(params);
    else if (command == "move_note")
        response = handleMoveNote(params);
    else if (command == "quantize_clip")
        response = handleQuantizeClip(params);
    else if (command == "get_notes")
        response = handleGetNotes(params);

    // Phase 6/7: Undo/redo and queries
    else if (command == "undo")
        response = handleUndo(params);
    else if (command == "redo")
        response = handleRedo(params);
    else if (command == "session_graph")
        response = handleGetSessionGraph(params);

    else
        response = createErrorResponse("Unknown command: " + command);

    return juce::JSON::toString(response);
}

//==============================================================================
// Phase 8: MIDI Note Commands
//==============================================================================

juce::var CommandAPI::handleAddNote(const juce::var& params)
{
    if (!params.isObject())
        return createErrorResponse("Invalid params: expected object");

    auto obj = params.getDynamicObject();
    if (obj == nullptr)
        return createErrorResponse("Invalid params object");

    // Extract required parameters
    juce::String clipId = obj->getProperty("clipId").toString();
    if (clipId.isEmpty())
        return createErrorResponse("Missing required parameter: clipId");

    int pitch = obj->getProperty("pitch");
    double startBeats = obj->getProperty("startBeats");
    double lengthBeats = obj->getProperty("lengthBeats");

    // Validate
    if (pitch < 0 || pitch > 127)
        return createErrorResponse("Invalid pitch: must be 0-127");

    if (startBeats < 0.0)
        return createErrorResponse("Invalid startBeats: must be >= 0");

    if (lengthBeats <= 0.0)
        return createErrorResponse("Invalid lengthBeats: must be > 0");

    // Optional parameters
    int velocity = obj->getProperty("velocity", 100);
    velocity = juce::jlimit(0, 127, static_cast<int>(velocity));

    // Create note spec
    ProjectState::MidiNoteSpec note;
    note.pitch = pitch;
    note.startBeats = startBeats;
    note.lengthBeats = lengthBeats;
    note.velocity = velocity;
    note.muted = false;

    // Add note via ProjectState (undoable)
    juce::String noteId = projectState.addMidiNote(clipId, note, "Wingman: add_note");

    if (noteId.isEmpty())
        return createErrorResponse("Failed to add note - clip not found: " + clipId);

    // Return success with note ID
    auto data = new juce::DynamicObject();
    data->setProperty("noteId", noteId);
    data->setProperty("clipId", clipId);

    return createSuccessResponse(juce::var(data));
}

juce::var CommandAPI::handleDeleteNote(const juce::var& params)
{
    if (!params.isObject())
        return createErrorResponse("Invalid params: expected object");

    auto obj = params.getDynamicObject();
    if (obj == nullptr)
        return createErrorResponse("Invalid params object");

    juce::String clipId = obj->getProperty("clipId").toString();
    juce::String noteId = obj->getProperty("noteId").toString();

    if (clipId.isEmpty() || noteId.isEmpty())
        return createErrorResponse("Missing required parameters: clipId, noteId");

    // Delete note via ProjectState (undoable)
    projectState.removeMidiNote(clipId, noteId, "Wingman: delete_note");

    auto data = new juce::DynamicObject();
    data->setProperty("clipId", clipId);
    data->setProperty("noteId", noteId);

    return createSuccessResponse(juce::var(data));
}

juce::var CommandAPI::handleMoveNote(const juce::var& params)
{
    if (!params.isObject())
        return createErrorResponse("Invalid params: expected object");

    auto obj = params.getDynamicObject();
    if (obj == nullptr)
        return createErrorResponse("Invalid params object");

    juce::String clipId = obj->getProperty("clipId").toString();
    juce::String noteId = obj->getProperty("noteId").toString();

    if (clipId.isEmpty() || noteId.isEmpty())
        return createErrorResponse("Missing required parameters: clipId, noteId");

    // Get new position parameters
    int newPitch = obj->getProperty("newPitch", -1);
    double newStartBeats = obj->getProperty("newStartBeats", -1.0);

    // If not provided, keep current values (would need to query current note)
    // For now, require both parameters
    if (newPitch < 0 || newPitch > 127)
        return createErrorResponse("Invalid newPitch: must be 0-127");

    if (newStartBeats < 0.0)
        return createErrorResponse("Invalid newStartBeats: must be >= 0");

    // Move note via ProjectState (undoable)
    projectState.moveMidiNote(clipId, noteId, newStartBeats, newPitch, "Wingman: move_note");

    auto data = new juce::DynamicObject();
    data->setProperty("clipId", clipId);
    data->setProperty("noteId", noteId);
    data->setProperty("newPitch", newPitch);
    data->setProperty("newStartBeats", newStartBeats);

    return createSuccessResponse(juce::var(data));
}

juce::var CommandAPI::handleQuantizeClip(const juce::var& params)
{
    if (!params.isObject())
        return createErrorResponse("Invalid params: expected object");

    auto obj = params.getDynamicObject();
    if (obj == nullptr)
        return createErrorResponse("Invalid params object");

    juce::String clipId = obj->getProperty("clipId").toString();
    juce::String gridStr = obj->getProperty("grid").toString();

    if (clipId.isEmpty() || gridStr.isEmpty())
        return createErrorResponse("Missing required parameters: clipId, grid");

    // Parse grid string
    double gridBeats = parseGrid(gridStr);
    if (gridBeats <= 0.0)
        return createErrorResponse("Invalid grid format: " + gridStr);

    // Quantize via ProjectState (undoable)
    projectState.quantizeClip(clipId, gridBeats, "Wingman: quantize_clip");

    auto data = new juce::DynamicObject();
    data->setProperty("clipId", clipId);
    data->setProperty("grid", gridStr);
    data->setProperty("gridBeats", gridBeats);

    return createSuccessResponse(juce::var(data));
}

juce::var CommandAPI::handleGetNotes(const juce::var& params)
{
    if (!params.isObject())
        return createErrorResponse("Invalid params: expected object");

    auto obj = params.getDynamicObject();
    if (obj == nullptr)
        return createErrorResponse("Invalid params object");

    juce::String clipId = obj->getProperty("clipId").toString();
    if (clipId.isEmpty())
        return createErrorResponse("Missing required parameter: clipId");

    // Get notes from ProjectState
    auto notes = projectState.getMidiNotesForClip(clipId);

    // Convert to JSON array
    juce::Array<juce::var> notesArray;
    for (const auto& note : notes)
    {
        auto noteObj = new juce::DynamicObject();
        noteObj->setProperty("id", note.id);
        noteObj->setProperty("pitch", note.pitch);
        noteObj->setProperty("startBeats", note.startBeats);
        noteObj->setProperty("lengthBeats", note.lengthBeats);
        noteObj->setProperty("velocity", note.velocity);
        noteObj->setProperty("muted", note.muted);

        notesArray.add(juce::var(noteObj));
    }

    auto data = new juce::DynamicObject();
    data->setProperty("clipId", clipId);
    data->setProperty("notes", notesArray);
    data->setProperty("count", notes.size());

    return createSuccessResponse(juce::var(data));
}

//==============================================================================
// Phase 6/7: Previous commands
//==============================================================================

juce::var CommandAPI::handleUndo(const juce::var& params)
{
    if (projectState.canUndo())
    {
        projectState.undo();
        return createSuccessResponse();
    }
    else
    {
        return createErrorResponse("Nothing to undo");
    }
}

juce::var CommandAPI::handleRedo(const juce::var& params)
{
    if (projectState.canRedo())
    {
        projectState.redo();
        return createSuccessResponse();
    }
    else
    {
        return createErrorResponse("Nothing to redo");
    }
}

juce::var CommandAPI::handleGetSessionGraph(const juce::var& params)
{
    // Return basic project structure
    auto data = new juce::DynamicObject();
    data->setProperty("projectName", projectState.getProjectName());
    data->setProperty("tempo", projectState.getTempo());
    data->setProperty("numTracks", projectState.getNumTracks());

    return createSuccessResponse(juce::var(data));
}

//==============================================================================
// Helper Methods
//==============================================================================

double CommandAPI::parseGrid(const juce::String& grid) const
{
    // Parse common grid values (assumes 4/4 time signature)
    // Format: "1/16", "1/8", "1/4", "1/2", "1/1"

    if (grid == "1/16")
        return 0.25;  // 1/16 note = 0.25 beats in 4/4
    else if (grid == "1/8")
        return 0.5;   // 1/8 note = 0.5 beats
    else if (grid == "1/4")
        return 1.0;   // 1/4 note = 1 beat
    else if (grid == "1/2")
        return 2.0;   // 1/2 note = 2 beats
    else if (grid == "1/1")
        return 4.0;   // Whole note = 4 beats
    else
    {
        // Try to parse as "numerator/denominator"
        int slashPos = grid.indexOfChar('/');
        if (slashPos > 0)
        {
            int numerator = grid.substring(0, slashPos).getIntValue();
            int denominator = grid.substring(slashPos + 1).getIntValue();

            if (numerator > 0 && denominator > 0)
            {
                // Calculate beat value (assuming 4/4 time)
                // A 1/4 note = 1 beat, so 1/16 = 0.25 beats
                return (4.0 * numerator) / denominator;
            }
        }
    }

    return -1.0;  // Invalid
}

juce::var CommandAPI::createSuccessResponse(const juce::var& data) const
{
    auto response = new juce::DynamicObject();
    response->setProperty("status", "ok");

    if (!data.isVoid())
        response->setProperty("data", data);

    return juce::var(response);
}

juce::var CommandAPI::createErrorResponse(const juce::String& error) const
{
    auto response = new juce::DynamicObject();
    response->setProperty("status", "error");
    response->setProperty("error", error);

    return juce::var(response);
}
