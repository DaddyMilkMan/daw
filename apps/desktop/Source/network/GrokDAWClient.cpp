/*
  ==============================================================================

    GrokDAWClient.cpp
    Created: 2025-11-29
    (Renamed from GrokAPIClient.cpp)

  ==============================================================================
*/

#include "GrokDAWClient.h"
#include "SecureKeyStore.h"
#include <juce_core/juce_core.h>

namespace zenith {

//==============================================================================
// Implementation class (Pimpl pattern for clean interface)
//==============================================================================

class GrokDAWClient::Impl
{
public:
    Impl() = default;
    ~Impl() = default;
    
    //==========================================================================
    // Configuration
    juce::String apiKey;
    static constexpr const char* API_BASE_URL = "https://api.x.ai/v1";
    static constexpr const char* CHAT_ENDPOINT = "/chat/completions";
    
    //==========================================================================
    // Conversation state
    juce::Array<juce::var> conversationHistory;
    
    //==========================================================================
    // Active request tracking
    std::unique_ptr<juce::URL::DownloadTask> activeRequest;
    
    //==========================================================================
    /**
        Build chat completion request JSON
    */
    juce::var buildChatRequest(
        const juce::String& userMessage,
        GrokMode mode,
        const juce::Array<GrokFunction>& functions,
        const juce::String& systemPrompt)
    {
        auto request = new juce::DynamicObject();
        
        // Model selection - Updated for Grok 4.1
        request->setProperty("model", mode == GrokMode::Thinking ? "grok-4.1-reasoning" : "grok-4.1");
        
        // Reasoning mode control
        request->setProperty("reasoning", mode == GrokMode::Thinking);
        
        // Temperature settings
        const float TEMP_THINKING = 0.3f; // Lower temperature for precise reasoning
        const float TEMP_CREATIVE = 0.7f; // Higher temperature for creative tasks
        request->setProperty("temperature", mode == GrokMode::Thinking ? TEMP_THINKING : TEMP_CREATIVE);
        
        // Build messages array
        juce::Array<juce::var> messages;
        
        // Add system prompt if provided
        if (systemPrompt.isNotEmpty())
        {
            auto systemMsg = new juce::DynamicObject();
            systemMsg->setProperty("role", "system");
            systemMsg->setProperty("content", systemPrompt);
            messages.add(juce::var(systemMsg));
        }
        
        // Add conversation history (Sliding Window: Last 10 messages)
        int historySize = conversationHistory.size();
        int startIndex = std::max(0, historySize - 10);
        
        for (int i = startIndex; i < historySize; ++i)
            messages.add(conversationHistory[i]);
        
        // Add current user message
        auto userMsg = new juce::DynamicObject();
        userMsg->setProperty("role", "user");
        userMsg->setProperty("content", userMessage);
        messages.add(juce::var(userMsg));
        
        request->setProperty("messages", messages);
        
        // Add function/tool definitions if provided
        if (!functions.isEmpty())
        {
            juce::Array<juce::var> tools;
            for (const auto& func : functions)
            {
                auto tool = new juce::DynamicObject();
                tool->setProperty("type", "function");
                tool->setProperty("function", func.toJSON());
                tools.add(juce::var(tool));
            }
            request->setProperty("tools", tools);
            request->setProperty("tool_choice", "auto");
        }
        
        return juce::var(request);
    }
    
    //==========================================================================
    /**
        Send HTTP POST request to Grok API
    */
    void sendRequest(
        const juce::var& requestBody,
        std::function<void(juce::var response)> onSuccess,
        std::function<void(juce::String error)> onError)
    {
        // Build URL
        juce::URL url(juce::String(API_BASE_URL) + CHAT_ENDPOINT);
        
        // Convert request to JSON string
        juce::String jsonRequest = juce::JSON::toString(requestBody);
        url = url.withPOSTData(jsonRequest);
        
        // Headers
        juce::String headersStr = "Authorization: Bearer " + apiKey + "\n" +
                                  "Content-Type: application/json";

        // Launch thread
        juce::Thread::launch([url, headersStr, onSuccess, onError]() {
            juce::StringPairArray responseHeaders;
            int statusCode = 0;
            auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inPostData)
                .withExtraHeaders(headersStr)
                .withConnectionTimeoutMs(15000)
                .withStatusCode(&statusCode)
                .withResponseHeaders(&responseHeaders);
                
            std::unique_ptr<juce::InputStream> stream = url.createInputStream(options);
            
            if (stream != nullptr) {
                juce::String responseText = stream->readEntireStreamAsString();
                
                // Post back to message thread
                juce::MessageManager::callAsync([statusCode, responseText, onSuccess, onError]() {
                    if (statusCode == 200) {
                        auto json = juce::JSON::parse(responseText);
                        if (json.isObject()) onSuccess(json);
                        else onError("Invalid JSON response");
                    } else {
                        onError("HTTP error: " + juce::String(statusCode));
                    }
                });
            } else {
                juce::MessageManager::callAsync([onError]() { onError("Failed to connect to Grok API"); });
            }
        });
    }
    
    //==========================================================================
    /**
        Parse Grok response and extract message/function calls
    */
    struct ParsedResponse
    {
        juce::String textResponse;
        juce::Array<GrokFunctionCall> functionCalls;
        bool hasError = false;
        juce::String errorMessage;
    };
    
    ParsedResponse parseResponse(const juce::var& response)
    {
        ParsedResponse result;
        
        // Check for API error
        if (response.hasProperty("error"))
        {
            result.hasError = true;
            result.errorMessage = response.getProperty("error", "Unknown error").toString();
            return result;
        }
        
        // Get choices array
        auto choices = response.getProperty("choices", juce::var());
        if (!choices.isArray() || choices.getArray()->isEmpty())
        {
            result.hasError = true;
            result.errorMessage = "No choices in response";
            return result;
        }
        
        // Get first choice
        auto choice = choices[0];
        auto message = choice.getProperty("message", juce::var());
        
        // Extract text content
        if (message.hasProperty("content"))
        {
            result.textResponse = message.getProperty("content", "").toString();
        }
        
        // Extract function calls
        if (message.hasProperty("tool_calls"))
        {
            auto toolCalls = message.getProperty("tool_calls", juce::var());
            if (toolCalls.isArray())
            {
                for (int i = 0; i < toolCalls.size(); ++i)
                {
                    auto toolCall = toolCalls[i];
                    auto function = toolCall.getProperty("function", juce::var());
                    
                    GrokFunctionCall call;
                    call.callId = toolCall.getProperty("id", "").toString();
                    call.functionName = function.getProperty("name", "").toString();
                    
                    // Parse arguments JSON string
                    auto argsString = function.getProperty("arguments", "{}").toString();
                    call.arguments = juce::JSON::parse(argsString);
                    
                    result.functionCalls.add(call);
                }
            }
        }
        
        return result;
    }
};

//==============================================================================
// GrokDAWClient public interface
//==============================================================================

GrokDAWClient::GrokDAWClient()
    : pImpl(std::make_unique<Impl>())
{
    // Try to load API key from secure storage, otherwise it remains the factory default
    setAPIKey();
}

GrokDAWClient::~GrokDAWClient() = default;

bool GrokDAWClient::setAPIKey(const juce::String& apiKey)
{
    if (apiKey.isNotEmpty())
    {
        pImpl->apiKey = apiKey;
        return true;
    }
    
    // 1. Try environment variables
    juce::String envKey = juce::SystemStats::getEnvironmentVariable("GROK_API_KEY", "");
    if (envKey.isEmpty())
        envKey = juce::SystemStats::getEnvironmentVariable("XAI_API_KEY", "");

    if (envKey.isNotEmpty())
    {
        pImpl->apiKey = envKey;
        return true;
    }

    // 2. Try to retrieve from secure storage
    juce::String storedKey;
    if (SecureKeyStore::retrieveKey(SecureKeyStore::GrokAPIKey, storedKey))
    {
        pImpl->apiKey = storedKey;
        return true;
    }
    
    // No key found - return false (API client is not configured)
    return false;
}

bool GrokDAWClient::hasAPIKey() const
{
    return pImpl->apiKey.isNotEmpty();
}

void GrokDAWClient::sendChat(
    const juce::String& userMessage,
    GrokMode mode,
    const juce::Array<GrokFunction>& availableFunctions,
    const juce::String& systemPrompt,
    std::function<void(juce::String response)> onResponse,
    std::function<void(GrokFunctionCall call)> onFunctionCall,
    std::function<void(juce::String error)> onError)
{
    if (!hasAPIKey())
    {
        onError("No API key configured");
        return;
    }
    
    // Build request
    auto request = pImpl->buildChatRequest(userMessage, mode, availableFunctions, systemPrompt);
    
    // Send request
    pImpl->sendRequest(request,
        [this, onResponse, onFunctionCall, onError](juce::var response)
        {
            // Parse response
            auto parsed = pImpl->parseResponse(response);
            
            if (parsed.hasError)
            {
                onError(parsed.errorMessage);
                return;
            }
            
            // Add assistant's response to history
            if (parsed.textResponse.isNotEmpty())
            {
                addToHistory("assistant", parsed.textResponse);
            }
            
            // Handle function calls
            if (!parsed.functionCalls.isEmpty())
            {
                for (const auto& call : parsed.functionCalls)
                {
                    onFunctionCall(call);
                }
            }
            else if (parsed.textResponse.isNotEmpty())
            {
                // No function calls, just text response
                onResponse(parsed.textResponse);
            }
        },
        onError
    );
}

void GrokDAWClient::submitFunctionResult(
    const GrokFunctionCall& functionCall,
    const juce::var& result,
    std::function<void(juce::String response)> onResponse,
    std::function<void(juce::String error)> onError)
{
    // Add function result to conversation history
    auto functionMsg = new juce::DynamicObject();
    functionMsg->setProperty("role", "tool");
    functionMsg->setProperty("tool_call_id", functionCall.callId);
    functionMsg->setProperty("content", juce::JSON::toString(result));
    
    pImpl->conversationHistory.add(juce::var(functionMsg));
    
    // Continue conversation (Grok will process the function result)
    sendChat("", GrokMode::Fast, {}, "", onResponse, 
        [](GrokFunctionCall) {}, onError);
}

void GrokDAWClient::cancelRequest()
{
    if (pImpl->activeRequest != nullptr)
        pImpl->activeRequest.reset();
}

juce::Array<juce::var> GrokDAWClient::getConversationHistory() const
{
    return pImpl->conversationHistory;
}

void GrokDAWClient::clearHistory()
{
    pImpl->conversationHistory.clear();
}

void GrokDAWClient::addToHistory(const juce::String& role, const juce::String& content)
{
    auto msg = new juce::DynamicObject();
    msg->setProperty("role", role);
    msg->setProperty("content", content);
    pImpl->conversationHistory.add(juce::var(msg));
}

juce::String GrokDAWClient::callGrok(const juce::String& prompt, const juce::String& systemPrompt)
{
    if (!hasAPIKey())
        return "Error: No API key configured";
        
    // Build standard request
    auto request = pImpl->buildChatRequest(prompt, GrokMode::Fast, {}, systemPrompt);
    
    // Synchronous execution using juce::URL
    juce::URL url(juce::String(Impl::API_BASE_URL) + Impl::CHAT_ENDPOINT);
    url = url.withPOSTData(juce::JSON::toString(request));
    
    juce::String headersStr = "Authorization: Bearer " + pImpl->apiKey + "\n" +
                              "Content-Type: application/json";
                              
    auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inPostData)
        .withExtraHeaders(headersStr)
        .withConnectionTimeoutMs(20000);
        
    std::unique_ptr<juce::InputStream> stream = url.createInputStream(options);
    if (stream)
    {
        auto responseText = stream->readEntireStreamAsString();
        // Return raw response for AIMasteringAgent to parse
        return responseText;
    }
    
    return "Error: Connection failed";
}

} // namespace zenith
