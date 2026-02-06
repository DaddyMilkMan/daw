/*
  ==============================================================================

    SoundDesignAssistant.h
    Created: [Date] Author: Claude AI
    AI-powered sound design suggestions and parameter guidance

  ==============================================================================
*/

#pragma once

#include "../../../JuceLibraryCode/JuceHeader.h"
#include "TimbreTransfer.h"

namespace Zenith
{

class SoundDesignAssistant
{
public:
    // Sound descriptor structure
    struct SoundDescriptor
    {
        juce::String category;      // "bass", "lead", "pad", "pluck", "fx", etc.
        juce::String mood;          // "dark", "bright", "aggressive", "warm", "ethereal"
        float vintage;              // 0-1, how retro/vintage
        float movement;             // 0-1, modulation amount and complexity
        float complexity;           // 0-1, timbral complexity
        float spatiality;          // 0-1, spatial/width effects
        float expressiveness;      // 0-1, dynamic response
        float uniqueness;          // 0-1, departure from common sounds

        // Genre/style associations
        juce::StringArray genres;
        juce::StringArray eras;
        juce::StringArray artists;

        // Technical requirements
        float minFrequency;        // Minimum frequency (Hz)
        float maxFrequency;        // Maximum frequency (Hz)
        float minDuration;        // Minimum duration (seconds)
        float maxDuration;        // Maximum duration (seconds)
    };

    // Suggestion modes
    enum class SuggestionMode
    {
        Conservative,   // Small, safe variations
        Balanced,       // Moderate exploration
        Experimental,   // Bold, innovative sounds
        Genre,         // Genre-specific suggestions
        Artist,        // Artist-inspired suggestions
        Contextual     // Context-aware suggestions
    };

    // Parameter importance levels
    enum class ParameterImportance
    {
        Critical,       // Absolutely must be set
        Important,      // Strongly affects the result
        Moderate,      // Noticeable effect
        Minor,         // Subtle effect
        Optional       // Negligible effect
    };

    // Suggestion result
    struct SuggestionResult
    {
        std::map<juce::String, float> parameters;
        std::vector<juce::String> recommendations;
        std::vector<juce::String> warnings;
        SoundDescriptor descriptor;
        float confidence;          // 0-1, confidence in suggestion
        float compatibility;      // 0-1, compatibility with current sound
        float innovation;          // 0-1, innovation level
        juce::String suggestionId;
        juce::String reasoning;
    };

    // Learning history
    struct LearningHistory
    {
        std::vector<SuggestionResult> pastSuggestions;
        std::vector<SoundDescriptor> acceptedDescriptors;
        std::vector<SoundDescriptor> rejectedDescriptors;
        std::map<juce::String, float> successRates;
        std::map<juce::String, float> userPreferences;
    };

    SoundDesignAssistant();
    ~SoundDesignAssistant();

    // Engine initialization
    void initialize();
    void shutdown();
    void setSampleRate(double sampleRate);

    // Core suggestion functionality
    SuggestionResult suggestParameters(const SoundDescriptor& desc,
                                     SuggestionMode mode = SuggestionMode::Balanced);
    SuggestionResult suggestFromCurrentPatch(const juce::String& currentPresetId,
                                         SuggestionMode mode = SuggestionMode::Balanced);
    SuggestionResult suggestFromAudio(const juce::AudioBuffer<float>& audio,
                                    SuggestionMode mode = SuggestionMode::Balanced);

    // Analysis and understanding
    SoundDescriptor analyzeCurrentPatch(const juce::String& presetId);
    SoundDescriptor analyzeAudio(const juce::AudioBuffer<float>& audio);
    float calculateSoundSimilarity(const SoundDescriptor& desc1,
                                 const SoundDescriptor& desc2) const;

    // Learning and adaptation
    void recordUserFeedback(const SuggestionResult& suggestion,
                          bool accepted,
                          const juce::String& feedback = "");
    void updateUserPreferences(const SoundDescriptor& desc,
                              const std::map<juce::String, float>& preferences);
    void learnFromSuccess(const SuggestionResult& suggestion);
    void learnFromFailure(const SuggestionResult& suggestion);

    // Genre and artist modeling
    void addGenreModel(const juce::String& genre, const SoundDescriptor& prototype);
    void addArtistModel(const juce::String& artist, const SoundDescriptor& prototype);
    void addEraModel(const juce::String& era, const SoundDescriptor& prototype);
    SoundDescriptor getGenreModel(const juce::String& genre) const;
    SoundDescriptor getArtistModel(const juce::String& artist) const;

    // Suggestion customization
    void setSuggestionMode(SuggestionMode mode);
    SuggestionMode getSuggestionMode() const;
    void setParameterImportance(const juce::String& paramId,
                              ParameterImportance importance);
    ParameterImportance getParameterImportance(const juce::String& paramId) const;

    // Context awareness
    void setCurrentContext(const juce::String& context);
    juce::String getCurrentContext() const;
    void setTargetPlatform(const juce::String& platform);
    juce::String getTargetPlatform() const;

    // Batch processing
    std::vector<SuggestionResult> generateSuggestions(const SoundDescriptor& desc,
                                                    int count,
                                                    SuggestionMode mode = SuggestionMode::Balanced);
    SuggestionResult rankSuggestions(const std::vector<SuggestionResult>& suggestions,
                                  const SoundDescriptor& target);

    // Analysis and monitoring
    float getAverageConfidence() const;
    float getSuccessRate() const;
    int getTotalSuggestions() const;
    juce::StringArray getPopularCategories() const;
    std::map<juce::String, float> getParameterStatistics() const;

    // Export/Import
    bool saveLearningHistory(const juce::File& file);
    bool loadLearningHistory(const juce::File& file);
    bool exportSuggestionDatabase(const juce::File& directory);
    bool importSuggestionDatabase(const juce::File& directory);

private:
    // Engine state
    double sampleRate_;
    bool initialized_;
    SuggestionMode suggestionMode_;
    juce::String currentContext_;
    juce::String targetPlatform_;

    // Learning history
    LearningHistory learningHistory_;
    std::map<juce::String, ParameterImportance> parameterImportance_;

    // Genre and artist models
    std::map<juce::String, SoundDescriptor> genreModels_;
    std::map<juce::String, SoundDescriptor> artistModels_;
    std::map<juce::String, SoundDescriptor> eraModels_;

    // AI model components
    std::unique_ptr<juce::dsp::FFT> fft_;
    std::unique_ptr<TimbreTransferEngine> timbreTransfer_;

    // Neural networks for suggestions
    struct NeuralModel
    {
        std::vector<float> weights;
        std::vector<float> biases;
        std::map<juce::String, std::vector<float>> embeddings;
        bool loaded;
    } parameterModel_, genreModel_, innovationModel_;

    // Suggestion generation
    struct SuggestionEngine
    {
        std::vector<SuggestionResult> suggestions;
        float diversity;
        float relevance;
        float consistency;
    } suggestionEngine_;

    // Analysis components
    struct AnalysisEngine
    {
        std::vector<float> spectralFeatures;
        std::vector<float> temporalFeatures;
        std::vector<float> harmonicFeatures;
        float complexityScore;
        float brightnessScore;
        float warmthScore;
    } analysisEngine_;

    // Performance monitoring
    float averageConfidence_;
    float successRate_;
    int totalSuggestions_;
    std::map<juce::String, float> parameterStatistics_;

    // Private helper methods
    void initializeModels();
    void initializeFFT();
    void initializeTimbreTransfer();

    // Suggestion generation
    SuggestionResult generateSuggestion(const SoundDescriptor& desc,
                                    SuggestionMode mode);
    void applyGenreConstraints(SuggestionResult& result,
                             const SoundDescriptor& desc);
    void applyArtistInfluence(SuggestionResult& result,
                            const SoundDescriptor& desc);
    void applyContextualFactors(SuggestionResult& result,
                             const SoundDescriptor& desc);
    void calculateConfidence(SuggestionResult& result) const;
    void calculateCompatibility(SuggestionResult& result,
                              const SoundDescriptor& desc) const;
    void calculateInnovation(SuggestionResult& result) const;

    // Parameter mapping
    std::map<juce::String, float> mapDescriptorToParameters(const SoundDescriptor& desc) const;
    SoundDescriptor mapParametersToDescriptor(const std::map<juce::String, float>& params) const;
    void applyParameterConstraints(std::map<juce::String, float>& params,
                                 const SoundDescriptor& desc) const;

    // Neural network operations
    float runNeuralNetwork(const std::vector<float>& input,
                          const NeuralModel& model) const;
    std::vector<float> runInnovationModel(const SoundDescriptor& desc) const;
    std::vector<float> runGenreModel(const SoundDescriptor& desc) const;
    std::vector<float> runParameterModel(const SoundDescriptor& desc) const;

    // Statistical analysis
    void updateParameterStatistics(const std::map<juce::String, float>& params);
    void updateLearningHistory(const SuggestionResult& suggestion,
                             bool accepted);
    void updateUserPreferenceModel(const SoundDescriptor& desc,
                                const std::map<juce::String, float>& preferences);
    void updateSuccessRates(const SuggestionResult& suggestion,
                           bool accepted);

    // Context awareness
    void analyzeCurrentContext(const juce::String& context);
    void applyPlatformConstraints(SuggestionResult& result,
                               const juce::String& platform) const;
    float calculateContextualRelevance(const SoundDescriptor& desc,
                                   const juce::String& context) const;

    // Utility methods
    juce::String generateSuggestionId() const;
    juce::String generateReasoning(const SuggestionResult& result) const;
    std::vector<juce::String> generateRecommendations(const SuggestionResult& result) const;
    std::vector<juce::String> generateWarnings(const SuggestionResult& result) const;

    // Model persistence
    bool saveModels(const juce::File& directory);
    bool loadModels(const juce::File& directory);
    bool validateModel(const NeuralModel& model) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoundDesignAssistant)
};

} // namespace Zenith