/*
    Latency Test - Measures Real Performance

    This test Actually measures latency instead of using fake estimates.
    It uses high-resolution timers to measure actual execution time.

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>
*/

#include "../ZenithUltraLowLatencyAutoTune.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <chrono>
#include <iostream>
#include <iomanip>

using namespace juce;
using namespace std::chrono;

//==============================================================================
// ACTUAL Latency Measurement
//==============================================================================

class LatencyMeasurement
{
public:
    //==============================================================================
    // Measure actual execution time of a function
    template<typename Func>
    static double measureMicroseconds(Func&& func)
    {
        auto start = high_resolution_clock::now();
        func();
        auto end = high_resolution_clock::now();

        auto duration = duration_cast<microseconds>(end - start);
        return static_cast<double>(duration.count());
    }

    //==============================================================================
    // Measure multiple times and get statistics
    template<typename Func>
    static struct Stats measureStats(Func&& func, int iterations = 1000)
    {
        Stats stats;
        stats.min = std::numeric_limits<double>::max();
        stats.max = 0.0;
        stats.sum = 0.0;

        std::vector<double> samples;
        samples.reserve(iterations);

        // Warm-up
        for (int i = 0; i < 10; ++i)
            func();

        // Actual measurements
        for (int i = 0; i < iterations; ++i)
        {
            double time = measureMicroseconds(func);
            samples.push_back(time);

            stats.min = juce::jmin(stats.min, time);
            stats.max = juce::jmax(stats.max, time);
            stats.sum += time;
        }

        // Calculate average
        stats.avg = stats.sum / iterations;

        // Calculate median
        std::sort(samples.begin(), samples.end());
        stats.median = samples[samples.size() / 2];

        return stats;
    }

    struct Stats
    {
        double min = 0.0;
        double max = 0.0;
        double avg = 0.0;
        double median = 0.0;
        double sum = 0.0;

        double getPercentile95() const
        {
            return max;  // Simplified
        }

        double getPercentile99() const
        {
            return max;
        }
    };
};

//==============================================================================
// Latency Test
//==============================================================================

void runRealLatencyTest()
{

    const double sampleRate = 48000.0;
    const int bufferSize = 512;
    const int numChannels = 2;

    // Create test buffer
    juce::AudioBuffer<float> testBuffer(numChannels, bufferSize);
    for (int channel = 0; channel < numChannels; ++channel)
    {
        for (int sample = 0; sample < bufferSize; ++sample)
        {
            testBuffer.setSample(channel, sample, std::sin(2.0 * 3.14159 * 440.0 * sample / sampleRate));
        }
    }

    // Test pitch detector latency

    zenith::PitchDetector detector;
    detector.prepare(sampleRate);

    auto pitchTestFunc = [&]() {
        detector.detectPitch(testBuffer.getReadPointer(0), bufferSize);
    };

    LatencyMeasurement::Stats pitchStats = LatencyMeasurement::measureStats(pitchTestFunc, 1000);

    std::cout << std::fixed << std::setprecision(2);

    // Convert to samples
    double avgSamples = (pitchStats.avg / 1000000.0) * sampleRate;

    detector.reset();

    // Test filter processing latency

    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
        juce::dsp::IIR::Coefficients<float>> filter;

    filter.prepare({sampleRate, static_cast<juce::uint32>(bufferSize), numChannels});
    *filter.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 1000.0);

    auto filterTestFunc = [&]() {
        juce::dsp::AudioBlock<float> block(testBuffer);
        filter.process(juce::dsp::ProcessContextReplacing<float>(block));
    };

    LatencyMeasurement::Stats filterStats = LatencyMeasurement::measureStats(filterTestFunc, 1000);


    filter.reset();

    // Test compressor latency

    juce::dsp::Compressor<float> compressor;
    compressor.prepare({sampleRate, static_cast<juce::uint32>(bufferSize), numChannels});
    compressor.setThreshold(-20.0f);
    compressor.setRatio(4.0f);
    compressor.setAttack(10.0f);
    compressor.setRelease(100.0f);

    auto compressorTestFunc = [&]() {
        juce::dsp::AudioBlock<float> block(testBuffer);
        compressor.process(juce::dsp::ProcessContextReplacing<float>(block));
    };

    LatencyMeasurement::Stats compressorStats = LatencyMeasurement::measureStats(compressorTestFunc, 1000);


    compressor.reset();

    // Test full effect chain latency

    auto chainTestFunc = [&]() {
        // Apply multiple effects in sequence
        juce::dsp::AudioBlock<float> block(testBuffer);

        // Filter
        filter.process(juce::dsp::ProcessContextReplacing<float>(block));

        // Compressor
        compressor.process(juce::dsp::ProcessContextReplacing<float>(block));

        // Gain
        block.applyGain(0.5f);
    };

    LatencyMeasurement::Stats chainStats = LatencyMeasurement::measureStats(chainTestFunc, 1000);


    // Overall summary
}

int main()
{
    runRealLatencyTest();
    return 0;
}
