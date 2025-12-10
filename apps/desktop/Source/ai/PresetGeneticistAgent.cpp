/*
  ==============================================================================

    PresetGeneticistAgent.cpp
    Created: 2025-12-07
    Author:  Zenith DAW AI Team

    Implementation of the Preset Geneticist Agent - Evolutionary Sound Design

  ==============================================================================
*/

#include "PresetGeneticistAgent.h"
#include "../instruments/PresetGenerator.h"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace zenith {
namespace ai {

//==============================================================================
// Configuration Constants
//==============================================================================
namespace {
constexpr int kFFTSize = 1024;
constexpr int kRenderBlockSize = 512;
constexpr float kNoteOffTimeFraction = 0.8f; // 80% of duration
constexpr float kBlendProbability = 0.2f;
constexpr float kClippingThreshold = 0.999f;
constexpr size_t kMaxHistorySize = 50;
} // namespace

//==============================================================================
// Static parameter data for ZenithPolySynth
//==============================================================================

const std::vector<juce::String> &PresetGeneticistAgent::getParameterIds() {
  static const std::vector<juce::String> ids = {
      // Oscillator 1
      "osc1_wave", "osc1_detune", "osc1_mix",
      // Oscillator 2
      "osc2_wave", "osc2_detune", "osc2_mix",
      // Oscillator 3
      "osc3_wave", "osc3_detune", "osc3_mix",
      // Noise & Sub
      "noise_level", "sub_osc_level",
      // Unison
      "unison_voices", "unison_detune",
      // Filter
      "filter_type", "filter_cutoff", "filter_resonance", "filter_drive",
      "filter_env_amount",
      // Amp Envelope
      "amp_attack", "amp_decay", "amp_sustain", "amp_release",
      // Mod Envelope
      "mod_attack", "mod_decay", "mod_sustain", "mod_release",
      // LFO 1
      "lfo1_rate", "lfo1_amount", "lfo1_target",
      // LFO 2
      "lfo2_rate", "lfo2_amount", "lfo2_target",
      // Global
      "glide_time", "mono_mode", "master_gain",
      // Effects
      "distortion_amount", "chorus_amount", "reverb_amount"};
  return ids;
}

const std::map<juce::String, std::pair<float, float>> &
PresetGeneticistAgent::getParameterRanges() {
  // Parameter ranges: {min, max} in normalized 0-1 space
  // Some parameters have specific ranges that affect sound design
  static const std::map<juce::String, std::pair<float, float>> ranges = {
      // Oscillators (0-1 for most, specific ranges for some)
      {"osc1_wave", {0.0f, 1.0f}},
      {"osc1_detune", {0.3f, 0.7f}}, // Keep detune subtle
      {"osc1_mix", {0.0f, 1.0f}},
      {"osc2_wave", {0.0f, 1.0f}},
      {"osc2_detune", {0.3f, 0.7f}},
      {"osc2_mix", {0.0f, 1.0f}},
      {"osc3_wave", {0.0f, 1.0f}},
      {"osc3_detune", {0.3f, 0.7f}},
      {"osc3_mix", {0.0f, 0.7f}},    // Often lower for third osc
      {"noise_level", {0.0f, 0.3f}}, // Noise usually subtle
      {"sub_osc_level", {0.0f, 0.6f}},

      // Unison
      {"unison_voices", {0.0f, 1.0f}},
      {"unison_detune", {0.1f, 0.5f}}, // Subtle to moderate

      // Filter
      {"filter_type", {0.0f, 1.0f}},
      {"filter_cutoff", {0.2f, 1.0f}},    // Avoid very low cutoff
      {"filter_resonance", {0.0f, 0.8f}}, // Avoid extreme resonance
      {"filter_drive", {0.0f, 0.6f}},
      {"filter_env_amount", {0.0f, 1.0f}},

      // Amp Envelope (ensure notes are audible)
      {"amp_attack", {0.0f, 0.4f}}, // Not too slow
      {"amp_decay", {0.1f, 0.7f}},
      {"amp_sustain", {0.3f, 1.0f}}, // Ensure sustain
      {"amp_release", {0.05f, 0.5f}},

      // Mod Envelope
      {"mod_attack", {0.0f, 0.5f}},
      {"mod_decay", {0.1f, 0.8f}},
      {"mod_sustain", {0.0f, 1.0f}},
      {"mod_release", {0.05f, 0.6f}},

      // LFO
      {"lfo1_rate", {0.0f, 0.7f}},   // Not too fast
      {"lfo1_amount", {0.0f, 0.5f}}, // Subtle modulation
      {"lfo1_target", {0.0f, 1.0f}},
      {"lfo2_rate", {0.0f, 0.7f}},
      {"lfo2_amount", {0.0f, 0.5f}},
      {"lfo2_target", {0.0f, 1.0f}},

      // Global
      {"glide_time", {0.0f, 0.3f}},
      {"mono_mode", {0.0f, 1.0f}},
      {"master_gain", {0.5f, 0.9f}}, // Reasonable gain range

      // Effects
      {"distortion_amount", {0.0f, 0.5f}},
      {"chorus_amount", {0.0f, 0.7f}},
      {"reverb_amount", {0.0f, 0.6f}}};
  return ranges;
}

//==============================================================================
// Constructor / Destructor
//==============================================================================

PresetGeneticistAgent::PresetGeneticistAgent()
    : Thread("PresetGeneticistAgent") {
  // Seed random number generator with current time
  rng_.seed(static_cast<unsigned long>(juce::Time::currentTimeMillis()));

  // Initialize output directory
  auto userDir =
      juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
  outputDirectory_ =
      userDir.getChildFile("Zenith").getChildFile("AI_Generated_Presets");
  outputDirectory_.createDirectory();

  // Pre-initialize the synth processor for offline rendering
  synthProcessor_ = std::make_unique<ZenithPolySynthProcessor>();
  synthProcessor_->prepareToPlay(
      config_.renderSampleRate,
      static_cast<int>(config_.renderSampleRate *
                       config_.renderDurationSeconds));
}

PresetGeneticistAgent::~PresetGeneticistAgent() {
  stopEvolution();
  stopThread(2000);
}

//==============================================================================
// Evolution Control
//==============================================================================

void PresetGeneticistAgent::startEvolution(int maxGenerations) {
  if (isRunning_.load())
    return;

  maxGenerations_ = maxGenerations;
  isPaused_.store(false);
  isRunning_.store(true);

  // Seed population if empty
  if (population_.empty()) {
    seedPopulation();
  }

  // Start the evolution thread
  startThread();
}

void PresetGeneticistAgent::stopEvolution() {
  isRunning_.store(false);
  isPaused_.store(false);
  signalThreadShouldExit();
}

void PresetGeneticistAgent::pauseEvolution() { isPaused_.store(true); }

void PresetGeneticistAgent::resumeEvolution() { isPaused_.store(false); }

//==============================================================================
// Target Matching
//==============================================================================

void PresetGeneticistAgent::setTargetAudio(const juce::File &file) {
  juce::AudioFormatManager formatManager;
  formatManager.registerBasicFormats();

  std::unique_ptr<juce::AudioFormatReader> reader(
      formatManager.createReaderFor(file));
  if (reader) {
    juce::AudioBuffer<float> buffer(static_cast<int>(reader->numChannels),
                                    static_cast<int>(reader->lengthInSamples));
    reader->read(&buffer, 0, static_cast<int>(reader->lengthInSamples), 0, true,
                 true);
    setTargetAudio(buffer);
  }
}

void PresetGeneticistAgent::setTargetAudio(
    const juce::AudioBuffer<float> &buffer) {
  targetAudioBuffer_ = buffer;
  hasTarget_ = true;
  config_.useTargetMatching = true;

  // Analyze the target immediately to get its feature footprint
  targetFeatures_.preset.name = "Target"; // Dummy
  analyzeAudio(targetAudioBuffer_, targetFeatures_);

  DBG("Target Analyzed. Centroid: " +
      juce::String(targetFeatures_.spectralCentroid) +
      ", Richness: " + juce::String(targetFeatures_.harmonicRichness) +
      ", RMS: " + juce::String(targetFeatures_.rmsDb));

  {
    std::lock_guard<std::mutex> lock(spectrumMutex_);
    targetSpectrum_ = targetFeatures_.spectrum;
  }
}

std::vector<float> PresetGeneticistAgent::getCurrentBestSpectrum() const {
  std::lock_guard<std::mutex> lock(spectrumMutex_);
  return currentBestSpectrum_;
}

std::vector<float> PresetGeneticistAgent::getTargetSpectrum() const {
  std::lock_guard<std::mutex> lock(spectrumMutex_);
  return targetSpectrum_;
}

//==============================================================================
// Population Management
//==============================================================================

void PresetGeneticistAgent::seedPopulation() {
  std::lock_guard<std::mutex> lock(populationMutex_);

  population_.clear();
  population_.reserve(static_cast<size_t>(config_.populationSize));

  for (int i = 0; i < config_.populationSize; ++i) {
    Preset randomPatch = generateRandomPatch();
    population_.emplace_back(randomPatch);
  }

  stats_.generation = 0;
  stats_.totalEvaluated = 0;
  stats_.totalDead = 0;
  stats_.totalSurvived = 0;
}

void PresetGeneticistAgent::seedPopulation(const std::vector<Preset> &presets) {
  std::lock_guard<std::mutex> lock(populationMutex_);

  population_.clear();
  population_.reserve(static_cast<size_t>(config_.populationSize));

  // Add provided presets
  for (const auto &preset : presets) {
    if (static_cast<int>(population_.size()) >= config_.populationSize)
      break;
    population_.emplace_back(preset);
  }

  // Fill remaining with random patches
  while (static_cast<int>(population_.size()) < config_.populationSize) {
    Preset randomPatch = generateRandomPatch();
    population_.emplace_back(randomPatch);
  }

  stats_.generation = 0;
}

std::vector<Individual> PresetGeneticistAgent::getPopulation() const {
  std::lock_guard<std::mutex> lock(populationMutex_);
  return population_;
}

std::vector<Individual>
PresetGeneticistAgent::getBestIndividuals(int count) const {
  std::lock_guard<std::mutex> lock(populationMutex_);

  std::vector<Individual> sorted = population_;
  std::sort(sorted.begin(), sorted.end()); // Higher fitness first

  if (count > static_cast<int>(sorted.size())) {
    count = static_cast<int>(sorted.size());
  }

  return std::vector<Individual>(sorted.begin(), sorted.begin() + count);
}

int PresetGeneticistAgent::getPopulationSize() const {
  std::lock_guard<std::mutex> lock(populationMutex_);
  return static_cast<int>(population_.size());
}

//==============================================================================
// Statistics
//==============================================================================

EvolutionStats PresetGeneticistAgent::getStats() const { return stats_; }

//==============================================================================
// Export
//==============================================================================

int PresetGeneticistAgent::exportBestPresets(int count) {
  auto best = getBestIndividuals(count);
  int saved = 0;

  // Create batch folder with timestamp
  juce::String batchName = generateBatchFolderName();
  juce::File batchDir = outputDirectory_.getChildFile(batchName);
  batchDir.createDirectory();

  auto &presetManager = ZenithPresetManager::getInstance();

  for (size_t i = 0; i < best.size() && saved < config_.maxPresetsToSave; ++i) {
    const auto &individual = best[i];
    if (individual.isDead || individual.fitness < config_.fitnessThreshold)
      continue;

    // Create preset with evolved name
    Preset preset = individual.preset;
    preset.name = "Evolved_Gen" + juce::String(stats_.generation) + "_" +
                  juce::String(static_cast<int>(i + 1));
    preset.author = "Preset Geneticist AI";
    preset.category = "AI_Evolved";
    preset.instrumentId = "ZenithPolySynth";

    // Add fitness score to tags
    preset.tags.push_back("fitness:" +
                          juce::String(individual.fitness, 2).toStdString());
    preset.tags.push_back("generation:" + std::to_string(stats_.generation));

    // Save using preset manager
    if (presetManager.savePreset(preset, true)) {
      saved++;
    }
  }

  DBG("PresetGeneticistAgent: Exported " << saved << " presets to "
                                         << batchDir.getFullPathName());

  return saved;
}

juce::File PresetGeneticistAgent::getOutputDirectory() const {
  return outputDirectory_;
}

//==============================================================================
// Listeners
//==============================================================================

void PresetGeneticistAgent::addListener(Listener *listener) {
  listeners_.add(listener);
}

void PresetGeneticistAgent::removeListener(Listener *listener) {
  listeners_.remove(listener);
}

//==============================================================================
// Thread Implementation
//==============================================================================

void PresetGeneticistAgent::run() {
  DBG("PresetGeneticistAgent: Evolution started");

  while (!threadShouldExit() && isRunning_.load()) {
    // Handle pause
    while (isPaused_.load() && !threadShouldExit()) {
      wait(100);
    }

    if (threadShouldExit())
      break;

    // Check generation limit
    if (maxGenerations_ > 0 && stats_.generation >= maxGenerations_) {
      DBG("PresetGeneticistAgent: Reached max generations");
      break;
    }

    // PHASE 1: EVALUATE - Score all unevaluated individuals
    evaluatePopulation();

    // PHASE 2: UPDATE STATS
    updateStats();

    // PHASE 3: EVOLVE - Create next generation
    evolveGeneration();

    // Increment generation
    stats_.generation++;

    // Notify listeners
    listeners_.call([this](Listener &l) {
      l.generationCompleted(stats_.generation, stats_.bestFitness);
    });

    // Save elite presets periodically
    if (stats_.generation % config_.saveEveryNGenerations == 0) {
      saveElitePresets();
    }

    // Throttle to prevent CPU exhaustion
    wait(100);
  }

  // Final save of best presets
  saveElitePresets();

  // Notify completion
  auto best = getBestIndividuals(config_.maxPresetsToSave);
  listeners_.call([&best](Listener &l) { l.evolutionCompleted(best); });

  isRunning_.store(false);
  DBG("PresetGeneticistAgent: Evolution completed. Generation: "
      << stats_.generation << ", Best Fitness: " << stats_.bestFitness);
}

//==============================================================================
// Genetic Algorithm Core
//==============================================================================

void PresetGeneticistAgent::evaluateIndividual(Individual &individual) {
  if (individual.evaluated)
    return;

  // Render the preset to audio
  juce::AudioBuffer<float> buffer = renderPreset(individual.preset);

  // Check death conditions FIRST
  if (isSilent(buffer)) {
    individual.isDead = true;
    individual.deathReason = "Silent";
    individual.fitness = 0.0f;
    individual.evaluated = true;
    stats_.totalDead++;
    return;
  }

  if (isClipping(buffer)) {
    individual.isDead = true;
    individual.deathReason = "Clipping";
    individual.fitness = 0.0f;
    individual.evaluated = true;
    stats_.totalDead++;
    return;
  }

  // Analyze audio characteristics
  analyzeAudio(buffer, individual);

  // Calculate fitness score
  float fitness = 0.0f;

  if (config_.useTargetMatching && hasTarget_) {
    // TARGET MATCHING MODE
    fitness = calculatesimilarity(individual, targetFeatures_);
  } else {
    // TRADITIONAL ROLE-BASED SCORING
    switch (config_.targetRole) {
    case TargetRole::Bass: {
      // Bass: Low centroid, solid fundamental, mono compatible

      // 1. Spectral Centroid (Ideal: 100-300 Hz)
      float idealCentroid = 200.0f;
      float centroidScore = std::max(
          0.0f, 1.0f - (std::abs(individual.spectralCentroid - idealCentroid) /
                        500.0f));
      fitness += centroidScore * 0.5f;

      // 2. Harmonic Richness (Punish high richness/noise for sub bass, reward
      // for rich bass) We want some richness but not white noise
      float richnessScore = 1.0f - std::abs(individual.harmonicRichness - 0.3f);
      fitness += richnessScore * 0.3f;

      // 3. Dynamic Range (Bass should be relatively consistent)
      float dynamicScore =
          1.0f - juce::jlimit(0.0f, 1.0f, individual.dynamicRange / 20.0f);
      fitness += dynamicScore * 0.2f;
      break;
    }

    case TargetRole::Lead: {
      // Lead: High harmonic content, present, piercing

      // 1. Harmonic Richness (Reward high)
      fitness += individual.harmonicRichness * 0.5f;

      // 2. Spectral Centroid (Ideal: 1500-3000 Hz)
      float idealCentroid = 2000.0f;
      float centroidScore = std::max(
          0.0f, 1.0f - (std::abs(individual.spectralCentroid - idealCentroid) /
                        2000.0f));
      fitness += centroidScore * 0.3f;

      // 3. Dynamic Range (Moderate)
      float dynamicNorm =
          juce::jlimit(0.0f, 1.0f, individual.dynamicRange / 30.0f);
      fitness += dynamicNorm * 0.2f;
      break;
    }

    case TargetRole::Pad: {
      // Pad: Evolving, mid-range warmth, dynamic

      // 1. Dynamic Range (Reward high dynamics for evolving pads)
      float dynamicNorm =
          juce::jlimit(0.0f, 1.0f, individual.dynamicRange / 40.0f);
      fitness += dynamicNorm * 0.4f;

      // 2. Harmonic Richness (Rich but not harsh)
      fitness += individual.harmonicRichness * 0.3f;

      // 3. Spectral Centroid (Ideal: 500-1500 Hz)
      float idealCentroid = 1000.0f;
      float centroidScore = std::max(
          0.0f, 1.0f - (std::abs(individual.spectralCentroid - idealCentroid) /
                        1000.0f));
      fitness += centroidScore * 0.3f;
      break;
    }

    case TargetRole::FX: {
      // FX: Extreme values preferred

      // 1. Uniqueness (Extreme Centroid or Richness)
      float centroidExtremity =
          std::abs(individual.spectralCentroid - 2000.0f) / 2000.0f;
      fitness += centroidExtremity * 0.4f;

      // 2. Dynamic Range (High)
      float dynamicNorm =
          juce::jlimit(0.0f, 1.0f, individual.dynamicRange / 50.0f);
      fitness += dynamicNorm * 0.4f;

      // 3. Richness (High)
      fitness += individual.harmonicRichness * 0.2f;
      break;
    }

    case TargetRole::General:
    default: {
      // Balanced Profile
      fitness += individual.harmonicRichness * config_.harmonicRichnessWeight;

      float dynamicRangeNorm =
          juce::jlimit(0.0f, 1.0f, individual.dynamicRange / 40.0f);
      fitness += dynamicRangeNorm * config_.dynamicRangeWeight;

      float idealCentroid = 1500.0f;
      float centroidDiff =
          std::abs(individual.spectralCentroid - idealCentroid);
      float centroidScore = std::max(0.0f, 1.0f - (centroidDiff / 3000.0f));
      fitness += centroidScore * config_.spectralBalanceWeight;

      float idealRms = -15.0f;
      float rmsDiff = std::abs(individual.rmsDb - idealRms);
      float rmsScore = std::max(0.0f, 1.0f - (rmsDiff / 20.0f));
      fitness += rmsScore * config_.uniquenessWeight;
      break;
    }
    }
  }

  individual.fitness = juce::jlimit(0.0f, 1.0f, fitness);
  individual.evaluated = true;

  stats_.totalEvaluated++;
  stats_.totalSurvived++;

  // Notify listener
  listeners_.call(
      [&individual](Listener &l) { l.individualEvaluated(individual); });
}

void PresetGeneticistAgent::evaluatePopulation() {
  std::lock_guard<std::mutex> lock(populationMutex_);

  for (auto &individual : population_) {
    if (threadShouldExit())
      break;

    evaluateIndividual(individual);
  }
}

std::pair<Individual *, Individual *> PresetGeneticistAgent::selectParents() {
  // Tournament selection: pick 3 random individuals, return the two fittest
  std::vector<Individual *> candidates;

  {
    std::lock_guard<std::mutex> lock(populationMutex_);
    if (population_.size() < 3) {
      return {nullptr, nullptr};
    }

    // Pick 5 random indices for tournament
    std::vector<size_t> indices;
    for (int i = 0; i < 5; ++i) {
      indices.push_back(static_cast<size_t>(
          randomInt(0, static_cast<int>(population_.size()) - 1)));
    }

    for (size_t idx : indices) {
      candidates.push_back(&population_[idx]);
    }
  }

  // Sort by fitness (higher first)
  std::sort(candidates.begin(), candidates.end(),
            [](Individual *a, Individual *b) { return *a < *b; });

  // Return top 2, but ensure they're not dead
  Individual *p1 = nullptr;
  Individual *p2 = nullptr;

  for (auto *candidate : candidates) {
    if (!candidate->isDead && candidate->fitness > config_.fitnessThreshold) {
      if (!p1)
        p1 = candidate;
      else if (!p2) {
        p2 = candidate;
        break;
      }
    }
  }

  return {p1, p2};
}

Individual PresetGeneticistAgent::crossover(const Individual &parent1,
                                            const Individual &parent2) {
  Individual child;
  child.preset.instrumentId = "ZenithPolySynth";
  child.preset.name = "Crossover_" + juce::String(stats_.generation);
  child.preset.author = "Preset Geneticist AI";
  child.preset.category = "AI_Evolved";

  // Crossover parameters
  child.preset.parameters =
      crossoverParameters(parent1.preset.parameters, parent2.preset.parameters);

  return child;
}

void PresetGeneticistAgent::mutate(Individual &individual) {
  // Manual mutation to avoid type mismatch
  for (auto &param : individual.preset.parameters) {
    if (randomFloat() < 0.5f) { // Mutate 50% of params
      float delta =
          randomFloat(-config_.mutationStrength, config_.mutationStrength);
      param.second = juce::jlimit(0.0f, 1.0f, param.second + delta);
    }
  }

  // Mark as needing re-evaluation
  individual.evaluated = false;
  individual.isDead = false;
  individual.deathReason = "";
}

float PresetGeneticistAgent::calculatesimilarity(const Individual &candidate,
                                                 const Individual &target) {
  float score = 0.0f;

  // 1. Spectral Centroid Similarity (Weighted 40%)
  // Normalized difference. 5000Hz deviance = 0 score.
  float centroidDiff =
      std::abs(candidate.spectralCentroid - target.spectralCentroid);
  float centroidScore = std::max(0.0f, 1.0f - (centroidDiff / 2000.0f));
  score += centroidScore * 0.4f;

  // 2. Harmonic Richness Similarity (Weighted 30%)
  float richnessDiff =
      std::abs(candidate.harmonicRichness - target.harmonicRichness);
  float richnessScore = std::max(0.0f, 1.0f - (richnessDiff * 2.0f));
  score += richnessScore * 0.3f;

  // 3. Dynamic Range Similarity (Weighted 20%)
  float dynamicDiff = std::abs(candidate.dynamicRange - target.dynamicRange);
  float dynamicScore = std::max(0.0f, 1.0f - (dynamicDiff / 20.0f));
  score += dynamicScore * 0.2f;

  // 4. RMS Level Similarity (Weighted 10%) - ensuring rough volume match
  float rmsDiff = std::abs(candidate.rmsDb - target.rmsDb);
  float rmsScore = std::max(0.0f, 1.0f - (rmsDiff / 20.0f));
  score += rmsScore * 0.1f;

  return score;
}

void PresetGeneticistAgent::evolveGeneration() {
  std::lock_guard<std::mutex> lock(populationMutex_);

  // Sort population by fitness
  std::sort(population_.begin(), population_.end());

  // Calculate elite count
  int eliteCount = static_cast<int>(std::ceil(
      static_cast<float>(config_.populationSize) * config_.elitismRatio));
  eliteCount =
      std::max(1, std::min(eliteCount, static_cast<int>(population_.size())));

  // Create next generation
  std::vector<Individual> nextGeneration;
  nextGeneration.reserve(static_cast<size_t>(config_.populationSize));

  // Keep elite individuals (top performers survive)
  for (int i = 0; i < eliteCount && i < static_cast<int>(population_.size());
       ++i) {
    if (!population_[static_cast<size_t>(i)].isDead) {
      nextGeneration.push_back(population_[static_cast<size_t>(i)]);
    }
  }

  // Fill the rest with offspring
  while (static_cast<int>(nextGeneration.size()) < config_.populationSize) {
    auto [p1, p2] = selectParents();

    if (!p1 || !p2) {
      // Not enough fit parents - create random individual
      Individual random(generateRandomPatch());
      nextGeneration.push_back(random);
      continue;
    }

    // Crossover
    Individual child;
    if (randomFloat() < config_.crossoverRate) {
      child = crossover(*p1, *p2);
    } else {
      // Copy fitter parent
      child = (*p1 < *p2) ? *p1 : *p2;
      child.evaluated = false;
    }

    // Mutation
    mutate(child);

    nextGeneration.push_back(child);
  }

  // Replace population
  population_ = std::move(nextGeneration);
}

//==============================================================================
// Preset Generation
//==============================================================================

Preset PresetGeneticistAgent::generateRandomPatch() {
  Preset preset;
  preset.instrumentId = "ZenithPolySynth";
  preset.name = "Random_" + juce::String(juce::Time::currentTimeMillis());
  preset.author = "Preset Geneticist AI";
  preset.category = "AI_Generated";

  const auto &paramIds = getParameterIds();
  const auto &ranges = getParameterRanges();

  for (const auto &paramId : paramIds) {
    float value;
    auto it = ranges.find(paramId);
    if (it != ranges.end()) {
      value = randomFloat(it->second.first, it->second.second);
    } else {
      value = randomFloat(0.0f, 1.0f);
    }
    preset.parameters[paramId] = value;
  }

  // Ensure at least one oscillator has significant mix
  preset.parameters["osc1_mix"] = std::max(preset.parameters["osc1_mix"], 0.5f);

  return preset;
}

std::map<juce::String, float> PresetGeneticistAgent::crossoverParameters(
    const std::map<juce::String, float> &p1,
    const std::map<juce::String, float> &p2) {
  std::map<juce::String, float> child;

  // Strategy: Use multi-point crossover with gene groups
  // Group parameters logically and crossover by group

  // Define parameter groups
  static const std::vector<std::vector<juce::String>> groups = {
      // Oscillator 1 group
      {"osc1_wave", "osc1_detune", "osc1_mix"},
      // Oscillator 2 group
      {"osc2_wave", "osc2_detune", "osc2_mix"},
      // Oscillator 3 group
      {"osc3_wave", "osc3_detune", "osc3_mix"},
      // Noise/Sub group
      {"noise_level", "sub_osc_level"},
      // Unison group
      {"unison_voices", "unison_detune"},
      // Filter group
      {"filter_type", "filter_cutoff", "filter_resonance", "filter_drive",
       "filter_env_amount"},
      // Amp envelope group
      {"amp_attack", "amp_decay", "amp_sustain", "amp_release"},
      // Mod envelope group
      {"mod_attack", "mod_decay", "mod_sustain", "mod_release"},
      // LFO 1 group
      {"lfo1_rate", "lfo1_amount", "lfo1_target"},
      // LFO 2 group
      {"lfo2_rate", "lfo2_amount", "lfo2_target"},
      // Global group
      {"glide_time", "mono_mode", "master_gain"},
      // Effects group
      {"distortion_amount", "chorus_amount", "reverb_amount"}};

  // For each group, randomly choose parent
  for (const auto &group : groups) {
    bool useParent1 = randomFloat() < 0.5f;

    for (const auto &paramId : group) {
      auto it1 = p1.find(paramId);
      auto it2 = p2.find(paramId);

      float val1 = (it1 != p1.end()) ? it1->second : 0.5f;
      float val2 = (it2 != p2.end()) ? it2->second : 0.5f;

      // Sometimes do blending instead of pure inheritance
      if (randomFloat() < kBlendProbability) {
        // Blend 50/50
        child[paramId] = (val1 + val2) * 0.5f;
      } else {
        child[paramId] = useParent1 ? val1 : val2;
      }
    }
  }

  return child;
}

float PresetGeneticistAgent::mutateParameter(float value, float strength) {
  // Gaussian mutation with variable strength
  std::normal_distribution<float> dist(0.0f, strength);
  float mutation = dist(rng_);
  return juce::jlimit(0.0f, 1.0f, value + mutation);
}

//==============================================================================
// Audio Rendering & Analysis
//==============================================================================

juce::AudioBuffer<float>
PresetGeneticistAgent::renderPreset(const Preset &preset) {
  // Calculate buffer size for the render duration
  int numSamples = static_cast<int>(config_.renderSampleRate *
                                    config_.renderDurationSeconds);
  const int blockSize = kRenderBlockSize;

  // Create output buffer
  juce::AudioBuffer<float> outputBuffer(2, numSamples);
  outputBuffer.clear();

  // Prepare the synth processor
  synthProcessor_->prepareToPlay(config_.renderSampleRate, blockSize);

  // Apply preset parameters to synth
  auto &params = synthProcessor_->getParameters();
  for (const auto &[paramId, value] : preset.parameters) {
    if (auto *param = params.getParameter(paramId)) {
      param->setValueNotifyingHost(value);
    }
  }

  // Create MIDI note-on event
  juce::MidiBuffer midiBuffer;
  int midiNote = config_.renderMidiNote;
  float velocity = config_.renderVelocity;

  // Note on at sample 0
  midiBuffer.addEvent(
      juce::MidiMessage::noteOn(1, midiNote,
                                static_cast<juce::uint8>(velocity * 127.0f)),
      0);

  // Note off at 80% of duration (leave tail for release)
  int noteOffSample = static_cast<int>(numSamples * kNoteOffTimeFraction);
  midiBuffer.addEvent(juce::MidiMessage::noteOff(1, midiNote), noteOffSample);

  // Render in blocks
  int samplesRendered = 0;
  while (samplesRendered < numSamples) {
    int samplesThisBlock = std::min(blockSize, numSamples - samplesRendered);

    // Create block buffer
    juce::AudioBuffer<float> blockBuffer(2, samplesThisBlock);
    blockBuffer.clear();

    // Extract MIDI events for this block
    juce::MidiBuffer blockMidi;
    for (auto m : midiBuffer) {
      int pos = m.samplePosition;
      if (pos >= samplesRendered && pos < samplesRendered + samplesThisBlock) {
        blockMidi.addEvent(m.getMessage(), pos - samplesRendered);
      }
    }

    // Process the block
    synthProcessor_->processBlock(blockBuffer, blockMidi);

    // Copy to output buffer
    for (int ch = 0; ch < 2; ++ch) {
      outputBuffer.copyFrom(ch, samplesRendered, blockBuffer, ch, 0,
                            samplesThisBlock);
    }

    samplesRendered += samplesThisBlock;
  }

  // Release resources? NO - keep allocated for next run to avoid thrashing
  // synthProcessor_->releaseResources();

  return outputBuffer;
}

void PresetGeneticistAgent::analyzeAudio(const juce::AudioBuffer<float> &buffer,
                                         Individual &individual) {
  // Calculate peak and RMS levels
  auto [peak, rms] = calculateLevels(buffer);
  individual.peakDb = peak;
  individual.rmsDb = rms;

  // Calculate dynamic range
  individual.dynamicRange = peak - rms;

  // Compute full spectrum efficiently
  individual.spectrum = computeSpectrum(buffer);

  // Calculate harmonic richness from spectrum
  individual.harmonicRichness = calculateHarmonicRichness(individual.spectrum);

  // Calculate spectral centroid from spectrum
  individual.spectralCentroid =
      calculateSpectralCentroid(individual.spectrum, config_.renderSampleRate);
}

std::vector<float>
PresetGeneticistAgent::computeSpectrum(const juce::AudioBuffer<float> &buffer) {
  const int fftSize = kFFTSize;
  const int numSamples = buffer.getNumSamples();
  std::vector<float> spectrum(static_cast<size_t>(fftSize / 2),
                              0.0f); // Magnitude only

  if (numSamples < fftSize)
    return spectrum;

  // Prepare FFT data (time domain)
  std::vector<float> fftWorkBuffer(static_cast<size_t>(fftSize * 2), 0.0f);

  // Use mid-section
  int startSample = numSamples / 4;
  const float *data = buffer.getReadPointer(0);
  for (int i = 0; i < fftSize; ++i) {
    float window =
        0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi *
                                static_cast<float>(i) /
                                static_cast<float>(fftSize - 1)));
    fftWorkBuffer[static_cast<size_t>(i)] = data[startSample + i] * window;
  }

  // Perform FFT
  fft_.performFrequencyOnlyForwardTransform(fftWorkBuffer.data());

  // Copy magnitude to output (first half)
  for (int i = 0; i < fftSize / 2; ++i) {
    spectrum[static_cast<size_t>(i)] = fftWorkBuffer[static_cast<size_t>(i)];
  }

  return spectrum;
}

float PresetGeneticistAgent::calculateHarmonicRichness(
    const std::vector<float> &spectrum) {
  // Harmonic richness: ratio of harmonic energy to total energy
  // Approximated by spectrum flatness

  if (spectrum.empty())
    return 0.0f;

  float logSum = 0.0f;
  float linearSum = 0.0f;
  int count = 0;

  for (size_t i = 1; i < spectrum.size(); ++i) { // Skip DC
    float mag = std::abs(spectrum[i]);
    if (mag > 1e-10f) {
      logSum += std::log(mag);
      linearSum += mag;
      count++;
    }
  }

  if (count == 0 || linearSum == 0.0f)
    return 0.0f;

  float geometricMean = std::exp(logSum / static_cast<float>(count));
  float arithmeticMean = linearSum / static_cast<float>(count);
  float flatness = geometricMean / arithmeticMean;

  return 1.0f - juce::jlimit(0.0f, 1.0f, flatness);
}

float PresetGeneticistAgent::calculateSpectralCentroid(
    const std::vector<float> &spectrum, float sampleRate) {
  if (spectrum.empty())
    return 0.0f;

  const int fftSize = kFFTSize; // Implicit from generate
  float weightedSum = 0.0f;
  float totalMag = 0.0f;
  float binWidth = sampleRate / static_cast<float>(fftSize);

  for (size_t i = 1; i < spectrum.size(); ++i) {
    float mag = spectrum[i];
    float freq = static_cast<float>(i) * binWidth;

    weightedSum += mag * freq;
    totalMag += mag;
  }

  if (totalMag == 0.0f)
    return 0.0f;

  return weightedSum / totalMag;
}

bool PresetGeneticistAgent::isSilent(const juce::AudioBuffer<float> &buffer) {
  auto [peak, rms] = calculateLevels(buffer);
  return rms < config_.silenceThresholdDb;
}

bool PresetGeneticistAgent::isClipping(const juce::AudioBuffer<float> &buffer) {
  // Check for clipping (samples at or near ±1.0)
  for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
    const float *data = buffer.getReadPointer(ch);
    for (int i = 0; i < buffer.getNumSamples(); ++i) {
      if (std::abs(data[i]) >= kClippingThreshold) {
        return true;
      }
    }
  }
  return false;
}

std::pair<float, float>
PresetGeneticistAgent::calculateLevels(const juce::AudioBuffer<float> &buffer) {
  float peak = 0.0f;
  float sumSquares = 0.0f;
  int totalSamples = 0;

  for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
    const float *data = buffer.getReadPointer(ch);
    for (int i = 0; i < buffer.getNumSamples(); ++i) {
      float sample = std::abs(data[i]);
      peak = std::max(peak, sample);
      sumSquares += data[i] * data[i];
      totalSamples++;
    }
  }

  float rms = (totalSamples > 0)
                  ? std::sqrt(sumSquares / static_cast<float>(totalSamples))
                  : 0.0f;

  // Convert to dB
  float peakDb = (peak > 0.0f) ? juce::Decibels::gainToDecibels(peak) : -100.0f;
  float rmsDb = (rms > 0.0f) ? juce::Decibels::gainToDecibels(rms) : -100.0f;

  return {peakDb, rmsDb};
}

//==============================================================================
// Helpers
//==============================================================================

void PresetGeneticistAgent::updateStats() {
  std::lock_guard<std::mutex> lock(populationMutex_);

  if (population_.empty())
    return;

  float totalFitness = 0.0f;
  float best = 0.0f;
  float worst = 1.0f;
  juce::String bestName;

  for (const auto &individual : population_) {
    if (!individual.isDead && individual.evaluated) {
      totalFitness += individual.fitness;
      if (individual.fitness > best) {
        best = individual.fitness;
        bestName = individual.preset.name;

        // Update best spectrum for UI
        {
          std::lock_guard<std::mutex> spectrumLock(spectrumMutex_);
          currentBestSpectrum_ = individual.spectrum;
        }
      }
      if (individual.fitness < worst) {
        worst = individual.fitness;
      }
    }
  }

  int aliveCount = stats_.totalSurvived;
  stats_.averageFitness =
      (aliveCount > 0) ? totalFitness / static_cast<float>(aliveCount) : 0.0f;
  stats_.bestFitness = best;
  stats_.worstFitness = worst;
  stats_.bestPresetName = bestName;
  stats_.lastUpdateTime = juce::Time::getCurrentTime();
}

void PresetGeneticistAgent::saveElitePresets() {
  exportBestPresets(config_.maxPresetsToSave);
}

juce::String PresetGeneticistAgent::generateBatchFolderName() const {
  auto now = juce::Time::getCurrentTime();
  return "AI_Gen_Batch_" + now.formatted("%Y%m%d_%H%M%S");
}

float PresetGeneticistAgent::randomFloat(float min, float max) {
  std::uniform_real_distribution<float> dist(min, max);
  return dist(rng_);
}

int PresetGeneticistAgent::randomInt(int min, int max) {
  std::uniform_int_distribution<int> dist(min, max);
  return dist(rng_);
}

} // namespace ai
} // namespace zenith
