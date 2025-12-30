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
#include <memory>

namespace zenith {
namespace ai {

//==============================================================================
/**
    Grok API Client with XAI integration
    Uses Grok 4.1 reasoning model for mastering decisions
*/
class GrokAPIClient {
public:
  GrokAPIClient() {
    // Security: Load API key from environment variable, not hardcoded
    apiKey_ = juce::SystemStats::getEnvironmentVariable("GROK_API_KEY", "");

    if (apiKey_.isEmpty()) {
      DBG("WARNING: GROK_API_KEY environment variable is not set. "
          "GrokAPIClient will not function until a valid API key is provided.");
    } else {
      DBG("GrokAPIClient initialized with API key from environment");
    }
  }

  /**
   * Check if API key is configured
   */
  bool hasAPIKey() const { return apiKey_.isNotEmpty(); }

  /**
   * Set API key programmatically (for secure key store integration)
   */
  void setAPIKey(const juce::String &apiKey) { apiKey_ = apiKey; }

  struct GrokFunction {
    juce::String name;
    juce::String description;
    juce::var parameters;

    GrokFunction() = default;
    GrokFunction(const juce::String& n, const juce::String& d, const juce::var& p)
        : name(n), description(d), parameters(p) {}
  };

  enum class ModelType {
    Reasoning,     // grok-4.1 (High intelligence, "thinking")
    Fast,          // grok-4.1-fast (Low latency, tool use, non-reasoning)
    FastReasoning  // grok-4.1-fast-reasoning (Fast but with thinking)
  };

  /**
   * Call Grok 4.1 model synchronously
   */
  juce::String callGrok(const juce::String &prompt,
                        const juce::String &systemMessage,
                        ModelType modelType = ModelType::Reasoning) {
    if (!hasAPIKey()) {
      DBG("ERROR: No API key configured. Set GROK_API_KEY environment "
          "variable.");
      return "{}";
    }

    DBG("======================================");
    DBG("Calling Grok 4.1 API (" + getModelId(modelType) + ")...");
    DBG("======================================");

    // Build request JSON
    juce::String requestBody = buildRequestJSON(prompt, systemMessage, modelType);

    DBG("Request size: " + juce::String(requestBody.length()) + " bytes");

    // Make HTTP request
    juce::String response = makeHttpRequest(requestBody);

    if (response.isEmpty()) {
      DBG("ERROR: Empty response from Grok API");
      return "{}";
    }

    // Extract content from response
    juce::String content = extractContent(response);

    DBG("Grok response received: " + juce::String(content.length()) + " bytes");
    DBG("======================================");

    return content;
  }

  /**
   * Async version - calls Grok on background thread and invokes callback
   */
  void callGrokAsync(const juce::String &prompt,
                     const juce::String &systemMessage,
                     std::function<void(juce::String)> callback,
                     ModelType modelType = ModelType::Reasoning) {
    // Launch on background thread
    juce::Thread::launch([this, prompt, systemMessage, callback, modelType]() {
      auto response = callGrok(prompt, systemMessage, modelType);

      // Invoke callback on message thread
      juce::MessageManager::callAsync(
          [callback, response]() { callback(response); });
    });
  }

private:
  juce::String apiKey_;
  const juce::String apiEndpoint_ = "https://api.x.ai/v1/chat/completions";

  juce::String getModelId(ModelType type) const {
      switch (type) {
          case ModelType::Fast:          return "grok-4.1-fast";
          case ModelType::FastReasoning: return "grok-4.1-fast-reasoning";
          case ModelType::Reasoning: default: return "grok-4.1";
      }
  }

  juce::String buildRequestJSON(const juce::String &prompt,
                                const juce::String &systemMessage,
                                ModelType modelType) {
    juce::DynamicObject::Ptr request = new juce::DynamicObject();

    // Use selected Grok 4.1 model
    request->setProperty("model", getModelId(modelType));
    request->setProperty("temperature", 0.7);
    request->setProperty("max_tokens", 2000); // 4.1 has huge context, but we limit output 

    // Build messages array
    juce::Array<juce::var> messages;

    // System message
    juce::DynamicObject::Ptr sysMsg = new juce::DynamicObject();
    sysMsg->setProperty("role", "system");
    sysMsg->setProperty("content", juce::var(systemMessage));
    messages.add(juce::var(sysMsg));

    // User message
    juce::DynamicObject::Ptr userMsg = new juce::DynamicObject();
    userMsg->setProperty("role", "user");
    userMsg->setProperty("content", juce::var(prompt));
    messages.add(juce::var(userMsg));

    request->setProperty("messages", juce::var(messages));

    // Enable streaming for better responsiveness (optional)
    request->setProperty("stream", false);

    return juce::JSON::toString(juce::var(request));
  }

  juce::String makeHttpRequest(const juce::String &requestBody) {
    // Create URL with POST data
    juce::URL url(apiEndpoint_);
    url = url.withPOSTData(requestBody);

    // Set up headers for the request
    // Set up headers
    juce::String headerString = "Content-Type: application/json\r\n"
                                "Authorization: Bearer " +
                                apiKey_;

    // Chain options
    auto options =
        juce::URL::InputStreamOptions(juce::URL::ParameterHandling::ignoreAllParameters)
            .withExtraHeaders(headerString)
            .withConnectionTimeoutMs(30000);

    // Make the HTTP POST request
    std::unique_ptr<juce::InputStream> stream = url.createInputStream(options);

    if (stream == nullptr) {
      DBG("ERROR: Failed to create HTTP connection to Grok API");
      return "{}";
    }

    // Read the response
    juce::String response = stream->readEntireStreamAsString();

    if (response.isEmpty()) {
      DBG("ERROR: Empty response from Grok API stream");
      return "{}";
    }

    return response;
  }

  juce::String extractContent(const juce::String &responseJson) {
    // Parse JSON response
    auto json = juce::JSON::parse(responseJson);

    if (json.isVoid()) {
      DBG("ERROR: Failed to parse response JSON");
      DBG("Raw response: " + responseJson.substring(0, 500));
      return "{}";
    }

    auto *obj = json.getDynamicObject();
    if (!obj) {
      DBG("ERROR: Response is not a JSON object");
      return "{}";
    }

    // Check for API errors
    if (obj->hasProperty("error")) {
      auto *errorObj = obj->getProperty("error").getDynamicObject();
      if (errorObj) {
        juce::String errorMsg = errorObj->getProperty("message").toString();
        juce::String errorType = errorObj->getProperty("type").toString();
        DBG("API ERROR: " + errorType + " - " + errorMsg);
      }
      return "{}";
    }

    // Extract: response.choices[0].message.content
    auto choices = obj->getProperty("choices");
    if (!choices.isArray()) {
      DBG("ERROR: No choices array in response");
      return "{}";
    }

    auto *choicesArray = choices.getArray();
    if (choicesArray->isEmpty()) {
      DBG("ERROR: Choices array is empty");
      return "{}";
    }

    auto *firstChoice = (*choicesArray)[0].getDynamicObject();
    if (!firstChoice) {
      DBG("ERROR: First choice is not an object");
      return "{}";
    }

    auto *message = firstChoice->getProperty("message").getDynamicObject();
    if (!message) {
      DBG("ERROR: No message in first choice");
      return "{}";
    }

    juce::String content = message->getProperty("content").toString();

    if (content.isEmpty()) {
      DBG("WARNING: Content is empty");
      return "{}";
    }

    return content;
  }
};

} // namespace ai
} // namespace zenith
