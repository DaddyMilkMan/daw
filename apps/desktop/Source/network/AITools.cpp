/*
  ==============================================================================

    AITools.cpp
    Created: 2025-12-03
    Author:  Zenith DAW

  ==============================================================================
*/

#include "AITools.h"

namespace zenith {

// Helper to create function definition from JSON schema string
static GrokFunction createFunctionDef(
    const juce::String& name,
    const juce::String& description,
    const juce::String& schemaJson)
{
    auto schema = juce::JSON::parse(schemaJson);
    return GrokFunction(name, description, schema);
}

juce::Array<GrokFunction> AITools::getAvailableFunctions()
{
    juce::Array<GrokFunction> myFunctions;
    
    // Track functions
    myFunctions.add(createFunctionDef(
        "create_track",
        "Create a new audio or MIDI track",
        "{"
        "  \"type\": \"object\","
        "  \"properties\": {"
        "    \"name\": {\"type\": \"string\", \"description\": \"Track name\"},"
        "    \"type\": {\"type\": \"string\", \"enum\": [\"audio\", \"midi\"], \"description\": \"Track type\"}"
        "  },"
        "  \"required\": [\"name\", \"type\"]"
        "}"
    ));
    
    myFunctions.add(createFunctionDef(
        "list_tracks",
        "Get list of all tracks in the project",
        "{\"type\": \"object\", \"properties\": {}}"
    ));
    
    myFunctions.add(createFunctionDef(
        "delete_track",
        "Delete a track by ID",
        "{"
        "  \"type\": \"object\","
        "  \"properties\": {"
        "    \"trackId\": {\"type\": \"string\", \"description\": \"Track ID to delete\"}"
        "  },"
        "  \"required\": [\"trackId\"]"
        "}"
    ));

    // Audio Analysis
    myFunctions.add(createFunctionDef(
        "analyze_track",
        "Analyze the audio of a specific track or the master mix to provide feedback",
        "{"
        "  \"type\": \"object\","
        "  \"properties\": {"
        "    \"trackId\": {\"type\": \"string\", \"description\": \"Track ID to analyze, or 'master' for the full mix\"},"
        "    \"duration\": {\"type\": \"number\", \"description\": \"Duration to analyze in seconds (default 10.0)"}"
        "  },"
        "  \"required\": [\"trackId\"]"
        "}"
    ));
    
    // Preset functions
    myFunctions.add(createFunctionDef(
        "list_presets",
        "List available presets for an instrument",
        "{"
        "  \"type\": \"object\","
        "  \"properties\": {"
        "    \"instrumentId\": {\"type\": \"string\", \"description\": \"Instrument ID\"},"
        "    \"name\": {\"type\": \"string\", \"description\": \"Preset name\"},"
        "    \"parameters\": {\"type\": \"object\", \"description\": \"Preset parameters\"}"
        "  },"
        "  \"required\": [\"instrumentId\", \"name\", \"parameters\"]"
        "}"
    ));

    // AI Audio Processing
    myFunctions.add(createFunctionDef(
        "separate_stems",
        "Separate audio track into stems (vocals, drums, bass, other)",
        "{"
        "  \"type\": \"object\","
        "  \"properties\": {"
        "    \"trackId\": {\"type\": \"string\", \"description\": \"Track ID to separate\"}"
        "  },"
        "  \"required\": [\"trackId\"]"
        "}"
    ));

    // MIDI functions
    myFunctions.add(createFunctionDef(
        "add_note",
        "Add a MIDI note to a clip",
        "{"
        "  \"type\": \"object\","
        "  \"properties\": {"
        "    \"trackId\": {\"type\": \"string\"},"
        "    \"clipId\": {\"type\": \"string\"},"
        "    \"pitch\": {\"type\": \"integer\", \"description\": \"MIDI note number (0-127)"},"
        "    \"startBeats\": {\"type\": \"number\", \"description\": \"Start position in beats"},"
        "    \"lengthBeats\": {\"type\": \"number\", \"description\": \"Note length in beats"},"
        "    \"velocity\": {\"type\": \"integer\", \"description\": \"Velocity (0-127)"}"
        "  },"
        "  \"required\": [\"trackId\", \"clipId\", \"pitch\", \"startBeats\", \"lengthBeats\"]"
        "}"
    ));
    
    // Tempo control
    myFunctions.add(createFunctionDef(
        "set_tempo",
        "Set the project tempo",
        "{"
        "  \"type\": \"object\","
        "  \"properties\": {"
        "    \"bpm\": {\"type\": \"number\", \"description\": \"Tempo in BPM\"}"
        "  },"
        "  \"required\": [\"bpm\"]"
        "}"
    ));

    // MIDI Generation
    myFunctions.add(createFunctionDef(
        "generate_midi_pattern",
        "Generate a MIDI pattern (bassline, melody, or full chord progression) on a track",
        "{"
        "  \"type\": \"object\","
        "  \"properties\": {"
        "    \"trackId\": {\"type\": \"string\", \"description\": \"Track ID\"},"
        "    \"description\": {\"type\": \"string\", \"description\": \"Description (e.g., 'Neo-Soul chord progression in Eb Minor')\"},"
        "    \"lengthBeats\": {\"type\": \"number\", \"description\": \"Length in beats (default 16)"},"
        "    \"type\": {\"type\": \"string\", \"enum\": [\"melody\", \"bass\", \"chords\"], \"description\": \"Pattern type\"}"
        "  },"
        "  \"required\": [\"trackId\", \"description\"]"
        "}"
    ));

    // Lyric Generation
    myFunctions.add(createFunctionDef(
        "generate_lyrics",
        "Generate lyrics for a song based on a theme or mood",
        "{"
        "  \"type\": \"object\","
        "  \"properties\": {"
        "    \"theme\": {\"type\": \"string\", \"description\": \"Theme, topic, or mood\"},"
        "    \"style\": {\"type\": \"string\", \"description\": \"Style (e.g., 'Rap', 'Pop', 'Country')\"},"
        "    \"structure\": {\"type\": \"string\", \"description\": \"Structure (e.g., 'Verse-Chorus-Verse')\"}"
        "  },"
        "  \"required\": [\"theme\"]"
        "}"
    ));

    return myFunctions;
}

} // namespace zenith
