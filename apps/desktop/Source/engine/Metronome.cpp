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

  // Optimization: Calculate beat info once per block if tempo is constant
  // (Approximation: We assume tempo doesn't change drastically WITHIN a 10ms block for the metronome click)
  double startBeat = tempoMap.samplesToBeats(currentTransportSample, sampleRate_);
  double endBeat = tempoMap.samplesToBeats(currentTransportSample + numSamples, sampleRate_);
  double beatsPerSample = (endBeat - startBeat) / (double)numSamples;

  // Track the current beat as we iterate
  double currentBeat = startBeat;

  for (int i = 0; i < numSamples; ++i) {
    // Current sample index in this process block is 'i'
    // Transport sample is currentTransportSample + i
    
    // Instead of: double currentBeat = tempoMap.samplesToBeats(samplePos, sampleRate_);
    // We increment:
    
    // Check for integer crossing
    if (std::floor(currentBeat) > std::floor(lastBeat_)) {
      // Trigger!
      int beatIndex = static_cast<int>(std::floor(currentBeat));

      // Get Time Signature from TempoMap properly
      int numerator = std::max(1, tempoMap.getTimeSignatureNumerator());

      if (beatIndex % numerator == 0)
        triggerClick(kHighClickFreq);
      else
        triggerClick(kLowClickFreq);
    }
    
    lastBeat_ = currentBeat;
    currentBeat += beatsPerSample;

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
