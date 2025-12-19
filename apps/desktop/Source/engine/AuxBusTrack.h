#pragma once

#include "Track.h"

namespace zenith {

class AuxBusTrack : public Track {
public:
    AuxBusTrack(const juce::String& name) : Track(name, Type::Bus) {}
    ~AuxBusTrack() override = default;

    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill, 
                           int64_t playheadSamples,
                           const juce::MidiBuffer* incomingMidi = nullptr,
                           const std::vector<juce::AudioBuffer<float>*>& auxBuffers = {},
                           const TempoMap* tempoMap = nullptr) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AuxBusTrack)
};

} // namespace zenith
