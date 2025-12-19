#pragma once

#include "MIDITrack.h"

namespace zenith {

class InstrumentTrack : public MIDITrack {
public:
    InstrumentTrack(const juce::String& name);
    ~InstrumentTrack() override = default;

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InstrumentTrack)
};

} // namespace zenith
