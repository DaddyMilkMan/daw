/**
 * @file OpenAIClient.h
 * @brief xAI Grok API client for Grok 4.1 integration
 *
 * Provides a JUCE-based HTTP client for the xAI Chat Completions API.
 * Supports:
 * - Grok 4.1 Fast (non-reasoning) - $0.20/1M input, $0.50/1M output
 * - Grok 4.1 Reasoning - Deep thinking mode with same pricing
 * - 2M context window
 * - Async request/response with callbacks
 * - Conversation history management
 * - Error handling and retry logic
 *
 * Thread Safety:
 * - All API calls are made asynchronously via JUCE's URL class
 * - Callbacks are invoked on the message thread
 * - Safe to call from UI components
 */

#pragma once

#include <juce_core/juce_core.h>
#include <functional>
#include <vector>
#include <memory>

namespace zenith {

/**
 * @brief AI Chat model selection
 */
enum class OpenAIModel
{
    Grok41Fast,          ///< Grok 4.1 Fast (non-reasoning) - Ultra-fast responses
    Grok41Reasoning      ///< Grok 4.1 Fast (reasoning) - Deep thinking mode
};

/**
 * @brief Single chat message
 */
struct ChatMessage
{
    juce::String role;     ///< "system", "user", or "assistant"
    juce::String content;  ///< Message text
};

/**
 * @brief OpenAI API client for chat completions
 */
class OpenAIClient
{
public:
    /**
     * @brief Constructor
     */
    OpenAIClient();

    /**
     * @brief Set the xAI API key
     * @param apiKey Your xAI API key (from console.x.ai)
     */
    void setApiKey(const juce::String& apiKey);

    /**
     * @brief Get the current API key (redacted for security)
     * @return Redacted API key (first 8 chars + "...")
     */
    juce::String getApiKeyRedacted() const;

    /**
     * @brief Send a chat completion request to xAI Grok API
     * @param messages Conversation history (system, user, assistant messages)
     * @param model Model to use (Grok41Fast or Grok41Reasoning)
     * @param callback Callback invoked with response (on message thread)
     * @param errorCallback Callback invoked on error (on message thread)
     */
    void sendChatRequest(
        const std::vector<ChatMessage>& messages,
        OpenAIModel model,
        std::function<void(const juce::String& response)> callback,
        std::function<void(const juce::String& error)> errorCallback = nullptr
    );

    /**
     * @brief Cancel all pending requests
     */
    void cancelAllRequests();

    /**
     * @brief Set custom system prompt
     * @param prompt System message to prepend to all conversations
     */
    void setSystemPrompt(const juce::String& prompt);

    /**
     * @brief Get the current system prompt
     */
    juce::String getSystemPrompt() const { return systemPrompt_; }

    /**
     * @brief Get model name string for xAI API
     * @param model Model enum value
     * @return API model identifier (e.g., "grok-4-1-fast-non-reasoning", "grok-4-1-fast-reasoning")
     */
    static juce::String getModelName(OpenAIModel model);

private:
    /**
     * @brief Build JSON request body for chat completion
     */
    juce::var buildRequestBody(const std::vector<ChatMessage>& messages, OpenAIModel model);

    /**
     * @brief Parse chat completion response
     * @param json Response JSON from API
     * @param outResponse Parsed response text
     * @param outError Error message (if parsing failed)
     * @return true if successful, false on error
     */
    bool parseResponse(const juce::var& json, juce::String& outResponse, juce::String& outError);

    juce::String apiKey_;
    juce::String systemPrompt_;

    // Active request tracking (for cancellation)
    struct PendingRequest
    {
        std::unique_ptr<juce::URL::DownloadTask> task;
        std::function<void(const juce::String&)> callback;
        std::function<void(const juce::String&)> errorCallback;
    };

    std::vector<std::shared_ptr<PendingRequest>> pendingRequests_;
    juce::CriticalSection requestLock_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OpenAIClient)
};

} // namespace zenith
