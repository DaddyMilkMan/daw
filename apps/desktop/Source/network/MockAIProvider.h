/*
  ==============================================================================

    MockAIProvider.h
    Created: 2025-12-02
    Author:  Zenith DAW

    Provides mock responses for AI Bridge requests, replacing the external Node.js server.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>

namespace zenith {

class MockAIProvider {
public:
    static juce::String processRequest(const juce::String& jsonRequest);

private:
    static juce::Array<juce::var> generateMelody(const juce::String& description);
    static juce::Array<juce::var> generateDrums(const juce::String& description);
    static juce::Array<juce::var> generateBass(const juce::String& description);
    static juce::Array<juce::var> generateChords(const juce::String& description);
    
    // Helper to create a standard response
    static juce::String createResponse(const juce::String& requestId, 
                                     const juce::String& thought, 
                                     const juce::Array<juce::var>& commands);
};

} // namespace zenith
