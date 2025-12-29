/*
  ==============================================================================

    RealTimeAudioBufferTests.cpp
    QA & Verification Sentinel - Heartbeat Suite

  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include "../audio/RealTimeAudioBuffer.h"
#include <thread>
#include <vector>
#include <cmath>

class RealTimeAudioBufferTests : public juce::UnitTest
{
public:
    RealTimeAudioBufferTests() : juce::UnitTest("RealTimeAudioBuffer", "Heartbeat") {}

    void runTest() override
    {
        testConcurrentReadWrite();
        testSampleRateMismatch();
        testClippingDetection();
    }

private:
    void testConcurrentReadWrite()
    {
        beginTest("Concurrent Read/Write - 10 Minutes Stress (Simulated)");
        
        // Note: 10 minutes is too long for a single unit test run in CI, 
        // but we can simulate the load with high frequency and shorter duration (e.g., 5 seconds) 
        // to verify stability.
        
        const int numChannels = 2;
        const int ringBufferSize = 65536;
        const int processBlockSize = 512;
        zenith::audio::RealTimeAudioBuffer rtBuffer(numChannels, ringBufferSize);
        
        std::atomic<bool> shouldStop{false};
        std::atomic<int64_t> samplesWritten{0};
        std::atomic<int64_t> samplesRead{0};
        
        // Writer thread (Sine wave)
        std::thread writer([&]() {
            juce::AudioBuffer<float> writeBuffer(numChannels, processBlockSize);
            float phase = 0.0f;
            float phaseDelta = 2.0f * juce::MathConstants<float>::pi * 440.0f / 44100.0f;
            
            while (!shouldStop.load()) {
                for (int ch = 0; ch < numChannels; ++ch) {
                    float* data = writeBuffer.getWritePointer(ch);
                    for (int s = 0; s < processBlockSize; ++s) {
                        data[s] = std::sin(phase);
                        phase += phaseDelta;
                    }
                }
                
                if (rtBuffer.writeAudio(writeBuffer)) {
                    samplesWritten += processBlockSize;
                }
                
                // Sleep to simulate 44.1kHz rate roughly
                std::this_thread::sleep_for(std::chrono::microseconds(10000)); 
            }
        });
        
        // Reader thread
        std::thread reader([&]() {
            juce::AudioBuffer<float> readBuffer(numChannels, processBlockSize);
            
            while (!shouldStop.load()) {
                if (rtBuffer.readAudio(readBuffer)) {
                    samplesRead += processBlockSize;
                    
                    // Basic verification: data should not be silent if written
                    for (int ch = 0; ch < numChannels; ++ch) {
                        expect(readBuffer.getRMSLevel(ch, 0, processBlockSize) > 0.0f, "Silent buffer read during sine wave test");
                    }
                }
                
                std::this_thread::sleep_for(std::chrono::microseconds(10000));
            }
        });
        
        // Run for 5 seconds
        std::this_thread::sleep_for(std::chrono::seconds(5));
        shouldStop.store(true);
        
        if (writer.joinable()) writer.join();
        if (reader.joinable()) reader.join();
        
        expect(rtBuffer.getDropouts() == 0, "Dropouts detected during concurrent R/W");
        logMessage("Samples Written: " + juce::String(samplesWritten.load()));
        logMessage("Samples Read: " + juce::String(samplesRead.load()));
    }

    void testSampleRateMismatch()
    {
        beginTest("Sample Rate Mismatch (Epsilon Resampler)");
        
        zenith::audio::SampleRateConverter converter;
        juce::AudioBuffer<float> input(1, 1024);
        juce::AudioBuffer<float> output;
        
        // Fill input with 1kHz sine
        float phase = 0.0f;
        float phaseDelta = 2.0f * juce::MathConstants<float>::pi * 1000.0f / 48000.0f;
        for (int i = 0; i < 1024; ++i) {
            input.setSample(0, i, std::sin(phase));
            phase += phaseDelta;
        }
        
        // Convert 48k to 44.1k
        bool success = converter.convert(input, output, 48000.0, 44100.0, zenith::audio::SampleRateConverter::Quality::Sinc);
        
        expect(success, "Conversion failed");
        expect(output.getNumSamples() > 0, "Output buffer empty");
        
        // Verify output size (should be floor(1024 * 44100 / 48000))
        int expectedSamples = static_cast<int>(1024.0 * 44100.0 / 48000.0);
        expectWithin(output.getNumSamples(), expectedSamples, 2);
        
        // Check for NaNs
        for (int i = 0; i < output.getNumSamples(); ++i) {
            expect(!std::isnan(output.getSample(0, i)), "NaN detected in resampled output");
        }
    }

    void testClippingDetection()
    {
        beginTest("Clipping Detection (Atomic)");
        
        zenith::audio::RealTimeAudioBuffer rtBuffer(1, 1024);
        juce::AudioBuffer<float> buffer(1, 128);
        
        // Test normal signal
        buffer.clear();
        for (int i = 0; i < 128; ++i) buffer.setSample(0, i, 0.5f);
        rtBuffer.writeAudio(buffer);
        expect(!rtBuffer.isClipping(0), "False positive clipping detected");
        
        // Test clipping signal
        for (int i = 0; i < 128; ++i) buffer.setSample(0, i, 1.2f);
        rtBuffer.writeAudio(buffer);
        expect(rtBuffer.isClipping(0), "Clipping not detected at +1.2dB");
        
        // Test reset
        rtBuffer.resetLevels();
        expect(!rtBuffer.isClipping(0), "Clipping flag not reset");
    }
};

static RealTimeAudioBufferTests realTimeAudioBufferTests;
