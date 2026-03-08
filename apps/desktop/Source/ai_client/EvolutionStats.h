/*
  ==============================================================================

    PresetGeneticistAgent.h
    Created: 2025-12-07
    Author:  Zenith DAW AI Team

    "The Preset Geneticist" - Evolutionary Sound Design Agent

    Role: The Mad Scientist. Uses Genetic Algorithms to breed new synthesizer
    patches for ZenithPolySynth. It doesn't just randomize knobs; it "listens"
    to the result and evolves the best ones.

    Features:
    - Automated patch generation using genetic algorithms
    - Audio-based fitness evaluation (silence, clipping, harmonic richness)
    - Crossover breeding of successful patches
    - Mutation for genetic diversity
    - Batch saving of evolved presets

    Time Factor: Rendering audio and running spectral analysis on 1,000
    generations of patches can easily take 20+ minutes - this is CPU
    intensive, long-running background work.

  ==============================================================================
*/

#pragma once

#include <zenith_core/instruments/ZenithPolySynth.h>
#include <zenith_core/instruments/ZenithPolySynthDefs.h>
#include <zenith_core/instruments/ZenithPresetManager.h>
#include <zenith_network/network/AudioAnalysisService.h>
#include <atomic>
#include <cmath>
#include <functional>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_graphics/juce_graphics.h>
#include <memory>
#include <mutex>
#include <random>
#include <vector>

namespace zenith {
namespace ai {

//==============================================================================
/**
    Target Role for evolution context
*/
enum class TargetRole {
  Bass,   // Low frequency, mono-compatible, solid fundamental
  Lead,   // High harmonic content, present, piercing
  Pad,    // Evolving, wide stereo, mid-range warmth
  FX,     // Noisy, high dynamic range, weird spectral characteristics
  General // Balanced (default behavior)
};

//==============================================================================
/**
    Evolution configuration parameters
*/
struct EvolutionStats {
  int generation = 0;
  int totalEvaluated = 0;
  int totalDead = 0;
  int totalSurvived = 0;

  float averageFitness = 0.0f;
  float bestFitness = 0.0f;
  float worstFitness = 1.0f;

  juce::String bestPresetName;
  juce::Time lastUpdateTime;

  EvolutionStats() : lastUpdateTime(juce::Time::getCurrentTime()) {}

  // Helper methods
  void update(int currentGen, const std::vector<Individual>& population);
  juce::String toString() const;
  float getSurvivalRate() const;
  float getDeathRate() const;
  bool hasConverged(float threshold = 0.01f, int windowSize = 10) const;
  juce::String getElapsedTime() const;
};

//==============================================================================
/**
    Main Preset Geneticist Agent

    Evolves synthesizer patches using genetic algorithms with audio-based
    fitness evaluation.
*/

} // namespace
