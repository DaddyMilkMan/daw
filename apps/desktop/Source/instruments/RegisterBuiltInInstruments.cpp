/*
  ==============================================================================

    RegisterBuiltInInstruments.cpp
    Created: 2025-11-18
    Author:  Zenith DAW

    Implementation of built-in instrument registration.

  ==============================================================================
*/

#include "RegisterBuiltInInstruments.h"
#include "../utils/FactoryPresetGenerator.h"
#include "InstrumentRegistry.h"
#include "ZenithPolySynth.h"
#include "ZenithSampler.h"


namespace zenith {

// Helper to reduce copy-paste code (Bug 66)
static void registerSamplerPreset(InstrumentRegistry& registry,
                                  const juce::String& id,
                                  const juce::String& name,
                                  const juce::String& category,
                                  const juce::String& description,
                                  const std::vector<juce::String>& tags,
                                  const char* bankJson) {
  auto metadata = ZenithSampler::createMetadata();
  metadata.instrumentId = "zenith_sampler." + id;
  metadata.name = name;
  metadata.category = category;
  metadata.description = description;
  juce::StringArray tagArray;
  for (const auto& t : tags) tagArray.add(t);
  metadata.tags = tagArray;

  // Capture json pointer by value (literal string persistence assumed/guaranteed by usage)
  registry.registerInstrument(metadata.instrumentId, metadata, [bankJson, name]() {
    auto sampler = std::make_unique<ZenithSampler>();
    auto *proc =
        dynamic_cast<ZenithSamplerProcessor *>(sampler->getAudioProcessor());
    if (proc && bankJson) {
      proc->loadSampleBankFromJson(bankJson, name);
    }
    return sampler;
  });
}

void registerBuiltInInstruments(InstrumentRegistry& registry) {
  // Register ZenithPolySynth
  registry.registerInstrument(
      "zenith_poly_synth", ZenithPolySynth::createMetadata(),
      []() { return std::make_unique<ZenithPolySynth>(); });

  // Register ZenithSampler (base/empty sampler)
  registry.registerInstrument(
      "zenith_sampler", ZenithSampler::createMetadata(),
      []() { return std::make_unique<ZenithSampler>(); });

  // Register canonical sampler instruments with pre-defined sample maps
  // These are factory templates that ship with Zenith

  // 1. 808 Essentials - Classic drum machine
  registerSamplerPreset(registry, "808_essentials", "808 Essentials", "drums",
      "Classic 808 drum sounds - kick, snare, hi-hats, and bass",
      {"drums", "808", "classic", "electronic"},
      R"({
                        "name": "808 Essentials",
                        "category": "drums",
                        "parameters": {
                            "attack": 0.001, "decay": 0.2, "sustain": 0.0, "release": 0.5,
                            "filterCutoff": 0.8, "filterResonance": 0.2,
                            "sampleStartOffset": 0.0, "pitchFine": 0.0, "pitchSemitones": 0.0,
                            "globalPan": 0.5, "globalGain": 0.9
                        },
                        "regions": [
                            { "filePath": "808-kick.wav", "rootNote": 36, "lowNote": 36, "highNote": 36, "lowVel": 0, "highVel": 127, "loopMode": "none", "gain": 1.0, "tune": 0.0 },
                            { "filePath": "808-snare.wav", "rootNote": 38, "lowNote": 38, "highNote": 38, "lowVel": 0, "highVel": 127, "loopMode": "none", "gain": 1.0, "tune": 0.0 },
                            { "filePath": "808-hihat-closed.wav", "rootNote": 42, "lowNote": 42, "highNote": 42, "lowVel": 0, "highVel": 127, "loopMode": "none", "gain": 0.8, "tune": 0.0 },
                            { "filePath": "808-hihat-open.wav", "rootNote": 46, "lowNote": 46, "highNote": 46, "lowVel": 0, "highVel": 127, "loopMode": "none", "gain": 0.8, "tune": 0.0 },
                            { "filePath": "808-bass.wav", "rootNote": 48, "lowNote": 24, "highNote": 72, "lowVel": 0, "highVel": 127, "loopMode": "forward", "gain": 1.0, "tune": 0.0 }
                        ]
                    })");

  // 2. LoFi Keys - Vintage piano with character
  registerSamplerPreset(registry, "lofi_keys", "LoFi Keys", "keys",
      "Lo-fi piano with vintage character and tape saturation",
      {"piano", "keys", "lofi", "vintage"},
      R"({
                        "name": "LoFi Keys",
                        "category": "keys",
                        "parameters": {
                            "attack": 0.05, "decay": 0.3, "sustain": 0.6, "release": 0.8,
                            "filterCutoff": 0.7, "filterResonance": 0.1,
                            "sampleStartOffset": 0.0, "pitchFine": 0.0, "pitchSemitones": 0.0,
                            "globalPan": 0.5, "globalGain": 0.85
                        },
                        "regions": [
                            { "filePath": "lofi-piano-C3.wav", "rootNote": 48, "lowNote": 42, "highNote": 53, "lowVel": 0, "highVel": 63, "loopMode": "forward", "gain": 1.0, "tune": 0.0 },
                            { "filePath": "lofi-piano-C4.wav", "rootNote": 60, "lowNote": 54, "highNote": 65, "lowVel": 0, "highVel": 63, "loopMode": "forward", "gain": 1.0, "tune": 0.0 },
                            { "filePath": "lofi-piano-C5.wav", "rootNote": 72, "lowNote": 66, "highNote": 84, "lowVel": 0, "highVel": 63, "loopMode": "forward", "gain": 1.0, "tune": 0.0 },
                            { "filePath": "lofi-piano-C3-hard.wav", "rootNote": 48, "lowNote": 42, "highNote": 53, "lowVel": 64, "highVel": 127, "loopMode": "forward", "gain": 1.0, "tune": 0.0 },
                            { "filePath": "lofi-piano-C4-hard.wav", "rootNote": 60, "lowNote": 54, "highNote": 65, "lowVel": 64, "highVel": 127, "loopMode": "forward", "gain": 1.0, "tune": 0.0 },
                            { "filePath": "lofi-piano-C5-hard.wav", "rootNote": 72, "lowNote": 66, "highNote": 84, "lowVel": 64, "highVel": 127, "loopMode": "forward", "gain": 1.0, "tune": 0.0 }
                        ]
                    })");

  // 3. Trap Pluck - Modern pluck synth
  registerSamplerPreset(registry, "trap_pluck", "Trap Pluck", "synth",
      "Modern trap pluck synth - short attack, punchy release",
      {"synth", "pluck", "trap", "modern"},
      R"({
                        "name": "Trap Pluck",
                        "category": "synth",
                        "parameters": {
                            "attack": 0.001, "decay": 0.15, "sustain": 0.3, "release": 0.2,
                            "filterCutoff": 0.85, "filterResonance": 0.4,
                            "sampleStartOffset": 0.0, "pitchFine": 0.0, "pitchSemitones": 0.0,
                            "globalPan": 0.5, "globalGain": 0.9
                        },
                        "regions": [
                            { "filePath": "trap-pluck-C2.wav", "rootNote": 36, "lowNote": 24, "highNote": 41, "lowVel": 0, "highVel": 127, "loopMode": "none", "gain": 1.0, "tune": 0.0 },
                            { "filePath": "trap-pluck-C3.wav", "rootNote": 48, "lowNote": 42, "highNote": 53, "lowVel": 0, "highVel": 127, "loopMode": "none", "gain": 1.0, "tune": 0.0 },
                            { "filePath": "trap-pluck-C4.wav", "rootNote": 60, "lowNote": 54, "highNote": 65, "lowVel": 0, "highVel": 127, "loopMode": "none", "gain": 1.0, "tune": 0.0 },
                            { "filePath": "trap-pluck-C5.wav", "rootNote": 72, "lowNote": 66, "highNote": 77, "lowVel": 0, "highVel": 127, "loopMode": "none", "gain": 1.0, "tune": 0.0 }
                        ]
                    })");

  // 4. Orchestral Strings - Lush string ensemble
  registerSamplerPreset(registry, "orchestral_strings", "Orchestral Strings", "orchestral",
      "Lush string ensemble with natural sustain and vibrato",
      {"strings", "orchestral", "ensemble", "classical"},
      R"({
                        "name": "Orchestral Strings",
                        "category": "orchestral",
                        "parameters": {
                            "attack": 0.15, "decay": 0.4, "sustain": 0.85, "release": 1.2,
                            "filterCutoff": 0.75, "filterResonance": 0.15,
                            "sampleStartOffset": 0.0, "pitchFine": 0.0, "pitchSemitones": 0.0,
                            "globalPan": 0.5, "globalGain": 0.75
                        },
                        "regions": [
                            { "filePath": "strings-C2.wav", "rootNote": 36, "lowNote": 24, "highNote": 47, "lowVel": 0, "highVel": 80, "loopMode": "forward", "gain": 1.0, "tune": 0.0 },
                            { "filePath": "strings-C3.wav", "rootNote": 48, "lowNote": 48, "highNote": 59, "lowVel": 0, "highVel": 80, "loopMode": "forward", "gain": 1.0, "tune": 0.0 },
                            { "filePath": "strings-C4.wav", "rootNote": 60, "lowNote": 60, "highNote": 71, "lowVel": 0, "highVel": 80, "loopMode": "forward", "gain": 1.0, "tune": 0.0 },
                            { "filePath": "strings-C5.wav", "rootNote": 72, "lowNote": 72, "highNote": 96, "lowVel": 0, "highVel": 80, "loopMode": "forward", "gain": 0.95, "tune": 0.0 }
                        ]
                    })");

  // 5. FX & Impacts - Cinematic sound effects
  registerSamplerPreset(registry, "fx_impacts", "FX & Impacts", "fx",
      "Cinematic sound effects - risers, impacts, whooshes",
      {"fx", "impacts", "cinematic", "soundfx"},
      R"({
                        "name": "FX & Impacts",
                        "category": "fx",
                        "parameters": {
                            "attack": 0.01, "decay": 0.5, "sustain": 0.3, "release": 1.5,
                            "filterCutoff": 1.0, "filterResonance": 0.3,
                            "sampleStartOffset": 0.0, "pitchFine": 0.0, "pitchSemitones": 0.0,
                            "globalPan": 0.5, "globalGain": 0.8
                        },
                        "regions": [
                            { "filePath": "fx-riser.wav", "rootNote": 60, "lowNote": 60, "highNote": 60, "lowVel": 0, "highVel": 127, "loopMode": "none", "gain": 1.0, "tune": 0.0 },
                            { "filePath": "fx-impact.wav", "rootNote": 62, "lowNote": 62, "highNote": 62, "lowVel": 0, "highVel": 127, "loopMode": "none", "gain": 1.0, "tune": 0.0 },
                            { "filePath": "fx-reverse.wav", "rootNote": 64, "lowNote": 64, "highNote": 64, "lowVel": 0, "highVel": 127, "loopMode": "none", "gain": 1.0, "tune": 0.0 },
                            { "filePath": "fx-whoosh.wav", "rootNote": 65, "lowNote": 65, "highNote": 65, "lowVel": 0, "highVel": 127, "loopMode": "none", "gain": 1.0, "tune": 0.0 }
                        ]
                    })");

  DBG("Built-in instruments registered successfully (2 synths + 5 sample-based "
      "instruments)");

  // Generate factory presets for testing (one-time generation logic could be
  // added here) For now, we regenerate them on startup to ensure they exist
  FactoryPresetGenerator::generateFactoryPresets();
}

} // namespace zenith
