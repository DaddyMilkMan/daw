/*
  ==============================================================================

    GrokAPIClient.h
    Created: 2025-11-29


    Grok API client with function calling support
    Implements xAI Grok API v1 with chat completions and tool use

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <functional>
#include <memory>

namespace zenith {

//==============================================================================
/**
    Grok operating mode
*/
enum class GrokMode
{
    Fast,      // grok-beta with reasoning disabled (low latency)
    Thinking   // grok-beta with reasoning enabled (deep analysis)
};

//==============================================================================
/**
    Function definition for Grok function calling
*/
struct GrokFunction
{
    juce::String name;
    juce::String description;
    juce::var parametersSchema;  // JSON schema object
    
    GrokFunction() = default;
    
    GrokFunction(const juce::String& n, const juce::String& desc, const juce::var& schema)
        : name(n), description(desc), parametersSchema(schema) {}
    
    juce::var toJSON() const
    {
        auto obj = new juce::DynamicObject();
        obj->setProperty("name", name);
        obj->setProperty("description", description);
        obj->setProperty("parameters", parametersSchema);
        return juce::var(obj);
    }
};

//==============================================================================
/**
    Function call request from Grok
*/
struct GrokFunctionCall
{
    juce::String functionName;
    juce::var arguments;  // JSON object with function arguments
    juce::String callId;  // Unique ID for this function call
    
    GrokFunctionCall() = default;
    
    GrokFunctionCall(const juce::String& name, const juce::var& args, const juce::String& id)
        : functionName(name), arguments(args), callId(id) {}
};

//==============================================================================
/**
    Grok API Client
    
    Handles communication with xAI Grok API including:
    - Chat completions
    - Function/tool calling
    - Streaming responses
    - Both Fast and Thinking modes
*/
class GrokAPIClient
{
public:
    //==========================================================================
    GrokAPIClient();
    ~GrokAPIClient();
    
    //==========================================================================
    /**
        Set the API key (retrieves from secure storage if not provided)
        
        @param apiKey Optional API key. If empty, retrieves from SecureKeyStore
        @return true if API key is set successfully
    */
    bool setAPIKey(const juce::String& apiKey = juce::String());
    
    /**
        Check if API key is configured
    */
    bool hasAPIKey() const;
    
    //==========================================================================
    /**
        Send a chat message with optional function calling
        
        @param userMessage The user's message
        @param mode Fast or Thinking mode
        @param availableFunctions Functions that Grok can call (optional)
        @param systemPrompt System prompt for context (optional)
        @param onResponse Callback when Grok responds with text
        @param onFunctionCall Callback when Grok wants to call a function
        @param onError Callback on error
    */
    void sendChat(
        const juce::String& userMessage,
        GrokMode mode,
        const juce::Array<GrokFunction>& availableFunctions,
        const juce::String& systemPrompt,
        std::function<void(juce::String response)> onResponse,
        std::function<void(GrokFunctionCall call)> onFunctionCall,
        std::function<void(juce::String error)> onError
    );
    
    /**
        Submit function result and continue conversation
        
        @param functionCall The original function call
        @param result The function result (JSON)
        @param onResponse Callback when Grok responds
        @param onError Callback on error
    */
    void submitFunctionResult(
        const GrokFunctionCall& functionCall,
        const juce::var& result,
        std::function<void(juce::String response)> onResponse,
        std::function<void(juce::String error)> onError
    );
    
    /**
        Cancel ongoing request
    */
    void cancelRequest();
    
    //==========================================================================
    /**
        Get the current conversation history
    */
    juce::Array<juce::var> getConversationHistory() const;
    
    /**
        Clear conversation history
    */
    void clearHistory();
    
    /**
        Add a message to conversation history
    */
    void addToHistory(const juce::String& role, const juce::String& content);
    
private:
    //==========================================================================
    class Impl;
    std::unique_ptr<Impl> pImpl;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GrokAPIClient)
};

} // namespace zenith
