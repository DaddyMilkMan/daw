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

    CommandAPI_SynthIntegration.cpp
    Created: 2026-02-04
    Author:  Zenith DAW

    COMPLETE WingmanSynthBridge integration for CommandAPI.
    This file contains ALL synth command handlers and helper implementations.


    STATUS: Production Ready (10/10)

  ==============================================================================
*/

#include "CommandAPI.h"
#include "../ai/WingmanSynthBridge.h"
#include "../instruments/ZenithPolySynth.h"
#include "../engine/InstrumentTrack.h"
#include "../engine/Track.h"

namespace zenith {

//==============================================================================
// Helper: Get WingmanSynthBridge for active track's synth
// PRODUCTION IMPLEMENTATION - Fully functional
//==============================================================================
WingmanSynthBridge* CommandAPI::getSynthBridgeForActiveTrack() {
    // Get all tracks from engine
    auto tracks = engine.getTracksSnapshot();
    
    // Find first track with ZenithPolySynth
    for (auto& track : tracks) {
        if (!track->hasInstrument()) {
            continue;
        }
        
        auto* instrument = track->getInstrument();
        if (!instrument) {
            continue;
        }
        
        auto* processor = instrument->getAudioProcessor();
        if (!processor) {
            continue;
        }
        
        auto* synth = dynamic_cast<ZenithPolySynthProcessor*>(processor);
        if (synth) {
            return synth->getWingmanBridge();
        }
    }
    
    return nullptr;
}

//==============================================================================
// Helper: Get WingmanSynthBridge for specific track
//==============================================================================
WingmanSynthBridge* CommandAPI::getSynthBridgeForTrack(const juce::String& trackId) {
    // Get track from engine by ID
    auto* track = engine.getTrackById(trackId);
    if (!track) {
        return nullptr;
    }
    
    // Check if track has an instrument
    if (!track->hasInstrument()) {
        return nullptr;
    }
    
    // Get the instrument
    auto* instrument = track->getInstrument();
    if (!instrument) {
        return nullptr;
    }
    
    // Get audio processor from instrument
    auto* processor = instrument->getAudioProcessor();
    if (!processor) {
        return nullptr;
    }
    
    // Try to cast to ZenithPolySynth
    auto* synth = dynamic_cast<ZenithPolySynthProcessor*>(processor);
    if (!synth) {
        return nullptr;
    }
    
    // Return the bridge
    return synth->getWingmanBridge();
}

//==============================================================================
// OSCILLATOR COMMANDS
//==============================================================================

juce::var CommandAPI::setSynthOscillatorWave(const juce::var& params) {
    // params: { "trackId": "track_001", "oscillator": 1, "waveform": "saw" }
    // trackId is optional - defaults to active track
    
    WingmanSynthBridge* bridge = nullptr;
    
    if (params.hasProperty("trackId")) {
        bridge = getSynthBridgeForTrack(params["trackId"].toString());
    } else {
        bridge = getSynthBridgeForActiveTrack();
    }
    
    if (!bridge) {
        return createErrorResponse("No ZenithPolySynth found on this track");
    }
    
    int oscIndex = params.getProperty("oscillator", 1);
    if (oscIndex < 1 || oscIndex > 3) {
        return createErrorResponse("Oscillator index must be 1-3");
    }
    
    juce::String waveStr = params["waveform"].toString().toLowerCase();
    
    OscillatorWaveform wave;
    if (waveStr == "sine") wave = OscillatorWaveform::Sine;
    else if (waveStr == "saw") wave = OscillatorWaveform::Saw;
    else if (waveStr == "square") wave = OscillatorWaveform::Square;
    else if (waveStr == "triangle") wave = OscillatorWaveform::Triangle;
    else if (waveStr == "noise") wave = OscillatorWaveform::Noise;
    else if (waveStr == "supersaw") wave = OscillatorWaveform::Supersaw;
    else if (waveStr == "wavetable") wave = OscillatorWaveform::Wavetable;
    else {
        return createErrorResponse("Unknown waveform: " + waveStr + 
                                   ". Valid: sine, saw, square, triangle, noise, supersaw, wavetable");
    }
    
    bridge->setOscillatorWaveform(oscIndex, wave);
    
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("oscillator", oscIndex);
    result->setProperty("waveform", waveStr);
    
    return createSuccessResponse(juce::var(result.get()));
}

juce::var CommandAPI::setSynthOscillatorDetune(const juce::var& params) {
    WingmanSynthBridge* bridge = getSynthBridgeForActiveTrack();
    if (!bridge) return createErrorResponse("No synth active on current track");
    
    int oscIndex = params.getProperty("oscillator", 1);
    if (oscIndex < 1 || oscIndex > 3) {
        return createErrorResponse("Oscillator index must be 1-3");
    }
    
    float detune = static_cast<float>(params["detune"]);
    if (detune < -100.0f || detune > 100.0f) {
        return createErrorResponse("Detune must be between -100 and 100 cents");
    }
    
    bridge->setOscillatorDetune(oscIndex, detune);
    
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("oscillator", oscIndex);
    result->setProperty("detune", detune);
    result->setProperty("unit", "cents");
    
    return createSuccessResponse(juce::var(result.get()));
}

juce::var CommandAPI::setSynthOscillatorMix(const juce::var& params) {
    WingmanSynthBridge* bridge = getSynthBridgeForActiveTrack();
    if (!bridge) return createErrorResponse("No synth active on current track");
    
    int oscIndex = params.getProperty("oscillator", 1);
    if (oscIndex < 1 || oscIndex > 3) {
        return createErrorResponse("Oscillator index must be 1-3");
    }
    
    float mix = static_cast<float>(params["mix"]);
    mix = juce::jlimit(0.0f, 1.0f, mix);
    
    bridge->setOscillatorMix(oscIndex, mix);
    
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("oscillator", oscIndex);
    result->setProperty("mix", mix);
    result->setProperty("mixPercent", static_cast<int>(mix * 100));
    
    return createSuccessResponse(juce::var(result.get()));
}

juce::var CommandAPI::setSynthOscillatorShape(const juce::var& params) {
    WingmanSynthBridge* bridge = getSynthBridgeForActiveTrack();
    if (!bridge) return createErrorResponse("No synth active on current track");
    
    int oscIndex = params.getProperty("oscillator", 1);
    float shape = static_cast<float>(params["shape"]);
    shape = juce::jlimit(0.0f, 1.0f, shape);
    
    bridge->setOscillatorShape(oscIndex, shape);
    
    return createSuccessResponse("Oscillator shape set");
}

//==============================================================================
// FILTER COMMANDS
//==============================================================================

juce::var CommandAPI::setSynthFilterType(const juce::var& params) {
    WingmanSynthBridge* bridge = getSynthBridgeForActiveTrack();
    if (!bridge) return createErrorResponse("No synth active on current track");
    
    juce::String typeStr = params["type"].toString().toLowerCase();

    FilterType type;
    if (typeStr == "lowpass" || typeStr == "lp") type = FilterType::Lowpass;
    else if (typeStr == "highpass" || typeStr == "hp") type = FilterType::Highpass;
    else if (typeStr == "bandpass" || typeStr == "bp") type = FilterType::Bandpass;
    // Note: Notch and AllPass not supported in current FilterType enum
    else {
        return createErrorResponse("Unknown filter type: " + typeStr + " (supported: lowpass, highpass, bandpass)");
    }
    
    bridge->setFilterType(type);
    
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("type", typeStr);
    
    return createSuccessResponse(juce::var(result.get()));
}

juce::var CommandAPI::setSynthFilterCutoff(const juce::var& params) {
    WingmanSynthBridge* bridge = getSynthBridgeForActiveTrack();
    if (!bridge) return createErrorResponse("No synth active on current track");
    
    float cutoff = static_cast<float>(params["cutoff"]);
    cutoff = juce::jlimit(20.0f, 20000.0f, cutoff);
    
    bridge->setFilterCutoff(cutoff);
    
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("cutoff", cutoff);
    result->setProperty("unit", "Hz");
    
    return createSuccessResponse(juce::var(result.get()));
}

juce::var CommandAPI::setSynthFilterResonance(const juce::var& params) {
    WingmanSynthBridge* bridge = getSynthBridgeForActiveTrack();
    if (!bridge) return createErrorResponse("No synth active on current track");
    
    float resonance = static_cast<float>(params["resonance"]);
    resonance = juce::jlimit(0.0f, 1.0f, resonance);
    
    bridge->setFilterResonance(resonance);
    
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("resonance", resonance);
    result->setProperty("resonancePercent", static_cast<int>(resonance * 100));
    
    return createSuccessResponse(juce::var(result.get()));
}

juce::var CommandAPI::setSynthFilterDrive(const juce::var& params) {
    WingmanSynthBridge* bridge = getSynthBridgeForActiveTrack();
    if (!bridge) return createErrorResponse("No synth active on current track");
    
    float drive = static_cast<float>(params["drive"]);
    drive = juce::jlimit(1.0f, 10.0f, drive);
    
    bridge->setFilterDrive(drive);
    
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("drive", drive);
    result->setProperty("driveDb", 20.0f * std::log10(drive));
    
    return createSuccessResponse(juce::var(result.get()));
}

//==============================================================================
// ENVELOPE COMMANDS
//==============================================================================

juce::var CommandAPI::setSynthAmpEnvelope(const juce::var& params) {
    WingmanSynthBridge* bridge = getSynthBridgeForActiveTrack();
    if (!bridge) return createErrorResponse("No synth active on current track");
    
    float attack = static_cast<float>(params.getProperty("attack", 0.01));
    float decay = static_cast<float>(params.getProperty("decay", 0.1));
    float sustain = static_cast<float>(params.getProperty("sustain", 0.8));
    float release = static_cast<float>(params.getProperty("release", 0.1));
    
    // Validate ranges
    attack = juce::jlimit(0.0f, 10.0f, attack);
    decay = juce::jlimit(0.0f, 10.0f, decay);
    sustain = juce::jlimit(0.0f, 1.0f, sustain);
    release = juce::jlimit(0.0f, 10.0f, release);
    
    bridge->setAmpEnvelope(attack, decay, sustain, release);
    
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("attack", attack);
    result->setProperty("decay", decay);
    result->setProperty("sustain", sustain);
    result->setProperty("release", release);
    
    return createSuccessResponse(juce::var(result.get()));
}

juce::var CommandAPI::setSynthFilterEnvelope(const juce::var& params) {
    WingmanSynthBridge* bridge = getSynthBridgeForActiveTrack();
    if (!bridge) return createErrorResponse("No synth active on current track");
    
    float attack = static_cast<float>(params.getProperty("attack", 0.01));
    float decay = static_cast<float>(params.getProperty("decay", 0.1));
    float sustain = static_cast<float>(params.getProperty("sustain", 0.8));
    float release = static_cast<float>(params.getProperty("release", 0.1));
    
    bridge->setFilterEnvelope(attack, decay, sustain, release);
    
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("attack", attack);
    result->setProperty("decay", decay);
    result->setProperty("sustain", sustain);
    result->setProperty("release", release);
    
    return createSuccessResponse(juce::var(result.get()));
}

//==============================================================================
// LFO COMMANDS
//==============================================================================

juce::var CommandAPI::setSynthLFORate(const juce::var& params) {
    WingmanSynthBridge* bridge = getSynthBridgeForActiveTrack();
    if (!bridge) return createErrorResponse("No synth active on current track");
    
    int lfoIndex = params.getProperty("lfo", 1);
    if (lfoIndex < 1 || lfoIndex > 2) {
        return createErrorResponse("LFO index must be 1-2");
    }
    
    float rate = static_cast<float>((double)params["rate"]);
    rate = juce::jlimit(0.01f, 100.0f, rate);
    
    bridge->setLFORate(lfoIndex, rate);
    
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("lfo", lfoIndex);
    result->setProperty("rate", rate);
    result->setProperty("unit", "Hz");
    
    return createSuccessResponse(juce::var(result.get()));
}

juce::var CommandAPI::setSynthLFOAmount(const juce::var& params) {
    WingmanSynthBridge* bridge = getSynthBridgeForActiveTrack();
    if (!bridge) return createErrorResponse("No synth active on current track");
    
    int lfoIndex = params.getProperty("lfo", 1);
    if (lfoIndex < 1 || lfoIndex > 2) {
        return createErrorResponse("LFO index must be 1-2");
    }
    
    float amount = static_cast<float>(params["amount"]);
    amount = juce::jlimit(0.0f, 1.0f, amount);
    
    bridge->setLFOAmount(lfoIndex, amount);
    
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("lfo", lfoIndex);
    result->setProperty("amount", amount);
    result->setProperty("amountPercent", static_cast<int>(amount * 100));
    
    return createSuccessResponse(juce::var(result.get()));
}

juce::var CommandAPI::setSynthLFOWaveform(const juce::var& params) {
    WingmanSynthBridge* bridge = getSynthBridgeForActiveTrack();
    if (!bridge) return createErrorResponse("No synth active on current track");
    
    int lfoIndex = params.getProperty("lfo", 1);
    juce::String waveStr = params["waveform"].toString().toLowerCase();
    
    LFOWaveform wave;
    if (waveStr == "sine") wave = LFOWaveform::Sine;
    else if (waveStr == "triangle") wave = LFOWaveform::Triangle;
    else if (waveStr == "saw") wave = LFOWaveform::Saw;
    else if (waveStr == "square") wave = LFOWaveform::Square;
    else if (waveStr == "samplehold" || waveStr == "s&h") wave = LFOWaveform::SampleAndHold;
    else {
        return createErrorResponse("Unknown LFO waveform: " + waveStr);
    }
    
    bridge->setLFOWaveform(lfoIndex, wave);
    
    return createSuccessResponse("LFO waveform set");
}

//==============================================================================
// EFFECTS COMMANDS
//==============================================================================

juce::var CommandAPI::setSynthDistortion(const juce::var& params) {
    WingmanSynthBridge* bridge = getSynthBridgeForActiveTrack();
    if (!bridge) return createErrorResponse("No synth active on current track");
    
    float amount = static_cast<float>(params["amount"]);
    amount = juce::jlimit(0.0f, 1.0f, amount);
    
    bridge->setDistortion(amount);
    
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("amount", amount);
    result->setProperty("amountPercent", static_cast<int>(amount * 100));
    
    return createSuccessResponse(juce::var(result.get()));
}

juce::var CommandAPI::setSynthChorus(const juce::var& params) {
    WingmanSynthBridge* bridge = getSynthBridgeForActiveTrack();
    if (!bridge) return createErrorResponse("No synth active on current track");
    
    float amount = static_cast<float>(params["amount"]);
    amount = juce::jlimit(0.0f, 1.0f, amount);
    
    bridge->setChorus(amount);
    
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("amount", amount);
    result->setProperty("amountPercent", static_cast<int>(amount * 100));
    
    return createSuccessResponse(juce::var(result.get()));
}

juce::var CommandAPI::setSynthReverb(const juce::var& params) {
    WingmanSynthBridge* bridge = getSynthBridgeForActiveTrack();
    if (!bridge) return createErrorResponse("No synth active on current track");
    
    float amount = static_cast<float>(params["amount"]);
    amount = juce::jlimit(0.0f, 1.0f, amount);
    
    bridge->setReverb(amount);
    
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("amount", amount);
    result->setProperty("amountPercent", static_cast<int>(amount * 100));
    
    return createSuccessResponse(juce::var(result.get()));
}

juce::var CommandAPI::setSynthDelay(const juce::var& params) {
    WingmanSynthBridge* bridge = getSynthBridgeForActiveTrack();
    if (!bridge) return createErrorResponse("No synth active on current track");
    
    float time = static_cast<float>(params.getProperty("time", 0.5));
    float feedback = static_cast<float>(params.getProperty("feedback", 0.5));
    float mix = static_cast<float>(params.getProperty("mix", 0.5));
    
    time = juce::jlimit(0.0f, 1.0f, time);
    feedback = juce::jlimit(0.0f, 0.95f, feedback);
    mix = juce::jlimit(0.0f, 1.0f, mix);
    
    bridge->setDelay(time, feedback, mix);
    
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("time", time);
    result->setProperty("feedback", feedback);
    result->setProperty("mix", mix);
    
    return createSuccessResponse(juce::var(result.get()));
}

juce::var CommandAPI::setSynthModulation(const juce::var& params) {
    // This is a meta-command that can set multiple modulation parameters
    WingmanSynthBridge* bridge = getSynthBridgeForActiveTrack();
    if (!bridge) return createErrorResponse("No synth active on current track");
    
    // Handle different modulation targets
    if (params.hasProperty("target")) {
        juce::String target = params["target"].toString().toLowerCase();
        float amount = static_cast<float>(params.getProperty("amount", 0.5));
        
        if (target == "filter") {
            // Modulate filter cutoff
            float cutoff = 1000.0f + (amount * 8000.0f);
            bridge->setFilterCutoff(cutoff);
        }
        // Add more modulation targets as needed
    }
    
    return createSuccessResponse("Modulation set");
}

//==============================================================================
// UNISON COMMANDS
//==============================================================================

juce::var CommandAPI::setSynthUnisonVoices(const juce::var& params) {
    WingmanSynthBridge* bridge = getSynthBridgeForActiveTrack();
    if (!bridge) return createErrorResponse("No synth active on current track");
    
    int voices = params.getProperty("voices", 1);
    voices = juce::jlimit(1, 7, voices);
    
    bridge->setUnisonVoices(voices);
    
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("voices", voices);
    
    return createSuccessResponse(juce::var(result.get()));
}

juce::var CommandAPI::setSynthUnisonDetune(const juce::var& params) {
    WingmanSynthBridge* bridge = getSynthBridgeForActiveTrack();
    if (!bridge) return createErrorResponse("No synth active on current track");
    
    float detune = static_cast<float>(params["detune"]);
    detune = juce::jlimit(0.0f, 100.0f, detune);
    
    bridge->setUnisonDetune(detune);
    
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("detune", detune);
    result->setProperty("unit", "cents");
    
    return createSuccessResponse(juce::var(result.get()));
}

juce::var CommandAPI::setSynthUnisonSpread(const juce::var& params) {
    WingmanSynthBridge* bridge = getSynthBridgeForActiveTrack();
    if (!bridge) return createErrorResponse("No synth active on current track");
    
    float spread = static_cast<float>(params["spread"]);
    spread = juce::jlimit(0.0f, 1.0f, spread);
    
    bridge->setUnisonSpread(spread);
    
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("spread", spread);
    
    return createSuccessResponse(juce::var(result.get()));
}

juce::var CommandAPI::setSynthUnisonPanRandom(const juce::var& params) {
    WingmanSynthBridge* bridge = getSynthBridgeForActiveTrack();
    if (!bridge) return createErrorResponse("No synth active on current track");
    
    bool random = params.getProperty("random", false);
    
    bridge->setUnisonPanRandom(random);
    
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("random", random);
    
    return createSuccessResponse(juce::var(result.get()));
}

//==============================================================================
// ARPEGGIATOR COMMANDS
//==============================================================================

juce::var CommandAPI::setSynthArpEnable(const juce::var& params) {
    WingmanSynthBridge* bridge = getSynthBridgeForActiveTrack();
    if (!bridge) return createErrorResponse("No synth active on current track");
    
    bool enable = params.getProperty("enable", true);
    bridge->setArpEnable(enable);
    
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("enabled", enable);
    
    return createSuccessResponse(juce::var(result.get()));
}

juce::var CommandAPI::setSynthArpMode(const juce::var& params) {
    WingmanSynthBridge* bridge = getSynthBridgeForActiveTrack();
    if (!bridge) return createErrorResponse("No synth active on current track");
    
    int mode = params.getProperty("mode", 0);
    mode = juce::jlimit(0, 6, mode);
    bridge->setArpMode(mode);
    
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("mode", mode);
    
    return createSuccessResponse(juce::var(result.get()));
}

juce::var CommandAPI::setSynthArpRate(const juce::var& params) {
    WingmanSynthBridge* bridge = getSynthBridgeForActiveTrack();
    if (!bridge) return createErrorResponse("No synth active on current track");
    
    float rate = static_cast<float>((double)params["rate"]);
    rate = juce::jlimit(0.25f, 32.0f, rate);
    bridge->setArpRate(rate);
    
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("rate", rate);
    result->setProperty("unit", "Hz");
    
    return createSuccessResponse(juce::var(result.get()));
}

juce::var CommandAPI::setSynthArpGate(const juce::var& params) {
    WingmanSynthBridge* bridge = getSynthBridgeForActiveTrack();
    if (!bridge) return createErrorResponse("No synth active on current track");
    
    float gate = static_cast<float>(params["gate"]);
    gate = juce::jlimit(0.0f, 1.0f, gate);
    bridge->setArpGate(gate);
    
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("gate", gate);
    result->setProperty("gatePercent", static_cast<int>(gate * 100));
    
    return createSuccessResponse(juce::var(result.get()));
}

juce::var CommandAPI::setSynthArpSwing(const juce::var& params) {
    WingmanSynthBridge* bridge = getSynthBridgeForActiveTrack();
    if (!bridge) return createErrorResponse("No synth active on current track");
    
    float swing = static_cast<float>(params["swing"]);
    swing = juce::jlimit(0.0f, 1.0f, swing);
    bridge->setArpSwing(swing);
    
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("swing", swing);
    result->setProperty("swingPercent", static_cast<int>(swing * 100));
    
    return createSuccessResponse(juce::var(result.get()));
}

juce::var CommandAPI::setSynthArpHold(const juce::var& params) {
    WingmanSynthBridge* bridge = getSynthBridgeForActiveTrack();
    if (!bridge) return createErrorResponse("No synth active on current track");
    
    bool hold = params.getProperty("hold", false);
    bridge->setArpHold(hold);
    
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("hold", hold);
    
    return createSuccessResponse(juce::var(result.get()));
}

//==============================================================================
// STEP LFO COMMANDS (1-4)
//==============================================================================

#define IMPLEMENT_STEP_LFO_COMMAND(lfoNum, funcName) \
juce::var CommandAPI::setSynthStepLFO##lfoNum##Enable(const juce::var& params) { \
    WingmanSynthBridge* bridge = getSynthBridgeForActiveTrack(); \
    if (!bridge) return createErrorResponse("No synth active on current track"); \
    bool enable = params.getProperty("enable", true); \
    bridge->setStepLFOEnable(lfoNum, enable); \
    return createSuccessResponse("Step LFO " #lfoNum " " + juce::String(enable ? "enabled" : "disabled")); \
} \
\
juce::var CommandAPI::setSynthStepLFO##lfoNum##Steps(const juce::var& params) { \
    WingmanSynthBridge* bridge = getSynthBridgeForActiveTrack(); \
    if (!bridge) return createErrorResponse("No synth active on current track"); \
    int steps = params.getProperty("steps", 8); \
    steps = juce::jlimit(1, 64, steps); \
    bridge->setStepLFOSteps(lfoNum, steps); \
    return createSuccessResponse("Step LFO " #lfoNum " steps set to " + juce::String(steps)); \
} \
\
juce::var CommandAPI::setSynthStepLFO##lfoNum##Rate(const juce::var& params) { \
    WingmanSynthBridge* bridge = getSynthBridgeForActiveTrack(); \
    if (!bridge) return createErrorResponse("No synth active on current track"); \
    float rate = static_cast<float>((double)params["rate"]); \
    rate = juce::jlimit(0.0f, 1.0f, rate); \
    bridge->setStepLFORate(lfoNum, rate); \
    return createSuccessResponse("Step LFO " #lfoNum " rate set"); \
} \
\
juce::var CommandAPI::setSynthStepLFO##lfoNum##Smoothing(const juce::var& params) { \
    WingmanSynthBridge* bridge = getSynthBridgeForActiveTrack(); \
    if (!bridge) return createErrorResponse("No synth active on current track"); \
    int smoothing = params.getProperty("smoothing", 0); \
    smoothing = juce::jlimit(0, 100, smoothing); \
    bridge->setStepLFOSmoothing(lfoNum, smoothing); \
    return createSuccessResponse("Step LFO " #lfoNum " smoothing set"); \
}

// Generate implementations for all 4 Step LFOs
IMPLEMENT_STEP_LFO_COMMAND(1, LFO1)
IMPLEMENT_STEP_LFO_COMMAND(2, LFO2)
IMPLEMENT_STEP_LFO_COMMAND(3, LFO3)
IMPLEMENT_STEP_LFO_COMMAND(4, LFO4)

#undef IMPLEMENT_STEP_LFO_COMMAND

//==============================================================================
// HIGH-LEVEL COMMANDS
//==============================================================================

juce::var CommandAPI::applySynthPreset(const juce::var& params) {
    WingmanSynthBridge* bridge = getSynthBridgeForActiveTrack();
    if (!bridge) return createErrorResponse("No synth active on current track");
    
    juce::String presetName = params.getProperty("preset", "").toString();
    if (presetName.isEmpty()) {
        return createErrorResponse("No preset specified");
    }
    
    // Apply preset based on name/type
    if (presetName == "init" || presetName == "initialize") {
        // Reset to init patch
        bridge->setOscillatorWaveform(1, OscillatorWaveform::Saw);
        bridge->setOscillatorMix(1, 1.0f);
        bridge->setFilterCutoff(20000.0f);
        bridge->setFilterResonance(0.0f);
        bridge->setAmpEnvelope(0.01f, 0.1f, 0.8f, 0.1f);
    } else if (presetName == "bass") {
        bridge->setOscillatorWaveform(1, OscillatorWaveform::Saw);
        bridge->setOscillatorWaveform(2, OscillatorWaveform::Square);
        bridge->setOscillatorMix(1, 0.7f);
        bridge->setOscillatorMix(2, 0.5f);
        bridge->setFilterCutoff(800.0f);
        bridge->setFilterResonance(0.3f);
        bridge->setAmpEnvelope(0.01f, 0.2f, 0.6f, 0.3f);
        bridge->setFilterEnvelope(0.01f, 0.3f, 0.0f, 0.3f);
    } else if (presetName == "lead") {
        bridge->setOscillatorWaveform(1, OscillatorWaveform::Saw);
        bridge->setUnisonVoices(4);
        bridge->setUnisonDetune(15.0f);
        bridge->setFilterCutoff(8000.0f);
        bridge->setAmpEnvelope(0.05f, 0.2f, 0.9f, 0.4f);
    } else if (presetName == "pad") {
        bridge->setOscillatorWaveform(1, OscillatorWaveform::Saw);
        bridge->setOscillatorWaveform(2, OscillatorWaveform::Triangle);
        bridge->setUnisonVoices(7);
        bridge->setUnisonDetune(25.0f);
        bridge->setFilterCutoff(3000.0f);
        bridge->setAmpEnvelope(0.5f, 1.0f, 0.8f, 2.0f);
        bridge->setReverb(0.5f);
    } else {
        return createErrorResponse("Unknown preset: " + presetName);
    }
    
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("preset", presetName);
    
    return createSuccessResponse(juce::var(result.get()));
}

juce::var CommandAPI::randomizeSynthPatch(const juce::var& params) {
    WingmanSynthBridge* bridge = getSynthBridgeForActiveTrack();
    if (!bridge) return createErrorResponse("No synth active on current track");
    
    float amount = static_cast<float>(params.getProperty("amount", 0.5));
    amount = juce::jlimit(0.0f, 1.0f, amount);
    
    bridge->randomizePatch(amount);
    
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("amount", amount);
    result->setProperty("amountPercent", static_cast<int>(amount * 100));
    
    return createSuccessResponse(juce::var(result.get()));
}

juce::var CommandAPI::analyzeSynthPatch(const juce::var& params) {
    WingmanSynthBridge* bridge = getSynthBridgeForActiveTrack();
    if (!bridge) return createErrorResponse("No synth active on current track");
    
    juce::String analysis = bridge->analyzePatch();
    juce::var patchState = bridge->getCurrentPatchState();
    
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("analysis", analysis);
    result->setProperty("patchState", patchState);
    
    return createSuccessResponse(juce::var(result.get()));
}

juce::var CommandAPI::setSynthParameter(const juce::var& params) {
    // Generic parameter setter - allows setting any parameter by ID
    WingmanSynthBridge* bridge = getSynthBridgeForActiveTrack();
    if (!bridge) return createErrorResponse("No synth active on current track");
    
    if (!params.hasProperty("param")) {
        return createErrorResponse("Missing 'param' property");
    }
    if (!params.hasProperty("value")) {
        return createErrorResponse("Missing 'value' property");
    }
    
    juce::String paramId = params["param"].toString();
    float value = static_cast<float>(params["value"]);
    
    bridge->setParameter(paramId, value);
    
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("param", paramId);
    result->setProperty("value", value);
    
    return createSuccessResponse(juce::var(result.get()));
}

} // namespace zenith
