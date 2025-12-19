#include "InstrumentTrack.h"

namespace zenith {

InstrumentTrack::InstrumentTrack(const juce::String& name) 
    : MIDITrack(name) {
    trackType = Type::Instrument;
}

void InstrumentTrack::prepareToPlay(int samplesPerBlockExpected, double sampleRate) {
    MIDITrack::prepareToPlay(samplesPerBlockExpected, sampleRate);
}

} // namespace zenith
