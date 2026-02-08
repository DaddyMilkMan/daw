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

#include "UltraSynthParameterManager.h"

namespace zenith {

//==============================================================================
// Parameter ID Constants
//==============================================================================

const juce::String UltraSynthParameterManager::LatentX = "latent_x";
const juce::String UltraSynthParameterManager::LatentY = "latent_y";
const juce::String UltraSynthParameterManager::LatentZ = "latent_z";

const juce::String UltraSynthParameterManager::Temperature = "temperature";
const juce::String UltraSynthParameterManager::TopK = "top_k";
const juce::String UltraSynthParameterManager::TopP = "top_p";
const juce::String UltraSynthParameterManager::Overlapping = "overlapping";

const juce::String UltraSynthParameterManager::TimbreParamPrefix = "timbre_param_";

//==============================================================================
// Helper
//==============================================================================

juce::String UltraSynthParameterManager::getTimbreParamID(int index) {
    return TimbreParamPrefix + juce::String(index);
}

//==============================================================================
// Constructor / Destructor
//==============================================================================

UltraSynthParameterManager::UltraSynthParameterManager(juce::AudioProcessorValueTreeState& apvts)
    : parameters_(apvts)
{
    // Initialize cache or callbacks if needed
}

//==============================================================================
// Parameter Layout Creation
//==============================================================================

juce::AudioProcessorValueTreeState::ParameterLayout UltraSynthParameterManager::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Neural Synthesis - Latent Space
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(LatentX, 1), "Latent X", 0.0f, 1.0f, 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(LatentY, 1), "Latent Y", 0.0f, 1.0f, 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(LatentZ, 1), "Latent Z", 0.0f, 1.0f, 0.5f));

    // Neural Synthesis - Generation Parameters
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(Temperature, 1), "Temperature", 0.0f, 1.0f, 0.8f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(TopK, 1), "Top K", 0.0f, 1.0f, 0.9f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(TopP, 1), "Top P", 0.0f, 1.0f, 0.95f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(Overlapping, 1), "Overlapping", 0.0f, 1.0f, 0.5f));

    // Neural Synthesis - Timbre Parameters (Array of 16)
    for (int i = 0; i < 16; ++i) {
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(getTimbreParamID(i), 1),
            "Timbre Param " + juce::String(i),
            0.0f, 1.0f, 0.5f));
    }

    return layout;
}

//==============================================================================
// Parameter Fetching
//==============================================================================

void UltraSynthParameterManager::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Initialize things related to playback
    // For now, this is a placeholder to match the usage in ZenithUltraSynth.cpp
}

} // namespace zenith
