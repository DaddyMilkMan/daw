/*
  ==============================================================================
    GrokAPIClient.h
    Production-ready Grok API client for AI mastering
  ==============================================================================
*/

#pragma once
#include <functional>
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>
#include <thread>
#include <atomic>
#include <unordered_map>
#include <queue>
#include <mutex>
#include <condition_variable>

namespace zenith {
namespace ai {

//==============================================================================
/**
    Grok API Client with XAI integration
    Uses Grok 4.1 reasoning model for mastering decisions
*/
class GrokAPIClient {
public:
  enum class LogLevel {
    Info,
    Warning,
    Error
  };

  enum class ModelType {
    Reasoning,     // grok-4.1 (High intelligence, "thinking")
    Fast,          // grok-4.1-fast (Low latency, tool use, non-reasoning)
    FastReasoning  // grok-4.1-fast-reasoning (Fast but with thinking)
  };

  GrokAPIClient();
  ~GrokAPIClient();

  /**
   * Check if API key is configured
   */
  bool hasAPIKey() const { return apiKey.isNotEmpty(); }

  /**
   * Set API key securely
   */
  bool setAPIKey(const juce::String& key);

  /**
   * Clear stored API key
   */
  void clearAPIKey();

  /**
   * Call Grok 4.1 model synchronously
   */
  juce::String callGrok(const juce::String &prompt,
                        const juce::String &systemMessage,
                        ModelType modelType = GrokAPIClient::ModelType::Reasoning);

  /**
   * Async version - calls Grok on background thread and invokes callback
   */
  void callGrokAsync(const juce::String &prompt,
                     const juce::String &systemMessage,
                     std::function<void(juce::String)> callback,
                     ModelType modelType = GrokAPIClient::ModelType::Reasoning);

private:
  juce::String apiKey;
  const juce::String apiEndpoint_ = "https://api.x.ai/v1/chat/completions";
  
  // Secure API key storage
  void loadAPIKeyFromSecureStorage();
  bool storeAPIKey(const juce::String& key);
  juce::String retrieveAPIKey();
  bool deleteAPIKey();
  juce::String encryptKey(const juce::String& key);
  juce::String decryptKey(const juce::String& encrypted);
  
  // Logging
  void logMessage(const juce::String& message, LogLevel level);

  juce::String buildSystemPrompt();
  juce::String getModelId(ModelType type) const;

  juce::String buildRequestJSON(const juce::String &prompt,
                                const juce::String &systemMessage,
                                ModelType modelType);

  juce::String makeHttpRequest(const juce::String &requestBody);
  juce::String extractContent(const juce::String &responseJson);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GrokAPIClient)
};

} // namespace ai
} // namespace zenith
