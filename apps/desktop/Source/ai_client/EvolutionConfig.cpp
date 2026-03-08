/*
  ==============================================================================

    EvolutionConfig.cpp
    Implementation of genetic algorithm configuration for preset evolution

  ==============================================================================
*/

#include "EvolutionConfig.h"

namespace zenith {
namespace ai {

//==============================================================================
// EvolutionConfig helper methods
//==============================================================================

juce::String EvolutionConfig::toString() const
{
    juce::StringArray lines;
    lines.add("=== Evolution Configuration ===");
    lines.add("");
    lines.add("Target Role: " + getTargetRoleName());
    lines.add("Population Size: " + juce::String(populationSize));
    lines.add("");
    lines.add("Selection:");
    lines.add("  Elitism Ratio: " + juce::String(elitismRatio * 100.0f, 1) + "%");
    lines.add("  Fitness Threshold: " + juce::String(fitnessThreshold * 100.0f, 1) + "%");
    lines.add("");
    lines.add("Reproduction:");
    lines.add("  Crossover Rate: " + juce::String(crossoverRate * 100.0f, 1) + "%");
    lines.add("  Mutation Rate: " + juce::String(mutationRate * 100.0f, 1) + "%");
    lines.add("  Mutation Strength: ±" + juce::String(mutationStrength * 100.0f, 1) + "%");
    lines.add("");
    lines.add("Fitness Weights:");
    lines.add("  Harmonic Richness: " + juce::String(harmonicRichnessWeight * 100.0f, 1) + "%");
    lines.add("  Dynamic Range: " + juce::String(dynamicRangeWeight * 100.0f, 1) + "%");
    lines.add("  Spectral Balance: " + juce::String(spectralBalanceWeight * 100.0f, 1) + "%");
    lines.add("  Uniqueness: " + juce::String(uniquenessWeight * 100.0f, 1) + "%");
    lines.add("");
    lines.add("Rendering:");
    lines.add("  Sample Rate: " + juce::String(renderSampleRate, 0) + " Hz");
    lines.add("  Duration: " + juce::String(renderDurationSeconds, 1) + " seconds");
    lines.add("  MIDI Note: " + juce::String(renderMidiNote));
    lines.add("  Velocity: " + juce::String(renderVelocity * 100.0f, 0) + "%");
    lines.add("");
    lines.add("Survival Thresholds:");
    lines.add("  Silence: < " + juce::String(silenceThresholdDb, 1) + " dB");
    lines.add("  Clipping: > " + juce::String(clippingThresholdDb, 1) + " dB");
    lines.add("  Min Dynamic Range: " + juce::String(minDynamicRangeDb, 1) + " dB");
    lines.add("");
    lines.add("Batch Saving:");
    lines.add("  Save Every: " + juce::String(saveEveryNGenerations) + " generations");
    lines.add("  Max Presets: " + juce::String(maxPresetsToSave) + " per batch");

    return lines.joinIntoString("\n");
}

juce::String EvolutionConfig::getTargetRoleName() const
{
    switch (targetRole)
    {
        case TargetRole::Bass:
            return "Bass";
        case TargetRole::Lead:
            return "Lead";
        case TargetRole::Pad:
            return "Pad";
        case TargetRole::FX:
            return "FX";
        case TargetRole::General:
        default:
            return "General";
    }
}

bool EvolutionConfig::validate() const
{
    // Check population size
    if (populationSize < 10 || populationSize > 1000)
        return false;

    // Check selection parameters
    if (elitismRatio < 0.0f || elitismRatio > 0.5f)
        return false;
    if (fitnessThreshold < 0.0f || fitnessThreshold > 1.0f)
        return false;

    // Check reproduction parameters
    if (crossoverRate < 0.0f || crossoverRate > 1.0f)
        return false;
    if (mutationRate < 0.0f || mutationRate > 1.0f)
        return false;
    if (mutationStrength < 0.0f || mutationStrength > 1.0f)
        return false;

    // Check fitness weights sum to ~1.0
    float totalWeight = harmonicRichnessWeight + dynamicRangeWeight +
                       spectralBalanceWeight + uniquenessWeight;
    if (totalWeight < 0.9f || totalWeight > 1.1f)
        return false;

    // Check audio rendering
    if (renderSampleRate < 22000.0 || renderSampleRate > 192000.0)
        return false;
    if (renderDurationSeconds < 0.1f || renderDurationSeconds > 10.0f)
        return false;
    if (renderMidiNote < 0 || renderMidiNote > 127)
        return false;
    if (renderVelocity < 0.0f || renderVelocity > 1.0f)
        return false;

    // Check thresholds
    if (silenceThresholdDb < -100.0f || silenceThresholdDb > 0.0f)
        return false;
    if (clippingThresholdDb < -10.0f || clippingThresholdDb > 10.0f)
        return false;
    if (minDynamicRangeDb < 0.0f || minDynamicRangeDb > 60.0f)
        return false;

    // Check batch saving
    if (saveEveryNGenerations < 1 || saveEveryNGenerations > 1000)
        return false;
    if (maxPresetsToSave < 1 || maxPresetsToSave > 1000)
        return false;

    return true;
}

EvolutionConfig EvolutionConfig::getPresetForRole(TargetRole role)
{
    EvolutionConfig config;
    config.targetRole = role;

    switch (role)
    {
        case TargetRole::Bass:
            // Bass: Focus on fundamentals, low frequencies
            config.harmonicRichnessWeight = 0.2f;
            config.dynamicRangeWeight = 0.3f;
            config.spectralBalanceWeight = 0.4f;  // Emphasize low end
            config.uniquenessWeight = 0.1f;
            config.silenceThresholdDb = -50.0f;
            config.minDynamicRangeDb = 4.0f;
            break;

        case TargetRole::Lead:
            // Lead: High harmonic content, presence
            config.harmonicRichnessWeight = 0.5f;  // Emphasize harmonics
            config.dynamicRangeWeight = 0.2f;
            config.spectralBalanceWeight = 0.1f;
            config.uniquenessWeight = 0.2f;
            config.silenceThresholdDb = -60.0f;
            config.minDynamicRangeDb = 8.0f;
            break;

        case TargetRole::Pad:
            // Pad: Evolving, warm, wide stereo
            config.harmonicRichnessWeight = 0.3f;
            config.dynamicRangeWeight = 0.2f;
            config.spectralBalanceWeight = 0.3f;
            config.uniquenessWeight = 0.2f;
            config.silenceThresholdDb = -50.0f;
            config.minDynamicRangeDb = 3.0f;
            break;

        case TargetRole::FX:
            // FX: Noisy, unusual characteristics
            config.harmonicRichnessWeight = 0.2f;
            config.dynamicRangeWeight = 0.3f;
            config.spectralBalanceWeight = 0.1f;
            config.uniquenessWeight = 0.4f;  // Emphasize uniqueness
            config.silenceThresholdDb = -70.0f;  // Allow quiet sounds
            config.minDynamicRangeDb = 10.0f;  // Require dynamics
            break;

        case TargetRole::General:
        default:
            // Balanced settings
            break;
    }

    return config;
}

} // namespace ai
} // namespace zenith
