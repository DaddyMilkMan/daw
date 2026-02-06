/*
  ==============================================================================

    WavetableMorpher.h
    Created: [Date] Author: Claude AI
    Multi-wavetable morphing with advanced interpolation

  ==============================================================================
*/

#pragma once

#include "../../../JuceLibraryCode/JuceHeader.h"
#include "AdvancedWavetableEngine.h"

namespace Zenith
{

class WavetableMorpher
{
public:
    // Morph types
    enum class MorphType
    {
        Linear,      // Linear interpolation
        Spectral,     // Spectral interpolation
        Phase,        // Phase interpolation
        Amplitude,    // Amplitude interpolation
        Frequency,    // Frequency domain interpolation
        Harmonic,     // Harmonic interpolation
        Custom        // Custom morphing algorithm
    };

    // Interpolation curves
    enum class InterpolationCurve
    {
        Linear,      // Linear curve
        Exponential,  // Exponential curve
        Logarithmic,  // Logarithmic curve
        Power,       // Power curve
        Sine,        // Sine curve
        Custom       // Custom curve
    };

    // Morph space
    enum class MorphSpace
    {
        2D,          // 2D morphing (X, Y)
        3D,          // 3D morphing (X, Y, Z)
        4D,          // 4D morphing (X, Y, Z, W)
        Radial,      // Radial morphing from center
        Grid,        // Grid morphing
        Path        // Path morphing
    };

    // Morphing parameters
    struct MorphParameters
    {
        MorphType morphType;
        InterpolationCurve curve;
        MorphSpace space;
        float smoothness;        // 0-1, morph smoothness
        float crossfade;        // 0-1, crossfade amount
        bool autoMorph;         // Enable automatic morphing
        float autoMorphSpeed;   // Automatic morphing speed
        float autoMorphRange;   // Automatic morphing range
        bool wrapMorphing;      // Wrap morphing around
        bool preservePhase;     // Preserve phase continuity
        bool normalizeOutput;   // Normalize output amplitude
        bool preserveFormants;  // Preserve formant characteristics
        bool preserveHarmonics; // Preserve harmonic structure
    };

    // Morph slot
    struct MorphSlot
    {
        AdvancedWavetableEngine::WavetableData wavetable;
        juce::String name;
        float position[4];       // Position in morph space (X, Y, Z, W)
        float amount;           // Blend amount
        bool enabled;
        juce::Colour color;
        std::vector<float> metadata;
        juce::uint64 lastModified;
    };

    // Morph state
    struct MorphState
    {
        std::vector<MorphSlot> slots;
        MorphParameters parameters;
        float currentPosition[4]; // Current position in morph space
        float targetPosition[4];   // Target position for morphing
        float morphProgress;      // 0-1, morph progress
        bool isMorphing;
        juce::uint64 morphStartTime;
        float morphDuration;
    };

    WavetableMorpher();
    ~WavetableMorpher();

    // Initialization
    void initialize(int sampleRate, int maxFrames = 256);
    void shutdown();

    // Slot management
    int addSlot(const AdvancedWavetableEngine::WavetableData& table, const juce::String& name);
    void removeSlot(int index);
    void updateSlot(int index, const AdvancedWavetableEngine::WavetableData& table);
    void enableSlot(int index, bool enabled);
    bool isSlotEnabled(int index) const;
    int getSlotCount() const;

    // Position control
    void setSlotPosition(int index, float x, float y, float z = 0.0f, float w = 0.0f);
    void getSlotPosition(int index, float& x, float& y, float& z, float& w) const;
    void setCurrentPosition(float x, float y, float z = 0.0f, float w = 0.0f);
    void getCurrentPosition(float& x, float& y, float& z, float& w) const;

    // Morph control
    void setMorphParameters(const MorphParameters& params);
    MorphParameters getMorphParameters() const;
    void morphToPosition(float x, float y, float z = 0.0f, float w = 0.0f, float duration = 0.0f);
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

    // Morphing
    AdvancedWavetableEngine::WavetableData getMorphedWavetable() const;
    std::vector<float> getMorphedFrame(int frameIndex = 0) const;
    float getMorphedSample(int frameIndex = 0, int sampleIndex = 0) const;

    // Real-time processing
    void processSample(float& sample, int frameIndex = 0);
    void processFrame(std::vector<float>& frame, int frameIndex = 0);
    void processBuffer(juce::AudioBuffer<float>& buffer, int frameIndex = 0);

    // Analysis and monitoring
    float getMorphComplexity() const;
    float getOutputRMS() const;
    float getOutputPeak() const;
    std::vector<float> getSlotContributions() const;
    std::vector<float> getFrequencyResponse() const;

    // Parameter smoothing
    void setSmoothingTime(float timeMs);
    float getSmoothingTime() const;
    void setCrossfadeCurve(InterpolationCurve curve);
    InterpolationCurve getCrossfadeCurve() const;

    // Path morphing
    void createMorphPath(const std::vector<juce::Point<float>>& points);
    void setMorphPath(const juce::String& pathName);
    juce::String getMorphPath() const;
    void clearMorphPath();

    // Presets and serialization
    void loadPreset(const juce::File& file);
    void savePreset(const juce::File& file);
    void loadPresetFromData(const juce::MemoryBlock& data);
    void savePresetToData(juce::MemoryBlock& data) const;
    juce::ValueTree getState() const;
    void setState(const juce::ValueTree& state);

    // Callbacks
    void setMorphChangeListener(std::function<void()> callback);
    void setSlotChangeListener(std::function<void(int)> callback);
    void setAutoMorphListener(std::function<void(float progress)> callback);

    // Performance optimization
    void setCacheEnabled(bool enabled);
    bool isCacheEnabled() const;
    void setCacheSize(int size);
    int getCacheSize() const;
    void clearCache();

    // Validation
    bool validateMorphConfiguration() const;
    bool validateSlot(int index) const;
    std::vector<juce::String> getValidationErrors() const;

private:
    // Morph state
    MorphState morphState_;
    int sampleRate_;
    int maxFrames_;
    bool initialized_;

    // Morphing parameters
    MorphParameters parameters_;
    float smoothingTime_;
    InterpolationCurve crossfadeCurve_;

    // Cache
    struct CacheEntry
    {
        std::vector<float> frame;
        float position[4];
        juce::uint64 timestamp;
    };
    std::vector<CacheEntry> cache_;
    int cacheSize_;
    bool cacheEnabled_;

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

    // Performance monitoring
    float morphComplexity_;
    float outputRMS_;
    float outputPeak_;

    // Callbacks
    std::function<void()> morphChangeListener_;
    std::function<void(int)> slotChangeListener_;
    std::function<void(float)> autoMorphListener_;

    // Private helper methods
    void initializeCache();
    void updateCache();
    void clearCacheInternal();

    // Morphing algorithms
    std::vector<float> performLinearMorph(const std::vector<float>& frame1,
                                        const std::vector<float>& frame2,
                                        float amount) const;
    std::vector<float> performSpectralMorph(const std::vector<float>& frame1,
                                           const std::vector<float>& frame2,
                                           float amount) const;
    std::vector<float> performPhaseMorph(const std::vector<float>& frame1,
                                       const std::vector<float>& frame2,
                                       float amount) const;
    std::vector<float> performAmplitudeMorph(const std::vector<float>& frame1,
                                           const std::vector<float>& frame2,
                                           float amount) const;
    std::vector<float> performFrequencyMorph(const std::vector<float>& frame1,
                                           const std::vector<float>& frame2,
                                           float amount) const;
    std::vector<float> performHarmonicMorph(const std::vector<float>& frame1,
                                          const std::vector<float>& frame2,
                                          float amount) const;

    // Interpolation curves
    float interpolate(float v1, float v2, float t, InterpolationCurve curve) const;
    float linearCurve(float t) const;
    float exponentialCurve(float t) const;
    float logarithmicCurve(float t) const;
    float powerCurve(float t, float exponent) const;
    float sineCurve(float t) const;

    // Space operations
    std::vector<float> interpolateInSpace(const std::vector<MorphSlot>& slots,
                                        const float position[4],
                                        const MorphSpace space) const;
    float calculateDistance(const float pos1[4], const float pos2[4], MorphSpace space) const;
    void normalizePosition(float position[4], MorphSpace space) const;

    // Path morphing
    void updatePathMorphing();
    juce::Point<float> getPathPosition(float progress) const;
    float calculatePathProgress() const;

    // Auto-morphing
    void updateAutoMorphing();
    juce::Point<float> generateAutoMorphPosition() const;
    float calculateAutoMorphProgress() const;

    // Analysis
    void updateAnalysis();
    float calculateMorphComplexity() const;
    std::vector<float> calculateFrequencyResponse(const std::vector<float>& frame) const;
    std::vector<float> calculateSlotContributions() const;

    // Validation
    void validateConfiguration();
    std::vector<juce::String> validationErrors_;
    void addValidationError(const juce::String& error);

    // State management
    juce::ValueTree createValueTreeFromState() const;
    void createStateFromValueTree(const juce::ValueTree& state);

    // Utility methods
    juce::String generateSlotId() const;
    juce::String generateMorphPathId() const;
    float normalize(float value, float min, float max) const;
    float clamp(float value, float min, float max) const;

    // Thread safety
    juce::CriticalSection morphLock_;
    juce::ScopedLock scopedLock_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WavetableMorpher)
};

} // namespace Zenith