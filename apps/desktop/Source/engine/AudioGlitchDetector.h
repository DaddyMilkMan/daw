/*
  ==============================================================================

    AudioGlitchDetector.h
    Created: 2026-02-18
    Author:  Zenith DAW - Month 7: Audio Engine Safety (Gap #4)

    Real-time audio glitch detection with comprehensive analysis.

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <vector>
#include <atomic>

namespace zenith {

//==============================================================================
/**
 * @brief Glitch type
 */
enum class GlitchType {
    Discontinuity,      // Sudden jump in signal
    NaN,               // Not-a-Number detected
    Infinity,          // Infinite value detected
    DCOffset,          // DC offset present
    Clipping,          // Signal clipping
    SuddenLevelChange, // Abrupt level change
    Dropout,           // Signal dropout
    Unknown
};

//==============================================================================
/**
 * @brief Glitch event
 */
struct GlitchEvent {
    GlitchType type;
    juce::String description;
    double timestamp = 0.0;        // Seconds
    int channel = 0;               // Audio channel
    int samplePosition = 0;        // Sample position
    double value = 0.0;            // Problematic value
    double severity = 0.0;         // 0-10

    juce::String toString() const {
        juce::String typeStr;
        switch (type) {
            case GlitchType::Discontinuity: typeStr = "Discontinuity"; break;
            case GlitchType::NaN: typeStr = "NaN"; break;
            case GlitchType::Infinity: typeStr = "Infinity"; break;
            case GlitchType::DCOffset: typeStr = "DC Offset"; break;
            case GlitchType::Clipping: typeStr = "Clipping"; break;
            case GlitchType::SuddenLevelChange: typeStr = "Level Change"; break;
            case GlitchType::Dropout: typeStr = "Dropout"; break;
            case GlitchType::Unknown: typeStr = "Unknown"; break;
        }
        return "[" + typeStr + "] Ch:" + juce::String(channel) +
               " Sample:" + juce::String(samplePosition) +
               " Value:" + juce::String(value, 6) +
               " " + description;
    }
};

//==============================================================================
/**
 * @brief Glitch detection statistics
 */
struct GlitchStatistics {
    std::atomic<int> totalGlitches{0};
    std::atomic<int> discontinuities{0};
    std::atomic<int> nanValues{0};
    std::atomic<int> infinityValues{0};
    std::atomic<int> dcOffsets{0};
    std::atomic<int> clips{0};
    std::atomic<int> dropouts{0};

    juce::String toString() const {
        return "Glitches: " +
               juce::String(totalGlitches.load()) + " total (" +
               juce::String(discontinuities.load()) + " disc, " +
               juce::String(nanValues.load()) + " NaN, " +
               juce::String(clips.load()) + " clips)";
    }
};

//==============================================================================
/**
 * @brief Glitch detection configuration
 */
struct GlitchDetectionConfig {
    bool enableDiscontinuityDetection = true;
    bool enableNaNInfinityDetection = true;
    bool enableDCOffsetDetection = true;
    bool enableClippingDetection = true;
    bool enableSuddenLevelChangeDetection = true;
    bool enableDropoutDetection = true;

    double discontinuityThreshold = 0.5;     // Max sample-to-sample change
    double dcOffsetThreshold = 0.01;         // 1% of full scale
    double clippingThreshold = 0.999;        // Clipping level
    double suddenLevelChangeThreshold = 20.0; // dB change
    double dropoutThreshold = -60.0;          // dB level for dropout
};

//==============================================================================
/**
 * @brief Real-time audio glitch detector
 *
 * Features:
 * - Discontinuity detection
 * - NaN/Infinity detection
 * - DC offset detection
 * - Clipping detection with severity levels
 * - Sudden level change detection
 * - Dropout detection
 */
class AudioGlitchDetector {
public:
    //==========================================================================
    AudioGlitchDetector();
    ~AudioGlitchDetector();

    //==========================================================================
    /**
     * @brief Analyze audio buffer for glitches
     * @param buffer Audio buffer to analyze
     * @param sampleRate Current sample rate
     * @return List of glitches found
     */
    std::vector<GlitchEvent> analyzeBuffer(
        const juce::AudioBuffer<float>& buffer,
        double sampleRate = 48000.0);

    //==========================================================================
    /**
     * @brief Real-time analysis (called from audio thread)
     * More efficient than analyzeBuffer for real-time use
     */
    std::vector<GlitchEvent> processBuffer(
        const juce::AudioBuffer<float>& buffer);

    //==========================================================================
    /**
     * @brief Get recent glitches
     */
    std::vector<GlitchEvent> getRecentGlitches() const;

    //==========================================================================
    /**
     * @brief Clear glitch history
     */
    void clearHistory();

    //==========================================================================
    /**
     * @brief Get statistics
     */
    GlitchStatistics getStatistics() const {
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
    GlitchDetectionConfig getConfig() const {
        return config_;
    }

    //==========================================================================
    /**
     * @brief Set configuration
     */
    void setConfig(const GlitchDetectionConfig& config) {
        config_ = config;
    }

private:
    //==========================================================================
    std::vector<GlitchEvent> detectDiscontinuities(
        const juce::AudioBuffer<float>& buffer);

    std::vector<GlitchEvent> detectNaNInfinity(
        const juce::AudioBuffer<float>& buffer);

    std::vector<GlitchEvent> detectDCOffset(
        const juce::AudioBuffer<float>& buffer);

    std::vector<GlitchEvent> detectClipping(
        const juce::AudioBuffer<float>& buffer);

    std::vector<GlitchEvent> detectSuddenLevelChanges(
        const juce::AudioBuffer<float>& buffer);

    std::vector<GlitchEvent> detectDropouts(
        const juce::AudioBuffer<float>& buffer);

    void recordGlitch(const GlitchEvent& event);

    //==========================================================================
    // Previous samples for discontinuity detection
    std::vector<float> previousSamples_;
    std::vector<double> previousLevels_;  // dB levels

    // Configuration
    GlitchDetectionConfig config_;

    // History
    std::vector<GlitchEvent> glitchHistory_;
    static constexpr int maxHistorySize = 100;

    // Statistics
    GlitchStatistics statistics_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioGlitchDetector)
};

//==============================================================================
/**
 * @brief Singleton accessor for audio glitch detector
 */
class AudioGlitchDetectorHolder {
public:
    static AudioGlitchDetector& getInstance() {
        static AudioGlitchDetector instance;
        return instance;
    }

    AudioGlitchDetectorHolder(const AudioGlitchDetectorHolder&) = delete;
    AudioGlitchDetectorHolder& operator=(const AudioGlitchDetectorHolder&) = delete;

private:
    AudioGlitchDetectorHolder() = default;
    ~AudioGlitchDetectorHolder() = default;
};

} // namespace zenith
