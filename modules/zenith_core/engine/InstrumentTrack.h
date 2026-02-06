/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

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
    midiBuffer_.ensureSize(65536);
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
      std::span<juce::AudioBuffer<float>* const> auxBuffers = {},
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
  juce::MidiBuffer midiBuffer_;
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InstrumentTrack)
};

} // namespace zenith
