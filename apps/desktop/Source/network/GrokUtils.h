/*
  ==============================================================================

    GrokUtils.h
    Created: 2025-12-03
    Author:  Zenith DAW

    Utility functions for Grok AI integration including JSON validation.
 
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>

namespace zenith {

/**
 * @brief Result of JSON schema validation
 */
struct JSONValidationResult {
    bool isValid = false;
    juce::String error;
    juce::var value;  // The validated/extracted value if valid
};

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
    
    //==========================================================================
    // JSON Schema Validation Functions
    //==========================================================================
    
    /**
     * @brief Validates and extracts a required property from a JSON object
     * @param obj The JSON object to extract from
     * @param propertyName The name of the property
     * @param expectedType "string", "number", "array", "object", or "any"
     * @return Validation result with error message if invalid
     */
    static JSONValidationResult validateProperty(
        const juce::var& obj,
        const juce::String& propertyName,
        const juce::String& expectedType);
    
    /**
     * @brief Validates an OpenAI/Grok API chat completion response structure
     * @param response The parsed JSON response
     * @return Validation result with content extracted if valid
     */
    static JSONValidationResult validateChatCompletionResponse(const juce::var& response);
    
    /**
     * @brief Safely extracts a string from JSON with default fallback
     */
    static juce::String safeGetString(const juce::var& obj, const juce::String& key, const juce::String& defaultValue = "");
    
    /**
     * @brief Safely extracts an integer from JSON with default fallback
     */
    static int safeGetInt(const juce::var& obj, const juce::String& key, int defaultValue = 0);
    
    /**
     * @brief Safely extracts a float from JSON with default fallback
     */
    static float safeGetFloat(const juce::var& obj, const juce::String& key, float defaultValue = 0.0f);
    
    /**
     * @brief Safely extracts an array from JSON, returns empty array if not present
     */
    static juce::Array<juce::var> safeGetArray(const juce::var& obj, const juce::String& key);
};

} // namespace zenith

