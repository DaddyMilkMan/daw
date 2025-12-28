/*
  ==============================================================================

    ZenithOscillator.cpp
    Created: 2025-12-06
    Refactored: 2025-12-09
    Author:  Zenith DAW

    Implementation of ZenithOscillator.
    Includes Flagship Wavetable support.

  ==============================================================================
*/

#include "ZenithOscillator.h"
#include <cmath>

namespace zenith {

void ZenithOscillator::setDetune(float detuneCents) {
  if (detuneCents_ != detuneCents) {
    detuneCents_ = detuneCents;
    if (supersawInit_) {
      updateSupersawRatios();
    }
  }
}

float ZenithOscillator::getNextSample(float frequency, float shape) {
  // Pro Upgrade: Real wavetable support
  if (waveform_ == OscillatorWaveform::Wavetable) {
    if (hasWavetable()) {
      return processRealWavetable(frequency, shape);
    } else {
      // Fallback to procedural wavetable if no real wavetable loaded
      return processWavetable(frequency, shape);
    }
  }

  // Apply detune
  float detuneMultiplier = std::exp2(detuneCents_ / 1200.0f);
  frequency *= detuneMultiplier;

  switch (waveform_) {
  case OscillatorWaveform::Sine:
    return processSine(frequency);
  case OscillatorWaveform::Saw:
    return processSaw(frequency);
  case OscillatorWaveform::Square:
    return processSquare(frequency, shape); // Classic PWM
  case OscillatorWaveform::Triangle:
    return processTriangle(frequency);
  case OscillatorWaveform::Noise:
    return processNoise();
  case OscillatorWaveform::Supersaw:
    return processSupersaw(frequency);
  default:
    return 0.0f;
  }
}

void ZenithOscillator::updateSupersawRatios() {
  float spread = detuneCents_ / 100.0f;

  supersawDetunes_[0] = 0.0f;
  supersawDetunes_[1] = -0.11002313f * spread;
  supersawDetunes_[2] = 0.11897400f * spread;
  supersawDetunes_[3] = -0.06288439f * spread;
  supersawDetunes_[4] = 0.05957643f * spread;
  supersawDetunes_[5] = -0.02756390f * spread;
  supersawDetunes_[6] = 0.03001021f * spread;

  for (int i = 0; i < 7; ++i) {
    supersawRatios_[i] = std::exp2(supersawDetunes_[i] / 12.0f);
  }
}

float ZenithOscillator::processSine(float frequency) {
  float sample = std::sin(phase_ * juce::MathConstants<double>::twoPi);
  phase_ += frequency / sampleRate_;
  if (phase_ >= 1.0)
    phase_ -= 1.0;
  return sample;
}

float ZenithOscillator::processSaw(float frequency) {
  float phaseInc = frequency / sampleRate_;
  float sample = 2.0f * static_cast<float>(phase_) - 1.0f;
  sample -= poly_blep(static_cast<float>(phase_), phaseInc);
  phase_ += phaseInc;
  if (phase_ >= 1.0)
    phase_ -= 1.0;
  return sample;
}

float ZenithOscillator::processSquare(float frequency, float pulseWidth) {
  float phaseInc = frequency / sampleRate_;
  float sample = (phase_ < pulseWidth) ? 1.0f : -1.0f;
  sample += poly_blep(static_cast<float>(phase_), phaseInc);
  float phase2 = static_cast<float>(phase_) - pulseWidth;
  if (phase2 < 0.0f)
    phase2 += 1.0f;
  sample -= poly_blep(phase2, phaseInc);

  phase_ += phaseInc;
  if (phase_ >= 1.0)
    phase_ -= 1.0;
  return sample;
}

float ZenithOscillator::processTriangle(float frequency) {
  float phaseInc = frequency / sampleRate_;

  // Leaky Integration of PolyBLEP Square Wave
  // 1. Calculate BL Square Sample (without advancing phase)
  float square = (phase_ < 0.5) ? 1.0f : -1.0f;
  square += poly_blep(static_cast<float>(phase_), phaseInc);

  float phase2 = static_cast<float>(phase_) - 0.5f;
  if (phase2 < 0.0f)
    phase2 += 1.0f;
  square -= poly_blep(phase2, phaseInc);

  // 2. Integrate
  // Scale factor 4*freq/SR makes slope 4, resulting in amplitude 1
  lastTriangleValue_ += 4.0f * phaseInc * square;

  // 3. Apply Leak (High-pass filter to center waveform and prevent drift)
  // Coeff 0.9999 is -3dB around 5-10Hz depending on SR
  lastTriangleValue_ *= 0.9999f;

  // 4. Advance Phase
  phase_ += phaseInc;
  if (phase_ >= 1.0)
    phase_ -= 1.0;

  return lastTriangleValue_;
}

float ZenithOscillator::processNoise() {
  return random_.nextFloat() * 2.0f - 1.0f;
}

float ZenithOscillator::processSupersaw(float frequency) {
  if (!supersawInit_) {
    supersawDetunes_[0] = 0.0f;
    supersawDetunes_[1] = -0.11002313f;
    supersawDetunes_[2] = 0.11897400f;
    supersawDetunes_[3] = -0.06288439f;
    supersawDetunes_[4] = 0.05957643f;
    supersawDetunes_[5] = -0.02756390f;
    supersawDetunes_[6] = 0.03001021f;

    for (auto &phase : supersawPhases_)
      phase = random_.nextFloat();
    supersawInit_ = true;
    updateSupersawRatios();
  }

  float sample = 0.0f;
  for (int i = 0; i < 7; ++i) {
    float detunedFreq = frequency * supersawRatios_[i];
    float phaseInc = detunedFreq / sampleRate_;
    float s = 2.0f * static_cast<float>(supersawPhases_[i]) - 1.0f;
    s -= poly_blep(static_cast<float>(supersawPhases_[i]), phaseInc);
    float gain = (i == 0) ? 1.2f : 1.0f;
    sample += s * gain;
    supersawPhases_[i] += phaseInc;
    if (supersawPhases_[i] >= 1.0)
      supersawPhases_[i] -= 1.0;
  }
  return sample * 0.14f;
}

float ZenithOscillator::processWavetable(float frequency, float shape) {
  // Procedural Wavetable: Sine -> Triangle -> Saw -> Square
  double originalPhase = phase_;
  float out = 0.0f;

  // Clamp shape
  shape = juce::jlimit(0.0f, 1.0f, shape);

  if (shape < 0.33f) {
    // Sine -> Triangle
    float t = shape * 3.03f; // Scale to 0..1 (approx)

    float sine = processSine(frequency);
    phase_ = originalPhase; // Restore
    float tri = processTriangle(frequency);
    phase_ = originalPhase; // Restore

    out = sine * (1.0f - t) + tri * t;
  } else if (shape < 0.66f) {
    // Triangle -> Saw
    float t = (shape - 0.33f) * 3.03f;

    float tri = processTriangle(frequency);
    phase_ = originalPhase;
    float saw = processSaw(frequency);
    phase_ = originalPhase;

    out = tri * (1.0f - t) + saw * t;
  } else {
    // Saw -> Square
    float t = (shape - 0.66f) * 2.94f;
    if (t > 1.0f)
      t = 1.0f;

    float saw = processSaw(frequency);
    phase_ = originalPhase;
    float square = processSquare(frequency, 0.5f);
    phase_ = originalPhase;

    out = saw * (1.0f - t) + square * t;
  }

  // Advance phase manually exactly as the basic waveforms do
  phase_ += frequency / sampleRate_;
  if (phase_ >= 1.0)
    phase_ -= 1.0;

  return out;
}

//==============================================================================
// PRO UPGRADE: Real Wavetable Playback with MIP-mapping
//==============================================================================
float ZenithOscillator::processRealWavetable(float frequency, float shape) {
  if (!wavetable_ || !wavetable_->isValid()) {
    // Fallback to procedural if no wavetable loaded
    return processWavetable(frequency, shape);
  }

  // Apply detune
  float detuneMultiplier = std::exp2(detuneCents_ / 1200.0f);
  float actualFreq = frequency * detuneMultiplier;

  // Calculate MIP level for anti-aliasing
  int mipLevel = calculateMipLevel(actualFreq, sampleRate_);

  // Get sample from wavetable with cross-frame interpolation
  // shape (0-1) controls frame position
  float sample =
      wavetable_->getSample(static_cast<float>(phase_), shape, mipLevel);

  // Advance phase
  double phaseInc = actualFreq / sampleRate_;
  phase_ += phaseInc;
  if (phase_ >= 1.0)
    phase_ -= 1.0;

  // Cache frequency for MIP calculation optimization
  lastWavetableFreq_ = actualFreq;

  return sample;
}

} // namespace zenith
