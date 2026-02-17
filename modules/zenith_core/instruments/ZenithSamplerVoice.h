/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#pragma once

#include "../engine/AudioFilePool.h"
#include "ContentPaths.h"
#include "Instrument.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

namespace zenith {

// Forward declarations for voice/sound classes
class ZenithSamplerVoice : public juce::SynthesiserVoice {
public:
  ZenithSamplerVoice();
  ~ZenithSamplerVoice() override;

  bool canPlaySound(juce::SynthesiserSound *sound) override;

  void startNote(int midiNoteNumber, float velocity,
                 juce::SynthesiserSound *sound,
                 int currentPitchWheelPosition) override;

  void stopNote(float velocity, bool allowTailOff) override;

  void pitchWheelMoved(int newPitchWheelValue) override;
  void controllerMoved(int controllerNumber, int newControllerValue) override;

  void renderNextBlock(juce::AudioBuffer<float> &outputBuffer, int startSample,
                       int numSamples) override;

  void setParameters(std::atomic<float> *attack, std::atomic<float> *decay,
                     std::atomic<float> *sustain, std::atomic<float> *release,
                     std::atomic<float> *filterCutoff,
                     std::atomic<float> *filterResonance,
                     std::atomic<float> *sampleStartOffset,
                     std::atomic<float> *pitchFine,
                     std::atomic<float> *pitchSemitones,
                     std::atomic<float> *globalPan,
                     std::atomic<float> *globalGain);

private:
  // Envelope
  juce::ADSR ampEnvelope;
  juce::ADSR::Parameters ampEnvParams;

  // Filter
  juce::dsp::StateVariableTPTFilter<float> filter;

  // Playback state
  double pitchRatio = 0.0;
  double sourceSamplePosition = 0.0;
  float velocity = 0.0f;
  bool loopDirection = true; // true = forward, false = backward (for ping-pong)

  // Parameter pointers (from APVTS)
  std::atomic<float> *attackParam = nullptr;
  std::atomic<float> *decayParam = nullptr;
  std::atomic<float> *sustainParam = nullptr;
  std::atomic<float> *releaseParam = nullptr;
  std::atomic<float> *filterCutoffParam = nullptr;
  std::atomic<float> *filterResonanceParam = nullptr;
  std::atomic<float> *sampleStartOffsetParam = nullptr;
  std::atomic<float> *pitchFineParam = nullptr;
  std::atomic<float> *pitchSemitonesParam = nullptr;
  std::atomic<float> *globalPanParam = nullptr;
  std::atomic<float> *globalGainParam = nullptr;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithSamplerVoice)
};

} // namespace zenith
