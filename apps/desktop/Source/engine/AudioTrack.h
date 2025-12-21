#pragma once

#include "ClipTrack.h"

namespace zenith {

class AudioTrack : public ClipTrack {
public:
    AudioTrack(const juce::String& name) : ClipTrack(name, Type::Audio) {}
    ~AudioTrack() override = default;

    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill, 
                           int64_t playheadSamples,
                           const juce::MidiBuffer* incomingMidi = nullptr,
                           const std::vector<juce::AudioBuffer<float>*>& auxBuffers = {},
                           const TempoMap* tempoMap = nullptr) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioTrack)
};

} // namespace zenith
