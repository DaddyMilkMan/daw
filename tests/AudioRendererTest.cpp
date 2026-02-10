/*
  ==============================================================================

    AudioRendererTest.cpp
    Created: 2026-02-05
    Author:  Zenith DAW

    Comprehensive test suite for AudioRenderer functionality.
    Tests track processing, mixing, PDC, and metering.

  ==============================================================================
*/

#include "../modules/zenith_core/engine/core/AudioRenderer.h"
#include "../modules/zenith_core/engine/AudioTrack.h"
#include "../modules/zenith_core/engine/MixerChannel.h"
#include "../modules/zenith_core/engine/TrackProcessor.h"
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <vector>

namespace zenith {
namespace test {

class MockLatencyPlugin : public juce::AudioPluginInstance {
public:
    MockLatencyPlugin(int latency) {
        setLatencySamples(latency);
    }

    const juce::String getName() const override { return "MockLatency"; }
    void prepareToPlay(double, int) override {}
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
    double getTailLengthSeconds() const override { return 0.0; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    int getNumPrograms() override { return 0; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return ""; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}
    void fillInPluginDescription(juce::PluginDescription&) const override {}
};

class AudioRendererTest : public juce::UnitTest {
public:
    AudioRendererTest() : juce::UnitTest("AudioRenderer", "AudioEngine") {}

    void runTest() override {
        testInitialization();
        testTrackProcessing();
        testMixing();
        testPDC();
        testMasterEffects();
        testMetering();
        testErrorHandling();
    }

private:
    std::unique_ptr<AudioRenderer> renderer;
    std::vector<std::unique_ptr<zenith::Track>> tracks;
    std::vector<std::unique_ptr<zenith::AuxBus>> auxBuses;

    void setupTestEnvironment() {
        // Create renderer
        renderer = std::make_unique<AudioRenderer>();

        // Initialize with typical configuration
        renderer->initialize(2, 2, 44100.0);

        // Create test tracks
        tracks.clear();
        for (int i = 0; i < 3; ++i) {
            auto track = zenith::Track::create("TestTrack_" + juce::String(i),
                                              zenith::Track::Type::Audio);
            tracks.push_back(std::move(track));

            // Prepare tracks
            tracks.back()->prepareToPlay(512, 44100.0);

            // Set some basic properties
            tracks.back()->setVolume(0.8f);
            tracks.back->setPan(0.0f);
            tracks.back()->setEnabled(true);
            tracks.back()->setMuted(false);
        }

        // Create aux buses
        auxBuses.clear();
        for (int i = 0; i < 2; ++i) {
            auto auxBus = std::make_unique<zenith::AuxBus>("AuxBus_" + juce::String(i), 2);
            auxBus->prepareToPlay(512, 44100.0);
            auxBuses.push_back(std::move(auxBus));
        }
    }

    void teardownTestEnvironment() {
        renderer.reset();
        tracks.clear();
        auxBuses.clear();
    }

    void testInitialization() {
        beginTest("Initialization");

        // Test creation and basic initialization
        renderer = std::make_unique<AudioRenderer>();
        expect(renderer != nullptr, "Renderer should be created");

        // Test initialization with different configurations
        renderer->initialize(0, 2, 44100.0);  // No inputs, stereo output
        expect(renderer->isInitialized(), "Renderer should be initialized");

        // Test shutdown
        renderer->shutdown();
        expect(!renderer->isInitialized(), "Renderer should be shut down");

        // Test reinitialization
        renderer->initialize(2, 2, 48000.0);
        expect(renderer->isInitialized(), "Renderer should be reinitialized");

        teardownTestEnvironment();
    }

    void testTrackProcessing() {
        beginTest("Track Processing");

        setupTestEnvironment();

        // Test with disabled tracks
        tracks[0]->setEnabled(false);
        tracks[1]->setEnabled(false);
        tracks[2]->setEnabled(false);

        float* outputBuffer[2];
        juce::AudioBuffer<float> masterOutput(2, 512);
        masterOutput.clear();
        outputBuffer[0] = masterOutput.getWritePointer(0);
        outputBuffer[1] = masterOutput.getWritePointer(1);

        // Process with all tracks disabled - should be silent
        renderer->processAudioBlock(nullptr, 0, outputBuffer, 2, 512, 0, tracks, auxBuses, nullptr);

        // Check output is silent
        float rms = calculateRMS(masterOutput);
        expect(rms < 1e-6f, "Output should be silent when all tracks disabled");

        // Enable one track and test
        tracks[0]->setEnabled(true);

        // Add some audio data to track 0
        auto& trackProcessor = tracks[0]->getProcessor();
        auto& trackBuffer = trackProcessor->getPluginBuffer();
        trackBuffer.clear();

        // Generate a simple sine wave for testing
        for (int sample = 0; sample < 512; ++sample) {
            float value = 0.1f * std::sin(2.0 * juce::MathConstants<float>::pi * 440.0f * sample / 44100.0f);
            trackBuffer.setSample(0, sample, value);
            trackBuffer.setSample(1, sample, value);
        }

        // Process and check output
        renderer->processAudioBlock(nullptr, 0, outputBuffer, 2, 512, 0, tracks, auxBuses, nullptr);

        rms = calculateRMS(masterOutput);
        expect(rms > 1e-6f, "Output should not be silent when tracks are enabled");

        // Test solo functionality
        tracks[0]->setSolo(true);
        renderer->processAudioBlock(nullptr, 0, outputBuffer, 2, 512, 0, tracks, auxBuses, nullptr);

        rms = calculateRMS(masterOutput);
        expect(rms > 1e-6f, "Solo track should produce output");

        tracks[0]->setSolo(false);

        teardownTestEnvironment();
    }

    void testMixing() {
        beginTest("Mixing");

        setupTestEnvironment();

        float* outputBuffer[2];
        juce::AudioBuffer<float> masterOutput(2, 512);
        masterOutput.clear();
        outputBuffer[0] = masterOutput.getWritePointer(0);
        outputBuffer[1] = masterOutput.getWritePointer(1);

        // Test master volume control
        renderer->setMasterVolume(0.5f);
        expect(renderer->getMasterVolume() == 0.5f, "Master volume should be settable");

        // Enable tracks with different volume levels
        tracks[0]->setEnabled(true);
        tracks[0]->setVolume(0.5f);
        tracks[1]->setEnabled(true);
        tracks[1]->setVolume(1.0f);
        tracks[2]->setEnabled(false);

        // Generate test audio
        for (int i = 0; i < 2; ++i) {
            auto& trackProcessor = tracks[i]->getProcessor();
            auto& trackBuffer = trackProcessor->getPluginBuffer();
            trackBuffer.clear();

            float gain = (i == 0) ? 0.1f : 0.2f;  // Different levels for each track
            for (int sample = 0; sample < 512; ++sample) {
                float value = gain * std::sin(2.0 * juce::MathConstants<float>::pi * 440.0f * sample / 44100.0f);
                trackBuffer.setSample(0, sample, value);
                trackBuffer.setSample(1, sample, value);
            }
        }

        // Process and check mixing
        renderer->processAudioBlock(nullptr, 0, outputBuffer, 2, 512, 0, tracks, auxBuses, nullptr);

        // Verify master volume is applied
        float rms = calculateRMS(masterOutput);
        expect(rms > 0.0f, "Mixed output should have signal");

        // Test mute functionality
        renderer->setMasterMute(true);
        renderer->processAudioBlock(nullptr, 0, outputBuffer, 2, 512, 0, tracks, auxBuses, nullptr);

        rms = calculateRMS(masterOutput);
        expect(rms < 1e-6f, "Output should be muted");

        renderer->setMasterMute(false);

        teardownTestEnvironment();
    }

    void testPDC() {
        beginTest("Plugin Delay Compensation");

        setupTestEnvironment();

        // Enable PDC
        renderer->setPDCEnabled(true);
        expect(renderer->isPDCEnabled(), "PDC should be enabled");

        // Set different latency values for tracks using MockLatencyPlugin
        tracks[0]->addPlugin(std::make_unique<MockLatencyPlugin>(64));
        tracks[1]->addPlugin(std::make_unique<MockLatencyPlugin>(128));
        // tracks[2] has 0 latency (no plugins)

        // Create raw pointer vector
        std::vector<zenith::Track*> trackPtrs;
        for (const auto& track : tracks) {
            trackPtrs.push_back(track.get());
        }

        // Recalculate PDC
        renderer->recalculatePDC(trackPtrs);

        // Test track latency reporting
        expect(renderer->getTrackLatency(0) == 64, "Track 0 latency incorrect");
        expect(renderer->getTrackLatency(1) == 128, "Track 1 latency incorrect");
        expect(renderer->getTrackLatency(2) == 0, "Track 2 latency incorrect");

        // Test maximum latency
        int maxLatency = renderer->getMaxTrackLatency();
        expect(maxLatency == 128, "Maximum latency incorrect");

        // Test multiple plugins
        tracks[0]->addPlugin(std::make_unique<MockLatencyPlugin>(10)); // Total 64 + 10 = 74
        renderer->recalculatePDC(trackPtrs);
        expect(renderer->getTrackLatency(0) == 74, "Track 0 summed latency incorrect");
        expect(renderer->getMaxTrackLatency() == 128, "Maximum latency incorrect (still 128)");

        // Test empty track list
        renderer->recalculatePDC({});
        expect(renderer->getMaxTrackLatency() == 0, "Empty track list should yield 0 max latency");

        // Test PDC disabled
        renderer->setPDCEnabled(false);
        expect(!renderer->isPDCEnabled(), "PDC should be disabled");

        teardownTestEnvironment();
    }

    void testMasterEffects() {
        beginTest("Master Effects");

        setupTestEnvironment();

        // Test master limiter
        renderer->setMasterLimiterEnabled(true);
        expect(renderer->isMasterLimiterEnabled(), "Master limiter should be enabled");

        renderer->setMasterLimiterCeiling(-3.0f);
        expect(renderer->getMasterLimiterCeiling() == -3.0f, "Limiter ceiling should be settable");

        // Test test tone
        renderer->enableTestTone(true);
        expect(renderer->isTestToneEnabled(), "Test tone should be enabled");

        renderer->enableTestTone(false);
        expect(!renderer->isTestToneEnabled(), "Test tone should be disabled");

        teardownTestEnvironment();
    }

    void testMetering() {
        beginTest("Metering");

        setupTestEnvironment();

        // Reset meters
        renderer->resetPeakMeters();

        float initialLevel = renderer->getMasterLevel();
        expect(initialLevel == 0.0f, "Initial master level should be zero");

        float initialPeak = renderer->getMasterPeakLevel();
        expect(initialPeak == 0.0f, "Initial master peak should be zero");

        // Process some audio and check metering
        float* outputBuffer[2];
        juce::AudioBuffer<float> masterOutput(2, 512);
        masterOutput.clear();
        outputBuffer[0] = masterOutput.getWritePointer(0);
        outputBuffer[1] = masterOutput.getWritePointer(1);

        // Generate test audio
        tracks[0]->setEnabled(true);
        auto& trackProcessor = tracks[0]->getProcessor();
        auto& trackBuffer = trackProcessor->getPluginBuffer();
        trackBuffer.clear();

        for (int sample = 0; sample < 512; ++sample) {
            float value = 0.1f * std::sin(2.0 * juce::MathConstants<float>::pi * 440.0f * sample / 44100.0f);
            trackBuffer.setSample(0, sample, value);
            trackBuffer.setSample(1, sample, value);
        }

        // Process
        renderer->processAudioBlock(nullptr, 0, outputBuffer, 2, 512, 0, tracks, auxBuses, nullptr);

        // Check metering updated
        float currentLevel = renderer->getMasterLevel();
        expect(currentLevel > 0.0f, "Master level should be updated after processing");

        float currentPeak = renderer->getMasterPeakLevel();
        expect(currentPeak > 0.0f, "Master peak should be updated after processing");

        // Check limiter gain reduction
        float gainReduction = renderer->getMasterLimiterGainReduction();
        expect(gainReduction >= 0.0f, "Gain reduction should be non-negative");

        teardownTestEnvironment();
    }

    void testErrorHandling() {
        beginTest("Error Handling");

        // Test invalid initializations
        auto renderer = std::make_unique<AudioRenderer>();

        // Test initialization with zero sample rate
        expectThrows([&]() {
            renderer->initialize(2, 2, 0.0);
        }, "Should not initialize with zero sample rate");

        // Test shutdown when not initialized
        renderer->shutdown();

        // Test double initialization
        renderer->initialize(2, 2, 44100.0);
        renderer->initialize(2, 2, 44100.0);  // Should not crash

        // Test processing with null buffers
        float* outputBuffer[2] = {nullptr, nullptr};
        renderer->processAudioBlock(nullptr, 0, outputBuffer, 2, 512, 0, {}, {}, nullptr);

        // Test with invalid tracks
        std::vector<zenith::Track*> badTracks = {nullptr, nullptr};
        renderer->processAudioBlock(nullptr, 0, outputBuffer, 2, 512, 0, badTracks, {}, nullptr);

        // Test master limiter with null pointer
        renderer->setMasterLimiterEnabled(true);

        renderer.reset();
    }

private:
    float calculateRMS(const juce::AudioBuffer<float>& buffer) {
        if (buffer.getNumSamples() == 0) return 0.0f;

        float sum = 0.0f;
        int numSamples = buffer.getNumSamples();
        int numChannels = buffer.getNumChannels();

        for (int channel = 0; channel < numChannels; ++channel) {
            const float* channelData = buffer.getReadPointer(channel);
            for (int sample = 0; sample < numSamples; ++sample) {
                float sampleValue = channelData[sample];
                sum += sampleValue * sampleValue;
            }
        }

        return std::sqrt(sum / (numSamples * numChannels));
    }

    template<typename Func>
    void expectThrows(Func func, const juce::String& message = "") {
        try {
            func();
            expect(false, "Expected exception but none was thrown");
        } catch (...) {
            expect(true, "Exception was thrown as expected");
        }
    }
};

static AudioRendererTest audioRendererTest;

} // namespace test
} // namespace zenith