#pragma once

#include "ClipTrack.h"
#include <juce_audio_basics/juce_audio_basics.h>

namespace zenith {

class Instrument;

class MIDITrack : public ClipTrack {
public:
    MIDITrack(const juce::String& name, Type type = Type::MIDI) 
        : ClipTrack(name, type) {}
    ~MIDITrack() override = default;

    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill, 
                           int64_t playheadSamples,
                           const juce::MidiBuffer* incomingMidi = nullptr,
                           const std::vector<juce::AudioBuffer<float>*>& auxBuffers = {},
                           const TempoMap* tempoMap = nullptr) override;

    virtual void generateMidiForBlock(double tempo, double sampleRate, 
                                     juce::int64 blockStartSample, int blockSize, 
                                     juce::MidiBuffer& midiOut);

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MIDITrack)
};

class InstrumentTrack : public MIDITrack {
public:
    InstrumentTrack(const juce::String& name) : MIDITrack(name, Type::Instrument) {
        instrumentBuffer_.setSize(2, 512); // Default
    }
    ~InstrumentTrack() override = default;

    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill, 
                           int64_t playheadSamples,
                           const juce::MidiBuffer* incomingMidi = nullptr,
                           const std::vector<juce::AudioBuffer<float>*>& auxBuffers = {},
                           const TempoMap* tempoMap = nullptr) override;

    void setInstrument(std::unique_ptr<Instrument> instrument);
    Instrument* getInstrument() const { return instrument_.get(); }
    bool hasInstrument() const { return instrument_ != nullptr; }

protected:
    std::unique_ptr<Instrument> instrument_;
    juce::AudioBuffer<float> instrumentBuffer_;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InstrumentTrack)
};

} // namespace zenith
