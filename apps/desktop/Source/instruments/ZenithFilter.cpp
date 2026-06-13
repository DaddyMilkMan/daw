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

#include "ZenithFilter.h"
#include <cmath>

namespace zenith {

namespace {
// Smooth saturating non-linearity used by the drive stage.
inline float softClip(float x) noexcept {
  return std::tanh(x);
}

// Normalised frequency clamp keeps every model stable near Nyquist.
inline double clampNormFreq(double f) noexcept {
  return juce::jlimit(0.0001, 0.49, f);
}
} // namespace

//==============================================================================
// STATE
//==============================================================================

void ZenithFilter::reset() {
  svf_ = {};
  moog_ = {};
  tb303_ = {};
  ms20_ = {};
  sem_ = {};
}

//==============================================================================
// PROCESSING
//==============================================================================

float ZenithFilter::processSample(float input, float midiNote) {
  // Key-tracking is folded into the working cutoff for this sample.
  const float trackedCutoff = calculateCutoffWithKeyTrack(midiNote);
  const float previousCutoff = cutoff_;
  cutoff_ = trackedCutoff;

  const float driven = applyDrive(input);

  float output;
  switch (model_) {
    case FilterModelType::Moog:  output = processMoog(driven); break;
    case FilterModelType::MS20:  output = processMS20(driven); break;
    case FilterModelType::SEM:   output = processSEM(driven); break;
    case FilterModelType::TB303: output = processTB303(driven); break;
    case FilterModelType::SVF:
    case FilterModelType::Ladder:
    default:                     output = processSVF(driven); break;
  }

  cutoff_ = previousCutoff;
  return output;
}

float ZenithFilter::calculateCutoffWithKeyTrack(float midiNote) {
  const float offset = (midiNote - 60.0f) * keyTrackAmount_;
  return juce::jlimit(20.0f, 20000.0f, cutoff_ * std::exp2(offset / 12.0f));
}

float ZenithFilter::applyDrive(float sample) {
  if (drive_ <= 0.0f) return sample;
  const float gain = 1.0f + drive_ * 9.0f;
  return softClip(sample * gain) / gain;
}

//==============================================================================
// SVF (State Variable Filter) - CHAMBERLIN
//==============================================================================

float ZenithFilter::processSVF(float input) {
  const double fs = sampleRate_ * oversamplingFactor_;
  const double f = clampNormFreq(cutoff_ / fs);
  const double q = resonance_ * 10.0 + 1.0;
  const double r = 1.0 / q;

  const double low1 = svf_.low + f * svf_.band;
  const double high1 = input - low1 - r * svf_.band;
  const double band1 = svf_.band + f * high1;

  svf_.low = low1;
  svf_.high = high1;
  svf_.band = band1;

  switch (type_) {
    case FilterType::Lowpass:  return static_cast<float>(svf_.low);
    case FilterType::Highpass: return static_cast<float>(svf_.high);
    case FilterType::Bandpass: return static_cast<float>(svf_.band);
    default:                   return static_cast<float>(svf_.low);
  }
}

//==============================================================================
// MOOG LADDER - one-pole cascade with non-linear feedback
//==============================================================================

float ZenithFilter::processMoog(float input) {
  const double fs = sampleRate_ * oversamplingFactor_;
  const double wc = 2.0 * juce::MathConstants<double>::pi * cutoff_;
  const double g = std::tan(clampNormFreq(wc / (4.0 * fs)));

  const double res = resonance_;
  const double feedback = res * 4.0;

  const double y = input - feedback * moog_.s4;

  moog_.s1 += g * (y - moog_.s1);
  moog_.s2 += g * (moog_.s1 - moog_.s2);
  moog_.s3 += g * (moog_.s2 - moog_.s3);
  moog_.s4 += g * (moog_.s3 - moog_.s4);

  const double yOut = std::tanh(moog_.s4);

  switch (type_) {
    case FilterType::Highpass: return static_cast<float>(input - yOut);
    case FilterType::Bandpass: return static_cast<float>(moog_.s2 - moog_.s4);
    case FilterType::Lowpass:
    default:                   return static_cast<float>(yOut);
  }
}

//==============================================================================
// KORG MS-20 - high-pass feeding a resonant 2-pole low-pass
//==============================================================================

float ZenithFilter::processMS20(float input) {
  const double fs = sampleRate_ * oversamplingFactor_;
  const double g = std::tan(clampNormFreq(juce::MathConstants<double>::pi * cutoff_ / fs));

  const double hp = input - ms20_.hp;
  ms20_.hp += g * hp;

  ms20_.lp1 += g * (hp - ms20_.lp1);
  ms20_.lp2 += g * (ms20_.lp1 - ms20_.lp2);

  const double res = resonance_ * 0.95;
  const double output = ms20_.lp2 - res * ms20_.lp2;

  return static_cast<float>(output);
}

//==============================================================================
// OBERHEIM SEM - topology-preserving state-variable filter
//==============================================================================

float ZenithFilter::processSEM(float input) {
  const double fs = sampleRate_ * oversamplingFactor_;
  const double g = std::tan(clampNormFreq(juce::MathConstants<double>::pi * cutoff_ / fs));
  const double k = 2.0 - 2.0 * juce::jlimit(0.0, 0.97, static_cast<double>(resonance_)); // damping

  // Zavalishin TPT SVF: s1/s2 are the integrator states.
  const double hp = (input - (k + g) * sem_.s1 - sem_.s2) / (1.0 + g * (k + g));
  const double bp = g * hp + sem_.s1;
  const double lp = g * bp + sem_.s2;

  sem_.s1 = g * hp + bp;
  sem_.s2 = g * bp + lp;

  switch (type_) {
    case FilterType::Highpass: return static_cast<float>(hp);
    case FilterType::Bandpass: return static_cast<float>(bp);
    case FilterType::Lowpass:
    default:                   return static_cast<float>(lp);
  }
}

//==============================================================================
// ROLAND TB-303 - diode ladder approximation
//==============================================================================

float ZenithFilter::processTB303(float input) {
  const double fs = sampleRate_ * oversamplingFactor_;
  const double wc = 2.0 * juce::MathConstants<double>::pi * cutoff_;
  const double g = std::tan(clampNormFreq(wc / (4.0 * fs)));

  const double res = resonance_ * 3.8;
  const double y = input - res * std::tanh(tb303_.s4);

  tb303_.s1 += g * (y - tb303_.s1);
  tb303_.s2 += g * (tb303_.s1 - tb303_.s2);
  tb303_.s3 += g * (tb303_.s2 - tb303_.s3);
  tb303_.s4 += g * (tb303_.s3 - tb303_.s4);

  switch (type_) {
    case FilterType::Highpass: return static_cast<float>(input - tb303_.s4);
    case FilterType::Lowpass:
    default:                   return static_cast<float>(tb303_.s4);
  }
}

//==============================================================================
// BLOCK PROCESSING
//==============================================================================

void ZenithFilter::process(juce::AudioBuffer<float>& buffer, float midiNote) {
  const auto numSamples = buffer.getNumSamples();
  const auto numChannels = buffer.getNumChannels();

  for (int ch = 0; ch < numChannels; ++ch) {
    float* channel = buffer.getWritePointer(ch);
    for (int i = 0; i < numSamples; ++i) {
      channel[i] = processSample(channel[i], midiNote);
    }
  }
}

} // namespace zenith
