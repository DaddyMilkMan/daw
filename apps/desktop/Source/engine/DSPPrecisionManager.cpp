/*
  ==============================================================================

    DSPPrecisionManager.cpp
    Implementation of DSP precision management

  ==============================================================================
*/

#include "DSPPrecisionManager.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <limits>

namespace zenith {

//==============================================================================
// DSPPrecisionManager Implementation
//==============================================================================

DSPPrecisionManager::DSPPrecisionManager() {
    std::cout << "DSPPrecisionManager: Initialized" << std::endl;
}

DSPPrecisionManager::~DSPPrecisionManager() {
    std::cout << "DSPPrecisionManager: Shut down ("
              << statistics_.overflows.load() << " overflows, "
              << statistics_.precisionLosses.load() << " precision losses)" << std::endl;
}

//==============================================================================
std::vector<PrecisionEvent> DSPPrecisionManager::validateBuffer(
    const juce::AudioBuffer<float>& buffer)
{
    std::vector<PrecisionEvent> events;

    const int channels = buffer.getNumChannels();
    const int samples = buffer.getNumSamples();

    for (int channel = 0; channel < channels; ++channel) {
        const float* data = buffer.getReadPointer(channel);

        for (int i = 0; i < samples; ++i) {
            float value = data[i];

            // Check for NaN
            if (std::isnan(value)) {
                PrecisionEvent event;
                event.error = PrecisionError::NaN;
                event.description = "NaN detected in buffer";
                event.value = 0.0;
                event.severity = 10.0;
                events.push_back(event);
                recordEvent(event);
                statistics_.nansDetected++;
            }

            // Check for infinity
            else if (std::isinf(value)) {
                PrecisionEvent event;
                event.error = PrecisionError::Infinity;
                event.description = "Infinity detected in buffer";
                event.value = value > 0 ? 1e30 : -1e30;
                event.severity = 10.0;
                events.push_back(event);
                recordEvent(event);
                statistics_.infinitiesDetected++;
            }

            // Check for overflow
            else if (isOverflow(value)) {
                PrecisionEvent event;
                event.error = PrecisionError::Overflow;
                event.description = "Floating-point overflow";
                event.value = static_cast<double>(value);
                event.severity = 9.0;
                events.push_back(event);
                recordEvent(event);
                statistics_.overflows++;
            }

            // Check for underflow
            else if (isUnderflow(value)) {
                PrecisionEvent event;
                event.error = PrecisionError::Underflow;
                event.description = "Floating-point underflow";
                event.value = static_cast<double>(value);
                event.severity = 3.0;  // Underflow is less severe
                events.push_back(event);
                recordEvent(event);
                statistics_.underflows++;
            }
        }
    }

    return events;
}

//==============================================================================
std::vector<PrecisionEvent> DSPPrecisionManager::validateValue(
    double value,
    const juce::String& context)
{
    std::vector<PrecisionEvent> events;

    // Check for NaN
    if (std::isnan(value)) {
        PrecisionEvent event;
        event.error = PrecisionError::NaN;
        event.description = "NaN detected" + (context.isNotEmpty() ? " in " + context : "");
        event.value = 0.0;
        event.severity = 10.0;
        events.push_back(event);
        recordEvent(event);
        statistics_.nansDetected++;
    }

    // Check for infinity
    else if (std::isinf(value)) {
        PrecisionEvent event;
        event.error = PrecisionError::Infinity;
        event.description = "Infinity detected" + (context.isNotEmpty() ? " in " + context : "");
        event.value = value > 0 ? 1e30 : -1e30;
        event.severity = 10.0;
        events.push_back(event);
        recordEvent(event);
        statistics_.infinitiesDetected++;
    }

    // Check for overflow
    else if (isOverflow(value)) {
        PrecisionEvent event;
        event.error = PrecisionError::Overflow;
        event.description = "Overflow" + (context.isNotEmpty() ? " in " + context : "");
        event.value = value;
        event.severity = 9.0;
        events.push_back(event);
        recordEvent(event);
        statistics_.overflows++;
    }

    // Check for underflow
    else if (isUnderflow(value)) {
        PrecisionEvent event;
        event.error = PrecisionError::Underflow;
        event.description = "Underflow" + (context.isNotEmpty() ? " in " + context : "");
        event.value = value;
        event.severity = 3.0;
        events.push_back(event);
        recordEvent(event);
        statistics_.underflows++;
    }

    return events;
}

//==============================================================================
bool DSPPrecisionManager::isOverflow(float value) {
    // Check if value exceeds maximum representable finite float
    return std::abs(value) > std::numeric_limits<float>::max() * 0.99f;
}

bool DSPPrecisionManager::isOverflow(double value) {
    return std::abs(value) > std::numeric_limits<double>::max() * 0.99;
}

//==============================================================================
bool DSPPrecisionManager::isUnderflow(float value) {
    // Check if value is below minimum positive normal
    return std::abs(value) < std::numeric_limits<float>::min() && value != 0.0f;
}

bool DSPPrecisionManager::isUnderflow(double value) {
    return std::abs(value) < std::numeric_limits<double>::min() && value != 0.0;
}

//==============================================================================
float DSPPrecisionManager::clamp(float value) {
    // Clamp to safe range
    constexpr float maxFloat = std::numeric_limits<float>::max();
    constexpr float minFloat = std::numeric_limits<float>::lowest();

    if (value > maxFloat) return maxFloat;
    if (value < minFloat) return minFloat;
    if (std::isnan(value)) return 0.0f;

    return value;
}

double DSPPrecisionManager::clamp(double value) {
    constexpr double maxDouble = std::numeric_limits<double>::max();
    constexpr double minDouble = std::numeric_limits<double>::lowest();

    if (value > maxDouble) return maxDouble;
    if (value < minDouble) return minDouble;
    if (std::isnan(value)) return 0.0;

    return value;
}

//==============================================================================
void DSPPrecisionManager::resetStatistics() {
    statistics_.overflows.store(0);
    statistics_.underflows.store(0);
    statistics_.nansDetected.store(0);
    statistics_.infinitiesDetected.store(0);
    statistics_.precisionLosses.store(0);
    eventHistory_.clear();

    std::cout << "DSPPrecisionManager: Statistics reset" << std::endl;
}

//==============================================================================
// Private Methods
//==============================================================================

void DSPPrecisionManager::recordEvent(const PrecisionEvent& event) {
    eventHistory_.push_back(event);

    // Limit history size
    if (static_cast<int>(eventHistory_.size()) > maxHistorySize) {
        eventHistory_.erase(eventHistory_.begin());
    }

    // Log high-severity events
    if (event.severity >= 8.0) {
        std::cerr << "DSPPrecisionManager: " << event.toString() << std::endl;
    }
}

} // namespace zenith
