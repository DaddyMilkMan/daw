/*
    Zenith Preset Bank - Factory Presets
    Copyright (C)2025 Micah Cooley <micahcooley@protonmail.com>
    AGPL-3.0
*/

#pragma once

#include <juce_core/juce_core.h>

namespace zenith {

/**
    Factory preset bank with 100+ professional presets
    Categories: Bass, Lead, Pad, Pluck, Keys, FX, Synth
*/
class ZenithPresetBank {
public:
    struct Preset {
        juce::String name;
        juce::String category;
        juce::StringArray tags;

        // Oscillator settings
        int osc1Wave = 0;  // 0=Saw, 1=Square, 2=Triangle, 3=Sine
        int osc2Wave = 0;
        int osc3Wave = 0;
        float osc1Mix = 0.8f;
        float osc2Mix = 0.0f;
        float osc3Mix = 0.0f;
        float osc1Detune = 0.0f;
        float osc2Detune = 5.0f;  // cents
        float osc3Detune = 7.0f;

        // Unison
        int unisonVoices = 1;
        float unisonDetune = 20.0f;
        float unisonSpread = 0.5f;

        // Filter
        int filterType = 0;  // 0=LP, 1=HP, 2=BP, 3=Notch
        int filterModel = 0;  // 0=SVF, 1=Moog, 2=MS20, 3=SEM, 4=TB303
        float filterCutoff = 2000.0f;
        float filterResonance = 0.3f;
        float filterEnvAmt = 0.5f;
        bool filterSerial = true;

        // Envelope
        float ampAttack = 0.01f;
        float ampDecay = 0.2f;
        float ampSustain = 0.7f;
        float ampRelease = 0.3f;
        float modAttack = 0.05f;
        float modDecay = 0.3f;
        float modSustain = 0.5f;
        float modRelease = 0.4f;

        // Effects
        float reverbMix = 0.2f;
        float delayMix = 0.0f;
        float chorusMix = 0.0f;
        float distortionAmt = 0.0f;

        // Modulation
        float lfo1Rate = 3.0f;
        float lfo1Amt = 0.3f;
        int lfo1Target = 0;  // 0=Filter, 1=Pitch, 2=Pulse

        float master = 0.8f;
    };

    static juce::Array<Preset> getFactoryPresets();

private:
    // Bass presets (20)
    static void addBassPresets(juce::Array<Preset>&);

    // Lead presets (25)
    static void addLeadPresets(juce::Array<Preset>&);

    // Pad presets (20)
    static void addPadPresets(juce::Array<Preset>&);

    // Pluck presets (15)
    static void addPluckPresets(juce::Array<Preset>&);

    // Keys presets (15)
    static void addKeysPresets(juce::Array<Preset>&);

    // FX presets (10)
    static void addFXPresets(juce::Array<Preset>&);

    // Helper to create preset
    static Preset make(const char* name, const char* category,
                      std::function<void(Preset&)> init);
};

} // namespace zenith
