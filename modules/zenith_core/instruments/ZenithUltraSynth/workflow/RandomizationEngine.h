/*
  ==============================================================================

    RandomizationEngine.h
    Created: [Date] Author: Claude AI
    Intelligent randomization with constraints and learning

  ==============================================================================
*/

#pragma once

#include "../../../JuceLibraryCode/JuceHeader.h"

namespace Zenith
{

class RandomizationEngine
{
public:
    // Randomization profiles
    enum class Profile
    {
        Subtle,       // Small variations (10% change)
        Moderate,     // Balanced changes (25% change)
        Extreme,      // Wild exploration (50% change)
        Musical,      // Musically useful results
        Experimental, // Avant-garde sounds
        Custom        // User-defined profile
    };

    // Parameter constraints
    struct ParameterConstraint
    {
        juce::String parameterId;
        float minValue;
        float maxValue;
        float probability;        // 0-1, likelihood of being randomized
        bool relative;            // Relative to current value
        float stepSize;           // Step size for discrete changes
        juce::StringArray dependencies; // Parameters that affect this one
        juce::StringArray exclusions; // Parameters that conflict with this one
        bool preserveRelative;    // Preserve relative relationships
        float weight;             // Randomization weight
        juce::String category;    // Parameter category
    };

    // Randomization history
    struct RandomizationHistory
    {
        juce::String presetId;
        Profile profile;
        std::vector<juce::String> randomizedParams;
        std::vector<float> originalValues;
        std::vector<float> randomizedValues;
        float qualityScore;
        bool accepted;
        juce::String feedback;
        juce::Date timestamp;
        float userSatisfaction;   // 1-10 user rating
    };

    // Learning model
    struct LearningModel
    {
        std::map<juce::String, float> parameterPopularity; // How often each parameter is randomized
        std::map<juce::String, float> parameterSuccess;    // Success rate for each parameter
        std::map<juce::String, float> userPreferences;   // User preference weights
        std::map<juce::String, std::vector<float>> valueDistribution; // Value distribution history
        std::map<juce::String, std::vector<float>> successDistribution; // Success by value range
        float adaptationRate;                             // Learning rate
        float explorationRate;                          // Exploration vs exploitation
        juce::uint64 lastUpdate;                        // Last model update
    };

    // Randomization state
    struct RandomizationState
    {
        std::vector<ParameterConstraint> constraints;
        Profile currentProfile;
        float mutationRate;
        float crossoverRate;
        float elitismRate;
        int generationSize;
        int maxGenerations;
        float convergenceThreshold;
        bool preserveStructure;
        bool preserveTimbre;
        bool normalizeOutput;
        std::vector<juce::String> lockedParameters;
        std::map<juce::String, juce::Range<float>> parameterRanges;
        LearningModel learningModel;
        std::vector<RandomizationHistory> history;
    };

    // "Randomize Until" predicate
    typedef std::function<bool(const juce::ValueTree&)> PredicateFunction;

    // Optimization results
    struct OptimizationResult
    {
        juce::ValueTree bestPreset;
        float bestScore;
        int generations;
        float convergence;
        std::vector<float> scores;
        float timeElapsed;
        bool success;
        juce::String reason;
    };

    RandomizationEngine();
    ~RandomizationEngine();

    // Initialization
    void initialize(double sampleRate);
    void shutdown();
    void setSampleRate(double sampleRate);

    // Profile management
    void setProfile(Profile profile);
    Profile getProfile() const;
    void setCustomProfile(const std::map<juce::String, float>& weights);
    std::map<juce::String, float> getCustomProfile() const;
    void setProfileParameter(Profile profile, const juce::String& paramId, float value);
    float getProfileParameter(Profile profile, const juce::String& paramId) const;

    // Constraint management
    void addConstraint(const ParameterConstraint& constraint);
    void removeConstraint(const juce::String& parameterId);
    void updateConstraint(const juce::String& parameterId, const ParameterConstraint& constraint);
    ParameterConstraint getConstraint(const juce::String& parameterId) const;
    std::vector<ParameterConstraint> getAllConstraints() const;
    void clearConstraints();

    // Parameter locking
    void lockParameter(const juce::String& paramId);
    void unlockParameter(const juce::String& paramId);
    bool isParameterLocked(const juce::String& paramId) const;
    std::vector<juce::String> getLockedParameters() const;
    void clearLocks();

    // Range management
    void setParameterRange(const juce::String& paramId, float min, float max);
    juce::Range<float> getParameterRange(const juce::String& paramId) const;
    void setGlobalRangeMultiplier(float multiplier);
    float getGlobalRangeMultiplier() const;

    // Randomization methods
    juce::ValueTree randomize(const juce::ValueTree& basePreset,
                             Profile profile = Profile::Moderate);
    juce::ValueTree randomize(const juce::ValueTree& basePreset,
                             const std::map<juce::String, float>& parameterWeights);
    juce::ValueTree evolutionaryRandomize(const juce::ValueTree& basePreset,
                                        Profile profile = Profile::Musical,
                                        int generations = 10);
    OptimizationResult optimize(const juce::ValueTree& basePreset,
                              const PredicateFunction& predicate,
                              int maxAttempts = 100);

    // "Randomize Until" functionality
    juce::ValueTree randomizeUntil(const juce::ValueTree& basePreset,
                                  const PredicateFunction& predicate,
                                  Profile profile = Profile::Moderate,
                                  int maxAttempts = 100);
    juce::ValueTree randomizeUntil(const juce::ValueTree& basePreset,
                                  const std::vector<PredicateFunction>& predicates,
                                  Profile profile = Profile::Moderate,
                                  int maxAttempts = 100);

    // Learning and adaptation
    void recordUserFeedback(const juce::ValueTree& preset,
                          bool accepted,
                          const juce::String& feedback = "",
                          float satisfaction = 5.0f);
    void updateUserPreferences(const juce::ValueTree& preset,
                             const std::map<juce::String, float>& preferences);
    void updateLearningModel();
    void resetLearningModel();

    // Analysis and monitoring
    float getRandomizationQuality(const juce::ValueTree& preset) const;
    float calculatePresetDistance(const juce::ValueTree& preset1,
                                const juce::ValueTree& preset2) const;
    std::vector<juce::String> getRecommendedRandomizations(const juce::ValueTree& preset) const;
    float getParameterSensitivity(const juce::String& paramId) const;
    std::vector<float> getParameterDistributions(const juce::String& paramId) const;

    // Performance optimization
    void setBatchProcessing(bool enabled);
    bool isBatchProcessing() const;
    void setParallelProcessing(bool enabled);
    bool isParallelProcessing() const;
    void setMaxThreads(int maxThreads);
    int getMaxThreads() const;
    void setOptimizationEnabled(bool enabled);
    bool isOptimizationEnabled() const;

    // Export/Import
    bool saveConfiguration(const juce::File& file);
    bool loadConfiguration(const juce::File& file);
    bool saveLearningData(const juce::File& file);
    bool loadLearningData(const juce::File& file);
    bool exportHistory(const juce::File& file);
    bool importHistory(const juce::File& file);

    // Statistics and analytics
    struct EngineStatistics
    {
        int totalRandomizations;
        int successfulRandomizations;
        int failedRandomizations;
        float averageQuality;
        float averageUserSatisfaction;
        float averageTime;
        std::map<Profile, int> profileUsage;
        std::map<juce::String, int> parameterRandomizationCount;
        std::map<juce::String, float> parameterSuccessRates;
        juce::Date lastRandomization;
        juce::Date lastLearningUpdate;
        int learningIterations;
        float convergenceRate;
    };
    EngineStatistics getStatistics() const;
    void resetStatistics();

    // Callbacks
    void setRandomizationCompleteListener(std::function<void(const juce::ValueTree&)> callback);
    void setOptimizationCompleteListener(std::function<void(const OptimizationResult&)> callback);
    void setLearningUpdateListener(std::function<void()> callback);
    void setUserFeedbackListener(std::function<void(const juce::ValueTree&, bool)> callback);

    // State management
    juce::ValueTree getState() const;
    void setState(const juce::ValueTree& state);
    void saveState(const juce::File& file);
    void loadState(const juce::File& file);

private:
    // Randomization state
    RandomizationState state_;
    double sampleRate_;
    bool initialized_;

    // Processing options
    bool batchProcessing_;
    bool parallelProcessing_;
    int maxThreads_;
    bool optimizationEnabled_;
    float globalRangeMultiplier_;

    // Performance monitoring
    juce::ScopedTimeMeasurement timer_;
    float lastRandomizationTime_;
    float lastOptimizationTime_;

    // Callbacks
    std::function<void(const juce::ValueTree&)> randomizationCompleteListener_;
    std::function<void(const OptimizationResult&)> optimizationCompleteListener_;
    std::function<void()> learningUpdateListener_;
    std::function<void(const juce::ValueTree&, bool)> userFeedbackListener_;

    // Private helper methods
    void initializeRandomization();
    void initializeConstraints();
    void initializeLearningModel();

    // Randomization algorithms
    juce::ValueTree performRandomization(const juce::ValueTree& basePreset,
                                      Profile profile);
    juce::ValueTree performWeightedRandomization(const juce::ValueTree& basePreset,
                                              const std::map<juce::String, float>& weights);
    juce::ValueTree performEvolutionaryOptimization(const juce::ValueTree& basePreset,
                                                   Profile profile,
                                                   int generations);
    OptimizationResult performGeneticOptimization(const juce::ValueTree& basePreset,
                                              const PredicateFunction& predicate,
                                              int maxGenerations);

    // Constraint processing
    void applyConstraints(juce::ValueTree& preset, Profile profile);
    bool validateParameter(const juce::String& paramId, float value) const;
    float applyParameterRange(const juce::String& paramId, float value) const;
    void applyParameterDependencies(juce::ValueTree& preset);

    // Evolutionary algorithms
    struct Individual
    {
        juce::ValueTree genome;
        float fitness;
        float diversity;
    };
    std::vector<Individual> createInitialPopulation(const juce::ValueTree& basePreset,
                                                   int size);
    std::vector<Individual> selection(const std::vector<Individual>& population,
                                    int count);
    std::vector<Individual> crossover(const Individual& parent1,
                                     const Individual& parent2);
    std::vector<Individual> mutation(const std::vector<Individual>& population,
                                    float rate);
    float calculateFitness(const Individual& individual,
                         const PredicateFunction& predicate);
    float calculateDiversity(const Individual& individual,
                          const std::vector<Individual>& population);

    // Learning algorithms
    void updateParameterPopularity(const juce::ValueTree& preset);
    void updateParameterSuccess(const juce::String& paramId, bool success);
    void updateUserPreferenceModel(const juce::ValueTree& preset,
                                 const std::map<juce::String, float>& preferences);
    void updateValueDistributions();
    void predictOptimalValues(juce::ValueTree& preset);

    // Analysis methods
    float calculatePresetComplexity(const juce::ValueTree& preset) const;
    float calculateTimbralDistance(const juce::ValueTree& preset1,
                                 const juce::ValueTree& preset2) const;
    float calculateStructuralDistance(const juce::ValueTree& preset1,
                                    const juce::ValueTree& preset2) const;
    std::vector<juce::String> analyzePresetCharacteristics(const juce::ValueTree& preset) const;
    float calculateParameterImpact(const juce::String& paramId,
                                const juce::ValueTree& preset) const;

    // Utility methods
    juce::String generateRandomizationId() const;
    juce::String generateConstraintId() const;
    bool validatePreset(const juce::ValueTree& preset) const;
    bool validateConstraints() const;
    float normalizeValue(float value, const juce::Range<float>& range) const;
    float denormalizeValue(float value, const juce::Range<float>& range) const;

    // Thread safety
    juce::CriticalSection randomizationLock_;
    juce::ScopedLock scopedLock_;

    // Parallel processing
    void processParallel(std::vector<std::function<void()>>& tasks);

    // Export/Import helpers
    juce::ValueTree createValueTreeFromState() const;
    void createStateFromValueTree(const juce::ValueTree& tree);
    juce::ValueTree createValueTreeFromConstraint(const ParameterConstraint& constraint) const;
    ParameterConstraint createConstraintFromValueTree(const juce::ValueTree& tree) const;

    // History management
    void addToHistory(const RandomizationHistory& entry);
    void updateStatistics();
    void pruneOldHistory();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RandomizationEngine)
};

} // namespace Zenith