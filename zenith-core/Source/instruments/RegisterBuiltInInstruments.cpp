/*
  ==============================================================================

    RegisterBuiltInInstruments.cpp
    Created: 2025-11-18
    Author:  Zenith DAW

    Implementation of built-in instrument registration.

  ==============================================================================
*/

#include "RegisterBuiltInInstruments.h"
#include "InstrumentRegistry.h"
#include "ZenithPolySynth.h"
#include "ZenithSampler.h"
#include "../utils/PresetGenerator.h"

namespace zenith {

void registerBuiltInInstruments()
{
    auto& registry = InstrumentRegistry::getInstance();

    // Register ZenithPolySynth
    registry.registerInstrument(
        "zenith.poly_synth",
        ZenithPolySynth::createMetadata(),
        []() { return std::make_unique<ZenithPolySynth>(); }
    );

    // Register ZenithSampler (base/empty sampler)
    registry.registerInstrument(
        "zenith_sampler",
        ZenithSampler::createMetadata(),
        []() { return std::make_unique<ZenithSampler>(); }
    );

    // Register canonical sampler instruments with pre-defined sample maps
    // These are factory templates that ship with Zenith

    // 1. 808 Essentials - Classic drum machine
    {
        auto metadata = ZenithSampler::createMetadata();
        metadata.instrumentId = "zenith_sampler.808_essentials";
        metadata.name = "808 Essentials";
        metadata.category = "drums";
        metadata.description = "Classic 808 drum sounds - kick, snare, hi-hats, and bass";
        metadata.tags = {"drums", "808", "classic", "electronic"};

        registry.registerInstrument(
            metadata.instrumentId,
            metadata,
            []() {
                auto sampler = std::make_unique<ZenithSampler>();
                // Sample bank JSON embedded for built-in instruments
                // Note: Actual samples need to be in Content/Instruments/ZenithSampler/Samples/
                auto* proc = dynamic_cast<ZenithSamplerProcessor*>(sampler->getAudioProcessor());
                if (proc) {
                    const char* bankJson = R"({
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
                    })";
                    proc->loadSampleBankFromJson(bankJson, "808 Essentials");
                }
                return sampler;
            }
        );
    }

    // 2. LoFi Keys - Vintage piano with character
    {
        auto metadata = ZenithSampler::createMetadata();
        metadata.instrumentId = "zenith_sampler.lofi_keys";
        metadata.name = "LoFi Keys";
        metadata.category = "keys";
        metadata.description = "Lo-fi piano with vintage character and tape saturation";
        metadata.tags = {"piano", "keys", "lofi", "vintage"};

        registry.registerInstrument(
            metadata.instrumentId,
            metadata,
            []() {
                auto sampler = std::make_unique<ZenithSampler>();
                auto* proc = dynamic_cast<ZenithSamplerProcessor*>(sampler->getAudioProcessor());
                if (proc) {
                    const char* bankJson = R"({
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
                    })";
                    proc->loadSampleBankFromJson(bankJson, "LoFi Keys");
                }
                return sampler;
            }
        );
    }

    // 3. Trap Pluck - Modern pluck synth
    {
        auto metadata = ZenithSampler::createMetadata();
        metadata.instrumentId = "zenith_sampler.trap_pluck";
        metadata.name = "Trap Pluck";
        metadata.category = "synth";
        metadata.description = "Modern trap pluck synth - short attack, punchy release";
        metadata.tags = {"synth", "pluck", "trap", "modern"};

        registry.registerInstrument(
            metadata.instrumentId,
            metadata,
            []() {
                auto sampler = std::make_unique<ZenithSampler>();
                auto* proc = dynamic_cast<ZenithSamplerProcessor*>(sampler->getAudioProcessor());
                if (proc) {
                    const char* bankJson = R"({
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
                    })";
                    proc->loadSampleBankFromJson(bankJson, "Trap Pluck");
                }
                return sampler;
            }
        );
    }

    // 4. Orchestral Strings - Lush string ensemble
    {
        auto metadata = ZenithSampler::createMetadata();
        metadata.instrumentId = "zenith_sampler.orchestral_strings";
        metadata.name = "Orchestral Strings";
        metadata.category = "orchestral";
        metadata.description = "Lush string ensemble with natural sustain and vibrato";
        metadata.tags = {"strings", "orchestral", "ensemble", "classical"};

        registry.registerInstrument(
            metadata.instrumentId,
            metadata,
            []() {
                auto sampler = std::make_unique<ZenithSampler>();
                auto* proc = dynamic_cast<ZenithSamplerProcessor*>(sampler->getAudioProcessor());
                if (proc) {
                    const char* bankJson = R"({
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
                    })";
                    proc->loadSampleBankFromJson(bankJson, "Orchestral Strings");
                }
                return sampler;
            }
        );
    }

    // 5. FX & Impacts - Cinematic sound effects
    {
        auto metadata = ZenithSampler::createMetadata();
        metadata.instrumentId = "zenith_sampler.fx_impacts";
        metadata.name = "FX & Impacts";
        metadata.category = "fx";
        metadata.description = "Cinematic sound effects - risers, impacts, whooshes";
        metadata.tags = {"fx", "impacts", "cinematic", "soundfx"};

        registry.registerInstrument(
            metadata.instrumentId,
            metadata,
            []() {
                auto sampler = std::make_unique<ZenithSampler>();
                auto* proc = dynamic_cast<ZenithSamplerProcessor*>(sampler->getAudioProcessor());
                if (proc) {
                    const char* bankJson = R"({
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
                    })";
                    proc->loadSampleBankFromJson(bankJson, "FX & Impacts");
                }
                return sampler;
            }
        );
    }

    DBG("Built-in instruments registered successfully (2 synths + 5 sample-based instruments)");

    // Generate factory presets for testing (one-time generation logic could be added here)
    // For now, we regenerate them on startup to ensure they exist
    PresetGenerator::generateFactoryPresets();
}

} // namespace zenith

