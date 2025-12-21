#pragma once

#include "ClipTrack.h"

namespace zenith {

class MIDITrack : public ClipTrack {
public:
    MIDITrack(const juce::String& name);
    ~MIDITrack() override = default;

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill,
                          int64_t playheadSamples,
                          const juce::MidiBuffer* incomingMidi = nullptr,
                          const std::vector<juce::AudioBuffer<float>*>& auxBuffers = {},
                          const TempoMap* tempoMap = nullptr) override;
                          
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MIDITrack)
};

} // namespace zenith
