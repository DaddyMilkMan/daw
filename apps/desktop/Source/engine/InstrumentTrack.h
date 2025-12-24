#pragma once

#include "ClipTrack.h"

namespace zenith {

/**
 * @brief InstrumentTrack - A track that hosts a virtual instrument and responds
 * to MIDI input.
 *
 * Inherits from ClipTrack to support MIDI clips. The instrument generates audio
 * based on incoming MIDI data from clips or real-time input.
 */
class InstrumentTrack : public ClipTrack {
public:
  InstrumentTrack(const juce::String &name);
  ~InstrumentTrack() override = default;

  void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;

  void getNextAudioBlock(
      const juce::AudioSourceChannelInfo &bufferToFill, int64_t playheadSamples,
      const juce::MidiBuffer *incomingMidi = nullptr,
      const std::vector<juce::AudioBuffer<float> *> &auxBuffers = {},
      const TempoMap *tempoMap = nullptr) override;

private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InstrumentTrack)
};

} // namespace zenith
