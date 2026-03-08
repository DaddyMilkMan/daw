/*
  ==============================================================================

    DenormalProtection.h
    Created: 2026-02-18
    Author:  Zenith DAW - Month 7: Audio Engine Safety (Gap #6)

    Detects and eliminates denormal floating-point numbers for performance.

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
 * @brief Denormal statistics
 */
struct DenormalStatistics {
    std::atomic<int> denormalsDetected{0};
    std::atomic<int> denormalsFlushed{0};
    std::atomic<int> buffersCleaned{0};
    std::atomic<double> totalFlushTime{0.0};  // microseconds

    juce::String toString() const {
        return "Denormals: " +
               juce::String(denormalsDetected.load()) + " detected, " +
               juce::String(denormalsFlushed.load()) + " flushed";
    }
};

//==============================================================================
/**
 * @brief Denormal protection configuration
 */
struct DenormalProtectionConfig {
    bool enableAutoFlush = true;        // Automatically flush denormals
    bool enableStatistics = true;
    double denormalThreshold = 1e-30;   // Threshold for denormal detection
    bool flushToZero = true;            // FTZ (Flush To Zero) mode
    bool denormalsAreZero = true;       // DAZ (Denormals Are Zero) mode
};

//==============================================================================
/**
 * @brief Denormal protection utility
 *
 * Features:
 * - Detect denormal numbers in audio buffers
 * - Flush denormals to zero efficiently
 * - Denormal statistics tracking
 * - Performance impact measurement
 */
class DenormalProtection {
public:
    //==========================================================================
    DenormalProtection();
    ~DenormalProtection();

    //==========================================================================
    /**
     * @brief Check if value is denormal
     * @param value Floating-point value to check
     * @return true if denormal
     */
    static bool isDenormal(float value);
    static bool isDenormal(double value);

    //==========================================================================
    /**
     * @brief Flush denormal to zero
     * @param value Value to flush
     * @return Zero if denormal, original value otherwise
     */
    static float flushDenormal(float value);
    static double flushDenormal(double value);

    //==========================================================================
    /**
     * @brief Clean audio buffer (remove denormals)
     * @param buffer Audio buffer to clean
     * @param config Protection configuration
     * @return Number of denormals flushed
     */
    int cleanBuffer(juce::AudioBuffer<float>& buffer,
                   const DenormalProtectionConfig& config = DenormalProtectionConfig());

    //==========================================================================
    /**
     * @brief Clean audio buffer (double precision)
     */
    int cleanBuffer(juce::AudioBuffer<double>& buffer,
                   const DenormalProtectionConfig& config = DenormalProtectionConfig());

    //==========================================================================
    /**
     * @brief Count denormals in buffer
     * @param buffer Audio buffer to check
     * @return Number of denormals found
     */
    int countDenormals(const juce::AudioBuffer<float>& buffer) const;

    //==========================================================================
    /**
     * @brief Get statistics
     */
    DenormalStatistics getStatistics() const {
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
    DenormalProtectionConfig getConfig() const {
        return config_;
    }

    //==========================================================================
    /**
     * @brief Set configuration
     */
    void setConfig(const DenormalProtectionConfig& config) {
        config_ = config;
    }

    //==========================================================================
    /**
     * @brief Enable hardware FTZ/DAZ (platform-specific)
     * Attempts to enable CPU-level flush-to-zero and denormals-are-zero modes
     */
    static bool enableHardwareDenormals();

private:
    //==========================================================================
    // Statistics
    DenormalStatistics statistics_;

    // Configuration
    DenormalProtectionConfig config_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DenormalProtection)
};

//==============================================================================
/**
 * @brief Singleton accessor for denormal protection
 */
class DenormalProtectionHolder {
public:
    static DenormalProtection& getInstance() {
        static DenormalProtection instance;
        return instance;
    }

    DenormalProtectionHolder(const DenormalProtectionHolder&) = delete;
    DenormalProtectionHolder& operator=(const DenormalProtectionHolder&) = delete;

private:
    DenormalProtectionHolder() = default;
    ~DenormalProtectionHolder() = default;
};

} // namespace zenith
