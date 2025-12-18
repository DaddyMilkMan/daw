/*
  ==============================================================================

    GrokUtils.cpp
    Created: 2025-12-03
    Author:  Zenith DAW

  ==============================================================================
*/

#include "GrokUtils.h"
#include <regex>
#include <string>

namespace zenith {

juce::var GrokUtils::parseJSONResponse(const juce::String& response)
{
    juce::String jsonString = response.trim();

    // 1. Try direct parsing first
    auto result = juce::JSON::parse(jsonString);
    if (result.isObject())
        return result;

    // 2. Use Regex to find ```json ... ``` or ``` ... ``` blocks
    // Pattern: ```(?:json)?\s*(\{[\s\S]*?\})\s*```
    // Matches ``` optionally followed by "json", then optional whitespace, 
    // then captures the content starting with { until }, then optional whitespace, then ```
    // Note: std::regex ECMAScript syntax. [\s\S] trick or dotAll equivalent is needed. 
    // C++ regex doesn't support dotAll flag easily in ECMAScript mode usually, but we can use [^] or similar.
    // Actually, simply scanning for the block markers with regex is safer than manual index.
    
    // Simplest robust regex for the block:
    // ```(json)?\s*(\{.*?\})\s*```
    // But C++ regex doesn't support multiline dot by default.
    // We will use a std::string approach with find_first_of but improved, or just stick to the manual logic *if* regex is too heavy.
    // Complaint explicitly asked for regex. Let's use it.
    
    try {
        // Match ``` ... ``` blocks containing a { ... } structure
        // Using [^]* to match any character including newlines (since . doesn't match \n)
        std::regex pattern("```(?:json)?\\s*(\\{[^]*?\\})\\s*```", std::regex_constants::ECMAScript | std::regex_constants::icase);
        std::string stdResponse = response.toStdString();
        std::smatch matches;
        
        if (std::regex_search(stdResponse, matches, pattern))
        {
            if (matches.size() > 1)
            {
                juce::String extracted = matches[1].str();
                result = juce::JSON::parse(extracted);
                if (result.isObject())
                    return result;
            }
        }
    }
    catch (const std::exception& e) {
        DBG("GrokUtils: Regex error during JSON extraction: " + juce::String(e.what()));
    }
    catch (...) {
        DBG("GrokUtils: Unknown error during regex-based JSON extraction");
    }

    // 3. Last resort: Scan for outer braces if no code blocks found
    // Use a brace counter to handle nested JSON correctly
    int depth = 0;
    int firstBrace = -1;
    int lastBrace = -1;
    
    // Convert to std::string for easier iteration or use JUCE
    // iterating chars
    for (int i = 0; i < jsonString.length(); ++i)
    {
        juce::juce_wchar c = jsonString[i];
        if (c == '{')
        {
            if (depth == 0) firstBrace = i;
            depth++;
        }
        else if (c == '}')
        {
            depth--;
            if (depth == 0)
            {
                lastBrace = i;
                // We found a complete outer object. Let's try to parse it.
                // If the string continues with another object, we might ignore it or fail.
                // For now, let's take the first valid object found.
                break; 
            }
        }
    }

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
