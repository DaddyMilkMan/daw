/*
  ==============================================================================

    SampleRateConverter.cpp
    Implementation of safe sample rate conversion

  ==============================================================================
*/

#include "SampleRateConverter.h"
#include <iostream>
#include <algorithm>
#include <cmath>

namespace zenith {

//==============================================================================
// SampleRateConverter Implementation
//==============================================================================

SampleRateConverter::SampleRateConverter() {
    std::cout << "SampleRateConverter: Initialized" << std::endl;
}

SampleRateConverter::~SampleRateConverter() {
    std::cout << "SampleRateConverter: Shut down ("
              << statistics_.totalConversions << " conversions, "
              << statistics_.artifactsDetected << " artifacts)" << std::endl;
}

//==============================================================================
juce::AudioBuffer<float>* SampleRateConverter::convertBuffer(
    const juce::AudioBuffer<float>& inputBuffer,
    double inputSampleRate,
    double outputSampleRate,
    const SRCConfig& config)
{
    // Validate inputs
    if (inputBuffer.getNumSamples() == 0) {
        std::cerr << "SampleRateConverter: Empty input buffer" << std::endl;
        return nullptr;
    }

    if (!isSupportedRatio(getRatio(inputSampleRate, outputSampleRate))) {
        std::cerr << "SampleRateConverter: Unsupported ratio: "
                  << getRatio(inputSampleRate, outputSampleRate) << std::endl;
        return nullptr;
    }

    // Check if conversion needed
    if (!needsConversion(inputSampleRate, outputSampleRate)) {
        // No conversion needed, return copy
        auto* output = new juce::AudioBuffer<float>(inputBuffer);
        return output;
    }

    double ratio = getRatio(inputSampleRate, outputSampleRate);

    // Determine quality
    SRCQuality quality = config.quality;
    if (quality == SRCQuality::Automatic) {
        quality = getRecommendedQuality(ratio);
    }

    // Perform conversion
    auto* output = performConversion(inputBuffer, ratio, config);

    if (output != nullptr && config.enableArtifactDetection) {
        detectArtifacts(*output, ratio, config);
    }

    // Update statistics
    statistics_.totalConversions++;
    statistics_.samplesConverted += inputBuffer.getNumSamples();
    statistics_.lastQuality = quality;

    return output;
}

//==============================================================================
bool SampleRateConverter::convertBufferInPlace(
    juce::AudioBuffer<float>& buffer,
    double inputSampleRate,
    double outputSampleRate,
    const SRCConfig& config)
{
    // For in-place conversion, we need to create temporary buffer
    auto* converted = convertBuffer(buffer, inputSampleRate, outputSampleRate, config);

    if (converted == nullptr) {
        return false;
    }

    // Copy back
    buffer = *converted;
    delete converted;

    return true;
}

//==============================================================================
int SampleRateConverter::calculateOutputSize(
    int inputSamples,
    double inputSampleRate,
    double outputSampleRate) const
{
    double ratio = getRatio(inputSampleRate, outputSampleRate);
    return static_cast<int>(std::ceil(inputSamples * ratio));
}

//==============================================================================
SRCQuality SampleRateConverter::getRecommendedQuality(double ratio) {
    // Upsampling or significant downsampling needs higher quality
    if (ratio > 2.0 || ratio < 0.5) {
        return SRCQuality::Best;
    } else if (ratio > 1.5 || ratio < 0.67) {
        return SRCQuality::High;
    } else if (ratio > 1.2 || ratio < 0.83) {
        return SRCQuality::Medium;
    } else {
        return SRCQuality::Low;
    }
}

//==============================================================================
void SampleRateConverter::resetStatistics() {
    statistics_.totalConversions = 0;
    statistics_.samplesConverted = 0;
    statistics_.artifactsDetected = 0;
    statistics_.averageProcessingTime = 0.0;
    artifacts_.clear();

    std::cout << "SampleRateConverter: Statistics reset" << std::endl;
}

//==============================================================================
// Private Methods
//==============================================================================

juce::AudioBuffer<float>* SampleRateConverter::performConversion(
    const juce::AudioBuffer<float>& input,
    double ratio,
    const SRCConfig& config)
{
    const int inputChannels = input.getNumChannels();
    const int inputSamples = input.getNumSamples();
    const int outputSamples = static_cast<int>(std::ceil(inputSamples * ratio));

    // Create output buffer
    auto* output = new juce::AudioBuffer<float>(inputChannels, outputSamples);
    output->clear();

    // Convert each channel
    for (int channel = 0; channel < inputChannels; ++channel) {
        const float* inputSamplesPtr = input.getReadPointer(channel);
        float* outputSamplesPtr = output->getWritePointer(channel);

        // Perform interpolation based on quality
        for (int outSample = 0; outSample < outputSamples; ++outSample) {
            double inPosition = outSample / ratio;

            float sample = 0.0f;

            switch (config.quality) {
                case SRCQuality::Fastest:
                case SRCQuality::Low:
                    sample = interpolateLinear(inputSamplesPtr, inputSamples, inPosition);
                    break;

                case SRCQuality::Medium:
                    sample = interpolateLagrange(inputSamplesPtr, inputSamples, inPosition, 4);
                    break;

                case SRCQuality::High:
                    sample = interpolateLagrange(inputSamplesPtr, inputSamples, inPosition, 8);
                    break;

                case SRCQuality::Best:
                    sample = interpolateLagrange(inputSamplesPtr, inputSamples, inPosition, 16);
                    break;

                case SRCQuality::Automatic:
                    // Should have been resolved before this
                    sample = interpolateLagrange(inputSamplesPtr, inputSamples, inPosition, 4);
                    break;
            }

            outputSamplesPtr[outSample] = sample;
        }
    }

    // Apply anti-aliasing filter if enabled
    if (config.enableFiltering && ratio < 1.0) {
        // Downsampling - need lowpass filter
        applyLowpassFilter(*output, config.filterCutoff * ratio);
    } else if (config.enableFiltering && ratio > 1.0) {
        // Upsampling - need filter to remove images
        applyLowpassFilter(*output, config.filterCutoff);
    }

    return output;
}

void SampleRateConverter::detectArtifacts(
    const juce::AudioBuffer<float>& output,
    double ratio,
    const SRCConfig& config)
{
    // Simple artifact detection
    // In production, this would use FFT-based analysis

    const int channels = output.getNumChannels();
    const int samples = output.getNumSamples();

    for (int channel = 0; channel < channels; ++channel) {
        const float* data = output.getReadPointer(channel);

        // Detect clipping (amplitude artifacts)
        for (int i = 1; i < samples - 1; ++i) {
            if (std::abs(data[i]) >= 1.0f) {
                SRCArtifactEvent artifact;
                artifact.type = SRCArtifactType::AmplitudeModulation;
                artifact.description = "Clipping detected";
                artifact.ratio = ratio;
                artifact.severity = std::abs(data[i]) - 1.0f;
                artifact.detectedAtSample = i;

                artifacts_.push_back(artifact);
                statistics_.artifactsDetected++;

                if (artifacts_.size() >= maxArtifacts) {
                    artifacts_.erase(artifacts_.begin());
                }
            }
        }

        // Detect sudden changes (phase distortion)
        for (int i = 1; i < samples; ++i) {
            float diff = std::abs(data[i] - data[i-1]);
            if (diff > 0.5f) {  // Sudden change threshold
                SRCArtifactEvent artifact;
                artifact.type = SRCArtifactType::PhaseDistortion;
                artifact.description = "Sudden phase change detected";
                artifact.ratio = ratio;
                artifact.severity = static_cast<double>(diff);
                artifact.detectedAtSample = i;

                artifacts_.push_back(artifact);
                statistics_.artifactsDetected++;

                if (artifacts_.size() >= maxArtifacts) {
                    artifacts_.erase(artifacts_.begin());
                }
            }
        }
    }
}

float SampleRateConverter::interpolateLinear(
    const float* samples,
    int size,
    double position) const
{
    int index1 = static_cast<int>(std::floor(position));
    int index2 = index1 + 1;
    double frac = position - index1;

    // Boundary handling
    if (index1 < 0) index1 = 0;
    if (index1 >= size) index1 = size - 1;
    if (index2 < 0) index2 = 0;
    if (index2 >= size) index2 = size - 1;

    // Linear interpolation
    return static_cast<float>((1.0 - frac) * samples[index1] + frac * samples[index2]);
}

float SampleRateConverter::interpolateLagrange(
    const float* samples,
    int size,
    double position,
    int order) const
{
    int center = static_cast<int>(std::round(position));
    int halfOrder = order / 2;

    float result = 0.0f;

    // Lagrange interpolation
    for (int i = 0; i < order; ++i) {
        int idx = center - halfOrder + i;

        // Boundary handling
        if (idx < 0) idx = 0;
        if (idx >= size) idx = size - 1;

        // Calculate Lagrange basis polynomial
        double numerator = 1.0;
        double denominator = 1.0;

        for (int j = 0; j < order; ++j) {
            if (i != j) {
                int jIdx = center - halfOrder + j;
                if (jIdx < 0) jIdx = 0;
                if (jIdx >= size) jIdx = size - 1;

                numerator *= (position - jIdx);
                denominator *= (idx - jIdx);
            }
        }

        result += samples[idx] * static_cast<float>(numerator / denominator);
    }

    return result;
}

void SampleRateConverter::applyLowpassFilter(
    juce::AudioBuffer<float>& buffer,
    double cutoffRatio) const
{
    // Simple IIR lowpass filter (first-order)
    // In production, would use higher quality filter

    const float feedback = static_cast<float>(std::exp(-2.0 * M_PI * cutoffRatio));
    const float feedforward = 1.0f - feedback;

    const int channels = buffer.getNumChannels();
    const int samples = buffer.getNumSamples();

    for (int channel = 0; channel < channels; ++channel) {
        float* data = buffer.getWritePointer(channel);
        float prev = 0.0f;

        for (int i = 0; i < samples; ++i) {
            data[i] = feedforward * data[i] + feedback * prev;
            prev = data[i];
        }
    }
}

} // namespace zenith
