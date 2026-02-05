/**
 * @file AudioBufferConverterTest.cpp
 * @brief Unit tests for AudioBufferConverter utility
 */

#include "../../../../tools/agents/cpp/TransportProtocolAgent/TransportProtocolAgent.h"
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>

namespace zenith {
namespace tests {

class AudioBufferConverterTest : public juce::UnitTest {
public:
    AudioBufferConverterTest() : juce::UnitTest("AudioBufferConverter", "AudioUtility") {}

    void runTest() override {
        using namespace zenith::agents;

        beginTest("ConvertInt16ToPlanarFloat");
        {
            const int numSamples = 4;
            const int numChannels = 2;
            std::vector<int16_t> sourceData(numSamples * numChannels);

            // Fill L with positive, R with negative
            for (int i = 0; i < numSamples; ++i) {
                sourceData[i * 2] = 16384;      // ~0.5
                sourceData[i * 2 + 1] = -16384; // ~-0.5
            }

            juce::AudioBuffer<float> destBuffer(numChannels, numSamples);
            AudioBufferConverter::convertToPlanarFloat(sourceData.data(), destBuffer, numSamples, numChannels, BitDepth::Int16);

            // Verify L channel (should be ~0.5)
            for (int i = 0; i < numSamples; ++i) {
                expectWithin(destBuffer.getSample(0, i), 0.5f, 0.001f);
            }
            // Verify R channel (should be ~-0.5)
            for (int i = 0; i < numSamples; ++i) {
                expectWithin(destBuffer.getSample(1, i), -0.5f, 0.001f);
            }
        }

        beginTest("ConvertPlanarFloatToInt24");
        {
            const int numSamples = 4;
            const int numChannels = 2;

            juce::AudioBuffer<float> sourceBuffer(numChannels, numSamples);
            // Fill L with 1.0, R with -1.0
            for (int i = 0; i < numSamples; ++i) {
                sourceBuffer.setSample(0, i, 1.0f);
                sourceBuffer.setSample(1, i, -1.0f);
            }

            // 3 bytes per sample * 2 channels * numSamples
            std::vector<uint8_t> destData(numSamples * numChannels * 3);

            AudioBufferConverter::convertFromPlanarFloat(sourceBuffer, destData.data(), numSamples, numChannels, BitDepth::Int24);

            // Verify first sample L (max positive 24-bit: 0x7FFFFF)
            // Little Endian: FF FF 7F
            expectEquals((int)destData[0], 0xFF);
            expectEquals((int)destData[1], 0xFF);
            expectEquals((int)destData[2], 0x7F);

            // Verify first sample R (max negative 24-bit: 0x800000)
            // Little Endian: 00 00 80
            expectEquals((int)destData[3], 0x00);
            expectEquals((int)destData[4], 0x00);
            expectEquals((int)destData[5], 0x80);
        }
    }

    // Helper since juce::UnitTest doesn't have expectNear
    void expectWithin(float actual, float expected, float tolerance) {
        expect(std::abs(actual - expected) <= tolerance,
            "Expected " + juce::String(expected) + ", got " + juce::String(actual));
    }
};

static AudioBufferConverterTest audioBufferConverterTest;

} // namespace tests
} // namespace zenith
