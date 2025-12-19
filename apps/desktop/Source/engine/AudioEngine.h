#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "ProjectState.h"

namespace Zenith {

class AudioEngine : public juce::AudioProcessor, 
                    public juce::ValueTree::Listener {
public:
    AudioEngine(zenith::ProjectState& s) : state(s.state) {
        state.addListener(this);
    }

    ~AudioEngine() override {
        state.removeListener(this);
    }

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override {}
    void releaseResources() override {}

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override {
        // High-performance DSP loop logic
        // The volume is updated via valueTreePropertyChanged and applied here
        
        // Placeholder for real DSP: Apply gain from atomic volume if we had one
        // for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        //    buffer.applyGain(channel, 0, buffer.getNumSamples(), currentVolume.load());
    }

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }

    //==============================================================================
    const juce::String getName() const override { return "ZenithAudioEngine"; }

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==============================================================================
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int index) override {}
    const juce::String getProgramName (int index) override { return {}; }
    void changeProgramName (int index, const juce::String& newName) override {}

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override {}
    void setStateInformation (const void* data, int sizeInBytes) override {}

    // Reacting to data changes
    void valueTreePropertyChanged(juce::ValueTree& v, const juce::Identifier& p) override {
        if (p == Zenith::IDs::volume) {
            float newVol = v.getProperty(p);
            // Update internal DSP gain parameter safely
            // currentVolume.store(newVol);
            DBG("AudioEngine: Volume changed to " << newVol);
        }
    }

private:
    juce::ValueTree state;
    // std::atomic<float> currentVolume{0.75f};
};

} // namespace Zenith
