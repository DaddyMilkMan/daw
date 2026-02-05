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

    // Plugin Search
    juce::String searchSchema = "{";
    searchSchema += "\"type\": \"object\",";
    searchSchema += "\"properties\": {";
    searchSchema += "\"query\": { \"type\": \"string\", \"description\": \"Search query for plugins (name, manufacturer, or category)\" }";
    searchSchema += "},";
    searchSchema += "\"required\": [\"query\"]";
    searchSchema += "}";

    myFunctions.add(createFunctionDef("search_plugins", "Search for VST/AudioUnit plugins by name or category", searchSchema));

    // AI Audio Processing
    juce::String stemsSchema = "{";
    stemsSchema += "\"type\": \"object\",";
    stemsSchema += "\"properties\": {";
    stemsSchema += "\"trackId\": { \"type\": \"string\", \"description\": \"Track ID to separate\" }";
    stemsSchema += "},";
    stemsSchema += "\"required\": [\"trackId\"]";
    stemsSchema += "}";

    myFunctions.add(createFunctionDef("separate_stems", "Separate audio track into stems", stemsSchema));

    // Routing Graph
    myFunctions.add(createFunctionDef("get_routing_graph", "Get the current audio routing graph (nodes and connections)", 
        "{ \"type\": \"object\", \"properties\": {} }"));

    // Evolution
    juce::String startEvoSchema = "{";
    startEvoSchema += "\"type\": \"object\",";
    startEvoSchema += "\"properties\": {";
    startEvoSchema += "\"maxGenerations\": { \"type\": \"integer\", \"description\": \"Maximum generations to evolve (default 100)\" }";
    startEvoSchema += "}";
    startEvoSchema += "}";

    myFunctions.add(createFunctionDef("start_evolution", "Start the Preset Geneticist evolutionary sound design process", startEvoSchema));
    myFunctions.add(createFunctionDef("stop_evolution", "Stop the current evolution process", "{ \"type\": \"object\", \"properties\": {} }"));
    myFunctions.add(createFunctionDef("get_evolution_stats", "Get current progress and best fitness of the evolution", "{ \"type\": \"object\", \"properties\": {} }"));

    // Synth Control Functions
    juce::String synthOscWaveSchema = "{";
    synthOscWaveSchema += "\"type\": \"object\",";
    synthOscWaveSchema += "\"properties\": {";
    synthOscWaveSchema += "\"oscillator\": { \"type\": \"integer\", \"minimum\": 1, \"maximum\": 3, \"description\": \"Oscillator number (1-3)\" },";
    synthOscWaveSchema += "\"waveform\": { \"type\": \"string\", \"enum\": [\"sine\", \"saw\", \"square\", \"triangle\", \"noise\", \"supersaw\", \"wavetable\"], \"description\": \"Waveform type\" }";
    synthOscWaveSchema += "},";
    synthOscWaveSchema += "\"required\": [\"oscillator\", \"waveform\"]";
    synthOscWaveSchema += "}";
    
    myFunctions.add(createFunctionDef("set_synth_oscillator_wave", "Change the waveform of a synthesizer oscillator", synthOscWaveSchema));
    
    juce::String synthFilterSchema = "{";
    synthFilterSchema += "\"type\": \"object\",";
    synthFilterSchema += "\"properties\": {";
    synthFilterSchema += "\"cutoff\": { \"type\": \"number\", \"minimum\": 20, \"maximum\": 20000, \"description\": \"Filter cutoff frequency in Hz (20-20000)\" },";
    synthFilterSchema += "\"resonance\": { \"type\": \"number\", \"minimum\": 0, \"maximum\": 1, \"description\": \"Filter resonance/peaking (0.0-1.0)\" }";
    synthFilterSchema += "},";
    synthFilterSchema += "\"required\": [\"cutoff\"]";
    synthFilterSchema += "}";
    
    myFunctions.add(createFunctionDef("set_synth_filter", "Set synthesizer filter cutoff and resonance", synthFilterSchema));
    
    juce::String synthEnvelopeSchema = "{";
    synthEnvelopeSchema += "\"type\": \"object\",";
    synthEnvelopeSchema += "\"properties\": {";
    synthEnvelopeSchema += "\"attack\": { \"type\": \"number\", \"minimum\": 0, \"maximum\": 10, \"description\": \"Attack time in seconds\" },";
    synthEnvelopeSchema += "\"decay\": { \"type\": \"number\", \"minimum\": 0, \"maximum\": 10, \"description\": \"Decay time in seconds\" },";
    synthEnvelopeSchema += "\"sustain\": { \"type\": \"number\", \"minimum\": 0, \"maximum\": 1, \"description\": \"Sustain level (0.0-1.0)\" },";
    synthEnvelopeSchema += "\"release\": { \"type\": \"number\", \"minimum\": 0, \"maximum\": 10, \"description\": \"Release time in seconds\" }";
    synthEnvelopeSchema += "},";
    synthEnvelopeSchema += "\"required\": [\"attack\", \"decay\", \"sustain\", \"release\"]";
    synthEnvelopeSchema += "}";
    
    myFunctions.add(createFunctionDef("set_synth_amp_envelope", "Set synthesizer amplitude envelope (ADSR)", synthEnvelopeSchema));
    
    juce::String synthEffectSchema = "{";
    synthEffectSchema += "\"type\": \"object\",";
    synthEffectSchema += "\"properties\": {";
    synthEffectSchema += "\"distortion\": { \"type\": \"number\", \"minimum\": 0, \"maximum\": 1, \"description\": \"Distortion amount (0.0-1.0)\" },";
    synthEffectSchema += "\"chorus\": { \"type\": \"number\", \"minimum\": 0, \"maximum\": 1, \"description\": \"Chorus amount (0.0-1.0)\" },";
    synthEffectSchema += "\"reverb\": { \"type\": \"number\", \"minimum\": 0, \"maximum\": 1, \"description\": \"Reverb amount (0.0-1.0)\" }";
    synthEffectSchema += "},";
    synthEffectSchema += "\"minProperties\": 1";
    synthEffectSchema += "}";
    
    myFunctions.add(createFunctionDef("set_synth_effects", "Set synthesizer effects (distortion, chorus, reverb)", synthEffectSchema));
    
    juce::String synthPresetSchema = "{";
    synthPresetSchema += "\"type\": \"object\",";
    synthPresetSchema += "\"properties\": {";
    synthPresetSchema += "\"preset\": { \"type\": \"string\", \"enum\": [\"init\", \"bass\", \"lead\", \"pad\"], \"description\": \"Preset type to apply\" }";
    synthPresetSchema += "},";
    synthPresetSchema += "\"required\": [\"preset\"]";
    synthPresetSchema += "}";
    
    myFunctions.add(createFunctionDef("apply_synth_preset", "Apply a preset sound to the synthesizer (init, bass, lead, pad)", synthPresetSchema));
    
    juce::String synthRandomSchema = "{";
    synthRandomSchema += "\"type\": \"object\",";
    synthRandomSchema += "\"properties\": {";
    synthRandomSchema += "\"amount\": { \"type\": \"number\", \"minimum\": 0, \"maximum\": 1, \"description\": \"Randomization amount (0.0-1.0, default 0.5)\" }";
    synthRandomSchema += "},";
    synthRandomSchema += "\"required\": []";
    synthRandomSchema += "}";
    
    myFunctions.add(createFunctionDef("randomize_synth_patch", "Randomize synthesizer parameters for experimental sounds", synthRandomSchema));
    
    myFunctions.add(createFunctionDef("analyze_synth_patch", "Analyze current synthesizer patch and provide description", "{ \"type\": \"object\", \"properties\": {} }"));

    return myFunctions;
}

} // namespace zenith
