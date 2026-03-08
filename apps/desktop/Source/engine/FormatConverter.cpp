/*
  ==============================================================================

    FormatConverter.cpp
    Implementation of safe audio format conversion

  ==============================================================================
*/

#include "FormatConverter.h"
#include <iostream>
#include <algorithm>
#include <cmath>

namespace zenith {

//==============================================================================
// FormatConverter Implementation
//==============================================================================

FormatConverter::FormatConverter() {
    std::cout << "FormatConverter: Initialized" << std::endl;
}

FormatConverter::~FormatConverter() {
    std::cout << "FormatConverter: Shut down ("
              << statistics_.totalConversions << " conversions, "
              << statistics_.clampedValues << " clamped)" << std::endl;
}

//==============================================================================
juce::AudioBuffer<float>* FormatConverter::convertBuffer(
    const juce::AudioBuffer<float>& inputBuffer,
    const AudioFormatSpec& inputFormat,
    const AudioFormatSpec& outputFormat,
    const FormatConverterConfig& config)
{
    // Validate formats
    auto& validator = AudioFormatValidatorHolder::getInstance();
    auto inputIssues = validator.validateFormat(inputFormat);
    auto outputIssues = validator.validateFormat(outputFormat);

    if (!inputIssues.empty() || !outputIssues.empty()) {
        std::cerr << "FormatConverter: Invalid format specified" << std::endl;
        return nullptr;
    }

    // Create output buffer (same size as input for format-only conversion)
    auto* output = new juce::AudioBuffer<float>(inputBuffer);
    output->clear();

    // Convert bit depth if different
    if (inputFormat.bitDepth != outputFormat.bitDepth) {
        convertBitDepth(*output, inputFormat.bitDepth, outputFormat.bitDepth, config);
    }

    // Clamp if enabled
    if (config.enableClamping) {
        clampBuffer(*output, config.clampThreshold);
    }

    // Update statistics
    statistics_.totalConversions++;
    statistics_.samplesConverted += inputBuffer.getNumSamples() * inputBuffer.getNumChannels();

    return output;
}

//==============================================================================
void FormatConverter::convertBitDepth(
    juce::AudioBuffer<float>& buffer,
    AudioBitDepth fromBitDepth,
    AudioBitDepth toBitDepth,
    const FormatConverterConfig& config)
{
    const int channels = buffer.getNumChannels();
    const int samples = buffer.getNumSamples();

    for (int channel = 0; channel < channels; ++channel) {
        float* data = buffer.getWritePointer(channel);

        for (int i = 0; i < samples; ++i) {
            data[i] = convertSample(data[i], fromBitDepth, toBitDepth, config);
        }
    }
}

//==============================================================================
int FormatConverter::clampBuffer(
    juce::AudioBuffer<float>& buffer,
    double threshold)
{
    const int channels = buffer.getNumChannels();
    const int samples = buffer.getNumSamples();
    int clampedCount = 0;

    for (int channel = 0; channel < channels; ++channel) {
        float* data = buffer.getWritePointer(channel);

        for (int i = 0; i < samples; ++i) {
            if (data[i] > threshold) {
                data[i] = static_cast<float>(threshold);
                clampedCount++;
                statistics_.clampedValues++;
            } else if (data[i] < -threshold) {
                data[i] = static_cast<float>(-threshold);
                clampedCount++;
                statistics_.clampedValues++;
            }
        }
    }

    return clampedCount;
}

//==============================================================================
std::pair<int, int> FormatConverter::detectOverflowUnderflow(
    const juce::AudioBuffer<float>& buffer,
    double threshold) const
{
    int overflows = 0;
    int underflows = 0;

    const int channels = buffer.getNumChannels();
    const int samples = buffer.getNumSamples();

    for (int channel = 0; channel < channels; ++channel) {
        const float* data = buffer.getReadPointer(channel);

        for (int i = 0; i < samples; ++i) {
            if (data[i] > threshold) {
                overflows++;
            } else if (data[i] < -threshold) {
                underflows++;
            }
        }
    }

    return {overflows, underflows};
}

//==============================================================================
void FormatConverter::resetStatistics() {
    statistics_.totalConversions = 0;
    statistics_.samplesConverted = 0;
    statistics_.clampedValues = 0;
    statistics_.overflows = 0;
    statistics_.underflows = 0;

    std::cout << "FormatConverter: Statistics reset" << std::endl;
}

//==============================================================================
// Private Methods
//==============================================================================

float FormatConverter::convertSample(
    float value,
    AudioBitDepth fromDepth,
    AudioBitDepth toDepth,
    const FormatConverterConfig& config)
{
    // For now, just return the value (all formats use float32 internally)
    // In a more complex implementation, this would do actual bit depth conversion

    // Clamp if enabled
    if (config.enableClamping) {
        if (value > config.clampThreshold) {
            statistics_.clampedValues++;
            return static_cast<float>(config.clampThreshold);
        } else if (value < -config.clampThreshold) {
            statistics_.clampedValues++;
            return static_cast<float>(-config.clampThreshold);
        }
    }

    // Detect overflow/underflow
    if (std::abs(value) > config.clampThreshold) {
        if (value > 0) {
            statistics_.overflows++;
        } else {
            statistics_.underflows++;
        }
    }

    return value;
}

} // namespace zenith
