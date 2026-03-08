/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0
*/

#include "SampleRateConverter.h"
#include <juce_core/juce_core.h>
#include <cmath>
#include <numeric>

namespace zenith {
namespace audio {

//==============================================================================
// Windowed sinc function for resampling
namespace {
    double sinc(double x)
    {
        if (std::abs(x) < 1e-10)
            return 1.0;

        x *= juce::MathConstants<double>::pi;
        return std::sin(x) / x;
    }

    // Kaiser window for better frequency response
    double kaiserWindow(double x, double beta)
    {
        double a = juce::dsp::SpecialFunctions::besselI0(beta * std::sqrt(1.0 - x * x));
        double b = juce::dsp::SpecialFunctions::besselI0(beta);
        return a / b;
    }
}

//==============================================================================
SampleRateConverter::SampleRateConverter()
{
    // Pre-build sinc kernels for common quality settings
    buildSincKernel(0.45);  // Default cutoff
}

SampleRateConverter::~SampleRateConverter() = default;

//==============================================================================
bool SampleRateConverter::convert(const juce::AudioBuffer<float>& input,
                                  juce::AudioBuffer<float>& output,
                                  double inputSampleRate,
                                  double outputSampleRate,
                                  Quality quality)
{
    if (inputSampleRate <= 0 || outputSampleRate <= 0)
        return false;

    if (input.getNumChannels() == 0)
        return false;

    double ratio = outputSampleRate / inputSampleRate;

    // Calculate output size
    auto inputSamples = input.getNumSamples();
    auto outputSamples = static_cast<int>(std::ceil(inputSamples * ratio));

    output.setSize(input.getNumChannels(), outputSamples, false, true, false);

    // Choose conversion algorithm based on quality
    switch (quality)
    {
        case Quality::Fast:
            // No quality, just use linear (fastest)
            return convertLinear(input, output, ratio);

        case Quality::Linear:
            return convertLinear(input, output, ratio);

        case Quality::Good:
        case Quality::Best:
        case Quality::Sinc:
            return convertSinc(input, output, ratio);
    }

    return false;
}

void SampleRateConverter::setQuality(Quality quality)
{
    currentQuality = quality;
    kernelSize = (quality == Quality::Best) ? 64 : 32;
    buildSincKernel(0.45);
}

double SampleRateConverter::getLatency() const
{
    return static_cast<double>(kernelSize / 2);
}

//==============================================================================
bool SampleRateConverter::convertLinear(const juce::AudioBuffer<float>& input,
                                        juce::AudioBuffer<float>& output,
                                        double ratio)
{
    auto numChannels = input.getNumChannels();
    auto inputSamples = input.getNumSamples();
    auto outputSamples = output.getNumSamples();

    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* inputPtr = input.getReadPointer(channel);
        auto* outputPtr = output.getWritePointer(channel);

        for (int i = 0; i < outputSamples; ++i)
        {
            double sourcePos = i / ratio;
            int index0 = static_cast<int>(std::floor(sourcePos));
            int index1 = juce::jmin(index0 + 1, inputSamples - 1);
            double frac = sourcePos - index0;

            // Linear interpolation
            float sample0 = inputPtr[index0];
            float sample1 = inputPtr[index1];
            outputPtr[i] = static_cast<float>(sample0 + frac * (sample1 - sample0));
        }
    }

    return true;
}

bool SampleRateConverter::convertSinc(const juce::AudioBuffer<float>& input,
                                      juce::AudioBuffer<float>& output,
                                      double ratio)
{
    if (sincKernel.empty())
        buildSincKernel(0.45);

    auto numChannels = input.getNumChannels();
    auto inputSamples = input.getNumSamples();
    auto outputSamples = output.getNumSamples();

    // Phase parameter for polyphase filtering
    double phase = 0.0;
    double phaseIncrement = 1.0 / ratio;

    int halfKernel = kernelSize / 2;

    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* inputPtr = input.getReadPointer(channel);
        auto* outputPtr = output.getWritePointer(channel);
        phase = 0.0;

        for (int i = 0; i < outputSamples; ++i)
        {
            int centerIndex = static_cast<int>(std::floor(phase));
            double frac = phase - centerIndex;

            // Convolve with sinc kernel
            double sum = 0.0;

            for (int k = -halfKernel; k <= halfKernel; ++k)
            {
                int srcIndex = centerIndex + k;

                // Handle boundaries
                if (srcIndex < 0)
                    srcIndex = -srcIndex;  // Mirror
                else if (srcIndex >= inputSamples)
                    srcIndex = 2 * inputSamples - srcIndex - 2;  // Mirror

                if (srcIndex >= 0 && srcIndex < inputSamples)
                {
                    // Get windowed sinc coefficient
                    double kernelIndex = static_cast<double>(k) - frac;
                    int kernelPos = static_cast<int>(std::abs(kernelIndex));

                    if (kernelPos < halfKernel)
                    {
                        // Linear interpolation between kernel samples
                        int k0 = kernelPos;
                        int k1 = juce::jmin(kernelPos + 1, halfKernel);
                        double kFrac = std::abs(kernelIndex) - kernelPos;

                        double weight = (1.0 - kFrac) * sincKernel[k0] + kFrac * sincKernel[k1];
                        sum += inputPtr[srcIndex] * weight;
                    }
                }
            }

            outputPtr[i] = static_cast<float>(sum);
            phase += phaseIncrement;
        }
    }

    return true;
}

void SampleRateConverter::buildSincKernel(double cutoff)
{
    int halfKernel = kernelSize / 2;
    sincKernel.resize(halfKernel + 1);

    // Build Kaiser-windowed sinc kernel
    double beta = 5.0;  // Beta parameter for Kaiser window (tradeoff between main lobe width and side lobe attenuation)

    for (int i = 0; i <= halfKernel; ++i)
    {
        double x = static_cast<double>(i);
        double sincValue = sinc(2.0 * cutoff * x);

        // Apply Kaiser window
        double window = kaiserWindow(x / halfKernel, beta);

        sincKernel[i] = static_cast<float>(sincValue * window);
    }

    // Normalize kernel
    double sum = std::accumulate(sincKernel.begin(), sincKernel.end(), 0.0);
    if (sum > 0)
    {
        for (auto& sample : sincKernel)
            sample /= static_cast<float>(sum);
    }
}

} // namespace audio
} // namespace zenith
