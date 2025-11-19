/*
  ==============================================================================

    GeneratePolySynthPresets.cpp
    Created: 2025-11-18
    Author:  Zenith DAW

    Procedural preset generator for ZenithPolySynth.
    Generates 500+ high-quality presets across popular sound categories.

  ==============================================================================
*/

#include <JuceHeader.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <random>
#include <cmath>

//==============================================================================
/**
 * @brief Preset parameter data structure
 */
struct PresetParams
{
    // Oscillators
    float osc1_wave = 0.0f;
    float osc1_detune = 0.5f;  // normalized 0-1 (maps to -50 to +50 cents)
    float osc1_mix = 1.0f;

    float osc2_wave = 0.0f;
    float osc2_detune = 0.5f;
    float osc2_mix = 0.5f;

    float osc3_wave = 0.0f;
    float osc3_detune = 0.5f;
    float osc3_mix = 0.0f;

    // Unison
    float unison_voices = 0.0f;  // normalized (1-7 voices)
    float unison_detune = 0.2f;  // normalized (0-50 cents)

    // Filter
    float filter_type = 0.0f;    // 0=LP, 0.5=BP, 1.0=HP
    float filter_cutoff = 0.7f;
    float filter_resonance = 0.3f;
    float filter_drive = 0.0f;   // normalized (1.0-5.0x)

    // Amp Envelope
    float amp_attack = 0.05f;
    float amp_decay = 0.2f;
    float amp_sustain = 0.8f;
    float amp_release = 0.3f;

    // Mod Envelope
    float mod_attack = 0.05f;
    float mod_decay = 0.5f;
    float mod_sustain = 0.0f;
    float mod_release = 0.1f;

    // LFO 1
    float lfo1_rate = 0.25f;     // normalized
    float lfo1_amount = 0.0f;
    float lfo1_target = 0.0f;    // 0=FilterCutoff, 0.25=Osc1Pitch, etc.

    // LFO 2
    float lfo2_rate = 0.1f;
    float lfo2_amount = 0.0f;
    float lfo2_target = 0.0f;

    // Global
    float glide_time = 0.0f;
    float mono_mode = 0.0f;      // 0 or 1
    float master_gain = 0.7f;
};

//==============================================================================
/**
 * @brief Preset metadata
 */
struct PresetMetadata
{
    std::string id;
    std::string name;
    std::string category;
    std::string description;
    std::vector<std::string> tags;
};

//==============================================================================
/**
 * @brief Preset category definition with parameter ranges
 */
struct CategoryDefinition
{
    std::string name;
    std::string description;

    // Parameter ranges (min, max)
    struct Range { float min; float max; };

    std::map<std::string, Range> paramRanges;
    std::vector<std::string> tags;

    // Number of variations to generate
    int numVariations = 20;
};

//==============================================================================
/**
 * @brief Deterministic random number generator
 */
class PresetRNG
{
public:
    PresetRNG(uint32_t seed) : gen(seed), dist(0.0f, 1.0f) {}

    float next() { return dist(gen); }

    float range(float min, float max)
    {
        return min + next() * (max - min);
    }

    int rangeInt(int min, int max)
    {
        return min + static_cast<int>(next() * (max - min + 1));
    }

private:
    std::mt19937 gen;
    std::uniform_real_distribution<float> dist;
};

//==============================================================================
/**
 * @brief Preset generator
 */
class PresetGenerator
{
public:
    PresetGenerator()
    {
        initializeCategoryDefinitions();
    }

    /**
     * @brief Generate all presets
     */
    std::vector<std::pair<PresetMetadata, PresetParams>> generateAllPresets()
    {
        std::vector<std::pair<PresetMetadata, PresetParams>> allPresets;

        int globalIndex = 0;

        for (const auto& category : categories_)
        {
            std::cout << "Generating " << category.numVariations
                      << " presets for category: " << category.name << std::endl;

            for (int i = 0; i < category.numVariations; ++i)
            {
                uint32_t seed = static_cast<uint32_t>(globalIndex * 12345 + 67890);

                auto [metadata, params] = generatePreset(category, i, seed);
                allPresets.push_back({metadata, params});

                globalIndex++;
            }
        }

        std::cout << "Total presets generated: " << allPresets.size() << std::endl;

        return allPresets;
    }

private:
    std::vector<CategoryDefinition> categories_;

    /**
     * @brief Initialize all category definitions
     */
    void initializeCategoryDefinitions()
    {
        // Waveform indices: 0=Sine, 0.2=Saw, 0.4=Square, 0.6=Triangle, 0.8=Noise, 1.0=Supersaw

        // =====================================================================
        // BASS CATEGORIES
        // =====================================================================

        // Sub Bass / 808
        {
            CategoryDefinition cat;
            cat.name = "Bass_Sub808";
            cat.description = "Classic sub bass and 808-style bass sounds";
            cat.numVariations = 30;
            cat.tags = {"bass", "sub", "808", "hip-hop", "trap"};

            cat.paramRanges["osc1_wave"] = {0.0f, 0.0f};  // Sine only
            cat.paramRanges["osc1_mix"] = {1.0f, 1.0f};
            cat.paramRanges["osc2_mix"] = {0.0f, 0.3f};   // Optional harmonics
            cat.paramRanges["osc2_wave"] = {0.0f, 0.6f};  // Sine to Triangle
            cat.paramRanges["filter_cutoff"] = {0.15f, 0.35f};
            cat.paramRanges["filter_resonance"] = {0.1f, 0.4f};
            cat.paramRanges["amp_attack"] = {0.0f, 0.02f};
            cat.paramRanges["amp_decay"] = {0.3f, 0.7f};
            cat.paramRanges["amp_sustain"] = {0.0f, 0.2f};
            cat.paramRanges["amp_release"] = {0.1f, 0.3f};

            categories_.push_back(cat);
        }

        // Reese Bass
        {
            CategoryDefinition cat;
            cat.name = "Bass_Reese";
            cat.description = "Detuned saw bass with thick, growling character";
            cat.numVariations = 25;
            cat.tags = {"bass", "reese", "dnb", "drum and bass", "dubstep"};

            cat.paramRanges["osc1_wave"] = {0.2f, 0.2f};  // Saw
            cat.paramRanges["osc1_mix"] = {1.0f, 1.0f};
            cat.paramRanges["osc2_wave"] = {0.2f, 0.2f};  // Saw
            cat.paramRanges["osc2_mix"] = {0.7f, 1.0f};
            cat.paramRanges["osc2_detune"] = {0.52f, 0.58f};  // 2-8 cents detune
            cat.paramRanges["unison_voices"] = {0.3f, 0.6f};  // 3-5 voices
            cat.paramRanges["unison_detune"] = {0.2f, 0.5f};  // 10-25 cents
            cat.paramRanges["filter_cutoff"] = {0.25f, 0.45f};
            cat.paramRanges["filter_resonance"] = {0.3f, 0.7f};
            cat.paramRanges["amp_attack"] = {0.0f, 0.03f};
            cat.paramRanges["amp_decay"] = {0.2f, 0.5f};
            cat.paramRanges["amp_sustain"] = {0.6f, 1.0f};

            categories_.push_back(cat);
        }

        // Wobble Bass
        {
            CategoryDefinition cat;
            cat.name = "Bass_Wobble";
            cat.description = "Dubstep-style wobble bass with LFO modulation";
            cat.numVariations = 20;
            cat.tags = {"bass", "wobble", "dubstep", "lfo", "modulation"};

            cat.paramRanges["osc1_wave"] = {0.2f, 0.4f};  // Saw to Square
            cat.paramRanges["osc1_mix"] = {1.0f, 1.0f};
            cat.paramRanges["filter_cutoff"] = {0.3f, 0.6f};
            cat.paramRanges["filter_resonance"] = {0.5f, 0.8f};
            cat.paramRanges["filter_drive"] = {0.2f, 0.6f};
            cat.paramRanges["lfo1_target"] = {0.0f, 0.0f};  // FilterCutoff
            cat.paramRanges["lfo1_rate"] = {0.15f, 0.35f};  // 1-8 Hz
            cat.paramRanges["lfo1_amount"] = {0.5f, 1.0f};
            cat.paramRanges["amp_attack"] = {0.0f, 0.02f};
            cat.paramRanges["amp_sustain"] = {0.8f, 1.0f};

            categories_.push_back(cat);
        }

        // FM Bass
        {
            CategoryDefinition cat;
            cat.name = "Bass_FM";
            cat.description = "FM-style bass with multiple oscillators";
            cat.numVariations = 15;
            cat.tags = {"bass", "fm", "modern", "digital"};

            cat.paramRanges["osc1_wave"] = {0.0f, 0.2f};  // Sine to Saw
            cat.paramRanges["osc1_mix"] = {1.0f, 1.0f};
            cat.paramRanges["osc2_wave"] = {0.0f, 0.4f};
            cat.paramRanges["osc2_mix"] = {0.3f, 0.7f};
            cat.paramRanges["osc2_detune"] = {0.6f, 0.8f};  // Higher detune
            cat.paramRanges["filter_cutoff"] = {0.2f, 0.5f};
            cat.paramRanges["filter_resonance"] = {0.3f, 0.6f};
            cat.paramRanges["amp_attack"] = {0.0f, 0.02f};
            cat.paramRanges["amp_decay"] = {0.2f, 0.5f};

            categories_.push_back(cat);
        }

        // =====================================================================
        // LEAD CATEGORIES
        // =====================================================================

        // Supersaw Lead
        {
            CategoryDefinition cat;
            cat.name = "Lead_Supersaw";
            cat.description = "Epic supersaw leads for festival/EDM";
            cat.numVariations = 40;
            cat.tags = {"lead", "supersaw", "edm", "festival", "trance"};

            cat.paramRanges["osc1_wave"] = {0.2f, 0.2f};  // Saw
            cat.paramRanges["osc1_mix"] = {1.0f, 1.0f};
            cat.paramRanges["osc2_wave"] = {0.2f, 0.2f};  // Saw
            cat.paramRanges["osc2_mix"] = {0.6f, 0.9f};
            cat.paramRanges["osc2_detune"] = {0.51f, 0.54f};  // Slight detune
            cat.paramRanges["unison_voices"] = {0.5f, 1.0f};  // 4-7 voices
            cat.paramRanges["unison_detune"] = {0.2f, 0.6f};  // 10-30 cents
            cat.paramRanges["filter_cutoff"] = {0.6f, 0.95f};
            cat.paramRanges["filter_resonance"] = {0.2f, 0.5f};
            cat.paramRanges["amp_attack"] = {0.05f, 0.25f};
            cat.paramRanges["amp_decay"] = {0.15f, 0.4f};
            cat.paramRanges["amp_sustain"] = {0.7f, 1.0f};

            categories_.push_back(cat);
        }

        // Pluck Lead
        {
            CategoryDefinition cat;
            cat.name = "Lead_Pluck";
            cat.description = "Sharp, plucky lead sounds";
            cat.numVariations = 20;
            cat.tags = {"lead", "pluck", "edm", "bright", "attack"};

            cat.paramRanges["osc1_wave"] = {0.2f, 0.6f};  // Saw to Triangle
            cat.paramRanges["osc1_mix"] = {1.0f, 1.0f};
            cat.paramRanges["filter_cutoff"] = {0.7f, 1.0f};
            cat.paramRanges["filter_resonance"] = {0.3f, 0.7f};
            cat.paramRanges["amp_attack"] = {0.0f, 0.01f};
            cat.paramRanges["amp_decay"] = {0.1f, 0.3f};
            cat.paramRanges["amp_sustain"] = {0.3f, 0.7f};
            cat.paramRanges["amp_release"] = {0.1f, 0.25f};
            cat.paramRanges["mod_attack"] = {0.0f, 0.01f};
            cat.paramRanges["mod_decay"] = {0.1f, 0.3f};

            categories_.push_back(cat);
        }

        // Sync Lead
        {
            CategoryDefinition cat;
            cat.name = "Lead_Sync";
            cat.description = "Aggressive sync-style leads";
            cat.numVariations = 15;
            cat.tags = {"lead", "sync", "aggressive", "harsh", "edm"};

            cat.paramRanges["osc1_wave"] = {0.4f, 0.4f};  // Square
            cat.paramRanges["osc1_mix"] = {1.0f, 1.0f};
            cat.paramRanges["osc2_wave"] = {0.2f, 0.2f};  // Saw
            cat.paramRanges["osc2_mix"] = {0.5f, 0.8f};
            cat.paramRanges["osc2_detune"] = {0.55f, 0.75f};  // Large detune
            cat.paramRanges["filter_cutoff"] = {0.5f, 0.85f};
            cat.paramRanges["filter_resonance"] = {0.4f, 0.8f};
            cat.paramRanges["amp_attack"] = {0.01f, 0.1f};

            categories_.push_back(cat);
        }

        // Brass Lead
        {
            CategoryDefinition cat;
            cat.name = "Lead_Brass";
            cat.description = "Brass-like synth leads";
            cat.numVariations = 15;
            cat.tags = {"lead", "brass", "trap", "orchestral", "warm"};

            cat.paramRanges["osc1_wave"] = {0.4f, 0.6f};  // Square to Triangle
            cat.paramRanges["osc1_mix"] = {1.0f, 1.0f};
            cat.paramRanges["osc2_wave"] = {0.2f, 0.4f};
            cat.paramRanges["osc2_mix"] = {0.3f, 0.6f};
            cat.paramRanges["filter_cutoff"] = {0.4f, 0.7f};
            cat.paramRanges["filter_resonance"] = {0.3f, 0.6f};
            cat.paramRanges["amp_attack"] = {0.1f, 0.3f};
            cat.paramRanges["amp_decay"] = {0.2f, 0.5f};
            cat.paramRanges["amp_sustain"] = {0.6f, 0.9f};

            categories_.push_back(cat);
        }

        // =====================================================================
        // PLUCK CATEGORIES
        // =====================================================================

        // EDM Pluck
        {
            CategoryDefinition cat;
            cat.name = "Pluck_EDM";
            cat.description = "Bright EDM pluck sounds";
            cat.numVariations = 30;
            cat.tags = {"pluck", "edm", "bright", "melodic"};

            cat.paramRanges["osc1_wave"] = {0.2f, 0.6f};
            cat.paramRanges["osc1_mix"] = {1.0f, 1.0f};
            cat.paramRanges["filter_cutoff"] = {0.7f, 1.0f};
            cat.paramRanges["filter_resonance"] = {0.2f, 0.6f};
            cat.paramRanges["amp_attack"] = {0.0f, 0.01f};
            cat.paramRanges["amp_decay"] = {0.05f, 0.2f};
            cat.paramRanges["amp_sustain"] = {0.1f, 0.4f};
            cat.paramRanges["amp_release"] = {0.05f, 0.15f};

            categories_.push_back(cat);
        }

        // Bell Pluck
        {
            CategoryDefinition cat;
            cat.name = "Pluck_Bell";
            cat.description = "Bell-like pluck sounds";
            cat.numVariations = 20;
            cat.tags = {"pluck", "bell", "melodic", "bright"};

            cat.paramRanges["osc1_wave"] = {0.0f, 0.0f};  // Sine
            cat.paramRanges["osc1_mix"] = {1.0f, 1.0f};
            cat.paramRanges["osc2_wave"] = {0.0f, 0.6f};
            cat.paramRanges["osc2_mix"] = {0.2f, 0.5f};
            cat.paramRanges["filter_cutoff"] = {0.75f, 1.0f};
            cat.paramRanges["filter_resonance"] = {0.5f, 0.85f};
            cat.paramRanges["amp_attack"] = {0.0f, 0.01f};
            cat.paramRanges["amp_decay"] = {0.15f, 0.4f};
            cat.paramRanges["amp_sustain"] = {0.2f, 0.5f};
            cat.paramRanges["amp_release"] = {0.2f, 0.5f};

            categories_.push_back(cat);
        }

        // Marimba
        {
            CategoryDefinition cat;
            cat.name = "Pluck_Marimba";
            cat.description = "Marimba-like mallet sounds";
            cat.numVariations = 15;
            cat.tags = {"pluck", "marimba", "mallet", "percussion"};

            cat.paramRanges["osc1_wave"] = {0.0f, 0.6f};  // Sine to Triangle
            cat.paramRanges["osc1_mix"] = {1.0f, 1.0f};
            cat.paramRanges["filter_cutoff"] = {0.6f, 0.9f};
            cat.paramRanges["filter_resonance"] = {0.1f, 0.4f};
            cat.paramRanges["amp_attack"] = {0.0f, 0.005f};
            cat.paramRanges["amp_decay"] = {0.1f, 0.3f};
            cat.paramRanges["amp_sustain"] = {0.0f, 0.2f};
            cat.paramRanges["amp_release"] = {0.1f, 0.3f};

            categories_.push_back(cat);
        }

        // =====================================================================
        // PAD CATEGORIES
        // =====================================================================

        // Warm Analog Pad
        {
            CategoryDefinition cat;
            cat.name = "Pad_WarmAnalog";
            cat.description = "Warm analog-style pads";
            cat.numVariations = 25;
            cat.tags = {"pad", "warm", "analog", "ambient", "lush"};

            cat.paramRanges["osc1_wave"] = {0.2f, 0.2f};  // Saw
            cat.paramRanges["osc1_mix"] = {1.0f, 1.0f};
            cat.paramRanges["osc2_wave"] = {0.2f, 0.2f};  // Saw
            cat.paramRanges["osc2_mix"] = {0.6f, 0.9f};
            cat.paramRanges["osc2_detune"] = {0.51f, 0.53f};  // Slight detune
            cat.paramRanges["osc3_wave"] = {0.2f, 0.4f};
            cat.paramRanges["osc3_mix"] = {0.3f, 0.6f};
            cat.paramRanges["filter_cutoff"] = {0.4f, 0.7f};
            cat.paramRanges["filter_resonance"] = {0.1f, 0.3f};
            cat.paramRanges["amp_attack"] = {0.2f, 0.5f};
            cat.paramRanges["amp_decay"] = {0.3f, 0.6f};
            cat.paramRanges["amp_sustain"] = {0.7f, 1.0f};
            cat.paramRanges["amp_release"] = {0.5f, 0.8f};

            categories_.push_back(cat);
        }

        // Glass/Digital Pad
        {
            CategoryDefinition cat;
            cat.name = "Pad_Glass";
            cat.description = "Bright, glassy digital pads";
            cat.numVariations = 20;
            cat.tags = {"pad", "glass", "digital", "bright", "ambient"};

            cat.paramRanges["osc1_wave"] = {0.0f, 0.0f};  // Sine
            cat.paramRanges["osc1_mix"] = {1.0f, 1.0f};
            cat.paramRanges["osc2_wave"] = {0.0f, 0.0f};  // Sine
            cat.paramRanges["osc2_mix"] = {0.4f, 0.7f};
            cat.paramRanges["filter_cutoff"] = {0.8f, 0.95f};
            cat.paramRanges["filter_resonance"] = {0.6f, 0.85f};
            cat.paramRanges["amp_attack"] = {0.2f, 0.4f};
            cat.paramRanges["amp_sustain"] = {0.7f, 0.9f};
            cat.paramRanges["amp_release"] = {0.4f, 0.7f};
            cat.paramRanges["lfo1_target"] = {0.0f, 0.0f};  // FilterCutoff
            cat.paramRanges["lfo1_rate"] = {0.05f, 0.15f};  // Slow
            cat.paramRanges["lfo1_amount"] = {0.1f, 0.3f};

            categories_.push_back(cat);
        }

        // Ambient Drone
        {
            CategoryDefinition cat;
            cat.name = "Pad_Drone";
            cat.description = "Dark ambient drone pads";
            cat.numVariations = 20;
            cat.tags = {"pad", "drone", "ambient", "dark", "atmospheric"};

            cat.paramRanges["osc1_wave"] = {0.2f, 0.4f};
            cat.paramRanges["osc1_mix"] = {1.0f, 1.0f};
            cat.paramRanges["osc2_wave"] = {0.2f, 0.4f};
            cat.paramRanges["osc2_mix"] = {0.7f, 1.0f};
            cat.paramRanges["osc3_wave"] = {0.0f, 0.4f};
            cat.paramRanges["osc3_mix"] = {0.4f, 0.7f};
            cat.paramRanges["filter_cutoff"] = {0.2f, 0.5f};
            cat.paramRanges["filter_resonance"] = {0.2f, 0.5f};
            cat.paramRanges["amp_attack"] = {0.4f, 0.8f};
            cat.paramRanges["amp_sustain"] = {0.8f, 1.0f};
            cat.paramRanges["amp_release"] = {0.6f, 0.9f};

            categories_.push_back(cat);
        }

        // String Pad
        {
            CategoryDefinition cat;
            cat.name = "Pad_Strings";
            cat.description = "String ensemble pads";
            cat.numVariations = 20;
            cat.tags = {"pad", "strings", "orchestral", "ensemble"};

            cat.paramRanges["osc1_wave"] = {0.2f, 0.2f};  // Saw
            cat.paramRanges["osc1_mix"] = {1.0f, 1.0f};
            cat.paramRanges["osc2_wave"] = {0.2f, 0.2f};
            cat.paramRanges["osc2_mix"] = {0.8f, 1.0f};
            cat.paramRanges["osc2_detune"] = {0.51f, 0.52f};
            cat.paramRanges["unison_voices"] = {0.2f, 0.5f};  // 2-4 voices
            cat.paramRanges["unison_detune"] = {0.1f, 0.3f};
            cat.paramRanges["filter_cutoff"] = {0.5f, 0.75f};
            cat.paramRanges["filter_resonance"] = {0.2f, 0.4f};
            cat.paramRanges["amp_attack"] = {0.15f, 0.35f};
            cat.paramRanges["amp_sustain"] = {0.8f, 1.0f};
            cat.paramRanges["amp_release"] = {0.4f, 0.7f};

            categories_.push_back(cat);
        }

        // =====================================================================
        // KEY CATEGORIES
        // =====================================================================

        // Electric Piano
        {
            CategoryDefinition cat;
            cat.name = "Keys_EP";
            cat.description = "Electric piano sounds";
            cat.numVariations = 20;
            cat.tags = {"keys", "ep", "electric piano", "rhodes"};

            cat.paramRanges["osc1_wave"] = {0.0f, 0.6f};  // Sine to Triangle
            cat.paramRanges["osc1_mix"] = {1.0f, 1.0f};
            cat.paramRanges["osc2_wave"] = {0.0f, 0.4f};
            cat.paramRanges["osc2_mix"] = {0.2f, 0.5f};
            cat.paramRanges["filter_cutoff"] = {0.6f, 0.85f};
            cat.paramRanges["filter_resonance"] = {0.1f, 0.3f};
            cat.paramRanges["amp_attack"] = {0.0f, 0.02f};
            cat.paramRanges["amp_decay"] = {0.2f, 0.5f};
            cat.paramRanges["amp_sustain"] = {0.5f, 0.8f};
            cat.paramRanges["amp_release"] = {0.2f, 0.4f};

            categories_.push_back(cat);
        }

        // Organ
        {
            CategoryDefinition cat;
            cat.name = "Keys_Organ";
            cat.description = "Organ-like key sounds";
            cat.numVariations = 15;
            cat.tags = {"keys", "organ", "hammond", "drawbar"};

            cat.paramRanges["osc1_wave"] = {0.0f, 0.0f};  // Sine
            cat.paramRanges["osc1_mix"] = {1.0f, 1.0f};
            cat.paramRanges["osc2_wave"] = {0.0f, 0.0f};
            cat.paramRanges["osc2_mix"] = {0.5f, 0.8f};
            cat.paramRanges["osc3_wave"] = {0.0f, 0.0f};
            cat.paramRanges["osc3_mix"] = {0.3f, 0.6f};
            cat.paramRanges["filter_cutoff"] = {0.7f, 0.95f};
            cat.paramRanges["amp_attack"] = {0.0f, 0.01f};
            cat.paramRanges["amp_sustain"] = {1.0f, 1.0f};
            cat.paramRanges["amp_release"] = {0.05f, 0.15f};

            categories_.push_back(cat);
        }

        // Clavinet
        {
            CategoryDefinition cat;
            cat.name = "Keys_Clav";
            cat.description = "Clavinet-style funky keys";
            cat.numVariations = 15;
            cat.tags = {"keys", "clav", "clavinet", "funky"};

            cat.paramRanges["osc1_wave"] = {0.4f, 0.4f};  // Square
            cat.paramRanges["osc1_mix"] = {1.0f, 1.0f};
            cat.paramRanges["filter_cutoff"] = {0.7f, 0.95f};
            cat.paramRanges["filter_resonance"] = {0.3f, 0.6f};
            cat.paramRanges["amp_attack"] = {0.0f, 0.005f};
            cat.paramRanges["amp_decay"] = {0.05f, 0.15f};
            cat.paramRanges["amp_sustain"] = {0.3f, 0.6f};
            cat.paramRanges["amp_release"] = {0.05f, 0.1f};

            categories_.push_back(cat);
        }

        // =====================================================================
        // BELL CATEGORIES
        // =====================================================================

        // Glass Bell
        {
            CategoryDefinition cat;
            cat.name = "Bell_Glass";
            cat.description = "Glassy, crystalline bell sounds";
            cat.numVariations = 20;
            cat.tags = {"bell", "glass", "crystal", "bright"};

            cat.paramRanges["osc1_wave"] = {0.0f, 0.0f};  // Sine
            cat.paramRanges["osc1_mix"] = {1.0f, 1.0f};
            cat.paramRanges["filter_cutoff"] = {0.85f, 1.0f};
            cat.paramRanges["filter_resonance"] = {0.7f, 0.95f};
            cat.paramRanges["amp_attack"] = {0.0f, 0.01f};
            cat.paramRanges["amp_decay"] = {0.3f, 0.6f};
            cat.paramRanges["amp_sustain"] = {0.3f, 0.6f};
            cat.paramRanges["amp_release"] = {0.4f, 0.8f};

            categories_.push_back(cat);
        }

        // Metal Bell
        {
            CategoryDefinition cat;
            cat.name = "Bell_Metal";
            cat.description = "Metallic bell sounds";
            cat.numVariations = 15;
            cat.tags = {"bell", "metal", "metallic", "percussion"};

            cat.paramRanges["osc1_wave"] = {0.4f, 0.4f};  // Square
            cat.paramRanges["osc1_mix"] = {1.0f, 1.0f};
            cat.paramRanges["osc2_wave"] = {0.0f, 0.0f};  // Sine
            cat.paramRanges["osc2_mix"] = {0.3f, 0.6f};
            cat.paramRanges["filter_cutoff"] = {0.75f, 0.95f};
            cat.paramRanges["filter_resonance"] = {0.6f, 0.85f};
            cat.paramRanges["amp_attack"] = {0.0f, 0.01f};
            cat.paramRanges["amp_decay"] = {0.3f, 0.6f};
            cat.paramRanges["amp_sustain"] = {0.2f, 0.5f};
            cat.paramRanges["amp_release"] = {0.4f, 0.8f};

            categories_.push_back(cat);
        }

        // =====================================================================
        // FX CATEGORIES
        // =====================================================================

        // Riser
        {
            CategoryDefinition cat;
            cat.name = "FX_Riser";
            cat.description = "Riser effects for builds and transitions";
            cat.numVariations = 15;
            cat.tags = {"fx", "riser", "build", "transition"};

            cat.paramRanges["osc1_wave"] = {0.8f, 0.8f};  // Noise
            cat.paramRanges["osc1_mix"] = {1.0f, 1.0f};
            cat.paramRanges["filter_cutoff"] = {0.1f, 0.3f};  // Start low
            cat.paramRanges["filter_resonance"] = {0.4f, 0.7f};
            cat.paramRanges["amp_attack"] = {0.5f, 0.9f};  // Long attack
            cat.paramRanges["amp_sustain"] = {1.0f, 1.0f};
            cat.paramRanges["mod_attack"] = {0.5f, 0.9f};  // Filter sweep
            cat.paramRanges["mod_decay"] = {0.3f, 0.6f};
            cat.paramRanges["mod_sustain"] = {1.0f, 1.0f};

            categories_.push_back(cat);
        }

        // Impact
        {
            CategoryDefinition cat;
            cat.name = "FX_Impact";
            cat.description = "Impact and hit sounds";
            cat.numVariations = 15;
            cat.tags = {"fx", "impact", "hit", "percussion"};

            cat.paramRanges["osc1_wave"] = {0.8f, 0.8f};  // Noise
            cat.paramRanges["osc1_mix"] = {1.0f, 1.0f};
            cat.paramRanges["osc2_wave"] = {0.0f, 0.4f};
            cat.paramRanges["osc2_mix"] = {0.3f, 0.7f};
            cat.paramRanges["filter_cutoff"] = {0.3f, 0.7f};
            cat.paramRanges["filter_resonance"] = {0.3f, 0.7f};
            cat.paramRanges["amp_attack"] = {0.0f, 0.005f};
            cat.paramRanges["amp_decay"] = {0.1f, 0.3f};
            cat.paramRanges["amp_sustain"] = {0.0f, 0.2f};
            cat.paramRanges["amp_release"] = {0.1f, 0.3f};

            categories_.push_back(cat);
        }

        // =====================================================================
        // ARP CATEGORIES
        // =====================================================================

        // Pulse Arp
        {
            CategoryDefinition cat;
            cat.name = "Arp_Pulse";
            cat.description = "Pulsing arpeggio sounds";
            cat.numVariations = 15;
            cat.tags = {"arp", "pulse", "sequence", "rhythmic"};

            cat.paramRanges["osc1_wave"] = {0.4f, 0.4f};  // Square
            cat.paramRanges["osc1_mix"] = {1.0f, 1.0f};
            cat.paramRanges["filter_cutoff"] = {0.5f, 0.8f};
            cat.paramRanges["filter_resonance"] = {0.3f, 0.6f};
            cat.paramRanges["amp_attack"] = {0.0f, 0.01f};
            cat.paramRanges["amp_decay"] = {0.05f, 0.15f};
            cat.paramRanges["amp_sustain"] = {0.2f, 0.5f};
            cat.paramRanges["amp_release"] = {0.05f, 0.1f};

            categories_.push_back(cat);
        }

        // Sequence Arp
        {
            CategoryDefinition cat;
            cat.name = "Arp_Sequence";
            cat.description = "Bright sequencer-style arpeggios";
            cat.numVariations = 15;
            cat.tags = {"arp", "sequence", "bright", "trance"};

            cat.paramRanges["osc1_wave"] = {0.2f, 0.6f};  // Saw to Triangle
            cat.paramRanges["osc1_mix"] = {1.0f, 1.0f};
            cat.paramRanges["filter_cutoff"] = {0.7f, 1.0f};
            cat.paramRanges["filter_resonance"] = {0.2f, 0.5f};
            cat.paramRanges["amp_attack"] = {0.0f, 0.01f};
            cat.paramRanges["amp_decay"] = {0.05f, 0.15f};
            cat.paramRanges["amp_sustain"] = {0.3f, 0.6f};
            cat.paramRanges["amp_release"] = {0.05f, 0.15f};
            cat.paramRanges["mod_attack"] = {0.0f, 0.01f};
            cat.paramRanges["mod_decay"] = {0.05f, 0.15f};

            categories_.push_back(cat);
        }

        std::cout << "Initialized " << categories_.size() << " preset categories" << std::endl;
    }

    /**
     * @brief Generate a single preset from a category definition
     */
    std::pair<PresetMetadata, PresetParams> generatePreset(
        const CategoryDefinition& category,
        int variationIndex,
        uint32_t seed)
    {
        PresetRNG rng(seed);

        // Create metadata
        PresetMetadata meta;
        meta.id = category.name + "_" + std::to_string(variationIndex + 1);
        meta.name = generatePresetName(category, variationIndex, rng);
        meta.category = category.name;
        meta.description = category.description;
        meta.tags = category.tags;

        // Create parameters with defaults
        PresetParams params;

        // Apply category parameter ranges
        for (const auto& [paramName, range] : category.paramRanges)
        {
            float value = rng.range(range.min, range.max);

            // Set the parameter using string matching
            if (paramName == "osc1_wave") params.osc1_wave = value;
            else if (paramName == "osc1_detune") params.osc1_detune = value;
            else if (paramName == "osc1_mix") params.osc1_mix = value;
            else if (paramName == "osc2_wave") params.osc2_wave = value;
            else if (paramName == "osc2_detune") params.osc2_detune = value;
            else if (paramName == "osc2_mix") params.osc2_mix = value;
            else if (paramName == "osc3_wave") params.osc3_wave = value;
            else if (paramName == "osc3_detune") params.osc3_detune = value;
            else if (paramName == "osc3_mix") params.osc3_mix = value;
            else if (paramName == "unison_voices") params.unison_voices = value;
            else if (paramName == "unison_detune") params.unison_detune = value;
            else if (paramName == "filter_type") params.filter_type = value;
            else if (paramName == "filter_cutoff") params.filter_cutoff = value;
            else if (paramName == "filter_resonance") params.filter_resonance = value;
            else if (paramName == "filter_drive") params.filter_drive = value;
            else if (paramName == "amp_attack") params.amp_attack = value;
            else if (paramName == "amp_decay") params.amp_decay = value;
            else if (paramName == "amp_sustain") params.amp_sustain = value;
            else if (paramName == "amp_release") params.amp_release = value;
            else if (paramName == "mod_attack") params.mod_attack = value;
            else if (paramName == "mod_decay") params.mod_decay = value;
            else if (paramName == "mod_sustain") params.mod_sustain = value;
            else if (paramName == "mod_release") params.mod_release = value;
            else if (paramName == "lfo1_rate") params.lfo1_rate = value;
            else if (paramName == "lfo1_amount") params.lfo1_amount = value;
            else if (paramName == "lfo1_target") params.lfo1_target = value;
            else if (paramName == "lfo2_rate") params.lfo2_rate = value;
            else if (paramName == "lfo2_amount") params.lfo2_amount = value;
            else if (paramName == "lfo2_target") params.lfo2_target = value;
        }

        // Ensure sensible loudness (limit master gain based on complexity)
        float complexity = calculateComplexity(params);
        params.master_gain = 0.7f / std::sqrt(complexity);  // Scale down for complex sounds
        params.master_gain = std::min(params.master_gain, 0.85f);

        return {meta, params};
    }

    /**
     * @brief Generate a creative name for the preset
     */
    std::string generatePresetName(const CategoryDefinition& category, int index, PresetRNG& rng)
    {
        // Name prefixes/suffixes for variety
        static const std::vector<std::string> adjectives = {
            "Deep", "Bright", "Dark", "Warm", "Cold", "Crisp", "Soft",
            "Hard", "Wide", "Tight", "Fat", "Thin", "Smooth", "Rough",
            "Clean", "Dirty", "Punchy", "Mellow", "Aggressive", "Gentle",
            "Epic", "Minimal", "Rich", "Sparse", "Dense", "Airy",
            "Analog", "Digital", "Modern", "Classic", "Vintage", "Future"
        };

        // Use index for deterministic naming
        int adjIndex = (index * 7) % adjectives.size();

        std::string baseName = category.name;
        // Remove prefix (e.g., "Bass_Sub808" -> "Sub808")
        size_t underscorePos = baseName.find('_');
        if (underscorePos != std::string::npos)
            baseName = baseName.substr(underscorePos + 1);

        return adjectives[adjIndex] + " " + baseName + " " + std::to_string(index + 1);
    }

    /**
     * @brief Calculate complexity factor for gain compensation
     */
    float calculateComplexity(const PresetParams& params)
    {
        float complexity = 1.0f;

        // More oscillators = more gain
        if (params.osc1_mix > 0.1f) complexity += params.osc1_mix;
        if (params.osc2_mix > 0.1f) complexity += params.osc2_mix;
        if (params.osc3_mix > 0.1f) complexity += params.osc3_mix;

        // Unison multiplies complexity
        float unisonVoices = 1.0f + params.unison_voices * 6.0f;  // 1-7 voices
        if (unisonVoices > 1.5f)
            complexity *= std::sqrt(unisonVoices);

        return complexity;
    }
};

//==============================================================================
/**
 * @brief Serialize preset to JSON
 */
std::string serializePresetToJSON(const PresetMetadata& meta, const PresetParams& params)
{
    std::ostringstream json;
    json << std::fixed << std::setprecision(4);

    json << "{\n";
    json << "  \"id\": \"" << meta.id << "\",\n";
    json << "  \"name\": \"" << meta.name << "\",\n";
    json << "  \"instrumentId\": \"zenith.poly_synth\",\n";
    json << "  \"category\": \"" << meta.category << "\",\n";
    json << "  \"author\": \"Factory\",\n";
    json << "  \"description\": \"" << meta.description << "\",\n";
    json << "  \"version\": \"1.0.0\",\n";

    // Tags
    json << "  \"tags\": [";
    for (size_t i = 0; i < meta.tags.size(); ++i)
    {
        json << "\"" << meta.tags[i] << "\"";
        if (i < meta.tags.size() - 1) json << ", ";
    }
    json << "],\n";

    // Parameters
    json << "  \"params\": {\n";
    json << "    \"osc1_wave\": " << params.osc1_wave << ",\n";
    json << "    \"osc1_detune\": " << params.osc1_detune << ",\n";
    json << "    \"osc1_mix\": " << params.osc1_mix << ",\n";
    json << "    \"osc2_wave\": " << params.osc2_wave << ",\n";
    json << "    \"osc2_detune\": " << params.osc2_detune << ",\n";
    json << "    \"osc2_mix\": " << params.osc2_mix << ",\n";
    json << "    \"osc3_wave\": " << params.osc3_wave << ",\n";
    json << "    \"osc3_detune\": " << params.osc3_detune << ",\n";
    json << "    \"osc3_mix\": " << params.osc3_mix << ",\n";
    json << "    \"unison_voices\": " << params.unison_voices << ",\n";
    json << "    \"unison_detune\": " << params.unison_detune << ",\n";
    json << "    \"filter_type\": " << params.filter_type << ",\n";
    json << "    \"filter_cutoff\": " << params.filter_cutoff << ",\n";
    json << "    \"filter_resonance\": " << params.filter_resonance << ",\n";
    json << "    \"filter_drive\": " << params.filter_drive << ",\n";
    json << "    \"amp_attack\": " << params.amp_attack << ",\n";
    json << "    \"amp_decay\": " << params.amp_decay << ",\n";
    json << "    \"amp_sustain\": " << params.amp_sustain << ",\n";
    json << "    \"amp_release\": " << params.amp_release << ",\n";
    json << "    \"mod_attack\": " << params.mod_attack << ",\n";
    json << "    \"mod_decay\": " << params.mod_decay << ",\n";
    json << "    \"mod_sustain\": " << params.mod_sustain << ",\n";
    json << "    \"mod_release\": " << params.mod_release << ",\n";
    json << "    \"lfo1_rate\": " << params.lfo1_rate << ",\n";
    json << "    \"lfo1_amount\": " << params.lfo1_amount << ",\n";
    json << "    \"lfo1_target\": " << params.lfo1_target << ",\n";
    json << "    \"lfo2_rate\": " << params.lfo2_rate << ",\n";
    json << "    \"lfo2_amount\": " << params.lfo2_amount << ",\n";
    json << "    \"lfo2_target\": " << params.lfo2_target << ",\n";
    json << "    \"glide_time\": " << params.glide_time << ",\n";
    json << "    \"mono_mode\": " << params.mono_mode << ",\n";
    json << "    \"master_gain\": " << params.master_gain << "\n";
    json << "  }\n";
    json << "}";

    return json.str();
}

//==============================================================================
/**
 * @brief Save preset to individual JSON file
 */
bool savePresetToFile(const std::string& outputDir,
                      const PresetMetadata& meta,
                      const PresetParams& params)
{
    std::string categoryDir = outputDir + "/" + meta.category;

    // Create category directory
    juce::File dir(categoryDir);
    if (!dir.exists())
        dir.createDirectory();

    std::string filename = categoryDir + "/" + meta.id + ".json";
    std::string jsonContent = serializePresetToJSON(meta, params);

    juce::File file(filename);
    file.replaceWithText(jsonContent);

    return true;
}

//==============================================================================
/**
 * @brief Main entry point
 */
int main(int argc, char* argv[])
{
    std::cout << "========================================" << std::endl;
    std::cout << "ZenithPolySynth Preset Generator" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // Determine output directory
    std::string outputDir = "/home/user/daw/zenith-core/Content/Presets/PolySynth_Generated";

    if (argc > 1)
        outputDir = argv[1];

    std::cout << "Output directory: " << outputDir << std::endl;
    std::cout << std::endl;

    // Create output directory
    juce::File outDir(outputDir);
    if (!outDir.exists())
    {
        std::cout << "Creating output directory..." << std::endl;
        outDir.createDirectory();
    }

    // Generate presets
    PresetGenerator generator;
    auto allPresets = generator.generateAllPresets();

    std::cout << std::endl;
    std::cout << "Saving presets to disk..." << std::endl;

    // Save all presets
    int savedCount = 0;
    for (const auto& [meta, params] : allPresets)
    {
        if (savePresetToFile(outputDir, meta, params))
            savedCount++;
    }

    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Successfully generated " << savedCount << " presets!" << std::endl;
    std::cout << "Presets saved to: " << outputDir << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}
