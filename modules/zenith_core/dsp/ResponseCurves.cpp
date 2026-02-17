/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux
    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>
    SPDX-License-Identifier: Apache-2.0 
*/
// ResponseCurves.cpp
#include "ResponseCurves.h"
#include <algorithm>
#include <cmath>
namespace zenith {
//==============================================================================
// VELOCITY CURVE IMPLEMENTATION
//==============================================================================
VelocityCurve::VelocityCurve() {
    customCurve_.setPreset(ResponseCurve::Preset::Linear);
}
float VelocityCurve::process(float velocityInput) const {
    if (preset_ == Preset::Fixed) {
        return fixedValue_;
    }
    // Clamp input to 0-1 range
    float vel = juce::jlimit(0.0f, 1.0f, velocityInput);
    // Apply preset curve
    float output = useCustom_ ?
        customCurve_.process(vel) :
        processPreset(vel, preset_);
    // Apply humanization
    output = applyHumanization(output);
    // Apply sensitivity
    output *= sensitivity_;
    // Apply offset
    output += offset_;
    // Clamp and track
    output = juce::jlimit(0.0f, 1.0f, output);
    if (trackMode_) {
        trackedVelocity_ = output;
    }
    return output;
}
void VelocityCurve::processBlock(const float* velocities, float* outputs, int numSamples) const {
    for (int i = 0; i < numSamples; ++i) {
        outputs[i] = process(velocities[i]);
    }
}
void VelocityCurve::setPreset(Preset preset) {
    preset_ = preset;
    useCustom_ = false;
}
void VelocityCurve::setPresetByName(const juce::String& name) {
    juce::String lowerName = name.toLowerCase();
    if (lowerName == "linear") setPreset(Preset::Linear);
    else if (lowerName == "exponential" || lowerName == "exp") setPreset(Preset::Exponential);
    else if (lowerName == "logarithmic" || lowerName == "log") setPreset(Preset::Logarithmic);
    else if (lowerName == "soft") setPreset(Preset::Soft);
    else if (lowerName == "hard") setPreset(Preset::Hard);
    else if (lowerName == "velocity1" || lowerName == "vel1") setPreset(Preset::Velocity1);
    else if (lowerName == "velocity2" || lowerName == "vel2") setPreset(Preset::Velocity2);
    else if (lowerName == "velocity3" || lowerName == "vel3") setPreset(Preset::Velocity3);
    else if (lowerName == "velocity4" || lowerName == "vel4") setPreset(Preset::Velocity4);
    else if (lowerName == "large") setPreset(Preset::Large);
    else if (lowerName == "constant") setPreset(Preset::Constant);
    else if (lowerName == "noteon") setPreset(Preset::NoteOn);
    else if (lowerName == "gate") setPreset(Preset::Gate);
    else if (lowerName == "inverted") setPreset(Preset::Inverted);
    else if (lowerName == "bump") setPreset(Preset::Bump);
    else if (lowerName == "dip") setPreset(Preset::Dip);
    else if (lowerName == "sshaped" || lowerName == "s-curve") setPreset(Preset::SShaped);
    else if (lowerName == "reverses" || lowerName == "reverse s") setPreset(Preset::ReverseS);
    else if (lowerName == "humanized") setPreset(Preset::Humanized);
    else if (lowerName == "fixed") setPreset(Preset::Fixed);
    else setPreset(Preset::Linear);
}
juce::String VelocityCurve::getPresetName(Preset preset) {
    switch (preset) {
        case Preset::Linear: return "Linear";
        case Preset::Exponential: return "Exponential";
        case Preset::Logarithmic: return "Logarithmic";
        case Preset::Soft: return "Soft";
        case Preset::Hard: return "Hard";
        case Preset::Velocity1: return "Velocity 1";
        case Preset::Velocity2: return "Velocity 2";
        case Preset::Velocity3: return "Velocity 3";
        case Preset::Velocity4: return "Velocity 4";
