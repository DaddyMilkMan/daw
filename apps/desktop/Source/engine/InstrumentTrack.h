#pragma once
#include "../instruments/Instrument.h"
#include "../instruments/InstrumentRegistry.h"
#include "ClipTrack.h"

namespace zenith {

class InstrumentTrack : public ClipTrack {
public:
  InstrumentTrack(const juce::String &name)
      : ClipTrack(name, Type::Instrument) {}
  ~InstrumentTrack() override = default;

  void setInstrument(std::unique_ptr<Instrument> newInstrument) {
    instrument = std::move(newInstrument);
  }

  Instrument *getInstrument() const { return instrument.get(); }

  void setInstrumentRegistry(InstrumentRegistry *registry) {
    instrumentRegistry = registry;
  }

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
      const TempoMap *tempoMap = nullptr) override {
    if (instrument != nullptr && incomingMidi != nullptr &&
        instrument->getAudioProcessor() != nullptr) {
      // Create a mutable copy - processBlock may modify the MIDI buffer
      juce::MidiBuffer midiCopy(*incomingMidi);
      instrument->getAudioProcessor()->processBlock(*bufferToFill.buffer,
                                                    midiCopy);
    }
    ClipTrack::getNextAudioBlock(bufferToFill, playheadSamples, incomingMidi,
                                 auxBuffers, tempoMap);
  }

  juce::ValueTree getState() const {
    auto state = ClipTrack::getState();
    if (instrument != nullptr) {
      juce::ValueTree instrState("INSTRUMENT");
      instrState.setProperty("id", instrument->getMetadata().instrumentId,
                             nullptr);
      state.appendChild(instrState, nullptr);
    }
    return state;
  }

  void loadState(const juce::ValueTree &state) {
    ClipTrack::loadState(state);

    // Restore the instrument from saved state
    auto instrState = state.getChildWithName("INSTRUMENT");
    if (instrState.isValid() && instrumentRegistry != nullptr) {
      auto instrumentId = instrState.getProperty("id").toString();
      if (instrumentId.isNotEmpty()) {
        auto newInstrument = instrumentRegistry->createInstrument(instrumentId);
        if (newInstrument != nullptr) {
          setInstrument(std::move(newInstrument));
        }
      }
    }
  }

private:
  std::unique_ptr<Instrument> instrument;
  InstrumentRegistry *instrumentRegistry = nullptr;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InstrumentTrack)
};

} // namespace zenith
