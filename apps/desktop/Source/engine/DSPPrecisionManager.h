/*
  ==============================================================================

    DSPPrecisionManager.h
    Created: 2026-02-18
    Author:  Zenith DAW - Month 7: Audio Engine Safety (Gap #9)

    Manages numerical precision and detects floating-point issues in DSP.

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
 * @brief Precision error type
 */
enum class PrecisionError {
    Overflow,          // Floating-point overflow
    Underflow,         // Floating-point underflow
    AccumulationError, // Accumulation error
    RoundingError,     // Rounding error
    PrecisionLoss,     // Loss of precision
    NaN,              // Not-a-Number detected
    Infinity,         // Infinity detected
    Unknown
};

//==============================================================================
/**
 * @brief Precision event
 */
struct PrecisionEvent {
    PrecisionError error;
    juce::String description;
    double value = 0.0;
    double severity = 0.0;  // 0-10

    juce::String toString() const {
        juce::String errorStr;
        switch (error) {
            case PrecisionError::Overflow: errorStr = "Overflow"; break;
            case PrecisionError::Underflow: errorStr = "Underflow"; break;
            case PrecisionError::AccumulationError: errorStr = "Accum Error"; break;
            case PrecisionError::RoundingError: errorStr = "Rounding"; break;
            case PrecisionError::PrecisionLoss: errorStr = "Precision Loss"; break;
            case PrecisionError::NaN: errorStr = "NaN"; break;
            case PrecisionError::Infinity: errorStr = "Infinity"; break;
            case PrecisionError::Unknown: errorStr = "Unknown"; break;
        }
        return "[" + errorStr + "] " + description + " (value: " + juce::String(value, 6) + ")";
    }
};

//==============================================================================
/**
 * @brief DSP precision statistics
 */
struct DSPPrecisionStatistics {
    std::atomic<int> overflows{0};
    std::atomic<int> underflows{0};
    std::atomic<int> nansDetected{0};
    std::atomic<int> infinitiesDetected{0};
    std::atomic<int> precisionLosses{0};

    juce::String toString() const {
        return "DSP Precision: " +
               juce::String(overflows.load()) + " overflows, " +
               juce::String(underflows.load()) + " underflows, " +
               juce::String(nansDetected.load()) + " NaNs";
    }
};

//==============================================================================
/**
 * @brief DSP precision manager
 *
 * Features:
 * - Overflow/underflow detection
 * - Numerical precision validation
 * - Accumulation error detection
 * - Double-precision processing option
 */
class DSPPrecisionManager {
public:
    //==========================================================================
    DSPPrecisionManager();
    ~DSPPrecisionManager();

    //==========================================================================
    /**
     * @brief Validate audio buffer for precision issues
     * @param buffer Buffer to validate
     * @return List of precision events found
     */
    std::vector<PrecisionEvent> validateBuffer(
        const juce::AudioBuffer<float>& buffer);

    //==========================================================================
    /**
     * @brief Validate single value
     * @param value Value to validate
     * @param context Description of where value came from
     * @return Precision event (empty if valid)
     */
    std::vector<PrecisionEvent> validateValue(
        double value,
        const juce::String& context = "");

    //==========================================================================
    /**
     * @brief Check for overflow
     * @param value Value to check
     * @return true if overflow detected
     */
    static bool isOverflow(float value);
    static bool isOverflow(double value);

    //==========================================================================
    /**
     * @brief Check for underflow
     * @param value Value to check
     * @return true if underflow detected
     */
    static bool isUnderflow(float value);
    static bool isUnderflow(double value);

    //==========================================================================
    /**
     * @brief Clamp value to safe range
     * @param value Value to clamp
     * @return Clamped value
     */
    static float clamp(float value);
    static double clamp(double value);

    //==========================================================================
    /**
     * @brief Get statistics
     */
    DSPPrecisionStatistics getStatistics() const {
        return statistics_;
    }

    //==========================================================================
    /**
     * @brief Reset statistics
     */
    void resetStatistics();

private:
    //==========================================================================
    void recordEvent(const PrecisionEvent& event);

    //==========================================================================
    // Statistics
    DSPPrecisionStatistics statistics_;

    // Event history
    std::vector<PrecisionEvent> eventHistory_;
    static constexpr int maxHistorySize = 50;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DSPPrecisionManager)
};

//==============================================================================
/**
 * @brief Singleton accessor for DSP precision manager
 */
class DSPPrecisionManagerHolder {
public:
    static DSPPrecisionManager& getInstance() {
        static DSPPrecisionManager instance;
        return instance;
    }

    DSPPrecisionManagerHolder(const DSPPrecisionManagerHolder&) = delete;
    DSPPrecisionManagerHolder& operator=(const DSPPrecisionManagerHolder&) = delete;

private:
    DSPPrecisionManagerHolder() = default;
    ~DSPPrecisionManagerHolder() = default;
};

} // namespace zenith
