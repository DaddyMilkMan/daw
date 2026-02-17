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
struct EvolutionConfig {
  // Target Context
  TargetRole targetRole = TargetRole::General;

  // Population size
  int populationSize = 50;

  // Selection
  float elitismRatio = 0.1f;     // Top 10% survive automatically
  float fitnessThreshold = 0.4f; // Minimum fitness to reproduce

  // Reproduction
  float crossoverRate = 0.8f;    // 80% chance of crossover
  float mutationRate = 0.15f;    // 15% chance per gene
  float mutationStrength = 0.2f; // Max mutation deviation (±20%)

  // Fitness weights
  float harmonicRichnessWeight = 0.4f;
  float dynamicRangeWeight = 0.2f;
  float spectralBalanceWeight = 0.2f;
  float uniquenessWeight = 0.2f;

  // Audio rendering
  double renderSampleRate = 44100.0;
  float renderDurationSeconds = 1.0f; // 1 second test note
  int renderMidiNote = 60;            // C3
  float renderVelocity = 0.8f;        // 80% velocity

  // Thresholds for survival
  float silenceThresholdDb = -60.0f; // Below this = silent = dead
  float clippingThresholdDb = 0.0f;  // Above this = clipping = dead
  float minDynamicRangeDb = 6.0f;    // Minimum dynamic range

  // Batch saving
  int saveEveryNGenerations = 10; // Save elite every N generations
  int maxPresetsToSave = 10;      // Max presets saved per batch
};

//==============================================================================
/**
    Individual in the population - a synth patch with its fitness score
*/

} // namespace
