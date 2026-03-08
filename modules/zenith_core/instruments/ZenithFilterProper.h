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

#include "ZenithPolySynthDefs.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <memory>
#include <cmath>

namespace zenith {

//==============================================================================
// PROFESSIONAL FILTER WITH WORKING OVERSAMPLING
//==============================================================================

class ZenithFilterProper {
public:
    ZenithFilterProper() = default;

    //==========================================================================
    // Configuration
    //==========================================================================

    void setType(FilterType type) { type_ = type; }
    void setModel(FilterModelType model) { model_ = model; }
    void setCutoff(float cutoffHz) { cutoff_ = juce::jlimit(20.0f, 20000.0f, cutoffHz); }
    void setResonance(float res) { resonance_ = juce::jlimit(0.0f, 1.0f, res); }
    void setDrive(float drive) { drive_ = juce::jlimit(0.0f, 1.0f, drive); }
    void setKeyTrackAmount(float amount) { keyTrackAmount_ = juce::jlimit(0.0f, 1.0f, amount); }

    //==========================================================================
    // Sample Rate
    //==========================================================================

    void setSampleRate(double sr);
    void reset();
    void setOversampling(int factor);

    //==========================================================================
    // Processing
    //==========================================================================

    float processSample(float input, float midiNote = 60.0f);
    void process(juce::AudioBuffer<float>& buffer, float midiNote = 60.0f);

private:
private:
    //==========================================================================
    // State Variables
    //==========================================================================

    FilterType type_ = FilterType::LowPass;
    FilterModelType model_ = FilterModelType::SVF;
    float cutoff_ = 1000.0f;
    float resonance_ = 0.5f;
    float drive_ = 0.0f;
    float keyTrackAmount_ = 0.0f;
    double sampleRate_ = 44100.0;
    int oversamplingFactor_ = 1;

    //==========================================================================
    // Oversampling
    //==========================================================================

    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler2x_;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler4x_;
    juce::AudioBuffer<float> oversamplerBuffer_;

    //==========================================================================
    // Filter State (SVF)
    //==========================================================================

    struct SVFState {
        double low = 0.0;
        double band = 0.0;
        double high = 0.0;
    } svf_;

    //==========================================================================
    // Internal Helpers
    //==========================================================================

    float calculateCutoffWithKeyTrack(float midiNote);
    float applyDrive(float sample);
    float processSVF(float input, float& output, double fs);

    // Helper
    inline float softClip(float x) { return std::tanh(x); }
    inline float hardClip(float x) { return juce::jlimit(-1.0f, 1.0f, x); }

    // Moog Ladder (4-pole, transistor ladder)
    float processMoog(float input);

    // MS-20 (Korg style)
    float processMS20(float input);

    // SEM (Oberheim)
    float processSEM(float input);

    // TB-303 (Roland)
    float processTB303(float input);

    //==========================================================================
    // Filter Model State
    //==========================================================================

    struct MoogState {
        double s1 = 0.0, s2 = 0.0, s3 = 0.0, s4 = 0.0;
    } moogState_;

    struct MS20State {
        double hp = 0.0, lp1 = 0.0, lp2 = 0.0;
    } ms20State_;

    struct SEMState {
        double low = 0.0, high = 0.0, band = 0.0;
    } semState_;

    struct TB303State {
        double s1 = 0.0, s2 = 0.0, s3 = 0.0, s4 = 0.0;
    } tb303State_;
};

} // namespace zenith
