/*
  ==============================================================================

    SampleRateConverter.h
    Created: 2026-02-18
    Author:  Zenith DAW - Month 7: Audio Engine Safety (Gap #2)

    Safe sample rate conversion with quality management and artifact detection.

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <vector>

namespace zenith {

//==============================================================================
/**
 * @brief SRC quality level
 */
enum class SRCQuality {
    Fastest,      // Linear interpolation (fastest, lowest quality)
    Low,          // 2-point interpolation
    Medium,       // 4-point interpolation
    High,         // 8-point interpolation
    Best,         // 16-point interpolation (slowest, highest quality)
    Automatic     // Automatically choose based on ratio
};

//==============================================================================
/**
 * @brief SRC artifact type
 */
enum class SRCArtifactType {
    Aliasing,          // Aliasing artifacts
    Imaging,           // Spectral imaging
    NoiseModulation,   // Noise modulation
    PhaseDistortion,   // Phase distortion
    AmplitudeModulation, // Amplitude modulation
    Unknown
};

//==============================================================================
/**
 * @brief SRC artifact event
 */
struct SRCArtifactEvent {
    SRCArtifactType type;
    juce::String description;
    double inputSampleRate = 0.0;
    double outputSampleRate = 0.0;
    double ratio = 0.0;           // Conversion ratio
    double severity = 0.0;        // 0-10
    int detectedAtSample = 0;     // Sample position

    juce::String toString() const {
        juce::String typeStr;
        switch (type) {
            case SRCArtifactType::Aliasing: typeStr = "Aliasing"; break;
            case SRCArtifactType::Imaging: typeStr = "Imaging"; break;
            case SRCArtifactType::NoiseModulation: typeStr = "Noise Mod"; break;
            case SRCArtifactType::PhaseDistortion: typeStr = "Phase Dist"; break;
            case SRCArtifactType::AmplitudeModulation: typeStr = "Amp Mod"; break;
            case SRCArtifactType::Unknown: typeStr = "Unknown"; break;
        }
        return "[" + typeStr + "] " + description +
               " (" + juce::String(inputSampleRate, 0) + "Hz -> " +
               juce::String(outputSampleRate, 0) + "Hz)";
    }
};

//==============================================================================
/**
 * @brief SRC statistics
 */
struct SRCStatistics {
    int totalConversions = 0;
    int samplesConverted = 0;
    int artifactsDetected = 0;
    double averageProcessingTime = 0.0;  // microseconds
    SRCQuality lastQuality = SRCQuality::Medium;

    juce::String toString() const {
        return "SRC: " + juce::String(totalConversions) + " conversions, " +
               juce::String(samplesConverted) + " samples, " +
               juce::String(artifactsDetected) + " artifacts";
    }
};

//==============================================================================
/**
 * @brief SRC configuration
 */
struct SRCConfig {
    SRCQuality quality = SRCQuality::High;
    bool enableArtifactDetection = true;
    double artifactThreshold = 0.01;     // 1% THD threshold
    bool enableFiltering = true;
    double filterCutoff = 0.45;          // 45% of Nyquist
    bool enableDithering = false;        // Dither when reducing quality
};

//==============================================================================
/**
 * @brief Safe sample rate converter
 *
 * Features:
 * - Multiple quality algorithms
 * - Artifact detection
 * - Automatic quality selection
 * - Configurable filtering
 * - Comprehensive statistics
 */
class SampleRateConverter {
public:
    //==========================================================================
    SampleRateConverter();
    ~SampleRateConverter();

    //==========================================================================
    /**
     * @brief Convert audio buffer to new sample rate
     * @param inputBuffer Input audio buffer
     * @param inputSampleRate Input sample rate
     * @param outputSampleRate Output sample rate
     * @param config Conversion configuration
     * @return Converted buffer (may be null on failure)
     */
    juce::AudioBuffer<float>* convertBuffer(
        const juce::AudioBuffer<float>& inputBuffer,
        double inputSampleRate,
        double outputSampleRate,
        const SRCConfig& config = SRCConfig());

    //==========================================================================
    /**
     * @brief Convert in-place (modifies buffer)
     * @param buffer Audio buffer to convert
     * @param inputSampleRate Current sample rate
     * @param outputSampleRate Target sample rate
     * @param config Conversion configuration
     * @return true if successful
     */
    bool convertBufferInPlace(
        juce::AudioBuffer<float>& buffer,
        double inputSampleRate,
        double outputSampleRate,
        const SRCConfig& config = SRCConfig());

    //==========================================================================
    /**
     * @brief Get required output size for conversion
     */
    int calculateOutputSize(
        int inputSamples,
        double inputSampleRate,
        double outputSampleRate) const;

    //==========================================================================
    /**
     * @brief Check if conversion is needed
     */
    static bool needsConversion(double inputRate, double outputRate) {
        return std::abs(inputRate - outputRate) > 0.1;
    }

    //==========================================================================
    /**
     * @brief Get recommended quality for ratio
     */
    static SRCQuality getRecommendedQuality(double ratio);

    //==========================================================================
    /**
     * @brief Get conversion ratio
     */
    static double getRatio(double inputRate, double outputRate) {
        return outputRate / inputRate;
    }

    //==========================================================================
    /**
     * @brief Check if ratio is supported
     */
    static bool isSupportedRatio(double ratio) {
        return ratio > 0.0 && ratio < 100.0;  // Reasonable limits
    }

    //==========================================================================
    /**
     * @brief Get detected artifacts
     */
    std::vector<SRCArtifactEvent> getArtifacts() const {
        return artifacts_;
    }

    //==========================================================================
    /**
     * @brief Clear artifact history
     */
    void clearArtifacts() {
        artifacts_.clear();
    }

    //==========================================================================
    /**
     * @brief Get statistics
     */
    SRCStatistics getStatistics() const {
        return statistics_;
    }

    //==========================================================================
    /**
     * @brief Reset statistics
     */
    void resetStatistics();

    //==========================================================================
    /**
     * @brief Get configuration
     */
    SRCConfig getConfig() const {
        return config_;
    }

    //==========================================================================
    /**
     * @brief Set configuration
     */
    void setConfig(const SRCConfig& config) {
        config_ = config;
    }

private:
    //==========================================================================
    juce::AudioBuffer<float>* performConversion(
        const juce::AudioBuffer<float>& input,
        double ratio,
        const SRCConfig& config);

    void detectArtifacts(
        const juce::AudioBuffer<float>& output,
        double ratio,
        const SRCConfig& config);

    float interpolateLinear(
        const float* samples,
        int size,
        double position) const;

    float interpolateLagrange(
        const float* samples,
        int size,
        double position,
        int order) const;

    void applyLowpassFilter(
        juce::AudioBuffer<float>& buffer,
        double cutoffRatio) const;

    //==========================================================================
    // Statistics
    SRCStatistics statistics_;

    // Configuration
    SRCConfig config_;

    // Artifact tracking
    std::vector<SRCArtifactEvent> artifacts_;
    static constexpr int maxArtifacts = 50;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleRateConverter)
};

//==============================================================================
/**
 * @brief Singleton accessor for sample rate converter
 */
class SampleRateConverterHolder {
public:
    static SampleRateConverter& getInstance() {
        static SampleRateConverter instance;
        return instance;
    }

    SampleRateConverterHolder(const SampleRateConverterHolder&) = delete;
    SampleRateConverterHolder& operator=(const SampleRateConverterHolder&) = delete;

private:
    SampleRateConverterHolder() = default;
    ~SampleRateConverterHolder() = default;
};

} // namespace zenith
