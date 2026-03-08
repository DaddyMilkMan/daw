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
class ZenithSamplerSound : public juce::SynthesiserSound {
public:
  enum class LoopMode { None, Forward, PingPong };

  ZenithSamplerSound(const juce::String &name, juce::AudioFormatReader &source,
                     const juce::BigInteger &midiNotes,
                     int midiNoteForNormalPitch, int lowVelocity,
                     int highVelocity, double attackTimeSecs,
                     double releaseTimeSecs, double maxSampleLengthSeconds,
                     LoopMode loopMode = LoopMode::None, float gain = 1.0f,
                     float tune = 0.0f, int chokeGroup = 0);

  // Constructor that uses AudioFilePool handle
  ZenithSamplerSound(const juce::String &name,
                     AudioFilePool::HandlePtr audioHandle,
                     const juce::BigInteger &midiNotes,
                     int midiNoteForNormalPitch, int lowVelocity,
                     int highVelocity, LoopMode loopMode = LoopMode::None,
                     float gain = 1.0f, float tune = 0.0f, int chokeGroup = 0);

  ~ZenithSamplerSound() override;

  bool appliesToNote(int midiNoteNumber) override;
  bool appliesToChannel(int midiChannel) override;

  juce::String getName() const { return soundName; }
  const juce::AudioBuffer<float> *getAudioData() const { return data; }
  double getSampleRate() const { return sourceSampleRate; }
  int getRootNote() const { return rootNote; }
  LoopMode getLoopMode() const { return loopMode; }
  float getGain() const { return gain; }
  float getTune() const { return tune; }
  int getChokeGroup() const { return chokeGroup; }

  bool appliesToVelocity(int midiVelocity) const {
    return midiVelocity >= lowVelocity && midiVelocity <= highVelocity;
  }

  int getLowKey() const {
    for (int i = 0; i < 128; ++i)
      if (midiNotes[i])
        return i;
    return -1;
  }
  int getHighKey() const {
    for (int i = 127; i >= 0; --i)
      if (midiNotes[i])
        return i;
    return -1;
  }
  int getLowVelocity() const { return lowVelocity; }
  int getHighVelocity() const { return highVelocity; }

private:
  juce::String soundName;

  // Can hold audio data directly or via pool handle
  std::unique_ptr<juce::AudioBuffer<float>> ownedData;
  AudioFilePool::HandlePtr poolHandle;
  const juce::AudioBuffer<float> *data =
      nullptr; // Points to either ownedData or poolHandle->buffer

  double sourceSampleRate;
  juce::BigInteger midiNotes;
  int rootNote;
  int lowVelocity, highVelocity;
  LoopMode loopMode;
  float gain;
  float tune;
  int chokeGroup;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithSamplerSound)
};

//==============================================================================
/**
 * @brief Custom sampler voice with filter and envelope
 */

} // namespace
