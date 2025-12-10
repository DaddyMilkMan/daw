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

#include "../instruments/ZenithPolySynth.h"
#include "../instruments/ZenithPolySynthDefs.h"
#include "../instruments/ZenithPresetManager.h"
#include "../network/AudioAnalysisService.h"
#include <atomic>
#include <cmath>
#include <functional>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_dsp/juce_dsp.h>
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

  // Target Matching
  bool useTargetMatching = false; // If true, evolves to match target audio
};

//==============================================================================
/**
    Individual in the population - a synth patch with its fitness score
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

  // Spectrum for visualization (Frequency magnitudes)
  std::vector<float> spectrum;

  // Death flags
  bool isDead = false;
  juce::String deathReason;

  Individual() = default;
  explicit Individual(const Preset &p) : preset(p) {}

  bool operator<(const Individual &other) const {
    return fitness > other.fitness; // Higher fitness = better
  }
};

//==============================================================================
/**
    Evolution statistics for monitoring
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
};

//==============================================================================
/**
    Main Preset Geneticist Agent

    Evolves synthesizer patches using genetic algorithms with audio-based
    fitness evaluation.
*/
class PresetGeneticistAgent : public juce::Thread,
                              public juce::ChangeBroadcaster {
public:
  //==========================================================================
  PresetGeneticistAgent();
  ~PresetGeneticistAgent() override;

  //==========================================================================
  // Evolution Control
  //==========================================================================

  /**
   * @brief Start the evolution process
   * @param maxGenerations Maximum generations to evolve (0 = infinite)
   */
  void startEvolution(int maxGenerations = 100);

  /**
   * @brief Stop the evolution process gracefully
   */
  void stopEvolution();

  /**
   * @brief Pause evolution (can be resumed)
   */
  void pauseEvolution();

  /**
   * @brief Resume paused evolution
   */
  void resumeEvolution();

  /**
   * @brief Check if evolution is running
   */
  bool isRunning() const { return isRunning_.load(); }

  /**
   * @brief Check if evolution is paused
   */
  bool isPaused() const { return isPaused_.load(); }

  //==========================================================================
  // Population Management
  //==========================================================================

  /**
   * @brief Seed the initial population with random patches
   */
  void seedPopulation();

  /**
   * @brief Seed the population with existing presets
   * @param presets Vector of presets to use as initial population
   */
  void seedPopulation(const std::vector<Preset> &presets);

  /**
   * @brief Get the current population
   */
  std::vector<Individual> getPopulation() const;

  /**
   * @brief Get the best individuals from current population
   * @param count Number of top individuals to return
   */
  std::vector<Individual> getBestIndividuals(int count = 10) const;

  /**
   * @brief Get current population size
   */
  int getPopulationSize() const;

  //==========================================================================
  // Configuration
  //==========================================================================

  /**
   * @brief Get current evolution configuration
   */
  EvolutionConfig &getConfig() { return config_; }
  const EvolutionConfig &getConfig() const { return config_; }

  /**
   * @brief Set evolution configuration
   */
  void setConfig(const EvolutionConfig &config) { config_ = config; }

  //==========================================================================
  // Statistics
  //==========================================================================

  /**
   * @brief Get current evolution statistics
   */
  EvolutionStats getStats() const;

  /**
   * @brief Get current generation number
   */
  int getCurrentGeneration() const { return stats_.generation; }

  /**
   * @brief Get best fitness achieved
   */
  float getBestFitness() const { return stats_.bestFitness; }

  //==========================================================================
  // Export
  //==========================================================================

  /**
   * @brief Export the best evolved presets to disk
   * @param count Number of top presets to export
   * @return Number of presets successfully saved
   */
  int exportBestPresets(int count = 10);

  /**
   * @brief Get the output directory for saved presets
   */
  /**
   * @brief Get the output directory for saved presets
   */
  juce::File getOutputDirectory() const;

  //==========================================================================
  // Target Matching
  //==========================================================================

  /**
   * @brief Set a target audio file to mimic
   * @param file The audio file to analyze and match
   */
  void setTargetAudio(const juce::File &file);

  /**
   * @brief Set target from an audio buffer
   */
  /**
   * @brief Set target from an audio buffer
   */
  void setTargetAudio(const juce::AudioBuffer<float> &buffer);

  //==========================================================================
  // Visualization Data Expose
  //==========================================================================

  /**
   * @brief Get the current best spectrum for visualization
   * Thread-safe.
   */
  std::vector<float> getCurrentBestSpectrum() const;

  /**
   * @brief Get the target spectrum for visualization
   * Thread-safe.
   */
  std::vector<float> getTargetSpectrum() const;

  //==========================================================================
  // Listeners
  //==========================================================================

  class Listener {
  public:
    virtual ~Listener() = default;
    virtual void generationCompleted(int generation, float bestFitness) = 0;
    virtual void evolutionCompleted(const std::vector<Individual> &best) = 0;
    virtual void individualEvaluated(const Individual &individual) = 0;
  };

  void addListener(Listener *listener);
  void removeListener(Listener *listener);

private:
  //==========================================================================
  // Thread implementation
  void run() override;

  //==========================================================================
  // Genetic Algorithm Core
  //==========================================================================

  /**
   * @brief Evaluate the fitness of an individual
   */
  void evaluateIndividual(Individual &individual);

  /**
   * @brief Evaluate the entire population
   */
  void evaluatePopulation();

  /**
   * @brief Select parents for reproduction using tournament selection
   */
  std::pair<Individual *, Individual *> selectParents();

  /**
   * @brief Perform crossover between two parents
   */
  Individual crossover(const Individual &parent1, const Individual &parent2);

  /**
   * @brief Mutate an individual's genes
   */
  void mutate(Individual &individual);

  /**
   * @brief Create the next generation from current population
   */
  void evolveGeneration();

  //==========================================================================
  // Preset Generation
  //==========================================================================

  /**
   * @brief Generate a completely random patch
   */
  Preset generateRandomPatch();

  /**
   * @brief Mix parameters from two presets (crossover)
   */
  std::map<juce::String, float>
  crossoverParameters(const std::map<juce::String, float> &p1,
                      const std::map<juce::String, float> &p2);

  /**
   * @brief Randomly mutate a parameter value
   */
  float mutateParameter(float value, float strength);

  //==========================================================================
  // Audio Rendering & Analysis
  //==========================================================================

  /**
   * @brief Render a preset to an audio buffer
   */
  juce::AudioBuffer<float> renderPreset(const Preset &preset);

  /**
   * @brief Analyze audio buffer for fitness scoring
   */
  void analyzeAudio(const juce::AudioBuffer<float> &buffer,
                    Individual &individual);

  /**
   * @brief Calculate harmonic richness from spectral data
   */
  /**
   * @brief Calculate spectral centroid from pre-computed spectrum
   */
  float calculateSpectralCentroid(const std::vector<float> &spectrum,
                                  float sampleRate);

  /**
   * @brief Calculate harmonic richness from pre-computed spectrum
   */
  float calculateHarmonicRichness(const std::vector<float> &spectrum);

  /**
   * @brief Compute magnitude spectrum from audio buffer
   */
  std::vector<float> computeSpectrum(const juce::AudioBuffer<float> &buffer);

  /**
   * @brief Check if audio is silent
   */
  bool isSilent(const juce::AudioBuffer<float> &buffer);

  /**
   * @brief Check if audio is clipping
   */
  bool isClipping(const juce::AudioBuffer<float> &buffer);

  /**
   * @brief Calculate peak and RMS levels
   */
  std::pair<float, float>
  calculateLevels(const juce::AudioBuffer<float> &buffer);

  /**
   * @brief Calculate fitness based on similarity to target
   */
  float calculatesimilarity(const Individual &candidate,
                            const Individual &target);

  //==========================================================================
  // Helpers
  //==========================================================================

  void updateStats();
  void saveElitePresets();
  juce::String generateBatchFolderName() const;
  float randomFloat(float min = 0.0f, float max = 1.0f);
  int randomInt(int min, int max);

  //==========================================================================
  // Member Variables
  //==========================================================================

  EvolutionConfig config_;
  EvolutionStats stats_;

  // Population
  std::vector<Individual> population_;
  mutable std::mutex populationMutex_;

  // Evolution state
  std::atomic<bool> isRunning_{false};
  std::atomic<bool> isPaused_{false};
  int maxGenerations_ = 100;

  // Synth processor for rendering
  std::unique_ptr<ZenithPolySynthProcessor> synthProcessor_;

  // FFT for spectral analysis
  juce::dsp::FFT fft_{10}; // 1024-point FFT

  // Target Matching
  juce::AudioBuffer<float> targetAudioBuffer_;
  Individual targetFeatures_; // Stores the analyzed features of the target
  bool hasTarget_ = false;

  // Visualization Data
  std::vector<float> currentBestSpectrum_;
  std::vector<float> targetSpectrum_;
  mutable std::mutex spectrumMutex_;

  // ==== PRE-ALLOCATED BUFFERS (Avoid heap allocation in render loop) ====
  // Block buffer for rendering - pre-allocated to max block size
  juce::AudioBuffer<float> blockBuffer_;

  // FFT data buffer - pre-allocated for spectral analysis
  std::vector<float> fftData_;

  // MIDI buffer for block processing
  juce::MidiBuffer blockMidiBuffer_;
  // ======================================================================

  // Random number generator
  std::mt19937 rng_;

  // Listeners
  juce::ListenerList<Listener> listeners_;

  // Output directory
  juce::File outputDirectory_;

  // Parameter definitions for ZenithPolySynth
  static const std::vector<juce::String> &getParameterIds();
  static const std::map<juce::String, std::pair<float, float>> &
  getParameterRanges();

  // Discrete parameter registry - these need integer-snapping during mutation
  static const std::set<juce::String> &getDiscreteParameters();

  // Helper: Check if a parameter is discrete (needs integer snapping)
  static bool isDiscreteParameter(const juce::String &paramId);

  // Helper: Get the number of discrete steps for a parameter (0 = continuous)
  static int getDiscreteSteps(const juce::String &paramId);

  //==========================================================================
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetGeneticistAgent)
};

} // namespace ai
} // namespace zenith
