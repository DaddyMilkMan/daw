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

/*
    ==============================================================================
    Original file header:
*/

  ==============================================================================

    ZenithFilter.h
    Created: 2025-12-06
    Updated: 2025-02-01 (Professional Filter Overhaul)
    Author:  Zenith DAW

    Professional multimode filter with circuit-modeled implementations
    and 4x oversampling for premium sound quality.


  ==============================================================================
*/

#pragma once

#include "ZenithPolySynthDefs.h"
#include "ZenithAdvancedFilters.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>
#include <memory>
#include <array>

namespace zenith {

/**
    Professional multimode filter with multiple circuit models
*/
class ZenithFilter {
public:
  ZenithFilter() : oversamplingFactor_(4) {
    // Filters will be lazily initialized on first use
  }

  void setType(FilterType type) { type_ = type; }
  void setSampleRate(double sampleRate);
  void setCutoff(float cutoffHz);
  void setResonance(float resonance);
  void setDrive(float drive) { drive_ = drive; }
  void reset();
  float getResonance() const { return resonanceSmoothed_.getTargetValue(); }
  
  void setModel(FilterModelType model) { 
    model_ = static_cast<int>(model); 
    // Reset sub-filters when model changes
    if (moogFilter_) moogFilter_->reset();
    if (ms20Filter_) ms20Filter_->reset();
    if (prophetFilter_) prophetFilter_->reset();
    if (semFilter_) semFilter_->reset();
    if (tb303Filter_) tb303Filter_->reset();
  }
  
  void setOversamplingFactor(int factor) {
    oversamplingFactor_ = juce::jlimit(1, 4, factor);
  }

  /**
   * @brief Process one sample
   * @param input Input sample
   * @return Filtered sample
   */
  float processSample(float input);

  /**
   * @brief Process block of samples (more efficient)
   */
  void processBlock(juce::AudioBuffer<float>& buffer);

private:
  FilterType type_ = FilterType::Lowpass;
  int model_ = 0; // FilterModelType enum value
  double sampleRate_ = 44100.0;
  int oversamplingFactor_ = 4; // Default to 4x for best quality

  // Smoothed parameters to avoid zipper noise
  juce::SmoothedValue<float> cutoffSmoothed_;
  juce::SmoothedValue<float> resonanceSmoothed_;
  float drive_ = 1.0f;

  // SVF State (legacy, for backward compatibility)
  float ic1eq_ = 0.0f, ic2eq_ = 0.0f;

  // Advanced filter instances (lazy initialization)
  std::unique_ptr<MoogLadderFilter> moogFilter_;
  std::unique_ptr<MS20LowpassFilter> ms20Filter_;
  std::unique_ptr<Prophet5Filter> prophetFilter_;
  std::unique_ptr<SEMFilter> semFilter_;
  std::unique_ptr<TB303Filter> tb303Filter_;

  // Oversamplers for quality
  std::unique_ptr<juce::dsp::Oversampling<float>> oversampler2x_;
  std::unique_ptr<juce::dsp::Oversampling<float>> oversampler4x_;

  // Helper methods
  void initializeFilters();
  float processSVF(float input);
  float processWithOversampling(float input);
};

} // namespace zenith
