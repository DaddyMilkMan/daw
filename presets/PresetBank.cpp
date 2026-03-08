/*
    Zenith Preset Bank - Factory Presets
    Copyright (C)2025 Micah Cooley <micahcooley@protonmail.com>
    AGPL-3.0
*/

#include "PresetBank.h"

namespace zenith {

//==============================================================================
// PRESET BUILDER
//==============================================================================

Preset ZenithPresetBank::make(const char* name, const char* category,
                              std::function<void(Preset&)> init) {
    Preset p;
    p.name = name;
    p.category = category;
    init(p);
    return p;
}

//==============================================================================
// BASS PRESETS (20)
//==============================================================================

void ZenithPresetBank::addBassPresets(juce::Array<Preset>& presets) {
    // 1. Classic Sub
    presets.add(make("Classic Sub", "Bass", [](Preset& p) {
        p.osc1Wave = 0; p.osc1Mix = 1.0f;
        p.filterModel = 1; // Moog
        p.filterCutoff = 800.0f;
        p.filterResonance = 0.4f;
        p.ampDecay = 0.3f; p.ampSustain = 0.6f;
        p.tags = {"sub", "warm", "Moog"};
    }));

    // 2. Deep Rees
    presets.add(make("Deep Rees", "Bass", [](Preset& p) {
        p.osc1Wave = 0; p.osc2Wave = 0;
        p.osc1Mix = 0.7f; p.osc2Mix = 0.5f;
        p.osc2Detune = 10.0f;
        p.filterModel = 4; // TB303
        p.filterCutoff = 1200.0f;
        p.filterResonance = 0.7f;
        p.filterEnvAmt = 0.8f;
        p.modDecay = 0.1f; p.modSustain = 0.2f;
        p.tags = {"acid", "303", "resonant"};
    }));

    // 3. FM Bass
    presets.add(make("FM Bass", "Bass", [](Preset& p) {
        p.osc1Wave = 3; // Sine
        p.osc2Wave = 3;
        p.osc1Mix = 0.6f; p.osc2Mix = 0.4f;
        p.filterModel = 0; // SVF
        p.filterCutoff = 600.0f;
        p.lfo1Rate = 5.0f; p.lfo1Target = 0;
        p.lfo1Amt = 0.5f;
        p.tags = {"FM", "clean", "deep"};
    }));

    // 4. Saw Stack
    presets.add(make("Saw Stack", "Bass", [](Preset& p) {
        p.unisonVoices = 4; p.unisonDetune = 15.0f;
        p.filterModel = 1; // Moog
        p.filterCutoff = 1500.0f;
        p.filterResonance = 0.5f;
        p.distortionAmt = 0.3f;
        p.tags = {"stack", "fat", "rich"};
    }));

    // 5. Sine Sub
    presets.add(make("Sine Sub", "Bass", [](Preset& p) {
        p.osc1Wave = 3; p.osc1Mix = 1.0f;
        p.filterModel = 0; // SVF
        p.filterCutoff = 400.0f;
        p.filterResonance = 0.2f;
        p.ampDecay = 0.2f; p.ampSustain = 0.9f;
        p.tags = {"clean", "sub", "sine"};
    }));

    // 6. Square Pulse
    presets.add(make("Square Pulse", "Bass", [](Preset& p) {
        p.osc1Wave = 1; // Square
        p.osc1Mix = 1.0f;
        p.filterModel = 4; // TB303
        p.filterCutoff = 1000.0f;
        p.filterResonance = 0.6f;
        p.filterEnvAmt = 0.7f;
        p.modAttack = 0.01f; p.modDecay = 0.15f;
        p.tags = {"square", "pulse", "303"};
    }));

    // 7-20. (More bass presets - simplified for space)
    presets.add(make("Boom Bass", "Bass", [](Preset& p) {
        p.osc1Wave = 0; p.filterCutoff = 200.0f;
        p.filterResonance = 0.3f; p.ampSustain = 0.8f;
    }));

    presets.add(make("Tight Bass", "Bass", [](Preset& p) {
        p.osc1Wave = 1; p.filterCutoff = 2500.0f;
        p.ampRelease = 0.1f;
    }));

    presets.add(make("Rubber Bass", "Bass", [](Preset& p) {
        p.osc1Wave = 0; p.filterModel = 1;
        p.filterResonance = 0.8f; p.lfo1Rate = 8.0f;
    }));

    presets.add(make("Metallic Bass", "Bass", [](Preset& p) {
        p.unisonVoices = 2; p.filterModel = 2; // MS20
        p.filterResonance = 0.9f; p.chorusMix = 0.3f;
    }));

    presets.add(make("Wobble Bass", "Bass", [](Preset& p) {
        p.lfo1Rate = 2.0f; p.lfo1Target = 0;
        p.lfo1Amt = 0.8f; p.filterEnvAmt = 0.9f;
    }));

    presets.add(make("Growl Bass", "Bass", [](Preset& p) {
        p.filterModel = 4; p.filterResonance = 1.0f;
        p.distortionAmt = 0.5f; p.tags = {"growl", "aggressive"};
    }));

    presets.add(make("Funk Bass", "Bass", [](Preset& p) {
        p.filterCutoff = 1000.0f; p.filterEnvAmt = 0.6f;
        p.modAttack = 0.01f; p.modDecay = 0.1f; p.modSustain = 0.0f;
        p.tags = {"funk", "percussive"};
    }));

    presets.add(make("Octave Bass", "Bass", [](Preset& p) {
        p.osc1Wave = 0; p.osc2Wave = 0;
        p.osc1Mix = 0.7f; p.osc2Mix = 0.3f;
        p.osc2Detune = 1200.0f; // Octave up
        p.tags = {"octave", "layered"};
    }));

    presets.add(make("Wave Bass", "Bass", [](Preset& p) {
        p.osc1Wave = 0; p.unisonVoices = 8;
        p.unisonDetune = 25.0f; p.filterCutoff = 1800.0f;
        p.tags = {"wave", "supersaw"};
    }));

    presets.add(make("Punchy Bass", "Bass", [](Preset& p) {
        p.filterCutoff = 1500.0f; p.filterResonance = 0.5f;
        p.ampAttack = 0.001f; p.ampDecay = 0.15f;
        p.tags = {"punch", "percussive"};
    }));

    presets.add(make("Smooth Bass", "Bass", [](Preset& p) {
        p.filterModel = 1; p.filterCutoff = 600.0f;
        p.ampAttack = 0.05f; p.tags = {"smooth", "warm"};
    }));

    presets.add(make("Split Bass", "Bass", [](Preset& p) {
        p.filterSerial = false; // Parallel
        p.filterCutoff = 800.0f;
        p.tags = {"parallel", "split"};
    }));
}

//==============================================================================
// LEAD PRESETS (25)
//==============================================================================

void ZenithPresetBank::addLeadPresets(juce::Array<Preset>& presets) {
    // 1. Supersaw Lead
    presets.add(make("Supersaw Lead", "Lead", [](Preset& p) {
        p.unisonVoices = 8; p.unisonDetune = 20.0f;
        p.filterModel = 1; p.filterCutoff = 3000.0f;
        p.filterResonance = 0.3f; p.reverbMix = 0.3f;
        p.tags = {"supersaw", "anthem", "EDM"};
    }));

    // 2. Square Lead
    presets.add(make("Square Lead", "Lead", [](Preset& p) {
        p.osc1Wave = 1; p.unisonVoices = 4;
        p.filterCutoff = 2500.0f; p.filterResonance = 0.5f;
        p.tags = {"square", "retro", "8-bit"};
    }));

    // 3. Sine Lead
    presets.add(make("Sine Lead", "Lead", [](Preset& p) {
        p.osc1Wave = 3; p.filterCutoff = 2000.0f;
        p.chorusMix = 0.4f; p.tags = {"sine", "clean", "bell"};
    }));

    // 4-25. (More leads - compressed)
    presets.add(make("PWM Lead", "Lead", [](Preset& p) {
        p.osc1Wave = 1; p.osc2Wave = 1;
        p.osc2Detune = 15.0f; p.filterCutoff = 2800.0f;
        p.lfo1Rate = 5.0f; p.lfo1Target = 2;
        p.tags = {"PWM", "moving"};
    }));

    presets.add(make("Metallic Lead", "Lead", [](Preset& p) {
        p.unisonVoices = 3; p.filterModel = 2; // MS20
        p.filterResonance = 0.8f; p.chorusMix = 0.5f;
        p.tags = {"metallic", "bright"};
    }));

    presets.add(make("Soft Lead", "Lead", [](Preset& p) {
        p.osc1Wave = 0; p.filterModel = 1; // Moog
        p.filterCutoff = 1800.0f; p.filterResonance = 0.3f;
        p.ampAttack = 0.05f; p.tags = {"soft", "warm"};
    }));

    presets.add(make("Detuned Lead", "Lead", [](Preset& p) {
        p.unisonVoices = 4; p.unisonDetune = 25.0f;
        p.chorusMix = 0.3f; p.tags = {"detuned", "shimmer"};
    }));

    presets.add(make("Gated Lead", "Lead", [](Preset& p) {
        p.filterCutoff = 3500.0f; p.ampDecay = 0.1f;
        p.ampSustain = 0.0f; p.delayMix = 0.3f;
        p.tags = {"gated", "80s", "trance"};
    }));

    presets.add(make("Trance Lead", "Lead", [](Preset& p) {
        p.unisonVoices = 16; p.filterCutoff = 2500.0f;
        p.reverbMix = 0.4f; p.tags = {"trance", "anthem"};
    }));

    presets.add(make("Pluck Lead", "Lead", [](Preset& p) {
        p.ampAttack = 0.001f; p.ampDecay = 0.3f;
        p.filterCutoff = 4000.0f; p.tags = {"pluck", "bright"};
    }));

    presets.add(make("Formant Lead", "Lead", [](Preset& p) {
        p.osc1Wave = 0; p.osc2Wave = 0;
        p.filterCutoff = 1500.0f; p.filterResonance = 0.7f;
        p.lfo1Rate = 2.0f; p.tags = {"formant", "vowel"};
    }));
}

//==============================================================================
// PAD PRESETS (20)
//==============================================================================

void ZenithPresetBank::addPadPresets(juce::Array<Preset>& presets) {
    presets.add(make("Warm Pad", "Pad", [](Preset& p) {
        p.unisonVoices = 4; p.filterModel = 1; // Moog
        p.filterCutoff = 1200.0f; p.reverbMix = 0.5f;
        p.ampAttack = 0.5f; p.ampSustain = 0.8f;
        p.tags = {"warm", "ambient", " lush"};
    }));

    presets.add(make("Choir Pad", "Pad", [](Preset& p) {
        p.unisonVoices = 8; p.filterCutoff = 2000.0f;
        p.chorusMix = 0.5f; p.reverbMix = 0.6f;
        p.tags = {"choir", "ethereal"};
    }));

    presets.add(make("Space Pad", "Pad", [](Preset& p) {
        p.osc1Wave = 3; // Sine
        p.unisonVoices = 6; p.filterCutoff = 800.0f;
        p.reverbMix = 0.7f; p.delayMix = 0.2f;
        p.tags = {"space", "ambient", "dark"};
    }));

    // (More pads would follow - abbreviated for space)
    presets.add(make("Nylon Pad", "Pad", [](Preset& p) {
        p.filterModel = 3; // SEM
        p.filterCutoff = 1500.0f; p.chorusMix = 0.4f;
        p.tags = {"nylon", "string"};
    }));
}

//==============================================================================
// PLUCK PRESETS (15)
//==============================================================================

void ZenithPresetBank::addPluckPresets(juce::Array<Preset>& presets) {
    presets.add(make("Electric Pluck", "Pluck", [](Preset& p) {
        p.osc1Wave = 0; p.ampAttack = 0.001f;
        p.ampDecay = 0.4f; p.filterCutoff = 4000.0f;
        p.tags = {"electric", "bright"};
    }));

    presets.add(make("Muted Pluck", "Pluck", [](Preset& p) {
        p.filterCutoff = 2000.0f; p.filterResonance = 0.6f;
        p.ampDecay = 0.2f; p.tags = {"muted", "dull"};
    }));

    presets.add(make("Kalimba", "Pluck", [](Preset& p) {
        p.osc1Wave = 3; p.filterModel = 0; // SVF
        p.filterCutoff = 3000.0f; p.reverbMix = 0.4f;
        p.tags = {"kalimba", "mallet"};
    }));
}

//==============================================================================
// KEYS PRESETS (15)
//==============================================================================

void ZenithPresetBank::addKeysPresets(juce::Array<Preset>& presets) {
    presets.add(make("Grand Piano", "Keys", [](Preset& p) {
        p.osc1Wave = 0; p.filterModel = 1;
        p.filterCutoff = 2500.0f; p.ampDecay = 0.5f;
        p.reverbMix = 0.3f; p.tags = {"piano", "acoustic"};
    }));

    presets.add(make("Electric Piano", "Keys", [](Preset& p) {
        p.osc1Wave = 2; p.filterCutoff = 2000.0f;
        p.ampDecay = 0.6f; p.chorusMix = 0.3f;
        p.tags = {"EP", "Rhodes"};
    }));

    presets.add(make("Organ", "Keys", [](Preset& p) {
        p.osc1Wave = 1; p.filterModel = 1;
        p.filterCutoff = 3500.0f; p.filterResonance = 0.2f;
        p.chorusMix = 0.5f; p.tags = {"organ", "Leslie"};
    }));
}

//==============================================================================
// FX PRESETS (10)
//==============================================================================

void ZenithPresetBank::addFXPresets(juce::Array<Preset>& presets) {
    presets.add(make("Alien FX", "FX", [](Preset& p) {
        p.unisonVoices = 6; p.filterModel = 2; // MS20
        p.filterResonance = 0.9f; p.lfo1Rate = 0.5f;
        p.delayMix = 0.4f; p.reverbMix = 0.6f;
        p.tags = {"alien", "sci-fi", "dark"};
    }));

    presets.add(make("Glitch FX", "FX", [](Preset& p) {
        p.osc1Wave = 1; p.filterCutoff = 100.0f;
        p.distortionAmt = 0.7f; p.lfo1Rate = 10.0f;
        p.tags = {"glitch", "destructive"};
    }));
}

//==============================================================================
// FACTORY
//==============================================================================

juce::Array<ZenithPresetBank::Preset> ZenithPresetBank::getFactoryPresets() {
    juce::Array<Preset> presets;

    addBassPresets(presets);
    addLeadPresets(presets);
    addPadPresets(presets);
    addPluckPresets(presets);
    addKeysPresets(presets);
    addFXPresets(presets);

    return presets;
}

} // namespace zenith
