/**
 * @file AudioEngine.h
 * @brief TEST STUB ONLY - NOT for production use!
 * 
 * This is a minimal AudioProcessor stub used ONLY for unit testing.
 * The REAL audio engine is `zenith::Engine` in Engine.h/Engine.cpp.
 * 
 * DO NOT use this class in production code!
 */

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "ProjectState.h"

namespace zenith {
namespace test {

/**
 * @class AudioEngineStub
 * @brief Minimal AudioProcessor for unit testing - NOT FOR PRODUCTION
 */
class AudioEngineStub : public juce::AudioProcessor, 
                        public juce::ValueTree::Listener {
public:
    explicit AudioEngineStub(zenith::ProjectState& s) 
        : juce::AudioProcessor(), state(s.state) {
        state.addListener(this);
    }

    ~AudioEngineStub() override {
        state.removeListener(this);
    }

    //==========================================================================
    void prepareToPlay(double /*sampleRate*/, int /*samplesPerBlock*/) override {}
    void releaseResources() override {}

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midi*/) override {
        // Apply volume if set
        float vol = currentVolume.load(std::memory_order_relaxed);
        if (vol != 1.0f) {
            buffer.applyGain(vol);
        }
    }

    //==========================================================================
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }

    //==========================================================================
    const juce::String getName() const override { return "AudioEngineStub"; }

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==========================================================================
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int /*index*/) override {}
    const juce::String getProgramName(int /*index*/) override { return {}; }
    void changeProgramName(int /*index*/, const juce::String& /*newName*/) override {}

    //==========================================================================
    void getStateInformation(juce::MemoryBlock& /*destData*/) override {}
    void setStateInformation(const void* /*data*/, int /*sizeInBytes*/) override {}

    // ValueTree::Listener
    void valueTreePropertyChanged(juce::ValueTree& v, const juce::Identifier& p) override {
        if (p.toString() == "volume") {
            float newVol = static_cast<float>(v.getProperty(p));
            currentVolume.store(newVol, std::memory_order_relaxed);
        }
    }

private:
    juce::ValueTree state;
    std::atomic<float> currentVolume{1.0f};
};

} // namespace test
} // namespace zenith

