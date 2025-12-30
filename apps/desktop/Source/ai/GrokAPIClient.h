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
#include "GrokJourney.h"
#include "ReferenceMatcher.h"
#include "VisualAnalyzer.h"
#include "ProjectContext.h"
#include "GenreDetector.h"
#include "CreativePartner.h"

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
public:
  enum class ModelType {
    Reasoning,     // grok-4.1 (High intelligence, "thinking")
    Fast,          // grok-4.1-fast (Low latency, tool use, non-reasoning)
    FastReasoning  // grok-4.1-fast-reasoning (Fast but with thinking)
  };

  GrokAPIClient() {
    // IMPORTANT: Initialize all member objects FIRST before calling any methods
    // that might use them. This prevents undefined behavior from accessing
    // uninitialized members.
    
    // Initialize Phase 1 systems
    journey = std::make_unique<GrokJourney>();
    referenceMatcher = std::make_unique<ReferenceMatcher>();
    
    // Initialize Phase 2 systems
    visualAnalyzer = std::make_unique<VisualAnalyzer>();
    projectContext = std::make_unique<ProjectContext>();
    genreDetector = std::make_unique<GenreDetector>();
    creativePartner = std::make_unique<CreativePartner>();
    
    // Initialize learning database in user data directory
    auto dbPath = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                     .getChildFile("ZenithDAW")
                     .getChildFile("grok_learning.db");
    journey->initialize(dbPath);
    
    // NOW safe to load API key (all members initialized)
    loadAPIKeyFromSecureStorage();
    
    // Start background analysis thread LAST
    // This must be after all initialization as the worker thread uses members
    startBackgroundAnalysis();
  }

  ~GrokAPIClient() {
    stopBackgroundAnalysis();
  }



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
   * Phase 2: Context-Aware AI Analysis
   */
  struct AnalysisRequest {
    juce::AudioBuffer<float> audio;
    double sampleRate;
    juce::String trackName;
    juce::String projectContext;
    bool forceReanalysis = false;
  };

  struct AnalysisResult {
    VisualAnalysisResult visualAnalysis;
    GenrePrediction genrePrediction;
    CreativeInsight creativeInsight;
    juce::String contextualDescription;
    bool isComplete = false;
  };

  // Async analysis with caching
  void analyzeAudioAsync(const AnalysisRequest& request,
                         std::function<void(AnalysisResult)> onComplete,
                         std::function<void(juce::String)> onError = nullptr);

  // Get cached analysis or trigger new analysis
  AnalysisResult getAnalysis(const juce::String& trackName, 
                           const juce::AudioBuffer<float>& audio,
                           double sampleRate);

  // Phase 2 enhanced Grok call with full context
  juce::String callGrokWithContext(const juce::String& prompt,
                                  const juce::String& trackName,
                                  const juce::AudioBuffer<float>& audio,
                                  double sampleRate,
                                  const juce::var& projectState = juce::var(),
                                  ModelType modelType = ModelType::Reasoning);

  // Creative partnership interface
  std::vector<CreativeSuggestion> getCreativeSuggestions(const juce::String& trackName);
  juce::String askCreativePartner(const juce::String& question);
  void provideCreativeFeedback(const CreativeSuggestion& suggestion, 
                               bool wasHelpful, 
                               const juce::String& comment = "");

  // Project context management
  void updateProjectContext(const juce::var& dawState);
  ContextualInsights getProjectInsights() const;
  juce::String callGrokWithLearning(const juce::String &prompt,
                                   const juce::String &genre,
                                   const juce::String &instrument,
                                   const juce::AudioBuffer<float>* referenceAudio = nullptr,
                                   ModelType modelType = ModelType::Reasoning) {
    
    // Get learned preferences
    juce::var learnedSettings = journey->getRecommendedSettings(genre, instrument);
    
    // Analyze reference if provided
    juce::var referenceSettings;
    if (referenceAudio && referenceMatcher) {
      auto fingerprint = referenceMatcher->analyzeReference(*referenceAudio, 44100.0, "user_reference");
      referenceSettings = referenceMatcher->extractTargetSettings(fingerprint);
      
      // Store reference for future learning
      journey->storeReferenceFingerprint(fingerprint);
    }
    
    // Build enhanced prompt with learning context
    juce::String enhancedPrompt = buildLearningPrompt(prompt, genre, instrument, learnedSettings, referenceSettings);
    
    return callGrok(enhancedPrompt, buildSystemPrompt(), modelType);
  }

  /**
   * Record user feedback for learning
   */
  void recordUserFeedback(const juce::String &genre, const juce::String &instrument, const juce::var &settings, double satisfaction) {
    UserPreference pref;
    pref.genre = genre;
    pref.instrument = instrument;
    pref.confidence = satisfaction;
    pref.timestamp = juce::Time::getCurrentTime();

    if (settings.hasProperty("eq_low_gain")) {
      pref.parameter = "eq_low_gain";
      pref.value = (double)settings["eq_low_gain"];
      journey->recordUserAction(pref);
    }

    if (settings.hasProperty("compression_ratio")) {
      pref.parameter = "compression_ratio";
      pref.value = (double)settings["compression_ratio"];
      journey->recordUserAction(pref);
    }

    journey->recordSuccessfulMastering(genre, settings, satisfaction);
  }

  struct GrokFunction {
    juce::String name;
    juce::String description;
    juce::var parameters;

    GrokFunction() = default;
    GrokFunction(const juce::String& n, const juce::String& d, const juce::var& p)
        : name(n), description(d), parameters(p) {}
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
  juce::String apiKey;
  const juce::String apiEndpoint_ = "https://api.x.ai/v1/chat/completions";
  
  // Phase 1 systems
  std::unique_ptr<GrokJourney> journey;
  std::unique_ptr<ReferenceMatcher> referenceMatcher;
  
  // Phase 2 systems
  std::unique_ptr<VisualAnalyzer> visualAnalyzer;
  std::unique_ptr<ProjectContext> projectContext;
  std::unique_ptr<GenreDetector> genreDetector;
  std::unique_ptr<CreativePartner> creativePartner;
  
  // Background processing
  std::unique_ptr<std::thread> analysisThread;
  std::atomic<bool> shouldStopAnalysis{false};
  std::queue<AnalysisRequest> analysisQueue;
  std::mutex analysisQueueMutex;
  std::condition_variable analysisCondition;
  
  // Analysis caching
  std::unordered_map<juce::String, AnalysisResult> analysisCache;
  std::mutex cacheMutex;
  static constexpr size_t MAX_CACHE_SIZE = 50;

  // Background analysis worker
  void analysisWorker();
  void startBackgroundAnalysis();
  void stopBackgroundAnalysis();
  
  // Analysis pipeline
  AnalysisResult performAnalysis(const AnalysisRequest& request);
  void cacheAnalysis(const juce::String& key, const AnalysisResult& result);
  bool getCachedAnalysis(const juce::String& key, AnalysisResult& result);
  
  // Memory management
  void cleanupCache();
  juce::String generateCacheKey(const juce::String& trackName, 
                               const juce::AudioBuffer<float>& audio,
                               double sampleRate);

  // Secure API key storage
  void loadAPIKeyFromSecureStorage();
  bool storeAPIKey(const juce::String& key);
  juce::String retrieveAPIKey();
  bool deleteAPIKey();
  juce::String encryptKey(const juce::String& key);
  juce::String decryptKey(const juce::String& encrypted);
  
  // Logging
  void logMessage(const juce::String& message, LogLevel level);

  juce::String buildLearningPrompt(const juce::String& basePrompt,
                                  const juce::String& genre,
                                  const juce::String& instrument,
                                  const juce::var& learnedSettings,
                                  const juce::var& referenceSettings) {
    juce::String learningContext = "\n\nLEARNING CONTEXT:\n";
    
    if (learnedSettings.isObject()) {
      learningContext += "User's past preferences for " + genre + " " + instrument + ":\n";
      learningContext += juce::JSON::toString(learnedSettings) + "\n";
    }
    
    if (referenceSettings.isObject()) {
      learningContext += "Reference track analysis:\n";
      learningContext += juce::JSON::toString(referenceSettings) + "\n";
    }
    
    return basePrompt + learningContext;
  }

  juce::String buildContextualPrompt(const juce::String& basePrompt,
                                    const AnalysisResult& analysis,
                                    const ContextualInsights& context) {
    juce::String contextualPrompt = basePrompt;
    
    contextualPrompt += "\n\nAUDIO ANALYSIS:\n";
    contextualPrompt += analysis.visualAnalysis.visualDescription;
    
    contextualPrompt += "\n\nGENRE: " + analysis.genrePrediction.genre;
    if (analysis.genrePrediction.confidence > 0.7f) {
      contextualPrompt += " (" + analysis.genrePrediction.subgenre + ")";
    }
    
    contextualPrompt += "\n\nPROJECT CONTEXT:\n";
    contextualPrompt += "- Arrangement: " + context.arrangementType + "\n";
    contextualPrompt += "- Energy: " + context.energyLevel + "\n";
    contextualPrompt += "- Frequency Balance: " + context.frequencyBalance + "\n";
    contextualPrompt += "- Production Style: " + context.productionStyle + "\n";
    
    contextualPrompt += "\n\nCREATIVE INSIGHTS:\n";
    contextualPrompt += analysis.creativeInsight.observation + "\n";
    contextualPrompt += analysis.creativeInsight.explanation + "\n";
    
    return contextualPrompt;
  }

  juce::String buildSystemPrompt() {
    return "You are an AI mastering engineer with advanced context awareness and creative partnership capabilities. "
           "Consider visual waveform analysis, genre detection, project context, and user preferences when making decisions. "
           "Provide technical explanations and creative suggestions that enhance the music while respecting artistic intent.";
  }

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
                                apiKey;

    // Chain options
    auto options =
        juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
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
