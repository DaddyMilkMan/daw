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

//==============================================================================
// JSON Schema Validation Implementation
//==============================================================================

JSONValidationResult GrokUtils::validateProperty(
    const juce::var& obj,
    const juce::String& propertyName,
    const juce::String& expectedType)
{
    JSONValidationResult result;
    
    if (!obj.isObject()) {
        result.error = "Expected object, got " + juce::String(obj.isVoid() ? "void" : "non-object");
        return result;
    }
    
    auto* dynObj = obj.getDynamicObject();
    if (!dynObj) {
        result.error = "Failed to get dynamic object";
        return result;
    }
    
    if (!dynObj->hasProperty(propertyName)) {
        result.error = "Missing required property: " + propertyName;
        return result;
    }
    
    result.value = dynObj->getProperty(propertyName);
    
    // Type validation
    if (expectedType == "string") {
        if (!result.value.isString()) {
            result.error = "Property '" + propertyName + "' expected string, got other type";
            return result;
        }
    } else if (expectedType == "number") {
        if (!result.value.isDouble() && !result.value.isInt() && !result.value.isInt64()) {
            result.error = "Property '" + propertyName + "' expected number, got other type";
            return result;
        }
    } else if (expectedType == "array") {
        if (!result.value.isArray()) {
            result.error = "Property '" + propertyName + "' expected array, got other type";
            return result;
        }
    } else if (expectedType == "object") {
        if (!result.value.isObject()) {
            result.error = "Property '" + propertyName + "' expected object, got other type";
            return result;
        }
    }
    // "any" type accepts anything
    
    result.isValid = true;
    return result;
}

JSONValidationResult GrokUtils::validateChatCompletionResponse(const juce::var& response)
{
    JSONValidationResult result;
    
    // Check for error response first
    if (response.isObject()) {
        auto* obj = response.getDynamicObject();
        if (obj && obj->hasProperty("error")) {
            result.error = "API error: " + safeGetString(response["error"], "message", "Unknown error");
            return result;
        }
    }
    
    // Validate choices array
    auto choicesResult = validateProperty(response, "choices", "array");
    if (!choicesResult.isValid) {
        result.error = choicesResult.error;
        return result;
    }
    
    auto* choices = choicesResult.value.getArray();
    if (!choices || choices->isEmpty()) {
        result.error = "Empty choices array in response";
        return result;
    }
    
    // Validate first choice has message
    auto firstChoice = (*choices)[0];
    auto messageResult = validateProperty(firstChoice, "message", "object");
    if (!messageResult.isValid) {
        result.error = "Invalid message in choice: " + messageResult.error;
        return result;
    }
    
    // Validate message has content
    auto contentResult = validateProperty(messageResult.value, "content", "string");
    if (!contentResult.isValid) {
        // Content might be null for function calls, check for tool_calls
        auto toolCallsResult = validateProperty(messageResult.value, "tool_calls", "array");
        if (!toolCallsResult.isValid) {
            result.error = "Message has neither content nor tool_calls";
            return result;
        }
    }
    
    result.isValid = true;
    result.value = contentResult.isValid ? contentResult.value : messageResult.value;
    return result;
}

juce::String GrokUtils::safeGetString(const juce::var& obj, const juce::String& key, const juce::String& defaultValue)
{
    if (!obj.isObject()) return defaultValue;
    
    auto* dynObj = obj.getDynamicObject();
    if (!dynObj || !dynObj->hasProperty(key)) return defaultValue;
    
    auto val = dynObj->getProperty(key);
    return val.isString() ? val.toString() : defaultValue;
}

int GrokUtils::safeGetInt(const juce::var& obj, const juce::String& key, int defaultValue)
{
    if (!obj.isObject()) return defaultValue;
    
    auto* dynObj = obj.getDynamicObject();
    if (!dynObj || !dynObj->hasProperty(key)) return defaultValue;
    
    auto val = dynObj->getProperty(key);
    if (val.isInt() || val.isInt64()) return static_cast<int>(val);
    if (val.isDouble()) return static_cast<int>(static_cast<double>(val));
    return defaultValue;
}

float GrokUtils::safeGetFloat(const juce::var& obj, const juce::String& key, float defaultValue)
{
    if (!obj.isObject()) return defaultValue;
    
    auto* dynObj = obj.getDynamicObject();
    if (!dynObj || !dynObj->hasProperty(key)) return defaultValue;
    
    auto val = dynObj->getProperty(key);
    if (val.isDouble()) return static_cast<float>(static_cast<double>(val));
    if (val.isInt() || val.isInt64()) return static_cast<float>(static_cast<int>(val));
    return defaultValue;
}

juce::Array<juce::var> GrokUtils::safeGetArray(const juce::var& obj, const juce::String& key)
{
    if (!obj.isObject()) return {};
    
    auto* dynObj = obj.getDynamicObject();
    if (!dynObj || !dynObj->hasProperty(key)) return {};
    
    auto val = dynObj->getProperty(key);
    if (val.isArray()) {
        juce::Array<juce::var> result;
        for (int i = 0; i < val.size(); ++i) {
            result.add(val[i]);
        }
        return result;
    }
    return {};
}

} // namespace zenith

