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

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>

namespace zenith {

//==============================================================================
/**
    Manages parameters for ZenithUltraSynth.
*/
class UltraSynthParameterManager {
public:
    //==========================================================================
    // Parameter ID Constants
    //==========================================================================

    // Neural Synthesis - Latent Space
    static const juce::String LatentX;
    static const juce::String LatentY;
    static const juce::String LatentZ;

    // Neural Synthesis - Generation Parameters
    static const juce::String Temperature;
    static const juce::String TopK;
    static const juce::String TopP;
    static const juce::String Overlapping;

    // Neural Synthesis - Timbre Parameters (Array of 16)
    static const juce::String TimbreParamPrefix; // "timbre_param_"

    // Helper to get timbre parameter ID
    static juce::String getTimbreParamID(int index);

    //==========================================================================
    // Constructor / Destructor
    //==========================================================================

    explicit UltraSynthParameterManager(juce::AudioProcessorValueTreeState& apvts);
    ~UltraSynthParameterManager() = default;

    //==========================================================================
    // Parameter Layout Creation
    //==========================================================================

    /**
        Creates the full parameter layout for the synth.
        Called once during processor construction.
    */
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    //==========================================================================
    // Parameter Fetching
    //==========================================================================

    /**
        Prepare for playback.
    */
    void prepareToPlay(double sampleRate, int samplesPerBlock);

    /**
        Apply parameters to effects (stub for now).
    */
    // void applyToEffects(ZenithEffects& effects, double bpm) const;

private:
    juce::AudioProcessorValueTreeState& parameters_;

    // Cached values could be added here for optimization later

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UltraSynthParameterManager)
};

} // namespace zenith
