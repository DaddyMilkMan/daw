/*
  ==============================================================================

    EvolutionStats.cpp
    Implementation of evolution statistics tracking for genetic algorithm

  ==============================================================================
*/

#include "EvolutionStats.h"
#include <algorithm>

namespace zenith {
namespace ai {

//==============================================================================
// EvolutionStats helper methods
//==============================================================================

void EvolutionStats::update(int currentGen,
                            const std::vector<Individual>& population)
{
    generation = currentGen;
    lastUpdateTime = juce::Time::getCurrentTime();

    // Count individuals
    totalEvaluated = static_cast<int>(population.size());
    totalDead = 0;
    totalSurvived = 0;

    float fitnessSum = 0.0f;
    bestFitness = -1.0f;
    worstFitness = 1.0f;

    for (const auto& individual : population)
    {
        if (individual.evaluated)
        {
            fitnessSum += individual.fitness;

            if (individual.fitness > bestFitness)
            {
                bestFitness = individual.fitness;
                bestPresetName = individual.preset.name;
            }

            if (individual.fitness < worstFitness)
                worstFitness = individual.fitness;

            if (individual.isDead)
                totalDead++;
            else
                totalSurvived++;
        }
    }

    // Calculate average fitness
    if (totalEvaluated > 0)
        averageFitness = fitnessSum / static_cast<float>(totalEvaluated);
    else
        averageFitness = 0.0f;
}

juce::String EvolutionStats::toString() const
{
    juce::StringArray lines;
    lines.add("=== Evolution Statistics ===");
    lines.add("");
    lines.add("Generation: " + juce::String(generation));
    lines.add("");
    lines.add("Population:");
    lines.add("  Total Evaluated: " + juce::String(totalEvaluated));
    lines.add("  Survived: " + juce::String(totalSurvived));
    lines.add("  Dead: " + juce::String(totalDead));
    lines.add("");
    lines.add("Fitness:");
    lines.add("  Average: " + juce::String(averageFitness * 100.0f, 1) + "%");
    lines.add("  Best: " + juce::String(bestFitness * 100.0f, 1) + "%");
    lines.add("  Worst: " + juce::String(worstFitness * 100.0f, 1) + "%");
    lines.add("");
    if (bestPresetName.isNotEmpty())
        lines.add("Best Preset: " + bestPresetName);

    return lines.joinIntoString("\n");
}

float EvolutionStats::getSurvivalRate() const
{
    if (totalEvaluated == 0)
        return 0.0f;
    return static_cast<float>(totalSurvived) / static_cast<float>(totalEvaluated);
}

float EvolutionStats::getDeathRate() const
{
    if (totalEvaluated == 0)
        return 0.0f;
    return static_cast<float>(totalDead) / static_cast<float>(totalEvaluated);
}

bool EvolutionStats::hasConverged(float threshold, int windowSize) const
{
    // Convergence is detected when fitness improvement is below threshold
    // This is a simplified check - a full implementation would track history
    return (bestFitness - worstFitness) < threshold;
}

juce::String EvolutionStats::getElapsedTime() const
{
    auto now = juce::Time::getCurrentTime();
    auto elapsed = now - lastUpdateTime;

    if (elapsed.inMilliseconds() < 1000)
        return juce::String(elapsed.inMilliseconds()) + " ms";
    else if (elapsed.inSeconds() < 60)
        return juce::String(static_cast<int>(elapsed.inSeconds())) + " seconds";
    else if (elapsed.inMinutes() < 60)
        return juce::String(static_cast<int>(elapsed.inMinutes())) + " minutes";
    else
        return juce::String(static_cast<int>(elapsed.inHours())) + " hours";
}

} // namespace ai
} // namespace zenith
