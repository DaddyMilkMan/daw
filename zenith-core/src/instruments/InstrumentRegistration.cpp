/**
 * @file InstrumentRegistration.cpp
 * @brief Registers all built-in instruments with the InstrumentRegistry
 *
 * This file is called during application startup to register all built-in
 * instruments. Call registerAllInstruments() from your main initialization code.
 */

#include "../../include/instruments/InstrumentRegistry.h"
#include "../../include/instruments/ZenithPolySynth.h"
#include "../../include/instruments/ZenithSampler.h"

namespace zenith {

/**
 * @brief Register all built-in instruments
 *
 * This function should be called once during application startup.
 * It registers all built-in instruments with the InstrumentRegistry.
 *
 * Example usage:
 * @code
 * void Application::initialise()
 * {
 *     zenith::registerAllInstruments();
 *     // ... rest of initialization
 * }
 * @endcode
 */
void registerAllInstruments()
{
    auto& registry = InstrumentRegistry::getInstance();

    // Register ZenithPolySynth
    {
        auto metadata = ZenithPolySynth().getInstrumentMetadata();
        auto factory = []() -> std::unique_ptr<ZenithInstrumentProcessor> {
            return std::make_unique<ZenithPolySynth>();
        };
        registry.registerInstrument(metadata, factory);
    }

    // Register ZenithSampler
    {
        auto metadata = ZenithSampler().getInstrumentMetadata();
        auto factory = []() -> std::unique_ptr<ZenithInstrumentProcessor> {
            return std::make_unique<ZenithSampler>();
        };
        registry.registerInstrument(metadata, factory);
    }
}

/**
 * @brief Create factory presets for all instruments
 *
 * This function creates and saves factory presets for all built-in instruments.
 * Call this during first-time setup or when updating presets.
 *
 * @note This should only be called during development or on first run
 */
void createFactoryPresets()
{
    auto& registry = InstrumentRegistry::getInstance();
    auto& presetManager = registry.getPresetManager();

    // Create factory presets for ZenithPolySynth
    {
        // Preset 1: Warm Pad
        ZenithInstrumentPreset warmPad("Warm Pad", "zenith_poly_synth", "Factory");
        warmPad.description = "Warm, lush pad sound";
        warmPad.tags = {"pad", "warm", "lush"};
        warmPad.setParameter("osc_wave", 0.0f);  // Sine
        warmPad.setParameter("filter_cutoff", 0.6f);
        warmPad.setParameter("filter_resonance", 0.3f);
        warmPad.setParameter("amp_attack", 0.8f);
        warmPad.setParameter("amp_release", 2.0f);
        warmPad.setMacro("macro_warmth", 0.7f);
        warmPad.setMacro("macro_space", 0.8f);
        presetManager.saveFactoryPreset(warmPad);

        // Preset 2: Plucky Lead
        ZenithInstrumentPreset pluckyLead("Plucky Lead", "zenith_poly_synth", "Factory");
        pluckyLead.description = "Bright, punchy lead sound";
        pluckyLead.tags = {"lead", "pluck", "bright"};
        pluckyLead.setParameter("osc_wave", 1.0f);  // Saw
        pluckyLead.setParameter("filter_cutoff", 0.8f);
        pluckyLead.setParameter("filter_resonance", 0.4f);
        pluckyLead.setParameter("amp_attack", 0.01f);
        pluckyLead.setParameter("amp_decay", 0.3f);
        pluckyLead.setParameter("amp_sustain", 0.4f);
        pluckyLead.setParameter("amp_release", 0.2f);
        pluckyLead.setMacro("macro_bite", 0.6f);
        pluckyLead.setMacro("macro_movement", 0.4f);
        presetManager.saveFactoryPreset(pluckyLead);

        // Preset 3: Bass
        ZenithInstrumentPreset bass("Deep Bass", "zenith_poly_synth", "Factory");
        bass.description = "Deep, powerful bass sound";
        bass.tags = {"bass", "deep", "sub"};
        bass.setParameter("osc_wave", 2.0f);  // Square
        bass.setParameter("filter_cutoff", 0.3f);
        bass.setParameter("filter_resonance", 0.5f);
        bass.setParameter("amp_attack", 0.01f);
        bass.setParameter("amp_decay", 0.4f);
        bass.setParameter("amp_sustain", 0.6f);
        bass.setParameter("amp_release", 0.3f);
        bass.setMacro("macro_warmth", 0.3f);
        presetManager.saveFactoryPreset(bass);
    }

    // Create factory presets for ZenithSampler
    {
        // Preset 1: Natural
        ZenithInstrumentPreset natural("Natural", "zenith_sampler", "Factory");
        natural.description = "Clean, unprocessed sample playback";
        natural.tags = {"natural", "clean", "acoustic"};
        natural.setParameter("filter_cutoff", 1.0f);
        natural.setParameter("filter_resonance", 0.0f);
        natural.setParameter("env_attack", 0.001f);
        natural.setParameter("env_sustain", 1.0f);
        natural.setParameter("env_release", 0.1f);
        natural.setMacro("macro_body", 0.5f);
        natural.setMacro("macro_tone", 0.7f);
        presetManager.saveFactoryPreset(natural);

        // Preset 2: Punchy
        ZenithInstrumentPreset punchy("Punchy", "zenith_sampler", "Factory");
        punchy.description = "Tight, punchy sound with fast attack";
        punchy.tags = {"punchy", "tight", "percussion"};
        punchy.setParameter("filter_cutoff", 0.8f);
        punchy.setParameter("filter_resonance", 0.2f);
        punchy.setParameter("env_attack", 0.001f);
        punchy.setParameter("env_release", 0.05f);
        punchy.setMacro("macro_snap", 0.8f);
        punchy.setMacro("macro_tone", 0.6f);
        presetManager.saveFactoryPreset(punchy);

        // Preset 3: LoFi Vinyl
        ZenithInstrumentPreset lofi("LoFi Vinyl", "zenith_sampler", "Factory");
        lofi.description = "Warm, lo-fi vinyl character";
        lofi.tags = {"lofi", "vintage", "vinyl"};
        lofi.setParameter("filter_cutoff", 0.4f);
        lofi.setParameter("filter_resonance", 0.3f);
        lofi.setParameter("env_attack", 0.005f);
        lofi.setParameter("env_sustain", 0.9f);
        lofi.setParameter("env_release", 0.2f);
        lofi.setMacro("macro_lofi", 0.7f);
        lofi.setMacro("macro_body", 0.6f);
        presetManager.saveFactoryPreset(lofi);
    }
}

} // namespace zenith
