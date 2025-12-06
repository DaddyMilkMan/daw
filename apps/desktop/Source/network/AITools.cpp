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
    // Use simple string concatenation to avoid raw string literal issues with some compilers/settings
    juce::String trackSchema = "{";
    trackSchema += "\"type\": \"object\",";
    trackSchema += "\"properties\": {";
    trackSchema += "\"name\": { \"type\": \"string\", \"description\": \"Track name\" },";
    trackSchema += "\"type\": { \"type\": \"string\", \"enum\": [\"audio\", \"midi\"], \"description\": \"Track type\" }";
    trackSchema += "},";
    trackSchema += "\"required\": [\"name\", \"type\"]";
    trackSchema += "}";
    
    myFunctions.add(createFunctionDef("create_track", "Create a new audio or MIDI track", trackSchema));
    
    myFunctions.add(createFunctionDef("list_tracks", "Get list of all tracks in the project", 
        "{ \"type\": \"object\", \"properties\": {} }"));
    
    juce::String deleteSchema = "{";
    deleteSchema += "\"type\": \"object\",";
    deleteSchema += "\"properties\": {";
    deleteSchema += "\"trackId\": { \"type\": \"string\", \"description\": \"Track ID to delete\" }";
    deleteSchema += "},";
    deleteSchema += "\"required\": [\"trackId\"]";
    deleteSchema += "}";
    
    myFunctions.add(createFunctionDef("delete_track", "Delete a track by ID", deleteSchema));

    // Audio Analysis
    juce::String analyzeSchema = "{";
    analyzeSchema += "\"type\": \"object\",";
    analyzeSchema += "\"properties\": {";
    analyzeSchema += "\"trackId\": { \"type\": \"string\", \"description\": \"Track ID to analyze, or 'master' for the full mix\" },";
    analyzeSchema += "\"duration\": { \"type\": \"number\", \"description\": \"Duration to analyze in seconds (default 10.0)\" }";
    analyzeSchema += "},";
    analyzeSchema += "\"required\": [\"trackId\"]";
    analyzeSchema += "}";

    myFunctions.add(createFunctionDef("analyze_track", "Analyze audio to provide feedback", analyzeSchema));
    
    // Preset functions
    juce::String presetSchema = "{";
    presetSchema += "\"type\": \"object\",";
    presetSchema += "\"properties\": {";
    presetSchema += "\"instrumentId\": { \"type\": \"string\", \"description\": \"Instrument ID\" },";
    presetSchema += "\"name\": { \"type\": \"string\", \"description\": \"Preset name\" },";
    presetSchema += "\"parameters\": { \"type\": \"object\", \"description\": \"Preset parameters\" }";
    presetSchema += "},";
    presetSchema += "\"required\": [\"instrumentId\", \"name\", \"parameters\"]";
    presetSchema += "}";

    myFunctions.add(createFunctionDef("list_presets", "List available presets", presetSchema));

    // AI Audio Processing
    juce::String stemsSchema = "{";
    stemsSchema += "\"type\": \"object\",";
    stemsSchema += "\"properties\": {";
    stemsSchema += "\"trackId\": { \"type\": \"string\", \"description\": \"Track ID to separate\" }";
    stemsSchema += "},";
    stemsSchema += "\"required\": [\"trackId\"]";
    stemsSchema += "}";

    myFunctions.add(createFunctionDef("separate_stems", "Separate audio track into stems", stemsSchema));

    return myFunctions;
}

} // namespace zenith
