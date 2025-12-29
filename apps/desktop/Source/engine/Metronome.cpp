/*
  ==============================================================================

    Metronome.cpp
    Created: 2025-12-18
    Author:  Zenith DAW

  ==============================================================================
*/

#include "Metronome.h"
#include "TempoMap.h"
#include <cmath>

namespace zenith {

Metronome::Metronome() {}

void Metronome::prepareToPlay(double sampleRate, int samplesPerBlock) {
  juce::ignoreUnused(samplesPerBlock);
  sampleRate_ = sampleRate;
  currentNoteSamplesRemaining_ = 0;
  currentPhase_ = 0.0f;
  lastBeat_ = -1.0;
}

void Metronome::releaseResources() {}

void Metronome::setEnabled(bool shouldBeEnabled) {
  enabled_.store(shouldBeEnabled);
}

bool Metronome::isEnabled() const { return enabled_.load(); }

void Metronome::setLevel(float newLevel) {
  level_.store(juce::jlimit(0.0f, 1.0f, newLevel));
}

float Metronome::getLevel() const { return level_.load(); }

void Metronome::setCountInBars(int bars) {
  countInBars_.store(std::max(0, bars));
}

int Metronome::getCountInBars() const { return countInBars_.load(); }

void Metronome::triggerClick(float frequency) {
  currentFrequency_ = frequency;
  // Calculate phase increment: freq * 2pi / sampleRate
  float safeSampleRate = std::max(1.0f, static_cast<float>(sampleRate_));
  phaseIncrement_ = (frequency * juce::MathConstants<float>::twoPi) / safeSampleRate;

  // Reset or smooth phase? Hard reset for click consistency
  currentPhase_ = 0.0f;

  currentNoteSamplesRemaining_ =
      static_cast<int>(kClickDurationSec * sampleRate_);
}

void Metronome::getNextAudioBlock(juce::AudioBuffer<float> &bufferToFill,
                                  int64_t currentTransportSample,
                                  bool isPlaying, const TempoMap &tempoMap) {
  if (!enabled_.load() || !isPlaying) {
    currentNoteSamplesRemaining_ = 0; // stop any ringing if stopped
    return;
  }

  const int numSamples = bufferToFill.getNumSamples();
  const float outputLevel = level_.load();
  const int numChannels = bufferToFill.getNumChannels();

  float *channelData0 = bufferToFill.getWritePointer(0);
  float *channelData1 =
      (numChannels > 1) ? bufferToFill.getWritePointer(1) : nullptr;

  for (int i = 0; i < numSamples; ++i) {
    int64_t samplePos = currentTransportSample + i;
    double currentBeat = tempoMap.samplesToBeats(samplePos, sampleRate_);

    // Simple beat detection logic:
    // We need to trigger exactly when we cross a quarter note boundary

    // Because of floating point, "crossing" is better detected by observing the
    // change in integral beat But samplesToBeats might jump. Better approach:
    // Calculate the NEXT beat's sample position and see if it falls in this
    // buffer? Or check simply if the integer part of beat changed.

    // Let's use the 'samples to beat' approach for precision.
    double beatInteger;
    double beatFraction = std::modf(currentBeat, &beatInteger);

    // This checks if we are *very* close to the start of a beat.
    // A robust way used in DAWs is tracking the "last beat index" and firing if
    // "current beat index" > "last". However, we process sample by sample here
    // (or small blocks). Since we are iterating i, let's just check equality
    // with a epsilon relative to sample rate? No, `samplesToBeats` is precise.
    // The beat starts exactly when samplePos corresponds to beat X.0.
    //
    // We can invert it: `tempoMap.beatsToSamples(nextBeat)`.

    // Optimization: Don't call `beatsToSamples` every sample.
    // But `tempoMap.samplesToBeats` is fast (linear map lookup).

    // Initialize lastBeat_ on first run or discontinuity
    if (i == 0 && lastBeat_ < 0.0) {
      // Look back one sample to establish state
      lastBeat_ = tempoMap.samplesToBeats(samplePos - 1, sampleRate_);
    }

    double thisSampleBeat = currentBeat;

    // Check for integer crossing
    if (std::floor(thisSampleBeat) > std::floor(lastBeat_)) {
      // Trigger!
      int beatIndex = static_cast<int>(std::floor(thisSampleBeat));

      // Get Time Signature from TempoMap properly
      int numerator = std::max(1, tempoMap.getTimeSignatureNumerator());

      if (beatIndex % numerator == 0)
        triggerClick(kHighClickFreq);
      else
        triggerClick(kLowClickFreq);
    }

    lastBeat_ = thisSampleBeat;

    // Synthesis
    float sampleValue = 0.0f;

    if (currentNoteSamplesRemaining_ > 0) {
      float sineWave = std::sin(currentPhase_);

      // Apply envelope (simple exponential decay)
      float safeSampleRate = std::max(1.0f, static_cast<float>(sampleRate_));
      float envelope = static_cast<float>(currentNoteSamplesRemaining_) /
                       (kClickDurationSec * safeSampleRate);
      envelope = juce::jlimit(0.0f, 1.0f, envelope);
      envelope = envelope * envelope; // Squared for faster decay

      sampleValue = sineWave * envelope * outputLevel;

      currentPhase_ += phaseIncrement_;
      if (currentPhase_ >= juce::MathConstants<float>::twoPi)
        currentPhase_ -= juce::MathConstants<float>::twoPi;

      currentNoteSamplesRemaining_--;
    }

    // Add to buffer
    if (sampleValue != 0.0f) {
      // Mix into channels
      channelData0[i] += sampleValue;
      if (channelData1)
        channelData1[i] += sampleValue;
    }
  }
}

} // namespace zenith
