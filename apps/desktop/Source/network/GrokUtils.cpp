/*
  ==============================================================================

    GrokUtils.cpp
    Created: 2025-12-03
    Author:  Zenith DAW

  ==============================================================================
*/

#include "GrokUtils.h"

namespace zenith {

juce::var GrokUtils::parseJSONResponse(const juce::String& response)
{
    juce::String jsonString = response.trim();

    // 1. Try direct parsing first
    auto result = juce::JSON::parse(jsonString);
    if (result.isObject())
        return result;

    // 2. Look for markdown code blocks
    int start = jsonString.indexOf("```json");
    if (start != -1)
    {
        start += 7; // Skip ```json
        int end = jsonString.indexOf(start, "```");
        if (end != -1)
        {
            juce::String extracted = jsonString.substring(start, end).trim();
            result = juce::JSON::parse(extracted);
            if (result.isObject())
                return result;
        }
    }
    
    // 3. Look for generic code blocks
    start = jsonString.indexOf("```");
    if (start != -1)
    {
        start += 3;
        int end = jsonString.indexOf(start, "```");
        if (end != -1)
        {
            juce::String extracted = jsonString.substring(start, end).trim();
            // Sometimes it might be ```\n{...}\n```
            result = juce::JSON::parse(extracted);
            if (result.isObject())
                return result;
        }
    }

    // 4. Last resort: Find the first '{' and last '}'
    int firstBrace = jsonString.indexOfChar('{');
    int lastBrace = jsonString.lastIndexOfChar('}');

    if (firstBrace != -1 && lastBrace != -1 && lastBrace > firstBrace)
    {
        juce::String extracted = jsonString.substring(firstBrace, lastBrace + 1);
        result = juce::JSON::parse(extracted);
        if (result.isObject())
            return result;
    }

    return juce::var(); // Failed
}

} // namespace zenith
