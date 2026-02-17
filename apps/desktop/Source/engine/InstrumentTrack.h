#pragma once
#include "../instruments/Instrument.h"
#include "ClipTrack.h"

namespace zenith {

/**
 * @brief InstrumentTrack - A track that hosts a virtual instrument and responds
 * to MIDI input.
 */
class InstrumentTrack : public ClipTrack {
public:
  InstrumentTrack(const juce::String &name)
      : ClipTrack(name, Type::Instrument) {}
  ~InstrumentTrack() override = default;

  void setInstrument(std::unique_ptr<Instrument> newInstrument) {
    instrument = std::move(newInstrument);
  }
  Instrument *getInstrument() const { return instrument.get(); }

  void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override {
    ClipTrack::prepareToPlay(samplesPerBlockExpected, sampleRate);
    if (instrument != nullptr && instrument->getAudioProcessor() != nullptr) {
      instrument->getAudioProcessor()->prepareToPlay(sampleRate,
                                                     samplesPerBlockExpected);
    }
  }

  void releaseResources() override {
    ClipTrack::releaseResources();
    if (instrument != nullptr && instrument->getAudioProcessor() != nullptr) {
      instrument->getAudioProcessor()->releaseResources();
    }
  }

  void getNextAudioBlock(
      const juce::AudioSourceChannelInfo &bufferToFill, int64_t playheadSamples,
      const juce::MidiBuffer *incomingMidi = nullptr,
      const std::vector<juce::AudioBuffer<float> *> &auxBuffers = {},
      const TempoMap *tempoMap = nullptr,
      const juce::AudioBuffer<float> *sidechainBuffer = nullptr) override;

  juce::ValueTree getState() const override {
    auto state = ClipTrack::getState();
    if (instrument != nullptr) {
      juce::ValueTree instrState("INSTRUMENT");
      instrState.setProperty("id", instrument->getMetadata().instrumentId,
                             nullptr);
      state.appendChild(instrState, nullptr);
    }
    return state;
  }

  void loadState(const juce::ValueTree &state) override {
    ClipTrack::loadState(state);
  }

private:
  std::unique_ptr<Instrument> instrument;
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InstrumentTrack)
};

} // namespace zenith
