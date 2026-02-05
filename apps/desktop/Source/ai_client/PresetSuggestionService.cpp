/*
  ==============================================================================

    PresetSuggestionService.cpp
    Created: 2025-12-25
    Author:  Zenith DAW AI Team

  ==============================================================================
*/

#include "PresetSuggestionService.h"

namespace zenith {
namespace ai {

//==============================================================================
PresetSuggestionService::PresetSuggestionService()
    : juce::Thread("PresetSuggestionService"),
      cache_(AIResponseCache::getInstance()) {
    
    grokClient_ = std::make_unique<GrokAPIClient>();
    startThread();
}

PresetSuggestionService::~PresetSuggestionService() {
    signalThreadShouldExit();
    
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        shouldExit_ = true;
    }
    
    queueCondition_.notify_all();
    stopThread(2000);
}

//==============================================================================
void PresetSuggestionService::getSuggestions(const SuggestionRequest& request, SuggestionCallback callback) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    requestQueue_.push_back({request, callback});
    queueCondition_.notify_one();
}

void PresetSuggestionService::cancelAllRequests() {
    std::lock_guard<std::mutex> lock(queueMutex_);
    requestQueue_.clear();
}

//==============================================================================
void PresetSuggestionService::run() {
    while (!threadShouldExit()) {
        PendingRequest currentRequest;
        bool hasRequest = false;

        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            
            queueCondition_.wait(lock, [this] {
                return !requestQueue_.empty() || shouldExit_;
            });

            if (shouldExit_) break;

            if (!requestQueue_.empty()) {
                currentRequest = requestQueue_.front();
                requestQueue_.erase(requestQueue_.begin());
                hasRequest = true;
            }
        }

        if (hasRequest) {
            processRequest(currentRequest.request, currentRequest.callback);
        }
    }
}

//==============================================================================
void PresetSuggestionService::processRequest(const SuggestionRequest& request, SuggestionCallback callback) {
    // 1. Fetch available presets directly from manager
    // Note: ZenithPresetManager::loadAllPresetsForInstrument performs I/O on message thread, 
    // but getPresetList loads metadata which is lighter.
    // Ideally, this data would be passed in or cached, but we'll fetch it here.
    // Since we are on a background thread, we must be careful. 
    // ZenithPresetManager doc says "MESSAGE THREAD ONLY". 
    // We will use MessageManager::callAsync to fetch the list safely.
    
    // We need to block this thread until we get the list from the message thread
    std::vector<PresetMetadata> allPresets;
    std::promise<std::vector<PresetMetadata>> promise;
    auto future = promise.get_future();

    juce::MessageManager::callAsync([this, &promise, instrumentId = request.instrumentId]() {
        auto list = ZenithPresetManager::getInstance().getPresetList(instrumentId);
        promise.set_value(list);
    });

    // Wait for the list (with timeout)
    if (future.wait_for(std::chrono::seconds(2)) != std::future_status::ready) {
        // Timeout fetching local presets
        juce::MessageManager::callAsync([callback]() {
            callback({}, "Timeout retrieving preset library.");
        });
        return;
    }

    allPresets = future.get();

    if (allPresets.empty()) {
        juce::MessageManager::callAsync([callback]() {
            callback({}, "No presets found for this instrument.");
        });
        return;
    }

    // 2. Decide strategy: Local vs AI
    // If request is simple (just category, no query text), use local search
    bool isSimple = request.query.isEmpty() && request.categoryFilter.isNotEmpty();
    
    std::vector<PresetMetadata> results;
    
    if (isSimple || !request.useAI) {
        results = performLocalSearch(request, allPresets);
    } else {
        // 3. Try AI Search
        // We first do a quick local filter if category is provided to narrow scope
        auto candidates = allPresets;
        if (request.categoryFilter.isNotEmpty()) {
            // Optional: narrow down if category explicitly requested
             // candidates = performLocalSearch(request, allPresets);
             // Actually, for AI, we might want to let it loose on all presets or just filter by category firmly
             // Let's filter first if strict
        }

        results = performAISearch(request, candidates);
        
        // Fallback if AI returns nothing
        if (results.empty()) {
            results = performLocalSearch(request, allPresets);
        }
    }

    // 4. Return results on message thread
    juce::MessageManager::callAsync([callback, results]() {
        callback(results, "");
    });
}

//==============================================================================
std::vector<PresetMetadata> PresetSuggestionService::performLocalSearch(const SuggestionRequest& request, const std::vector<PresetMetadata>& allPresets) {
    std::vector<PresetMetadata> matches;
    juce::String query = request.query.toLowerCase();
    juce::String category = request.categoryFilter.toLowerCase();

    for (const auto& p : allPresets) {
        bool match = true;
        
        // Check category
        if (category.isNotEmpty()) {
            if (p.category.toLowerCase() != category) match = false;
        }

        // Check query (simple substring search in name/tags)
        if (match && query.isNotEmpty()) {
            bool textMatch = p.name.containsIgnoreCase(query);
            
            if (!textMatch) {
                // Check tags
                for (const auto& tag : p.tags) {
                    if (tag.containsIgnoreCase(query)) {
                        textMatch = true;
                        break;
                    }
                }
            }
            
            if (!textMatch) match = false;
        }

        if (match) {
            matches.push_back(p);
        }
        
        if (matches.size() >= request.maxResults) break;
    }

    return matches;
}

std::vector<PresetMetadata> PresetSuggestionService::performAISearch(const SuggestionRequest& request, const std::vector<PresetMetadata>& allPresets) {
    // Check Cache First
    juce::String systemMsg = buildSystemPrompt(allPresets);
    juce::String prompt = request.query;
    
    if (request.categoryFilter.isNotEmpty()) {
        prompt += " (Category: " + request.categoryFilter + ")";
    }

    juce::String promptHash = AIResponseCache::generateHash(systemMsg, prompt);
    auto cached = cache_.get(promptHash);

    juce::String jsonResponse;

    if (cached.has_value()) {
        jsonResponse = *cached;
    } else {
        // Call Grok
        if (!grokClient_->hasAPIKey()) {
            // Cannot use AI
            return {};
        }

        jsonResponse = grokClient_->callGrok(prompt, systemMsg, GrokAPIClient::ModelType::Fast);
        
        // Cache if valid
        if (jsonResponse.isNotEmpty() && jsonResponse != "{}") {
            cache_.put(promptHash, jsonResponse, 3600); // Cache for 1 hour
        }
    }

    // Parse Response
    std::vector<juce::String> suggestedIds = parseAIResponse(jsonResponse);
    
    // Map IDs back to Metadata
    std::vector<PresetMetadata> results;
    for (const auto& id : suggestedIds) {
        for (const auto& p : allPresets) {
            if (p.id == id) {
                results.push_back(p);
                break;
            }
        }
    }

    return results;
}

//==============================================================================
juce::String PresetSuggestionService::buildSystemPrompt(const std::vector<PresetMetadata>& presets) {
    juce::String s = "You are an expert sound designer and music producer assistant for Zenith DAW.\n";
    s += "Your task is to recommend the best synthesizer presets based on the user's description.\n";
    s += "The user will describe a mood, genre, or sound character.\n";
    s += "You must select from the following available presets and return a JSON array of their IDs.\n";
    s += "Return ONLY a valid JSON array of strings, e.g. [\"bass_01\", \"lead_05\"]. Do not include any other text.\n\n";
    
    s += "Available Presets:\n";
    
    // To save tokens, we list a compact representation
    // ID | Name | Category | Tags
    int count = 0;
    const int MAX_PRESETS_TO_LIST = 200; // Limit context window usage

    for (const auto& p : presets) {
        if (count++ > MAX_PRESETS_TO_LIST) break;
        
        s += "- ID: \"" + p.id + "\", Name: \"" + p.name + "\", Cat: \"" + p.category + "\"";
        if (!p.tags.empty()) {
            s += ", Tags: [";
            for (size_t i=0; i<p.tags.size(); ++i) {
                s += p.tags[i];
                if (i < p.tags.size()-1) s += ",";
            }
            s += "]";
        }
        s += "\n";
    }

    return s;
}

std::vector<juce::String> PresetSuggestionService::parseAIResponse(const juce::String& jsonResponse) {
    std::vector<juce::String> ids;
    
    // Allow for markdown code blocks (```json ... ```) which LLMs often output
    juce::String cleanJson = jsonResponse;
    if (cleanJson.contains("```json")) {
        int start = cleanJson.indexOf("```json") + 7;
        int end = cleanJson.indexOf(start, "```");
        if (end > start) {
            cleanJson = cleanJson.substring(start, end);
        }
    } else if (cleanJson.contains("```")) {
        int start = cleanJson.indexOf("```") + 3;
        int end = cleanJson.indexOf(start, "```");
        if (end > start) {
            cleanJson = cleanJson.substring(start, end);
        }
    }
    
    cleanJson = cleanJson.trim();

    auto var = juce::JSON::parse(cleanJson);
    if (var.isArray()) {
        auto* array = var.getArray();
        for (auto& item : *array) {
            ids.push_back(item.toString());
        }
    } else {
        DBG("PresetSuggestionService: Failed to parse AI response as array. Raw: " + jsonResponse);
    }
    
    return ids;
}

} // namespace ai
} // namespace zenith
