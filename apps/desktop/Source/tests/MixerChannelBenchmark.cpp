/*
  ==============================================================================

    MixerChannelBenchmark.cpp
    Created: 2025
    Author:  Zero-Latency Agent

  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "../engine/MixerChannel.h"
#include <chrono>
#include <iostream>

namespace zenith {
namespace tests {

class MixerChannelBenchmark : public juce::UnitTest {
public:
    MixerChannelBenchmark() : juce::UnitTest("MixerChannelPerformance", "Benchmarks") {}

    void runTest() override {
        beginTest("Compressor Benchmark");

        MixerChannel channel;
         double sampleRate = 48000.0;
        int samplesPerBlock = 512;

        channel.prepareToPlay(samplesPerBlock, sampleRate);
        
        // Enable Compressor
        channel.setCompressorEnabled(true);
        channel.setCompressorThreshold(-20.0f);
        channel.setCompressorRatio(4.0f);
        channel.setCompressorAttack(10.0f);
        channel.setCompressorRelease(100.0f);
        channel.setCompressorMakeup(0.0f);

        // Create buffer with random noise to simulate real signal
        juce::AudioBuffer<float> buffer(2, samplesPerBlock);
        juce::Random rng;
        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < samplesPerBlock; ++i) {
                buffer.setSample(ch, i, rng.nextFloat() * 2.0f - 1.0f);
            }
        }

        juce::AudioSourceChannelInfo bufferInfo(&buffer, 0, samplesPerBlock);
        
        // Warmup
        for (int i = 0; i < 100; ++i) {
            channel.getNextAudioBlock(bufferInfo);
        }

        const int numIterations = 20000; // ~3.5 minutes of audio processing
        
        auto startTime = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < numIterations; ++i) {
            // Re-fill buffer to avoid denormals (though random noise shouldn't have them)
            // Actually, let's just let it run, chaos is fine for perf test
            channel.getNextAudioBlock(bufferInfo);
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();

        double totalSeconds = duration / 1000000.0;
        double audioSeconds = (double)numIterations * samplesPerBlock / sampleRate;
        double speedup = audioSeconds / totalSeconds;

        std::cout << "\n[BENCHMARK] MixerChannel (Compressor ON): " << std::endl;
        std::cout << "  Iterations: " << numIterations << std::endl;
        std::cout << "  Total Time: " << duration << " us" << std::endl;
        std::cout << "  Time per block: " << (duration / (double)numIterations) << " us" << std::endl;
        std::cout << "  Real-time factor: " << speedup << "x" << std::endl;

        expect(speedup > 50.0, "Performance is below expected baseline (should be > 50x real-time)");
    }
};

static MixerChannelBenchmark mixerChannelBenchmark;

} // namespace tests
} // namespace zenith
