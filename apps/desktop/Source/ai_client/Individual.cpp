/*
  ==============================================================================

    Individual.cpp
    Implementation of Individual solution representation for genetic algorithm

  ==============================================================================
*/

#include "Individual.h"
#include <cmath>

namespace zenith {
namespace ai {

//==============================================================================
// Individual helper methods
//==============================================================================

float Individual::calculateFitness(const EvolutionConfig& config) const
{
    if (!evaluated)
        return 0.0f;

    float fitness = 0.0f;

    // Harmonic richness contribution
    fitness += harmonicRichness * config.harmonicRichnessWeight;

    // Dynamic range contribution
    float dynamicRangeScore = juce::jmap(dynamicRange, 0.0f, 30.0f, 0.0f, 1.0f);
    dynamicRangeScore = juce::jlimit(0.0f, 1.0f, dynamicRangeScore);
    fitness += dynamicRangeScore * config.dynamicRangeWeight;

    // Spectral balance contribution
    fitness += juce::jlimit(0.0f, 1.0f, spectralCentroid / 10000.0f) *
              config.spectralBalanceWeight;

    // Silence penalty
    if (peakDb < config.silenceThresholdDb)
        return 0.0f;  // Too quiet = zero fitness

    // Clipping penalty
    if (peakDb > config.clippingThresholdDb)
        return 0.0f;  // Clipping = zero fitness

    // Dynamic range penalty
    if (dynamicRange < config.minDynamicRangeDb)
        fitness *= 0.5f;  // Reduce fitness for low dynamic range

    return juce::jlimit(0.0f, 1.0f, fitness);
}

bool Individual::isViable(const EvolutionConfig& config) const
{
    // Check if preset meets minimum requirements
    if (peakDb < config.silenceThresholdDb)
    {
        isDead = true;
        deathReason = "Too quiet (" + juce::String(peakDb, 1) + " dB < " +
                     juce::String(config.silenceThresholdDb, 1) + " dB)";
        return false;
    }

    if (peakDb > config.clippingThresholdDb)
    {
        isDead = true;
        deathReason = "Clipping (" + juce::String(peakDb, 1) + " dB > " +
                     juce::String(config.clippingThresholdDb, 1) + " dB)";
        return false;
    }

    if (dynamicRange < config.minDynamicRangeDb)
    {
        isDead = true;
        deathReason = "Low dynamic range (" + juce::String(dynamicRange, 1) +
                     " dB < " + juce::String(config.minDynamicRangeDb, 1) + " dB)";
        return false;
    }

    isDead = false;
    deathReason.clear();
    return true;
}

juce::String Individual::toString() const
{
    juce::StringArray lines;
    lines.add("=== Individual: " + preset.name + " ===");
    lines.add("Fitness: " + juce::String(fitness * 100.0f, 1) + "%");
    lines.add("");

    if (evaluated)
    {
        lines.add("Audio Characteristics:");
        lines.add("  Peak Level: " + juce::String(peakDb, 1) + " dB");
        lines.add("  RMS Level: " + juce::String(rmsDb, 1) + " dB");
        lines.add("  Dynamic Range: " + juce::String(dynamicRange, 1) + " dB");
        lines.add("  Harmonic Richness: " + juce::String(harmonicRichness * 100.0f, 1) + "%");
        lines.add("  Spectral Centroid: " + juce::String(spectralCentroid, 0) + " Hz");
    }
    else
    {
        lines.add("[Not yet evaluated]");
    }

    if (isDead)
    {
        lines.add("");
        lines.add("[DEAD] " + deathReason);
    }

    return lines.joinIntoString("\n");
}

Individual Individual::crossover(const Individual& other,
                                 const EvolutionConfig& config,
                                 std::mt19937& rng) const
{
    // Create child by mixing parameters from both parents
    Individual child;

    // Cross preset parameters
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    // For each parameter, randomly choose from either parent
    child.preset.name = preset.name + " × " + other.preset.name;

    // Oscillator parameters
    if (dist(rng) < 0.5f)
        child.preset.osc1Type = preset.osc1Type;
    else
        child.preset.osc1Type = other.preset.osc1Type;

    if (dist(rng) < 0.5f)
        child.preset.osc2Type = preset.osc2Type;
    else
        child.preset.osc2Type = other.preset.osc2Type;

    // Mix continuous parameters
    float mixRatio = dist(rng);
    child.preset.osc1Detune = juce::jmap(mixRatio,
                                          0.0f, 1.0f,
                                          preset.osc1Detune,
                                          other.preset.osc1Detune);

    child.preset.osc2Detune = juce::jmap(mixRatio,
                                          0.0f, 1.0f,
                                          preset.osc2Detune,
                                          other.preset.osc2Detune);

    // Filter parameters
    if (dist(rng) < 0.5f)
        child.preset.filterType = preset.filterType;
    else
        child.preset.filterType = other.preset.filterType;

    child.preset.cutoff = juce::jmap(mixRatio,
                                     0.0f, 1.0f,
                                     preset.cutoff,
                                     other.preset.cutoff);

    child.preset.resonance = juce::jmap(mixRatio,
                                        0.0f, 1.0f,
                                        preset.resonance,
                                        other.preset.resonance);

    // Envelope parameters
    child.preset.attack = juce::jmap(mixRatio,
                                     0.0f, 1.0f,
                                     preset.attack,
                                     other.preset.attack);

    child.preset.decay = juce::jmap(mixRatio,
                                    0.0f, 1.0f,
                                    preset.decay,
                                    other.preset.decay);

    child.preset.sustain = juce::jmap(mixRatio,
                                      0.0f, 1.0f,
                                      preset.sustain,
                                      other.preset.sustain);

    child.preset.release = juce::jmap(mixRatio,
                                      0.0f, 1.0f,
                                      preset.release,
                                      other.preset.release);

    // Effects
    child.preset.distortionAmount = juce::jmap(mixRatio,
                                               0.0f, 1.0f,
                                               preset.distortionAmount,
                                               other.preset.distortionAmount);

    child.preset.reverbAmount = juce::jmap(mixRatio,
                                           0.0f, 1.0f,
                                           preset.reverbAmount,
                                           other.preset.reverbAmount);

    child.preset.delayAmount = juce::jmap(mixRatio,
                                          0.0f, 1.0f,
                                          preset.delayAmount,
                                          other.preset.delayAmount);

    child.evaluated = false;
    child.isDead = false;

    return child;
}

void Individual::mutate(const EvolutionConfig& config,
                        std::mt19937& rng)
{
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    std::normal_distribution<float> noise(0.0f, config.mutationStrength);

    // Mutate each parameter with probability equal to mutation rate
    auto mutateParam = [&](float& param, float minVal, float maxVal) {
        if (dist(rng) < config.mutationRate)
        {
            float delta = noise(rng) * (maxVal - minVal);
            param = juce::jlimit(minVal, maxVal, param + delta);
        }
    };

    // Mutate oscillator parameters
    mutateParam(preset.osc1Mix, 0.0f, 1.0f);
    mutateParam(preset.osc1Detune, -100.0f, 100.0f);
    mutateParam(preset.osc2Mix, 0.0f, 1.0f);
    mutateParam(preset.osc2Detune, -100.0f, 100.0f);

    // Mutate filter parameters
    mutateParam(preset.cutoff, 0.0f, 1.0f);
    mutateParam(preset.resonance, 0.0f, 1.0f);
    mutateParam(preset.filterEnvAmount, -1.0f, 1.0f);

    // Mutate envelope parameters
    mutateParam(preset.attack, 0.0f, 1.0f);
    mutateParam(preset.decay, 0.0f, 1.0f);
    mutateParam(preset.sustain, 0.0f, 1.0f);
    mutateParam(preset.release, 0.0f, 1.0f);

    // Mutate effects
    mutateParam(preset.distortionAmount, 0.0f, 1.0f);
    mutateParam(preset.reverbAmount, 0.0f, 1.0f);
    mutateParam(preset.delayAmount, 0.0f, 1.0f);
    mutateParam(preset.delayTime, 0.0f, 1.0f);
    mutateParam(preset.delayFeedback, 0.0f, 1.0f);

    // Mark as unevaluated
    evaluated = false;
}

Individual Individual::createRandom(const EvolutionConfig& config,
                                     const juce::String& name,
                                     std::mt19937& rng)
{
    Individual individual;
    individual.preset.name = name;

    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    std::uniform_int_distribution<int> typeDist(0, 3);  // Oscillator type count

    // Random oscillator types
    individual.preset.osc1Type = static_cast<OscillatorType>(typeDist(rng));
    individual.preset.osc2Type = static_cast<OscillatorType>(typeDist(rng));

    // Random parameters
    individual.preset.osc1Mix = dist(rng);
    individual.preset.osc1Detune = juce::jmap(dist(rng), 0.0f, 1.0f, -100.0f, 100.0f);
    individual.preset.osc2Mix = dist(rng);
    individual.preset.osc2Detune = juce::jmap(dist(rng), 0.0f, 1.0f, -100.0f, 100.0f);

    // Random filter
    individual.preset.filterType = static_cast<FilterType>(typeDist(rng) % 3);
    individual.preset.cutoff = dist(rng);
    individual.preset.resonance = dist(rng);
    individual.preset.filterEnvAmount = juce::jmap(dist(rng), 0.0f, 1.0f, -1.0f, 1.0f);

    // Random envelope
    individual.preset.attack = std::pow(dist(rng), 2.0f);  // Bias toward shorter
    individual.preset.decay = dist(rng);
    individual.preset.sustain = dist(rng);
    individual.preset.release = std::pow(dist(rng), 2.0f);  // Bias toward shorter

    // Random effects
    individual.preset.distortionAmount = dist(rng) * dist(rng);  // Bias toward less
    individual.preset.reverbAmount = dist(rng) * 0.5f;  // Max 50%
    individual.preset.delayAmount = dist(rng) * 0.5f;
    individual.preset.delayTime = dist(rng);
    individual.preset.delayFeedback = dist(rng) * 0.7f;

    individual.evaluated = false;
    individual.isDead = false;

    return individual;
}

} // namespace ai
} // namespace zenith
