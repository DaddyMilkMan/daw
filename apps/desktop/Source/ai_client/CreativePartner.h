/*
  ==============================================================================
    CreativePartner.h
    AI creative partnership system with suggestions and explanations
    Phase 2: Context-Aware AI (9/10)
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "VisualAnalyzer.h"
#include "ProjectContext.h"
#include "GenreDetector.h"
#include "GrokJourney.h"
#include <memory>
#include <vector>

namespace zenith {
namespace ai {

struct CreativeSuggestion {
    juce::String type;              // "eq", "compression", "reverb", "arrangement"
    juce::String action;            // "boost", "cut", "add", "remove", "adjust"
    juce::String target;            // "kick", "vocal", "master", "specific_frequency"
    juce::String parameters;       // "2kHz +3dB", "ratio 4:1", "room size medium"
    juce::String reasoning;        // Why this suggestion makes sense
    float confidence;              // 0.0 to 1.0
    juce::String context;          // What led to this suggestion
    juce::var alternativeOptions;   // Other possibilities
};

struct CreativeInsight {
    juce::String observation;       // What the AI noticed
    juce::String explanation;       // Technical explanation
    juce::String creativeAdvice;    // Artistic interpretation
    juce::String genreContext;      // Genre-specific advice
    std::vector<CreativeSuggestion> suggestions;
};

struct CreativeConversation {
    std::vector<juce::String> userQuestions;
    std::vector<juce::String> aiResponses;
    juce::String currentTopic;
    juce::String projectState;
    std::vector<CreativeSuggestion> implementedSuggestions;
};

class CreativePartner {
public:
    CreativePartner();
    ~CreativePartner();

    // Main creative analysis
    CreativeInsight analyzeCreatively(const juce::AudioBuffer<float>& audio,
                                     const ProjectContext& context,
                                     const GenrePrediction& genre);

    // Suggestion generation
    std::vector<CreativeSuggestion> generateSuggestions(const CreativeInsight& insight);
    CreativeSuggestion suggestEQ(const VisualAnalysisResult& visual, 
                                const ProjectContext& context);
    CreativeSuggestion suggestCompression(const VisualAnalysisResult& visual,
                                         const ProjectContext& context);
    CreativeSuggestion suggestReverb(const VisualAnalysisResult& visual,
                                   const ProjectContext& context);
    CreativeSuggestion suggestArrangement(const ProjectContext& context);

    // Explanations and reasoning
    juce::String explainReasoning(const CreativeSuggestion& suggestion);
    juce::String explainGenreContext(const juce::String& genre, const juce::String& suggestion);
    juce::String explainTechnicalConcept(const juce::String& topicName);

    // Interactive conversation
    juce::String respondToQuestion(const juce::String& question,
                                   const CreativeInsight& currentInsight);
    void addToConversation(const juce::String& userQuestion, const juce::String& aiResponse);
    CreativeConversation getConversation() const { return conversation; }

    // Learning integration
    void updateFromUserFeedback(const CreativeSuggestion& suggestion, 
                                bool wasHelpful, 
                                const juce::String& userComment);
    void learnFromSuccessfulSuggestions(const GrokJourney& journey);

    // Configuration
    void setCreativityLevel(float level = 0.5f);  // 0.0 = conservative, 1.0 = experimental
    void setGenreExpertise(const juce::String& genre, float expertise = 1.0f);

private:
    std::unique_ptr<VisualAnalyzer> visualAnalyzer;
    std::unique_ptr<GenreDetector> genreDetector;
    CreativeConversation conversation;
    
    float creativityLevel = 0.5f;
    std::unordered_map<juce::String, float> genreExpertise;
    
    // Analysis helpers
    juce::String analyzeArtisticIntent(const VisualAnalysisResult& visual,
                                      const ProjectContext& context);
    juce::String analyzeProductionTechniques(const VisualAnalysisResult& visual,
                                           const GenrePrediction& genre);
    std::vector<juce::String> identifyCreativeOpportunities(const VisualAnalysisResult& visual,
                                                           const ProjectContext& context);
    
    // Suggestion logic
    CreativeSuggestion createSuggestion(const juce::String& type,
                                       const juce::String& action,
                                       const juce::String& target,
                                       const juce::String& parameters,
                                       const juce::String& reasoning,
                                       float confidence);
    
    // Genre-specific knowledge
    std::vector<juce::String> getGenreSpecificAdvice(const juce::String& genre);
    juce::String adaptSuggestionToGenre(const CreativeSuggestion& suggestion, 
                                       const juce::String& genre);
    
    // Conversation AI
    juce::String generateCreativeResponse(const juce::String& question,
                                         const CreativeInsight& insight);
    juce::String explainTechnicalDetails(const juce::String& topic);
    
    // Learning and adaptation
    void updateSuggestionConfidence(const CreativeSuggestion& suggestion, bool wasHelpful);
    void refineGenreKnowledge(const juce::String& genre, const juce::String& feedback);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CreativePartner)
};

} // namespace ai
} // namespace zenith
