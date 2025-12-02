/*
  ==============================================================================

    PresetGenerator.h
    Created: 2025-11-29
    Author:  Alex Chen (Synthesist)

    AI Preset Generation Helper
    Validates and generates synth presets based on descriptions

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include "../instruments/InstrumentPreset.h"

namespace zenith {

/**
    Helper class for AI preset generation
    
    Provides validation, templates, and utilities for generating
    synthesizer presets from natural language descriptions.
*/
class PresetGenerator
{
public:
    //==========================================================================
    /**
        Validate preset parameters against instrument schema
        
        @param instrumentId The instrument type
        @param parameters The preset parameters to validate
        @return true if valid, false if any parameters are out of range
    */
    static bool validatePresetParameters(
        const juce::String& instrumentId,
        const juce::var& parameters);
    
    /**
        Get parameter schema for an instrument
        
        @param instrumentId The instrument type
        @return JSON schema describing all parameters
    */
    static juce::var getParameterSchema(const juce::String& instrumentId);
    
    /**
        Generate a preset template based on sound type
        
        @param soundType Type of sound (pad, bass, lead, pluck, etc.)
        @return Template parameters for that sound type
    */
    static juce::var generateTemplate(const juce::String& soundType);
    
    /**
        Clamp parameter value to valid range
        
        @param paramName Parameter name
        @param value Input value
        @param instrumentId Instrument type
        @return Clamped value within valid range
    */
    static juce::var clampParameter(
        const juce::String& paramName,
        const juce::var& value,
        const juce::String& instrumentId);
    
    /**
        Get sound type from description
        
        @param description Natural language description
        @return Detected sound type (pad, bass, lead, etc.)
    */
    static juce::String detectSoundType(const juce::String& description);
    
    /**
        Create a complete preset from AI-generated parameters
        
        @param instrumentId Instrument type
        @param presetName Preset name
        @param description Description
        @param parameters AI-generated parameters
        @param genre Optional genre tag
        @return Complete InstrumentPreset object
    */
    static ZenithInstrumentPreset createPresetFromParameters(
        const juce::String& instrumentId,
        const juce::String& presetName,
        const juce::String& description,
        const juce::var& parameters,
        const juce::String& genre = juce::String());
    
private:
    //==========================================================================
    // Parameter validation helpers
    
    static bool isValidWaveform(const juce::String& waveform);
    static bool isValidFilterType(const juce::String& filterType);
    static bool isValidLFOTarget(const juce::String& target);
    static bool isValidModSource(const juce::String& source);
    static bool isValidModDestination(const juce::String& destination);
    
    static float clampFloat(float value, float min, float max);
    static int clampInt(int value, int min, int max);
    
    JUCE_DECLARE_NON_COPYABLE(PresetGenerator)
};

} // namespace zenith
