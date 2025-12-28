/*
  ==============================================================================

    PresetSuggestionService.h
    Created: 2025-12-25
    Author:  Zenith DAW AI Team

    "The Preset Sommelier" - AI-Powered Preset Recommendation Engine

    Role: Bridges the gap between user intent ("I need a dark sci-fi bass")
    and the static preset library. It uses a hybrid approach:
    1. Fast filtering for direct category matches
    2. Large Language Model (Grok) for semantic matching/mood queries

  ==============================================================================
*/

#pragma once

#include "GrokAPIClient.h"
#include "../instruments/ZenithPresetManager.h"
#include "AIResponseCache.h"
#include <juce_core/juce_core.h>
#include <functional>
#include <memory>
#include <future>

namespace zenith {
namespace ai {

//==============================================================================
/**
    Request criteria for preset suggestions
*/
struct SuggestionRequest {
    juce::String instrumentId;      // Required: Target instrument
    juce::String query;             // Free text (e.g. "Dark cinematic bass")
    juce::String categoryFilter;    // Optional: Restrict to category
    int maxResults = 10;            // Max number of suggestions allowed
    
    // Internal options
    bool useAI = true;              // Whether to use LLM or just simple search
    int timeoutMs = 5000;           // Timeout for the request
};

//==============================================================================
/**
    Service for suggesting presets based on user criteria.
*/
class PresetSuggestionService : public juce::Thread {
public:
    //==========================================================================
    /**
     * @brief Callback for suggestion results
     * @param presets List of suggested presets (metadata)
     * @param error Error message if any
     */
    using SuggestionCallback = std::function<void(const std::vector<PresetMetadata>& presets, const juce::String& error)>;

    //==========================================================================
    PresetSuggestionService();
    ~PresetSuggestionService() override;

    //==========================================================================
    /**
     * @brief Request preset suggestions
     * 
     * If the query is simple (e.g., just a category), it returns immediately via callback.
     * If complex, it queries the AI mechanism.
     * 
     * @param request The suggestion criteria
     * @param callback Function to call with results
     */
    void getSuggestions(const SuggestionRequest& request, SuggestionCallback callback);

    /**
     * @brief Cancel any pending AI requests
     */
    void cancelAllRequests();

private:
    //==========================================================================
    void run() override;

    // Internal processing
    void processRequest(const SuggestionRequest& request, SuggestionCallback callback);
    
    // Logic branches
    std::vector<PresetMetadata> performLocalSearch(const SuggestionRequest& request, const std::vector<PresetMetadata>& allPresets);
    std::vector<PresetMetadata> performAISearch(const SuggestionRequest& request, const std::vector<PresetMetadata>& allPresets);

    // AI Helpers
    juce::String buildSystemPrompt(const std::vector<PresetMetadata>& presets);
    std::vector<juce::String> parseAIResponse(const juce::String& jsonResponse);

    //==========================================================================
    std::unique_ptr<GrokAPIClient> grokClient_;
    AIResponseCache& cache_;
    
    // Threading
    struct PendingRequest {
        SuggestionRequest request;
        SuggestionCallback callback;
    };
    
    std::mutex queueMutex_;
    std::vector<PendingRequest> requestQueue_;
    std::condition_variable queueCondition_;
    std::atomic<bool> shouldExit_ { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetSuggestionService)
};

} // namespace ai
} // namespace zenith
