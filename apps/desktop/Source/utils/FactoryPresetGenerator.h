/*
  ==============================================================================

    FactoryPresetGenerator.h
    Created: 2025-11-20
    Author:  Zenith DAW

    Utility to generate procedural presets for built-in instruments.
*/
#pragma once

#include <juce_data_structures/juce_data_structures.h>
#include "../instruments/InstrumentPreset.h"
#include "../instruments/ZenithPolySynth.h"
#include "../../Source/instruments/ZenithPresetManager.h"

namespace zenith {

class FactoryPresetGenerator
{
public:
    static void generateFactoryPresets()
    {
        generatePolySynthPresets();
    }

private:
    static void generatePolySynthPresets()
    {
        // Check if presets already exist to avoid re-generation
        auto userDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
        auto presetDir = userDataDir.getChildFile("Zenith/Instruments/Factory/zenith.poly_synth");
        
        if (presetDir.exists() && presetDir.getNumberOfChildFiles(juce::File::findFiles, "*.zpreset") > 0)
        {
            DBG("Factory presets for Zenith Poly Synth already exist. Skipping generation.");
            return;
        }

        zenith::ZenithPresetManager& manager = zenith::ZenithPresetManager::getInstance();
        juce::Random random(12345); // Fixed seed for reproducibility

        const std::vector<std::string> types = { "Bass", "Lead", "Pad", "Pluck", "Keys", "FX", "Sequence" };
        const std::vector<std::string> characters = { "Dark", "Bright", "Warm", "Cold", "Aggressive", "Soft", "Evolving", "Punchy" };

        // Generate 500 presets
        for (int i = 0; i < 500; ++i)
        {
            // Pick random type and character
            std::string type = types[random.nextInt(types.size())];
            std::string character = characters[random.nextInt(characters.size())];

            // Create preset
            std::string name = character + " " + type + " " + juce::String(i + 1).toStdString();
            zenith::Preset preset;
            preset.name = name;
            preset.instrumentId = "zenith.poly_synth";
            preset.author = "Zenith AI";
            
            preset.category = type;
            preset.tags.push_back(character);
            preset.tags.push_back(type);

            // Set parameters based on type and character
            setPolySynthParameters(preset, type, character, random);

            // Save as factory preset
            manager.savePreset(preset, true); // true for factory
        }
        
        DBG("Generated 500 presets for Zenith Poly Synth");
    }

    static void setPolySynthParameters(zenith::Preset& preset, const std::string& type, const std::string& character, juce::Random& random)
    {
        // Helper to set param
        auto set = [&](const std::string& id, float val) { preset.parameters[id] = val; };
        
        // Base parameters
        float cutoff = 2000.0f;
        float resonance = 0.7f;
        float attack = 0.01f;
        float decay = 0.5f;
        float sustain = 0.5f;
        float release = 0.2f;
        int oscWave = 1; // Saw

        // Type logic
        if (type == "Bass")
        {
            cutoff = random.nextFloat() * 500.0f + 100.0f; // Low cutoff
            resonance = random.nextFloat() * 0.5f;
            attack = 0.01f;
            decay = random.nextFloat() * 0.4f + 0.1f;
            sustain = random.nextFloat() * 0.8f;
            release = random.nextFloat() * 0.3f;
            oscWave = 1; // Saw
            set("osc1_detune", random.nextFloat() * 5.0f); // Slight detune
            set("osc1_mix", 1.0f);
            set("osc2_mix", 0.5f);
            set("osc2_detune", -5.0f);
            if (character == "Aggressive") set("distortion", random.nextFloat() * 0.5f);
        }
        else if (type == "Pad")
        {
            cutoff = random.nextFloat() * 2000.0f + 500.0f;
            attack = random.nextFloat() * 1.0f + 0.5f; // Slow attack
            decay = random.nextFloat() * 1.0f + 1.0f;
            sustain = 0.8f + random.nextFloat() * 0.2f;
            release = random.nextFloat() * 1.5f + 0.5f; // Long release
            oscWave = 5; // Supersaw
            set("osc1_mix", 0.7f);
            set("osc2_mix", 0.7f);
            set("chorus", 0.2f + random.nextFloat() * 0.3f);
        }
        else if (type == "Lead")
        {
            cutoff = random.nextFloat() * 5000.0f + 2000.0f; // High cutoff
            attack = 0.01f;
            decay = random.nextFloat() * 0.5f;
            sustain = 1.0f;
            release = random.nextFloat() * 0.3f;
            oscWave = 1; // Saw
            set("osc1_detune", random.nextFloat() * 2.0f);
            if (character == "Aggressive") set("distortion", 0.3f);
        }
        else if (type == "Pluck")
        {
            cutoff = random.nextFloat() * 1000.0f + 500.0f;
            attack = 0.0f;
            decay = random.nextFloat() * 0.3f + 0.1f; // Short decay
            sustain = 0.0f;
            release = random.nextFloat() * 0.3f + 0.1f;
            oscWave = 2; // Square
        }
        else if (type == "FX")
        {
            oscWave = 4; // Noise
            if (random.nextFloat() > 0.5f) oscWave = 5; // Supersaw
            set("lfo1_rate", random.nextFloat() * 15.0f);
            set("distortion", random.nextFloat());
        }
        
        // Character logic adjustments
        if (character == "Dark")
        {
            cutoff *= 0.6f;
        }
        else if (character == "Bright")
        {
            cutoff = std::min(20000.0f, cutoff * 1.4f);
            set("distortion", 0.1f);
        }

        // Apply calculated values
        set("osc1_wave", (float)oscWave);
        set("filter_cutoff", cutoff);
        set("filter_resonance", resonance);
        set("amp_attack", attack);
        set("amp_decay", decay);
        set("amp_sustain", sustain);
        set("amp_release", release);
        
        // Randomize others slightly
        set("lfo1_rate", random.nextFloat() * 10.0f);
        set("lfo2_rate", random.nextFloat() * 5.0f);
        
        set("filter2_cutoff", random.nextFloat() * 10000.0f + 1000.0f);
        set("filter2_resonance", random.nextFloat() * 0.5f);
    }
};

} // namespace zenith
