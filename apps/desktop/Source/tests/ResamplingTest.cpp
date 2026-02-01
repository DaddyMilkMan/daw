/*
  ==============================================================================

    ResamplingTest.cpp
    Created: 2025-12-09
    Author:  Zenith DAW

    Unit tests for Windowed Sinc Interpolation.

  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "../dsp/Resampling.h"
#include <random>
#include <iostream>

class ResamplingTest : public juce::UnitTest {
public:
    ResamplingTest() : juce::UnitTest("ResamplingTest") {}

    void runTest() override {
        beginTest("Impulse Response");
        {
            zenith::dsp::WindowedSincInterpolator resampler;
            juce::AudioBuffer<float> source(1, 64);
            juce::AudioBuffer<float> dest(1, 64);
            source.clear();
            dest.clear();

            // Impulse at index 10
            source.setSample(0, 10, 1.0f);

            double sourcePos = 0.0;
            // 1:1 ratio
            resampler.process(source, dest, 0, 0, 0, sourcePos, 32, 1.0);

            expectWithinAbsoluteError(dest.getSample(0, 10), 1.0f, 0.01f);
            expectWithinAbsoluteError(dest.getSample(0, 5), 0.0f, 0.1f); // Sinc ring
        }

        beginTest("Resampling Ratio 0.5 (Downpitch)");
        {
            zenith::dsp::WindowedSincInterpolator resampler;
            juce::AudioBuffer<float> source(1, 100);
            juce::AudioBuffer<float> dest(1, 200);
            source.clear();
            dest.clear();

            // Fill source with DC 1.0
            for (int i = 0; i < 100; ++i) source.setSample(0, i, 1.0f);

            double sourcePos = 10.0; // Start away from edge to avoid boundary effects

            resampler.process(source, dest, 0, 0, 0, sourcePos, 50, 0.5);

            // Should stay 1.0 (DC gain 1.0)
            float midSample = dest.getSample(0, 25);
            expectWithinAbsoluteError(midSample, 1.0f, 0.05f);

            // sourcePos should have advanced by 50 * 0.5 = 25
            expectWithinAbsoluteError(sourcePos, 10.0 + 25.0, 0.0001);
        }

        beginTest("Looping Handling");
        {
            zenith::dsp::WindowedSincInterpolator resampler;
            juce::AudioBuffer<float> source(1, 20); // Longer buffer
            juce::AudioBuffer<float> dest(1, 20);
            source.clear();

            // Single cycle sine wave (continuous loop)
            for (int i=0; i<20; ++i) {
                float val = std::sin(2.0f * juce::MathConstants<float>::pi * i / 20.0f);
                source.setSample(0, i, val);
            }

            double sourcePos = 19.5; // Near end

            // Should wrap around smoothly
            resampler.processLooping(source, dest, 0, 0, 0, sourcePos, 4, 1.0, 1.0f);

            float val0 = dest.getSample(0, 0); // 19.5 -> sin(19.5/20 * 2pi) = sin(1.95pi) = sin(-0.05pi) approx -0.156
            float val1 = dest.getSample(0, 1); // 0.5  -> sin(0.5/20 * 2pi) = sin(0.05pi) approx 0.156

            float expected0 = std::sin(2.0f * juce::MathConstants<float>::pi * 19.5f / 20.0f);
            float expected1 = std::sin(2.0f * juce::MathConstants<float>::pi * 0.5f / 20.0f);

            std::cout << "Looping Test: Val0=" << val0 << " (Exp " << expected0 << "), Val1=" << val1 << " (Exp " << expected1 << ")" << std::endl;

            expectWithinAbsoluteError(val0, expected0, 0.01f);
            expectWithinAbsoluteError(val1, expected1, 0.01f);
        }
    }
};

static ResamplingTest resamplingTest;
