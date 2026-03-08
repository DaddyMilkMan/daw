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

// Forward declarations
struct EvolutionConfig;

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
struct Individual {
  Preset preset;
  float fitness = 0.0f;
  bool evaluated = false;

  // Audio characteristics (from evaluation)
  float peakDb = -100.0f;
  float rmsDb = -100.0f;
  float harmonicRichness = 0.0f;
  float dynamicRange = 0.0f;
  float spectralCentroid = 0.0f;

  // Death flags
  bool isDead = false;
  juce::String deathReason;

  Individual() = default;
  explicit Individual(const Preset &p) : preset(p) {}

  bool operator<(const Individual &other) const {
    return fitness > other.fitness; // Higher fitness = better
  }

  // Helper methods
  float calculateFitness(const EvolutionConfig& config) const;
  bool isViable(const EvolutionConfig& config);
  juce::String toString() const;
  Individual crossover(const Individual& other, const EvolutionConfig& config,
                      std::mt19937& rng) const;
  void mutate(const EvolutionConfig& config, std::mt19937& rng);
  static Individual createRandom(const EvolutionConfig& config,
                                 const juce::String& name,
                                 std::mt19937& rng);
};

//==============================================================================
/**
    Evolution statistics for monitoring
*/

} // namespace
