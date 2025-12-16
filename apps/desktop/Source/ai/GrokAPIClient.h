/*
  ==============================================================================
    GrokAPIClient.h
    Production-ready Grok API client for AI mastering
  ==============================================================================
*/

#pragma once
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>

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

  /**
   * Call Grok 4.1 reasoning model synchronously
   */
  juce::String callGrok(const juce::String &prompt,
                        const juce::String &systemMessage) {
    if (!hasAPIKey()) {
      DBG("ERROR: No API key configured. Set GROK_API_KEY environment "
          "variable.");
      return "{}";
    }

    DBG("======================================");
    DBG("Calling Grok 4.1 API...");
    DBG("======================================");

    // Build request JSON
    juce::String requestBody = buildRequestJSON(prompt, systemMessage);

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
                     std::function<void(juce::String)> callback) {
    // Launch on background thread
    juce::Thread::launch([this, prompt, systemMessage, callback]() {
      auto response = callGrok(prompt, systemMessage);

      // Invoke callback on message thread
      juce::MessageManager::callAsync(
          [callback, response]() { callback(response); });
    });
  }

private:
  juce::String apiKey_;
  const juce::String apiEndpoint_ = "https://api.x.ai/v1/chat/completions";

  juce::String buildRequestJSON(const juce::String &prompt,
                                const juce::String &systemMessage) {
    juce::DynamicObject::Ptr request = new juce::DynamicObject();

    // Use grok-2-1212 (Grok 4.1 reasoning model)
    request->setProperty("model", "grok-2-1212");
    request->setProperty("temperature", 0.7);
    request->setProperty("max_tokens", 2000);

    // Build messages array
    juce::Array<juce::var> messages;

    // System message
    juce::DynamicObject::Ptr sysMsg = new juce::DynamicObject();
    sysMsg->setProperty("role", "system");
    sysMsg->setProperty("content", systemMessage);
    messages.add(juce::var(sysMsg.get()));

    // User message
    juce::DynamicObject::Ptr userMsg = new juce::DynamicObject();
    userMsg->setProperty("role", "user");
    userMsg->setProperty("content", prompt);
    messages.add(juce::var(userMsg.get()));

    request->setProperty("messages", juce::var(messages));

    // Enable streaming for better responsiveness (optional)
    request->setProperty("stream", false);

    return juce::JSON::toString(request.get());
  }

  juce::String makeHttpRequest(const juce::String &requestBody) {
    // Create URL with POST data
    juce::URL url(apiEndpoint_);
    url = url.withPOSTData(requestBody);

    // Set up headers for the request
    juce::StringPairArray headers;
    headers.set("Content-Type", "application/json");
    headers.set("Authorization", "Bearer " + apiKey_);

    // Create input stream options
    // Create input stream options
    // Use ignoreAllParameters because we're sending raw JSON body via
    // withPOSTData()
    juce::URL::InputStreamOptions options(
        juce::URL::ParameterHandling::ignoreAllParameters);
    options = options.withExtraHeaders(headers.getHeadersAsString());
    options = options.withConnectionTimeoutMs(30000); // 30 second timeout

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
