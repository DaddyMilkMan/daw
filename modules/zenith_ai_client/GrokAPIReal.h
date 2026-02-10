/*
  ==============================================================================
    GrokAPIReal.h
    Real Grok 4.1 API integration - no stubs, no shortcuts
    Phase 1: Production API Integration
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_cryptography/juce_cryptography.h>
#include <juce_events/juce_events.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <memory>
#include <thread>
#include <atomic>
#include <chrono>
#include <queue>
#include <mutex>

namespace zenith {
namespace ai {

// Real API response structures
struct GrokAPIResponse {
    bool success = false;
    juce::String content;
    juce::String model;
    int tokensUsed = 0;
    juce::String requestId;
    juce::Time timestamp;
    juce::String error;
    int httpStatusCode = 0;
    int responseTimeMs = 0;
    juce::String rawResponse;
};

// API request configuration
struct APIRequestConfig {
    juce::String model = "grok-4.1";
    float temperature = 0.7f;
    int maxTokens = 4096;
    float topP = 1.0f;
    bool stream = false;
    juce::String systemPrompt;
    std::vector<juce::String> stopSequences;
};

// Rate limiting
struct RateLimiter {
    std::atomic<int> requestsPerMinute{60};
    std::atomic<int> tokensPerMinute{100000};
    mutable std::queue<juce::Time> requestTimes;
    mutable std::mutex queueMutex;
    
    bool canMakeRequest() const;
    void recordRequest();
    juce::Time getNextAvailableTime() const;
};

// API key management with encryption
class APIKeyManager {
public:
    APIKeyManager();
    ~APIKeyManager();
    
    // Key storage (encrypted)
    bool storeAPIKey(const juce::String& apiKey);
    juce::String getAPIKey() const;
    bool hasValidKey() const;
    void clearKey();
    
    // Key validation
    bool validateKey(const juce::String& apiKey) const;
    juce::String getKeyHash() const;
    
    // Security
    void rotateKey();
    bool isKeyExpired() const;
    
private:
    juce::String encryptedKey;
    juce::String keyHash;
    juce::Time keyCreationTime;
    mutable juce::CriticalSection keyMutex;
    
    juce::String encryptKey(const juce::String& key) const;
    juce::String decryptKey(const juce::String& encrypted) const;
    juce::String generateKeyHash(const juce::String& key) const;
};

// Request/response logging
class APILogger {
public:
    struct LogEntry {
        juce::String requestId;
        juce::String method;
        juce::String endpoint;
        juce::String requestBody;
        juce::String responseBody;
        int statusCode;
        juce::Time timestamp;
        int responseTimeMs;
        juce::String error;
    };
    
    APILogger();
    ~APILogger();
    
    void logRequest(const LogEntry& entry);
    std::vector<LogEntry> getRecentLogs(int count = 100) const;
    void clearLogs();
    void exportLogs(const juce::File& filePath) const;
    
    // Analytics
    float getAverageResponseTime() const;
    float getSuccessRate() const;
    int getTotalRequests() const;
    
private:
    std::queue<LogEntry> logQueue;
    mutable juce::CriticalSection logMutex;
    static constexpr size_t MAX_LOG_ENTRIES = 10000;
    
    void writeLogToFile(const LogEntry& entry) const;
    juce::String formatLogEntry(const LogEntry& entry) const;
};

// Real HTTP client with proper error handling
class GrokHTTPClient {
public:
    GrokHTTPClient();
    ~GrokHTTPClient();
    
    // HTTP methods
    GrokAPIResponse post(const juce::String& endpoint,
                        const juce::String& body,
                        const juce::String& apiKey);
    
    GrokAPIResponse get(const juce::String& endpoint,
                       const juce::String& apiKey);
    
    // Configuration
    void setTimeout(int timeoutMs);
    void setUserAgent(const juce::String& userAgent);
    void setProxy(const juce::String& host, int port);
    
    // Connection management
    bool testConnection(const juce::String& apiKey);
    juce::String getServerStatus() const;
    
private:
    int timeoutMs = 30000;
    juce::String userAgent = "ZenithDAW-GrokAI/1.0";
    juce::String proxyHost;
    int proxyPort = 0;
    
    juce::String buildHeaders(const juce::String& apiKey, int contentLength) const;
    juce::String parseResponse(const juce::String& rawResponse, GrokAPIResponse& response) const;
    bool isValidResponse(const GrokAPIResponse& response) const;
};

// Main real Grok API client
class GrokAPIReal {
public:
    GrokAPIReal();
    ~GrokAPIReal();
    
    // Initialization
    bool initialize(const juce::String& apiKey);
    bool isInitialized() const;
    void shutdown();
    
    // Core API methods
    GrokAPIResponse chatCompletion(const juce::String& prompt,
                                  const APIRequestConfig& config = {});
    
    GrokAPIResponse streamChatCompletion(const juce::String& prompt,
                                       const APIRequestConfig& config = {},
                                       std::function<void(const juce::String&)> chunkCallback = nullptr);
    
    // Advanced features
    GrokAPIResponse analyzeAudio(const juce::AudioBuffer<float>& audio,
                                 double sampleRate,
                                 const juce::String& analysisType);
    
    GrokAPIResponse getMasteringSuggestion(const juce::AudioBuffer<float>& audio,
                                         double sampleRate,
                                         const juce::String& genre = "");
    
    // Batch operations
    std::vector<GrokAPIResponse> batchRequest(const std::vector<juce::String>& prompts,
                                             const APIRequestConfig& config = {});
    
    // Rate limiting and quotas
    bool checkRateLimit() const;
    juce::Time getNextRequestTime() const;
    int getRemainingRequests() const;
    int getRemainingTokens() const;
    
    // Monitoring and analytics
    APILogger& getLogger() { return logger; }
    float getAverageLatency() const;
    int getTotalRequests() const;
    float getErrorRate() const;
    
    // Configuration
    void setRateLimits(int requestsPerMinute, int tokensPerMinute);
    void setDefaultConfig(const APIRequestConfig& config);
    
private:
    std::unique_ptr<APIKeyManager> keyManager;
    std::unique_ptr<GrokHTTPClient> httpClient;
    std::unique_ptr<RateLimiter> rateLimiter;
    APILogger logger;
    
    APIRequestConfig defaultConfig;
    std::atomic<bool> initialized{false};
    
    // Request processing
    juce::String buildChatRequest(const juce::String& prompt, const APIRequestConfig& config) const;
    juce::String buildAudioAnalysisRequest(const juce::AudioBuffer<float>& audio,
                                         double sampleRate,
                                         const juce::String& analysisType) const;
    
    // Error handling
    GrokAPIResponse handleError(const juce::String& error, int statusCode = 0) const;
    bool shouldRetry(const GrokAPIResponse& response) const;
    GrokAPIResponse retryWithBackoff(const juce::String& endpoint,
                                   const juce::String& body,
                                   int maxRetries = 3) const;
    
    // Audio processing for API
    juce::String encodeAudioForAPI(const juce::AudioBuffer<float>& audio, double sampleRate) const;
    std::vector<float> extractAudioFeatures(const juce::AudioBuffer<float>& audio, double sampleRate) const;
    
    // Request tracking
    juce::String generateRequestId() const;
    void trackRequest(const juce::String& requestId, const GrokAPIResponse& response);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GrokAPIReal)
};

// Factory for easy access
class GrokAPIFactory {
public:
    static std::unique_ptr<GrokAPIReal> createProductionClient();
    static std::unique_ptr<GrokAPIReal> createTestClient();
    static bool validateAPIKey(const juce::String& apiKey);
};

} // namespace ai
} // namespace zenith
