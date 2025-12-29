#pragma once

#include <span>
#include "ClipTrack.h"

namespace zenith {

class MIDITrack : public ClipTrack {
public:
  MIDITrack(const juce::String &name);
  ~MIDITrack() override = default;

  void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;

  void getNextAudioBlock(
      const juce::AudioSourceChannelInfo &bufferToFill, juce::int64 playheadSamples,
      const juce::MidiBuffer *incomingMidi = nullptr,
      std::span<juce::AudioBuffer<float> * const> auxBuffers = {},
      const TempoMap *tempoMap = nullptr,
      const juce::AudioBuffer<float> *sidechainBuffer = nullptr) override;

private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MIDITrack)
};

} // namespace zenith
