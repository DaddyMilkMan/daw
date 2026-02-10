/*
  ==============================================================================
    GrokAPIClient.h
    Production-ready Grok API client for AI mastering
  ==============================================================================
*/

#pragma once
#include <functional>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_events/juce_events.h>
#include <juce_core/juce_core.h>
#include <memory>
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <vector>
#include <map>
#include <atomic>

// Include existing AI data structures
#include "GrokJourney.h"
#include "ProjectContext.h"
#include "CreativePartner.h"
#include "GenreDetector.h"
#include "VisualAnalyzer.h"

namespace zenith {
namespace ai {

//==============================================================================
struct AnalysisRequest {
    std::shared_ptr<juce::AudioBuffer<float>> audio;
    double sampleRate;
    juce::String trackName;
    bool forceReanalysis = false;
};

struct AnalysisResult {
    VisualAnalysisResult visualAnalysis;
    GenrePrediction genrePrediction;
    CreativeInsight creativeInsight;
    juce::String contextualDescription;
    bool isComplete = false;
};

//==============================================================================
/**
    Grok API Client with XAI integration
    Uses Grok 4.1 reasoning model for mastering decisions
*/
class GrokAPIClient {
public:
  enum class ModelType {
    Reasoning,     // grok-4.1 (High intelligence, "thinking")
    Fast,          // grok-4.1-fast (Low latency, tool use, non-reasoning)
    FastReasoning  // grok-4.1-fast-reasoning (Fast but with thinking)
  };

  enum class LogLevel {
      Info,
      Warning,
      Error
  };

  GrokAPIClient();

  /**
   * Check if API key is configured
   */
  bool hasAPIKey() const { return apiKey.isNotEmpty(); }

  /**
   * Set API key programmatically
   */
  bool setAPIKey(const juce::String &key);

  /**
   * Call Grok 4.1 model synchronously
   */
  juce::String callGrok(const juce::String &prompt,
                        const juce::String &systemMessage,
                        ModelType modelType = ModelType::Reasoning);

  void callGrokAsync(const juce::String &prompt,
                     const juce::String &systemMessage,
                     std::function<void(juce::String)> callback,
                     ModelType modelType = ModelType::Reasoning);

  /**
   * Simple query interface used by some agents
   */
  void sendQuery(const juce::String& prompt, 
                 std::function<void(const juce::String&)> onComplete);

  //==============================================================================
  // Integrated Analysis Features
  //==============================================================================

  void analyzeAudioAsync(const AnalysisRequest& request,
                        std::function<void(AnalysisResult)> onComplete,
                        std::function<void(juce::String)> onError);

  AnalysisResult getAnalysis(const juce::String& trackName, 
                             const juce::AudioBuffer<float>& audio,
                             double sampleRate);

  juce::String callGrokWithContext(const juce::String& prompt,
                                  const juce::String& trackName,
                                  const juce::AudioBuffer<float>& audio,
                                  double sampleRate,
                                  const juce::var& projectState = juce::var(),
                                  ModelType modelType = ModelType::Reasoning);

  std::vector<CreativeSuggestion> getCreativeSuggestions(const juce::String& trackName);
  juce::String askCreativePartner(const juce::String& question);
  
  void provideCreativeFeedback(const CreativeSuggestion& suggestion, 
                               bool wasHelpful, 
                               const juce::String& comment);

  void updateProjectContext(const juce::var& dawState);
  ContextualInsights getProjectInsights() const;

  // API Key Management
  void loadAPIKeyFromSecureStorage();
  void clearAPIKey();

private:
  juce::String apiKey;
  const juce::String apiEndpoint_ = "https://api.x.ai/v1/chat/completions";

  // Background analysis
  std::unique_ptr<std::thread> analysisThread;
  std::condition_variable analysisCondition;
  std::mutex analysisQueueMutex;
  std::queue<AnalysisRequest> analysisQueue;
  std::atomic<bool> shouldStopAnalysis{false};

  void analysisWorker();
  void startBackgroundAnalysis();
  void stopBackgroundAnalysis();

  AnalysisResult performAnalysis(const AnalysisRequest& request);
  
  // Caching
  std::map<juce::String, AnalysisResult> analysisCache;
  std::mutex cacheMutex;
  static constexpr size_t MAX_CACHE_SIZE = 100;

  void cacheAnalysis(const juce::String& key, const AnalysisResult& result);
  bool getCachedAnalysis(const juce::String& key, AnalysisResult& result);
  void cleanupCache();
  
  juce::String generateCacheKey(const juce::String& trackName, 
                               const juce::AudioBuffer<float>& audio,
                               double sampleRate);

  // Secure storage
  bool storeAPIKey(const juce::String& key);
  juce::String retrieveAPIKey();
  bool deleteAPIKey();
  juce::String encryptKey(const juce::String& key);
  juce::String decryptKey(const juce::String& encrypted);

  // Logging
  void logMessage(const juce::String& message, LogLevel level);

  // Prompt building
  juce::String buildContextualPrompt(const juce::String& prompt, 
                                    const AnalysisResult& analysis,
                                    const ContextualInsights& insights);
  juce::String buildSystemPrompt();

  // Network helpers
  juce::String getModelId(ModelType type) const;
  juce::String buildRequestJSON(const juce::String& prompt,
                                const juce::String& systemMessage,
                                ModelType modelType);
  juce::String makeHttpRequest(const juce::String& requestBody);
  juce::String extractContent(const juce::String& responseJson);

  // Feature analysis helper components
  std::unique_ptr<VisualAnalyzer> visualAnalyzer = std::make_unique<VisualAnalyzer>();
  std::unique_ptr<GenreDetector> genreDetector = std::make_unique<GenreDetector>();
  std::unique_ptr<CreativePartner> creativePartner = std::make_unique<CreativePartner>();
  std::unique_ptr<ProjectContext> projectContext = std::make_unique<ProjectContext>();
  std::unique_ptr<GrokJourney> journey = std::make_unique<GrokJourney>();
};

} // namespace ai
} // namespace zenith
