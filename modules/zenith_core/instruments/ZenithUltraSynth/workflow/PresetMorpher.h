/*
  ==============================================================================

    PresetMorpher.h
    Created: [Date] Author: Claude AI
    Morph between multiple presets with smooth interpolation

  ==============================================================================
*/

#pragma once

#include "../../../JuceLibraryCode/JuceHeader.h"

namespace Zenith
{

class PresetMorpher
{
public:
    // Preset morph configuration
    static constexpr int maxMorphPresets = 4;
    static constexpr int maxMorphParameters = 512;
    static constexpr float defaultMorphSpeed = 1.0f;
    static constexpr float defaultMorphRange = 0.5f;

    // Morph interpolation types
    enum class InterpolationType
    {
        Linear,      // Linear interpolation
        SmoothStep,  // Smooth step interpolation
        SmoothestStep, // Smoothest step interpolation
        Spline,      // Spline interpolation
        Cosine,      // Cosine interpolation
        Exponential,  // Exponential interpolation
        Logarithmic, // Logarithmic interpolation
        Bezier,      // Bezier curve interpolation
        Custom       // Custom interpolation curve
    };

    // Morph space
    enum class MorphSpace
    {
        Linear,      // Linear morph space
        Curved,      // Curved morph space
        Grid,        // Grid-based morph space
        Radial,      // Radial morph space
        Path,        // Path-based morph space
        Spectral     // Spectral morph space
    };

    // Morph parameters
    struct MorphParameters
    {
        InterpolationType interpolation;
        MorphSpace space;
        float smoothness;         // 0-1, morph smoothness
        float speed;              // Morph speed
        float range;              // Morph range
        bool autoMorph;           // Enable automatic morphing
        float autoMorphSpeed;     // Automatic morphing speed
        float autoMorphRange;     // Automatic morphing range
        bool loopMorphing;        // Loop morphing
        bool bidirectional;       // Bidirectional morphing
        bool preserveTimbre;      // Preserve timbral characteristics
        bool preserveStructure;   // Preserve structural properties
        bool normalizeOutput;     // Normalize output
        bool morphAutomation;     // Morph automation data
        bool morphEffects;        // Morph effect parameters
        bool modulation;          // Morph modulation parameters
    };

    // Morph slot
    struct MorphSlot
    {
        juce::String presetId;             // Preset identifier
        juce::String presetName;           // Preset display name
        float amount;                      // Morph amount (0-1)
        juce::ValueTree presetData;        // Preset parameter data
        juce::Array<float> parameterValues; // Cached parameter values
        juce::Array<float> parameterWeights; // Parameter weights for morphing
        juce::Colour color;                // Visual color coding
        juce::StringArray categories;      // Preset categories
        juce::StringArray tags;            // Preset tags
        float similarity;                  // Similarity to other presets
        juce::uint64 lastUsed;             // Last usage timestamp
        bool enabled;                      // Slot enabled state
        bool isLoading;                    // Loading state
        float loadProgress;                // Loading progress
    };

    // Morph state
    struct MorphState
    {
        std::array<MorphSlot, maxMorphPresets> slots;
        MorphParameters parameters;
        float currentPosition[maxMorphPresets]; // Current position in morph space
        float targetPosition[maxMorphPresets];   // Target position for morphing
        float morphProgress;                     // 0-1, morph progress
        bool isMorphing;                         // Currently morphing
        bool isAutoMorphing;                     // Auto-morphing state
        juce::uint64 morphStartTime;            // Morph start time
        float morphDuration;                     // Morph duration in seconds
        juce::String currentPresetId;            // Current preset ID
        float currentParameterValues[maxMorphParameters]; // Current parameter values
        float targetParameterValues[maxMorphParameters];  // Target parameter values
        float smoothedParameterValues[maxMorphParameters]; // Smoothed parameter values
        float parameterSmoothingTimes[maxMorphParameters]; // Parameter smoothing times
    };

    // Preset compatibility analysis
    struct CompatibilityAnalysis
    {
        float overallSimilarity;              // Overall similarity score
        std::vector<float> parameterSimilarity; // Per-parameter similarity
        std::vector<juce::String> compatibleParameters; // Compatible parameters
        std::vector<juce::String> incompatibleParameters; // Incompatible parameters
        float timbreSimilarity;               // Timbral similarity
        float harmonicSimilarity;             // Harmonic similarity
        float dynamicSimilarity;              // Dynamic similarity
        float structureSimilarity;            // Structural similarity
        juce::String recommendation;         // Morphing recommendation
        bool isCompatible;                   // Overall compatibility
        int morphQuality;                     // Morph quality (1-10)
    };

    PresetMorpher();
    ~PresetMorpher();

    // Initialization
    void initialize(double sampleRate);
    void shutdown();
    void setSampleRate(double sampleRate);

    // Slot management
    bool addPreset(const juce::String& presetId, const juce::ValueTree& presetData);
    void removePreset(int slotIndex);
    void updatePreset(int slotIndex, const juce::String& presetId, const juce::ValueTree& presetData);
    bool setSlotPreset(int slotIndex, const juce::String& presetId);
    juce::String getSlotPreset(int slotIndex) const;
    void enableSlot(int slotIndex, bool enabled);
    bool isSlotEnabled(int slotIndex) const;
    int getEnabledSlotCount() const;

    // Morph control
    void setMorphAmount(int slotIndex, float amount);
    float getMorphAmount(int slotIndex) const;
    void setMorphParameters(const MorphParameters& params);
    MorphParameters getMorphParameters() const;
    void morphToPosition(const float position[maxMorphPresets], float duration = 0.0f);
    void stopMorphing();
    bool isMorphing() const;
    float getMorphProgress() const;

    // Auto-morphing
    void setAutoMorph(bool enabled);
    bool isAutoMorphEnabled() const;
    void setAutoMorphSpeed(float speed);
    float getAutoMorphSpeed() const;
    void setAutoMorphRange(float range);
    float getAutoMorphRange() const;
    void setAutoMorphPattern(const juce::String& pattern);
    juce::String getAutoMorphPattern() const;
    void setAutoMorphMode(int mode);
    int getAutoMorphMode() const;

    // Real-time morphing
    void updateMorph(float deltaTime);
    void applyMorphToParameters(juce::ValueTree& parameters);
    void applyMorphToParameter(const juce::String& paramId, float& value);
    std::vector<float> getMorphedParameterValues() const;
    juce::ValueTree getMorphedPreset() const;

    // Compatibility analysis
    CompatibilityAnalysis analyzeCompatibility(int slot1Index, int slot2Index) const;
    CompatibilityAnalysis analyzeMultiSlotCompatibility() const;
    float calculateParameterSimilarity(const MorphSlot& slot1, const MorphSlot& slot2) const;
    float calculateTimbreSimilarity(const MorphSlot& slot1, const MorphSlot& slot2) const;
    float calculateHarmonicSimilarity(const MorphSlot& slot1, const MorphSlot& slot2) const;
    juce::String getMorphRecommendation() const;

    // Morph space navigation
    void setMorphSpace(MorphSpace space);
    MorphSpace getMorphSpace() const;
    void setMorphCenter(const float center[maxMorphPresets]);
    void getMorphCenter(float center[maxMorphPresets]) const;
    float calculateMorphDistance(const float pos1[maxMorphPresets], const float pos2[maxMorphPresets]) const;
    void normalizeMorphPosition(float position[maxMorphPresets]) const;

    // Path morphing
    void createMorphPath(const std::vector<juce::Point<float>>& points);
    void setMorphPath(const juce::String& pathName);
    juce::String getMorphPath() const;
    void clearMorphPath();
    juce::Point<float> getCurrentMorphPosition() const;
    float getPathProgress() const;

    // Preset operations
    juce::ValueTree interpolatePresets(const juce::ValueTree& preset1, const juce::ValueTree& preset2, float t) const;
    juce::ValueTree morphPresets(const std::vector<juce::String>& presetIds, const std::vector<float>& amounts) const;
    bool saveMorphedPreset(const juce::String& name);
    juce::StringArray getMorphedPresetNames() const;
    void deleteMorphedPreset(const juce::String& name);

    // Analysis and monitoring
    float getMorphComplexity() const;
    float getMorphSmoothness() const;
    float getParameterDrift(const juce::String& paramId) const;
    std::vector<float> getParameterDrifts() const;
    std::vector<float> getParameterVelocities() const;
    std::vector<float> getParameterAccelerations() const;

    // Performance optimization
    void setCacheEnabled(bool enabled);
    bool isCacheEnabled() const;
    void setCacheSize(int size);
    int getCacheSize() const;
    void clearCache();
    void optimizeMorphing();

    // Export/Import
    bool saveMorphConfiguration(const juce::File& file);
    bool loadMorphConfiguration(const juce::File& file);
    bool exportMorphedPreset(const juce::String& presetName, const juce::File& file);
    bool importMorphedPreset(const juce::File& file);

    // Callbacks
    void setMorphCompleteListener(std::function<void()> callback);
    void setPChangeListener(std::function<void(float)> callback);
    void setSlotChangeListener(std::function<void(int)> callback);
    void setAutoMorphListener(std::function<void(float progress)> callback);

    // State management
    juce::ValueTree getState() const;
    void setState(const juce::ValueTree& state);
    void saveState(const juce::File& file);
    void loadState(const juce::File& file);

    // Statistics
    struct MorphStatistics
    {
        int totalMorphs;
        int successfulMorphs;
        int failedMorphs;
        float averageMorphTime;
        float averageMorphQuality;
        std::vector<float> parameterChanges;
        std::vector<juce::String> mostUsedPresets;
        juce::Date lastMorphDate;
        float totalMorphDuration;
        int autoMorphCount;
        int manualMorphCount;
    };
    MorphStatistics getStatistics() const;
    void resetStatistics();

private:
    // Morph state
    MorphState morphState_;
    double sampleRate_;
    bool initialized_;

    // Morph parameters
    MorphParameters parameters_;
    float smoothness_;
    float speed_;
    float range_;

    // Auto-morphing
    bool autoMorphEnabled_;
    float autoMorphSpeed_;
    float autoMorphRange_;
    int autoMorphMode_;
    juce::String autoMorphPattern_;
    juce::uint64 lastAutoMorphUpdate_;
    float autoMorphProgress_;

    // Path morphing
    struct MorphPath
    {
        juce::String name;
        std::vector<juce::Point<float>> points;
        bool isLooping;
        float speed;
        juce::uint64 lastUpdated;
    };
    std::vector<MorphPath> morphPaths_;
    juce::String currentPath_;

    // Cache
    struct CacheEntry
    {
        std::array<float, maxMorphPresets> position;
        juce::ValueTree morphedPreset;
        juce::Array<float> parameterValues;
        juce::uint64 timestamp;
    };
    std::vector<CacheEntry> cache_;
    int cacheSize_;
    bool cacheEnabled_;

    // Statistics
    MorphStatistics statistics_;
    juce::uint64 lastStatisticsUpdate_;

    // Callbacks
    std::function<void()> morphCompleteListener_;
    std::function<void(float)> progressListener_;
    std::function<void(int)> slotChangeListener_;
    std::function<void(float)> autoMorphListener_;

    // Private helper methods
    void initializeMorphing();
    void updateMorphProgress(float deltaTime);
    void updateAutoMorphing(float deltaTime);
    void updateParameterSmoothing(float deltaTime);
    void applyInterpolation(float& value, float target, float deltaTime);

    // Interpolation algorithms
    float interpolateLinear(float v1, float v2, float t) const;
    float interpolateSmoothStep(float v1, float v2, float t) const;
    float interpolateSmoothestStep(float v1, float v2, float t) const;
    float interpolateSpline(float v1, float v2, float t, float tension = 0.5f) const;
    float interpolateCosine(float v1, float v2, float t) const;
    float interpolateExponential(float v1, float v2, float t, float base = 2.0f) const;
    float interpolateLogarithmic(float v1, float v2, float t, float base = 2.0f) const;
    float interpolateBezier(float v1, float v2, float t, float cp1, float cp2) const;

    // Morph space operations
    void applyMorphSpace(float position[maxMorphPresets]) const;
    void applyLinearSpace(float position[maxMorphPresets]) const;
    void applyCurvedSpace(float position[maxMorphPresets]) const;
    void applyGridSpace(float position[maxMorphPresets]) const;
    void applyRadialSpace(float position[maxMorphPresets]) const;
    void applySpectralSpace(float position[maxMorphPresets]) const;

    // Parameter analysis
    void analyzePresetParameters(const juce::ValueTree& preset, MorphSlot& slot);
    void calculateParameterWeights(MorphSlot& slot);
    float calculateParameterSimilarity(float value1, float value2, const juce::String& paramId) const;
    void updateMorphCompatibility();

    // Cache management
    void updateCache();
    void clearCacheInternal();
    bool getFromCache(const std::array<float, maxMorphPresets>& position, CacheEntry& entry);
    void addToCache(const CacheEntry& entry);

    // Path morphing
    void updatePathMorphing();
    juce::Point<float> calculatePathPosition(float progress) const;
    float calculatePathProgress() const;

    // Statistics tracking
    void updateStatistics();
    void recordMorphStart();
    void recordMorphEnd(bool success, float quality);
    void recordAutoMorph();
    void recordManualMorph();

    // Export/Import helpers
    juce::ValueTree createValueTreeFromMorphState() const;
    void createMorphStateFromValueTree(const juce::ValueTree& tree);
    juce::ValueTree createValueTreeFromSlot(const MorphSlot& slot) const;
    MorphSlot createSlotFromValueTree(const juce::ValueTree& tree) const;

    // Utility methods
    juce::String generateMorphId() const;
    juce::String generatePresetId() const;
    juce::String generatePathId() const;
    bool validateSlotIndex(int index) const;
    bool validatePresetData(const juce::ValueTree& data) const;
    float normalize(float value, float min, float max) const;
    float clamp(float value, float min, float max) const;

    // Thread safety
    juce::CriticalSection morphLock_;
    juce::ScopedLock scopedLock_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetMorpher)
};

} // namespace Zenith