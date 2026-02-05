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

    UnisonManager.h
    Created: 2025-01-28
    Updated: 2025-02-02 - S-tier implementation
    Author:  Zenith DAW

    Professional 16-voice unison with phase-accurate detune and stereo spread.


  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>

namespace zenith {

//==============================================================================
// Unison Voice (single voice in unison group)
//==============================================================================

class UnisonVoice {
public:
    UnisonVoice();

    // Configuration
    void setDetune(float cents);
    void setPan(float pan);
    void setGain(float gain);
    void setPhase(float phase);

    // Processing - generates phase output for oscillator
    float processSample(float frequency, double sampleRate);

    // State
    void reset();

    // Query
    float getFrequencyRatio() const { return frequencyRatio_; }
    float getLeftGain() const { return leftGain_; }
    float getRightGain() const { return rightGain_; }
    float getPan() const { return pan_; }

private:
    // Phase state - persists across samples
    float phase_;           // Current phase (0 to 1)
    float phaseIncrement_;  // Phase advance per sample

    // Parameters
    float detuneCents_;     // Detune in cents
    float frequencyRatio_;  // Calculated frequency ratio
    float pan_;             // Pan position (-1 to 1)
    float gain_;            // Voice level

    // Output gains (after pan)
    float leftGain_;
    float rightGain_;

    // Internal helpers
    void updateFrequencyRatio();
    void updateGains();
};

//==============================================================================
// Unison Manager
//==============================================================================

class UnisonManager {
public:
    UnisonManager();

    // Configuration
    void setNumVoices(int voices);
    void setDetune(float cents);       // Total spread in cents
    void setSpread(float spread);       // Stereo width (0 to 1)
    void setPanRandom(bool randomize);
    void setSampleRate(double sampleRate);

    // Processing
    void process(float baseFrequency, float* left, float* right, int numSamples);

    // State
    void reset();

    // Query
    int getNumVoices() const { return numVoices_; }
    const std::array<UnisonVoice, 16>& getVoices() const { return voices_; }

private:
    std::array<UnisonVoice, 16> voices_;
    int numVoices_;
    float detune_;
    float spread_;
    bool panRandom_;
    double sampleRate_;

    // Internal helpers
    float calculateVolumeCompensation() const;
    void updateVoiceDetunes();
    void updateVoicePans();
};

} // namespace zenith
