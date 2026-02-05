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

    CommandAPI_SynthExtensions.cpp
    Created: 2025-01-29
    Author:  Zenith DAW

    Command handlers for Wingman → Synth communication.
    Add this to CommandAPI.cpp to enable AI synth control.


  ==============================================================================
*/

// Add these includes to CommandAPI.cpp:
// #include "../ai/WingmanSynthBridge.h"
// #include "../instruments/ZenithPolySynth.h"

namespace zenith {

//==============================================================================
// In CommandAPI class definition, add these CommandID enum values:
/*
    enum class CommandID {
        // ... existing commands ...
        
        // Synth Control Commands
        SetSynthParameter,
        SetSynthOscillatorWave,
        SetSynthOscillatorDetune,
        SetSynthOscillatorMix,
        SetSynthFilterType,
        SetSynthFilterCutoff,
        SetSynthFilterResonance,
        SetSynthFilterDrive,
        SetSynthAmpEnvelope,
        SetSynthFilterEnvelope,
        SetSynthLFORate,
        SetSynthLFOAmount,
        SetSynthDistortion,
        SetSynthChorus,
        SetSynthReverb,
        SetSynthDelay,
        SetSynthModulation,
        ApplySynthPreset,
        RandomizeSynthPatch,
        MorphSynthPatch,
        AnalyzeSynthPatch
    };
*/

//==============================================================================
// Helper: Get synth bridge for active instrument
WingmanSynthBridge* getSynthBridgeForActiveTrack() {
    // Get the active track
    auto tracks = projectState.getState().getChildWithName(ProjectState::ID_TRACKS);
    int activeTrackIndex = engine.getActiveTrackIndex();
    
    if (activeTrackIndex < 0 || activeTrackIndex >= tracks.getNumChildren()) {
        return nullptr;
    }
    
    auto track = tracks.getChild(activeTrackIndex);
    auto instrumentId = track.getProperty(ProjectState::PROP_INSTRUMENT).toString();
    
    // Check if it's ZenithPolySynth
    if (instrumentId != "ZenithPolySynth") {
        return nullptr;
    }
    
    // Get or create the synth bridge
    // This would be cached somewhere, for now return nullptr
    // In real implementation, you'd store bridge per track
    return nullptr;
}

//==============================================================================
// Command Handlers
//==============================================================================

juce::var CommandAPI::setSynthParameter(const juce::var& params) {
    // params: { "paramId": "Osc1Wave", "value": 0.5, "display": "Saw", "animate": 0.3 }
    
    if (auto* bridge = getSynthBridgeForActiveTrack()) {
        juce::String paramId = params["paramId"].toString();
        float value = static_cast<float>(params["value"].toDouble());
        juce::String display = params.getProperty("display", juce::String()).toString();
        float animate = static_cast<float>(params.getProperty("animate", 0.3).toDouble());
        
        bridge->setParameter(paramId, value, display, animate);
        
        return createSuccessResponse("Parameter " + paramId + " set to " + 
                                     (display.isNotEmpty() ? display : juce::String(value)));
    }
    
    return createErrorResponse("No synth active or not supported");
}

//==============================================================================
juce::var CommandAPI::setSynthOscillatorWave(const juce::var& params) {
    // params: { "oscillator": 1, "waveform": "saw" | "square" | "sine" | "triangle" }
    
    if (auto* bridge = getSynthBridgeForActiveTrack()) {
        int oscIndex = params.getProperty("oscillator", 1);
        juce::String waveStr = params["waveform"].toString().toLowerCase();
        
        OscillatorWaveform wave;
        if (waveStr == "sine") wave = OscillatorWaveform::Sine;
        else if (waveStr == "saw") wave = OscillatorWaveform::Saw;
        else if (waveStr == "square") wave = OscillatorWaveform::Square;
        else if (waveStr == "triangle") wave = OscillatorWaveform::Triangle;
        else if (waveStr == "noise") wave = OscillatorWaveform::Noise;
        else if (waveStr == "supersaw") wave = OscillatorWaveform::Supersaw;
        else {
            return createErrorResponse("Unknown waveform: " + waveStr);
        }
        
        bridge->setOscillatorWaveform(oscIndex, wave);
        
        return createSuccessResponse("Oscillator " + juce::String(oscIndex) + 
                                     " set to " + waveStr);
    }
    
    return createErrorResponse("No synth active");
}

//==============================================================================
juce::var CommandAPI::setSynthFilterCutoff(const juce::var& params) {
    // params: { "cutoff": 1200 } (Hz)
    
    if (auto* bridge = getSynthBridgeForActiveTrack()) {
        float cutoff = static_cast<float>(params["cutoff"].toDouble());
        bridge->setFilterCutoff(cutoff);
        
        return createSuccessResponse("Filter cutoff set to " + juce::String(cutoff) + " Hz");
    }
    
    return createErrorResponse("No synth active");
}

//==============================================================================
juce::var CommandAPI::setSynthFilterResonance(const juce::var& params) {
    // params: { "resonance": 0.5 } (0.0 to 1.0)
    
    if (auto* bridge = getSynthBridgeForActiveTrack()) {
        float resonance = static_cast<float>(params["resonance"].toDouble());
        bridge->setFilterResonance(resonance);
        
        return createSuccessResponse("Filter resonance set to " + juce::String(resonance));
    }
    
    return createErrorResponse("No synth active");
}

//==============================================================================
juce::var CommandAPI::setSynthFilterDrive(const juce::var& params) {
    // params: { "drive": 2.0 } (1.0 to 10.0)
    
    if (auto* bridge = getSynthBridgeForActiveTrack()) {
        float drive = static_cast<float>(params["drive"].toDouble());
        bridge->setFilterDrive(drive);
        
        return createSuccessResponse("Filter drive set to " + juce::String(drive) + "x");
    }
    
    return createErrorResponse("No synth active");
}

//==============================================================================
juce::var CommandAPI::setSynthAmpEnvelope(const juce::var& params) {
    // params: { "attack": 0.01, "decay": 0.2, "sustain": 0.7, "release": 0.3 }
    
    if (auto* bridge = getSynthBridgeForActiveTrack()) {
        float attack = static_cast<float>(params.getProperty("attack", 0.01).toDouble());
        float decay = static_cast<float>(params.getProperty("decay", 0.1).toDouble());
        float sustain = static_cast<float>(params.getProperty("sustain", 0.8).toDouble());
        float release = static_cast<float>(params.getProperty("release", 0.1).toDouble());
        
        bridge->setAmpEnvelope(attack, decay, sustain, release);
        
        return createSuccessResponse("Amp envelope set: A=" + juce::String(attack) + 
                                     " D=" + juce::String(decay) + 
                                     " S=" + juce::String(sustain) + 
                                     " R=" + juce::String(release));
    }
    
    return createErrorResponse("No synth active");
}

//==============================================================================
juce::var CommandAPI::setSynthLFORate(const juce::var& params) {
    // params: { "lfo": 1, "rate": 5.0 } (Hz)
    
    if (auto* bridge = getSynthBridgeForActiveTrack()) {
        int lfoIndex = params.getProperty("lfo", 1);
        float rate = static_cast<float>(params["rate"].toDouble());
        
        bridge->setLFORate(lfoIndex, rate);
        
        return createSuccessResponse("LFO " + juce::String(lfoIndex) + 
                                     " rate set to " + juce::String(rate) + " Hz");
    }
    
    return createErrorResponse("No synth active");
}

//==============================================================================
juce::var CommandAPI::setSynthDistortion(const juce::var& params) {
    // params: { "amount": 0.5 } (0.0 to 1.0)
    
    if (auto* bridge = getSynthBridgeForActiveTrack()) {
        float amount = static_cast<float>(params["amount"].toDouble());
        bridge->setDistortion(amount);
        
        return createSuccessResponse("Distortion set to " + juce::String(amount));
    }
    
    return createErrorResponse("No synth active");
}

//==============================================================================
juce::var CommandAPI::applySynthPreset(const juce::var& params) {
    // params: { "name": "Dark Bass", "parameters": { "Osc1Wave": 1, ... } }
    
    if (auto* bridge = getSynthBridgeForActiveTrack()) {
        juce::String presetName = params["name"].toString();
        
        juce::HashMap<juce::String, float> parameters;
        auto* paramObj = params["parameters"].getDynamicObject();
        
        if (paramObj != nullptr) {
            for (auto& prop : paramObj->getProperties()) {
                float value = static_cast<float>(prop.value.toDouble());
                parameters.set(prop.name.toString(), value);
            }
        }
        
        bridge->applyPreset(presetName, parameters);
        
        return createSuccessResponse("Applied preset: " + presetName);
    }
    
    return createErrorResponse("No synth active");
}

//==============================================================================
juce::var CommandAPI::randomizeSynthPatch(const juce::var& params) {
    // params: { "amount": 0.5 } (0.0 to 1.0, how much to randomize)
    
    if (auto* bridge = getSynthBridgeForActiveTrack()) {
        float amount = static_cast<float>(params.getProperty("amount", 0.5).toDouble());
        
        bridge->randomizePatch(amount);
        
        return createSuccessResponse("Patch randomized");
    }
    
    return createErrorResponse("No synth active");
}

//==============================================================================
juce::var CommandAPI::morphSynthPatch(const juce::var& params) {
    // params: { "target": { "Osc1Wave": 1, ... }, "duration": 1.0 }
    
    if (auto* bridge = getSynthBridgeForActiveTrack()) {
        juce::HashMap<juce::String, float> targetPatch;
        
        auto* targetObj = params["target"].getDynamicObject();
        if (targetObj != nullptr) {
            for (auto& prop : targetObj->getProperties()) {
                float value = static_cast<float>(prop.value.toDouble());
                targetPatch.set(prop.name.toString(), value);
            }
        }
        
        float duration = static_cast<float>(params.getProperty("duration", 1.0).toDouble());
        
        bridge->morphToPatch(targetPatch, duration);
        
        return createSuccessResponse("Morphing patch");
    }
    
    return createErrorResponse("No synth active");
}

//==============================================================================
juce::var CommandAPI::analyzeSynthPatch(const juce::var& params) {
    // params: {}
    
    if (auto* bridge = getSynthBridgeForActiveTrack()) {
        juce::String analysis = bridge->analyzePatch();
        
        juce::DynamicObject::Ptr result = new juce::DynamicObject();
        result->setProperty("analysis", analysis);
        result->setProperty("patchState", bridge->getCurrentPatchState());
        
        return createSuccessResponse(juce::var(result.get()));
    }
    
    return createErrorResponse("No synth active");
}

//==============================================================================
// In CommandAPI::initializeCommandMap(), add these mappings:
/*
    registerCommand("setSynthParameter", [this](const auto& p) { return setSynthParameter(p); });
    registerCommand("setSynthOscillatorWave", [this](const auto& p) { return setSynthOscillatorWave(p); });
    registerCommand("setSynthFilterCutoff", [this](const auto& p) { return setSynthFilterCutoff(p); });
    registerCommand("setSynthFilterResonance", [this](const auto& p) { return setSynthFilterResonance(p); });
    registerCommand("setSynthFilterDrive", [this](const auto& p) { return setSynthFilterDrive(p); });
    registerCommand("setSynthAmpEnvelope", [this](const auto& p) { return setSynthAmpEnvelope(p); });
    registerCommand("setSynthLFORate", [this](const auto& p) { return setSynthLFORate(p); });
    registerCommand("setSynthDistortion", [this](const auto& p) { return setSynthDistortion(p); });
    registerCommand("applySynthPreset", [this](const auto& p) { return applySynthPreset(p); });
    registerCommand("randomizeSynthPatch", [this](const auto& p) { return randomizeSynthPatch(p); });
    registerCommand("morphSynthPatch", [this](const auto& p) { return morphSynthPatch(p); });
    registerCommand("analyzeSynthPatch", [this](const auto& p) { return analyzeSynthPatch(p); });
*/

//==============================================================================
// Example Wingman Prompts for Synth Control:
/*
 
User: "Make a dark brass sound"
Wingman: [Executing: setSynthOscillatorWave {oscillator: 1, waveform: "saw"}]
        [Executing: setSynthFilterCutoff {cutoff: 2500}]
        [Executing: setSynthFilterResonance {resonance: 0.3}]
        [Executing: setSynthAmpEnvelope {attack: 0.01, decay: 0.2, sustain: 0.6, release: 0.3}]
        [Executing: setSynthDistortion {amount: 0.3}]
        
        Created a dark brass sound with:
        - Oscillator 1: Saw wave
        - Filter: Lowpass at 2.5kHz, 30% resonance
        - Envelope: Quick attack, medium decay
        - Distortion: 30% for warmth

User: "Make it brighter"
Wingman: [Executing: setSynthFilterCutoff {cutoff: 4500}]
        [Executing: setSynthFilterResonance {resonance: 0.5}]
        
        Brightened the sound by:
        - Increasing filter cutoff to 4.5kHz
        - Adding more resonance

User: "Add some movement"
Wingman: [Executing: setSynthLFORate {lfo: 1, rate: 0.5}]
        [Executing: setSynthModulation {source: "LFO1", destination: "FilterCutoff", amount: 0.4}]
        
        Added modulation:
        - LFO 1: 0.5Hz (slow movement)
        - Modulating filter cutoff by 40%

User: "Randomize this patch"
Wingman: [Executing: randomizeSynthPatch {amount: 0.3}]
        
        Randomized patch by 30%. New settings:
        - Filter cutoff: 3200Hz (was 2500Hz)
        - Oscillator detune: 7 cents
        - Envelope: Longer decay

User: "Create a trance lead"
Wingman: [Executing: applySynthPreset {
            name: "Trance Lead",
            parameters: {
                Osc1Wave: 1,
                Osc2Wave: 2,
                FilterType: 0,
                FilterCutoff: 3200,
                FilterResonance: 0.25,
                AmpAttack: 0.01,
                AmpDecay: 0.2,
                AmpSustain: 0.7,
                AmpRelease: 0.5,
                LFO1Rate: 0.3,
                LFO1Amount: 0.4,
                ChorusAmount: 0.3,
                DelayMix: 0.4
            }
        }]
        
        Applied trance lead preset:
        - Dual oscillators (saw + square)
        - Bright filter with subtle resonance
        - Plucky envelope with sustain
        - LFO modulating filter
        - Chorus + delay for width

User: "What's this patch like?"
Wingman: [Executing: analyzeSynthPatch {}]
        
        Current patch analysis:
        - Type: Bright pluck synth
        - Character: Filtered saw wave with quick envelope
        - Effects: Chorus (30%), Delay (40%)
        - Modulation: LFO 1 → Filter Cutoff
        - Suggested use: Melodic trance leads, pluck bass

*/

} // namespace zenith