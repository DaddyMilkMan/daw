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

/*
    ==============================================================================
    Original file header:
*/

  ==============================================================================

    ZenithAutoTune.h
    Created: 2026-01-29
    Author:  Zenith DAW

    Built-in professional pitch correction effect.
    Included FREE with Zenith DAW ($100 purchase).
    

    Competes with: Antares Auto-Tune, Waves Tune, Logic Flex Pitch
    
    Features:
    - Real-time pitch correction
    - Retune Speed (T-Pain effect to natural)
    - Humanize (preserve natural variation)
    - Formant Preservation (no chipmunk effect)
    - Scale/Key selector
    - Visual pitch display

  ==============================================================================
*/

#pragma once

#include "../dsp/PitchDetector.h"
#include "../dsp/PitchCorrector.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>

namespace zenith {
namespace effects {

//==============================================================================
/**
    Built-in Auto-Tune effect - INCLUDED FREE with Zenith DAW.
    
    No subscription. No additional purchase. Professional pitch correction
    that rivals Antares Auto-Tune Access ($49) and Waves Tune ($35).
*/
class ZenithAutoTune : public juce::AudioProcessor
{
public:
    //==============================================================================
    ZenithAutoTune();
    ~ZenithAutoTune() override;

    //==============================================================================
    // AudioProcessor overrides
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer, 
                      juce::MidiBuffer& midiMessages) override;
    
    //==============================================================================
    // Parameters
    juce::AudioProcessorValueTreeState& getParameters() { return *parameters; }
    
    //==============================================================================
    // Direct parameter control (for UI)
    void setRetuneSpeed(float ms);
    float getRetuneSpeed() const;
    
    void setHumanize(float amount);
    float getHumanize() const;
    
    void setCorrectionAmount(float amount);
    float getCorrectionAmount() const;
    
    void setFormantPreservation(float amount);
    float getFormantPreservation() const;
    
    void setKey(dsp::Note rootNote);
    dsp::Note getKey() const;
    
    void setScale(dsp::MusicalScale scale);
    dsp::MusicalScale getScale() const;
    
    //==============================================================================
    // For UI visualization
    float getDetectedPitch() const { return detectedPitch_.load(); }
    float getTargetPitch() const { return targetPitch_.load(); }
    bool isVoiced() const { return isVoiced_.load(); }
    bool isCorrecting() const { return isCorrecting_.load(); }
    
    //==============================================================================
    // AudioProcessor boilerplate
    const juce::String getName() const override { return "Zenith Auto-Tune"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}
    
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;
    
    //==============================================================================
    // Presets
    void loadPreset(const juce::String& presetName);
    static juce::StringArray getPresetNames();

private:
    //==============================================================================
    // Parameter management
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void parameterChanged(const juce::String& parameterID, float newValue);
    
    std::unique_ptr<juce::AudioProcessorValueTreeState> parameters;
    
    //==============================================================================
    // Core DSP
    dsp::PitchDetector pitchDetector_;
    dsp::PitchCorrector pitchCorrector_;
    
    // Processing buffer (mono pitch correction)
    juce::AudioBuffer<float> monoBuffer_;
    juce::AudioBuffer<float> correctedBuffer_;
    
    // State for UI
    std::atomic<float> detectedPitch_{0.0f};
    std::atomic<float> targetPitch_{0.0f};
    std::atomic<bool> isVoiced_{false};
    std::atomic<bool> isCorrecting_{false};
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithAutoTune)
};

} // namespace effects
} // namespace zenith
