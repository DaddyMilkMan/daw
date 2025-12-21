/*
  ==============================================================================

    GrokUtils.h
    Created: 2025-12-03
    Author:  Zenith DAW

    Utility functions for Grok AI integration.
 
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>

namespace zenith {

class GrokUtils {
public:
    /**
     * @brief Parses a response from Grok that might contain JSON wrapped in markdown.
     * 
     * AI models often return JSON wrapped in ```json ... ``` blocks or with conversational text.
     * This function attempts to extract the JSON part and parse it.
     * 
     * @param response The raw string from the AI
     * @return Parsed juce::var (object), or juce::var() if failed
     */
    static juce::var parseJSONResponse(const juce::String& response);
};

} // namespace zenith
