/*
  ==============================================================================
    CreativeNeuralNetwork.h
    Neural network for creative suggestion generation
    Phase 2: Context-Aware AI (10/10)
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include "CreativePartner.h"
#include "VisualAnalyzer.h"
#include "ProjectContext.h"
#include <vector>
#include <memory>

namespace zenith {
namespace ai {

// Creative suggestion neural network
class CreativeNeuralNetwork {
public:
    struct SuggestionEmbedding {
        std::vector<float> features;      // Audio features
        std::vector<float> context;       // Project context
        std::vector<float> userHistory;   // User preference history
        std::vector<float> creativity;    // Creativity parameters
    };
    
    struct SuggestionOutput {
        juce::String type;                // EQ, compression, etc.
        juce::String action;              // boost, cut, add, etc.
        juce::String target;              // frequency, instrument, etc.
        juce::String parameters;         // specific settings
        float confidence;                 // 0.0 to 1.0
        juce::String reasoning;           // AI explanation
    };
    
    CreativeNeuralNetwork();
    ~CreativeNeuralNetwork();
    
    // Training methods
    void trainOnSuccessfulSuggestions(const std::vector<std::pair<SuggestionEmbedding, SuggestionOutput>>& trainingData);
    void loadModel(const juce::File& modelPath);
    void saveModel(const juce::File& modelPath) const;
    
    // Generation methods
    std::vector<SuggestionOutput> generateSuggestions(const SuggestionEmbedding& embedding, int numSuggestions = 5);
    SuggestionOutput generateSingleSuggestion(const SuggestionEmbedding& embedding);
    
    // Creativity control
    void setCreativityLevel(float level) { creativityLevel = juce::jlimit(0.0f, 1.0f, level); }
    void setGenreBias(const juce::String& genre, float bias);
    
    // Learning from feedback
    void updateFromFeedback(const SuggestionEmbedding& embedding, const SuggestionOutput& suggestion, bool wasHelpful);
    
    // Feature processing
    SuggestionEmbedding createEmbedding(const VisualAnalysisResult& visual,
                                     const ContextualInsights& context,
                                     const std::vector<UserPreference>& userHistory);
    std::vector<float> encodeAudioFeatures(const VisualAnalysisResult& visual);
    std::vector<float> encodeContext(const ContextualInsights& context);
    std::vector<float> encodeUserHistory(const std::vector<UserPreference>& history);
    
private:
    // Neural network layers (simplified representation)
    std::vector<std::vector<float>> inputWeights;
    std::vector<std::vector<float>> hiddenWeights;
    std::vector<std::vector<float>> outputWeights;
    
    std::vector<float> inputBiases;
    std::vector<float> hiddenBiases;
    std::vector<float> outputBiases;
    
    // Training data
    std::vector<std::pair<SuggestionEmbedding, SuggestionOutput>> trainingHistory;
    
    // Parameters
    float creativityLevel = 0.5f;
    std::unordered_map<juce::String, float> genreBiases;
    
    // Network operations
    std::vector<float> forwardPass(const std::vector<float>& input);
    void backpropagate(const std::vector<float>& input, const std::vector<float>& target, float learningRate = 0.01f);
    void initializeWeights();
    
    // Moved to public section:
    // SuggestionEmbedding createEmbedding(...)
    // std::vector<float> encodeAudioFeatures(...)
    // std::vector<float> encodeContext(...)
    // std::vector<float> encodeUserHistory(...)
    
    // Output decoding
    SuggestionOutput decodeOutput(const std::vector<float>& networkOutput);
    juce::String decodeType(const std::vector<float>& typeVector);
    juce::String decodeAction(const std::vector<float>& actionVector);
    juce::String decodeTarget(const std::vector<float>& targetVector);
    juce::String decodeParameters(const std::vector<float>& paramVector);
    
    // Helper functions
    float sigmoid(float x) const;
    float tanh(float x) const;
    std::vector<float> softmax(const std::vector<float>& input) const;
    
    // Model architecture
    static constexpr int INPUT_SIZE = 64;      // Combined feature size
    static constexpr int HIDDEN_SIZE = 128;     // Hidden layer size
    static constexpr int OUTPUT_SIZE = 32;     // Output encoding size
};

// Creative AI engine that combines neural networks with rule-based systems
class CreativeAIEngine {
public:
    CreativeAIEngine();
    ~CreativeAIEngine();
    
    // Main interface
    std::vector<CreativeSuggestion> generateCreativeSuggestions(const VisualAnalysisResult& visual,
                                                               const ContextualInsights& context,
                                                               const std::vector<UserPreference>& userHistory,
                                                               const juce::String& genre = "",
                                                               int numSuggestions = 5);
    
    // Learning and adaptation
    void learnFromInteraction(const CreativeSuggestion& suggestion, bool wasHelpful, const juce::String& userComment);
    void adaptToUserStyle(const std::vector<CreativeSuggestion>& successfulSuggestions);
    
    // Creativity control
    void setCreativityMode(const juce::String& mode);  // "conservative", "balanced", "experimental"
    void setGenreExpertise(const juce::String& genre, float expertise);
    
    // Explanation generation
    juce::String generateExplanation(const CreativeSuggestion& suggestion);
    juce::String explainCreativeChoice(const juce::String& reasoning, const juce::String& context);
    
private:
    std::unique_ptr<CreativeNeuralNetwork> neuralNetwork;
    std::unique_ptr<CreativePartner> ruleBasedPartner;
    
    // Hybrid approach: combine neural and rule-based
    std::vector<CreativeSuggestion> combineSuggestions(const std::vector<CreativeSuggestion>& neural,
                                                       const std::vector<CreativeSuggestion>& ruleBased);
    
    // Learning data
    std::vector<std::tuple<VisualAnalysisResult, ContextualInsights, CreativeSuggestion, bool>> interactionHistory;
    
    // Creativity parameters
    juce::String creativityMode = "balanced";
    std::unordered_map<juce::String, float> genreExpertise;
    
    // Quality metrics
    float calculateSuggestionQuality(const CreativeSuggestion& suggestion,
                                     const VisualAnalysisResult& visual,
                                     const ContextualInsights& context);
    
    // Contextual adaptation
    void adaptToGenre(const juce::String& genre);
    void adaptToUserPreferences(const std::vector<UserPreference>& preferences);
};

} // namespace ai
} // namespace zenith
