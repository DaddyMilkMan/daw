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

    CommandAPI_SynthHandlers.cpp
    Created: 2025-01-29
    Author:  Zenith DAW

    Synth command handlers for CommandAPI.
    Add these methods to CommandAPI class and call from executeCommand().


  ==============================================================================*/

// Add this include to CommandAPI.cpp:
// #include "../ai/WingmanSynthBridge.h"
// #include "../instruments/ZenithPolySynth.h"

namespace zenith {

//==============================================================================
// Helper: Get WingmanSynthBridge for active track's synth
WingmanSynthBridge* CommandAPI::getSynthBridgeForActiveTrack() {
    // Get active track
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
    
    // Get the synth processor (you'll need to cache this somewhere)
    // For now, return nullptr - implement based on your architecture
    return nullptr;
}

//==============================================================================
// Command Handler Implementations
// Add these to CommandAPI.cpp

juce::var CommandAPI::setSynthOscillatorWave(const juce::var& params) {
    // params: { "oscillator": 1, "waveform": "saw" }
    
    auto* bridge = getSynthBridgeForActiveTrack();
    if (!bridge) {
        return createErrorResponse("No ZenithPolySynth active on this track");
    }
    
    int oscIndex = params.getProperty("oscillator", 1);
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
        return createErrorResponse("Unknown waveform: " + waveStr);
    }
    
    bridge->setOscillatorWaveform(oscIndex, wave);
    
    return createSuccessResponse("Oscillator " + juce::String(oscIndex) + 
                                 " set to " + waveStr);
}

//==============================================================================
juce::var CommandAPI::setSynthOscillatorDetune(const juce::var& params) {
    // params: { "oscillator": 1, "detune": 5.0 } (cents)
    
    auto* bridge = getSynthBridgeForActiveTrack();
    if (!bridge) return createErrorResponse("No synth active");
    
    int oscIndex = params.getProperty("oscillator", 1);
    float detune = static_cast<float>(params["detune"].toDouble());
    
    bridge->setOscillatorDetune(oscIndex, detune);
    
    return createSuccessResponse("Oscillator " + juce::String(oscIndex) + 
                                 " detune set to " + juce::String(detune, 1) + " ct");
}

//==============================================================================
juce::var CommandAPI::setSynthOscillatorMix(const juce::var& params) {
    // params: { "oscillator": 1, "mix": 0.8 } (0.0 to 1.0)
    
    auto* bridge = getSynthBridgeForActiveTrack();
    if (!bridge) return createErrorResponse("No synth active");
    
    int oscIndex = params.getProperty("oscillator", 1);
    float mix = static_cast<float>(params["mix"].toDouble());
    
    bridge->setOscillatorMix(oscIndex, mix);
    
    return createSuccessResponse("Oscillator " + juce::String(oscIndex) + 
                                 " mix set to " + juce::String(static_cast<int>(mix * 100)) + "%");
}

//==============================================================================
juce::var CommandAPI::setSynthFilterCutoff(const juce::var& params) {
    // params: { "cutoff": 1200 } (Hz)
    
    auto* bridge = getSynthBridgeForActiveTrack();
    if (!bridge) return createErrorResponse("No synth active");
    
    float cutoff = static_cast<float>(params["cutoff"].toDouble());
    bridge->setFilterCutoff(cutoff);
    
    return createSuccessResponse("Filter cutoff set to " + juce::String(cutoff) + " Hz");
}

//==============================================================================
juce::var CommandAPI::setSynthFilterResonance(const juce::var& params) {
    // params: { "resonance": 0.5 } (0.0 to 1.0)
    
    auto* bridge = getSynthBridgeForActiveTrack();
    if (!bridge) return createErrorResponse("No synth active");
    
    float resonance = static_cast<float>(params["resonance"].toDouble());
    bridge->setFilterResonance(resonance);
    
    return createSuccessResponse("Filter resonance set to " + juce::String(static_cast<int>(resonance * 100)) + "%");
}

//==============================================================================
juce::var CommandAPI::setSynthFilterDrive(const juce::var& params) {
    // params: { "drive": 2.0 } (1.0 to 10.0)
    
    auto* bridge = getSynthBridgeForActiveTrack();
    if (!bridge) return createErrorResponse("No synth active");
    
    float drive = static_cast<float>(params["drive"].toDouble());
    bridge->setFilterDrive(drive);
    
    return createSuccessResponse("Filter drive set to " + juce::String(drive, 1) + "x");
}

//==============================================================================
juce::var CommandAPI::setSynthAmpEnvelope(const juce::var& params) {
    // params: { "attack": 0.01, "decay": 0.2, "sustain": 0.7, "release": 0.3 }
    
    auto* bridge = getSynthBridgeForActiveTrack();
    if (!bridge) return createErrorResponse("No synth active");
    
    float attack = static_cast<float>(params.getProperty("attack", 0.01).toDouble());
    float decay = static_cast<float>(params.getProperty("decay", 0.1).toDouble());
    float sustain = static_cast<float>(params.getProperty("sustain", 0.8).toDouble());
    float release = static_cast<float>(params.getProperty("release", 0.1).toDouble());
    
    bridge->setAmpEnvelope(attack, decay, sustain, release);
    
    return createSuccessResponse("Amp envelope: A=" + juce::String(attack) + 
                                 " D=" + juce::String(decay) + 
                                 " S=" + juce::String(sustain) + 
                                 " R=" + juce::String(release));
}

//==============================================================================
juce::var CommandAPI::setSynthFilterEnvelope(const juce::var& params) {
    // params: { "attack": 0.01, "decay": 0.2, "sustain": 0.7, "release": 0.3 }
    
    auto* bridge = getSynthBridgeForActiveTrack();
    if (!bridge) return createErrorResponse("No synth active");
    
    float attack = static_cast<float>(params.getProperty("attack", 0.01).toDouble());
    float decay = static_cast<float>(params.getProperty("decay", 0.1).toDouble());
    float sustain = static_cast<float>(params.getProperty("sustain", 0.8).toDouble());
    float release = static_cast<float>(params.getProperty("release", 0.1).toDouble());
    
    bridge->setFilterEnvelope(attack, decay, sustain, release);
    
    return createSuccessResponse("Filter envelope set");
}

//==============================================================================
juce::var CommandAPI::setSynthLFORate(const juce::var& params) {
    // params: { "lfo": 1, "rate": 5.0 } (Hz)
    
    auto* bridge = getSynthBridgeForActiveTrack();
    if (!bridge) return createErrorResponse("No synth active");
    
    int lfoIndex = params.getProperty("lfo", 1);
    float rate = static_cast<float>(params["rate"].toDouble());
    
    bridge->setLFORate(lfoIndex, rate);
    
    return createSuccessResponse("LFO " + juce::String(lfoIndex) + 
                                 " rate set to " + juce::String(rate, 2) + " Hz");
}

//==============================================================================
juce::var CommandAPI::setSynthLFOAmount(const juce::var& params) {
    // params: { "lfo": 1, "amount": 0.5 } (0.0 to 1.0)
    
    auto* bridge = getSynthBridgeForActiveTrack();
    if (!bridge) return createErrorResponse("No synth active");
    
    int lfoIndex = params.getProperty("lfo", 1);
    float amount = static_cast<float>(params["amount"].toDouble());
    
    bridge->setLFOAmount(lfoIndex, amount);
    
    return createSuccessResponse("LFO " + juce::String(lfoIndex) + 
                                 " amount set to " + juce::String(static_cast<int>(amount * 100)) + "%");
}

//==============================================================================
juce::var CommandAPI::setSynthDistortion(const juce::var& params) {
    // params: { "amount": 0.5 } (0.0 to 1.0)
    
    auto* bridge = getSynthBridgeForActiveTrack();
    if (!bridge) return createErrorResponse("No synth active");
    
    float amount = static_cast<float>(params["amount"].toDouble());
    bridge->setDistortion(amount);
    
    return createSuccessResponse("Distortion set to " + juce::String(static_cast<int>(amount * 100)) + "%");
}

//==============================================================================
juce::var CommandAPI::setSynthChorus(const juce::var& params) {
    // params: { "amount": 0.3 } (0.0 to 1.0)
    
    auto* bridge = getSynthBridgeForActiveTrack();
    if (!bridge) return createErrorResponse("No synth active");
    
    float amount = static_cast<float>(params["amount"].toDouble());
    bridge->setChorus(amount);
    
    return createSuccessResponse("Chorus set to " + juce::String(static_cast<int>(amount * 100)) + "%");
}

//==============================================================================
juce::var CommandAPI::setSynthReverb(const juce::var& params) {
    // params: { "amount": 0.4 } (0.0 to 1.0)
    
    auto* bridge = getSynthBridgeForActiveTrack();
    if (!bridge) return createErrorResponse("No synth active");
    
    float amount = static_cast<float>(params["amount"].toDouble());
    bridge->setReverb(amount);
    
    return createSuccessResponse("Reverb set to " + juce::String(static_cast<int>(amount * 100)) + "%");
}

//==============================================================================
juce::var CommandAPI::setSynthDelay(const juce::var& params) {
    // params: { "time": 0.5, "feedback": 0.4, "mix": 0.3 }
    
    auto* bridge = getSynthBridgeForActiveTrack();
    if (!bridge) return createErrorResponse("No synth active");
    
    float time = static_cast<float>(params.getProperty("time", 0.5).toDouble());
    float feedback = static_cast<float>(params.getProperty("feedback", 0.5).toDouble());
    float mix = static_cast<float>(params.getProperty("mix", 0.5).toDouble());
    
    bridge->setDelay(time, feedback, mix);
    
    return createSuccessResponse("Delay set");
}

//==============================================================================
juce::var CommandAPI::randomizeSynthPatch(const juce::var& params) {
    // params: { "amount": 0.5 } (0.0 to 1.0, how much to randomize)
    
    auto* bridge = getSynthBridgeForActiveTrack();
    if (!bridge) return createErrorResponse("No synth active");
    
    float amount = static_cast<float>(params.getProperty("amount", 0.5).toDouble());
    
    bridge->randomizePatch(amount);
    
    return createSuccessResponse("Patch randomized by " + juce::String(static_cast<int>(amount * 100)) + "%");
}

//==============================================================================
juce::var CommandAPI::analyzeSynthPatch(const juce::var& params) {
    // params: {}
    
    auto* bridge = getSynthBridgeForActiveTrack();
    if (!bridge) return createErrorResponse("No synth active");
    
    juce::String analysis = bridge->analyzePatch();
    juce::var patchState = bridge->getCurrentPatchState();
    
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("analysis", juce::var(analysis));
    result->setProperty("patchState", patchState);
    
    return createSuccessResponse(juce::var(result.get()));
}

//==============================================================================
// Add to executeCommand() switch statement:
/*
    case CommandID::SetSynthOscillatorWave:
        return setSynthOscillatorWave(params);
    case CommandID::SetSynthOscillatorDetune:
        return setSynthOscillatorDetune(params);
    case CommandID::SetSynthOscillatorMix:
        return setSynthOscillatorMix(params);
    case CommandID::SetSynthFilterCutoff:
        return setSynthFilterCutoff(params);
    case CommandID::SetSynthFilterResonance:
        return setSynthFilterResonance(params);
    case CommandID::SetSynthFilterDrive:
        return setSynthFilterDrive(params);
    case CommandID::SetSynthAmpEnvelope:
        return setSynthAmpEnvelope(params);
    case CommandID::SetSynthFilterEnvelope:
        return setSynthFilterEnvelope(params);
    case CommandID::SetSynthLFORate:
        return setSynthLFORate(params);
    case CommandID::SetSynthLFOAmount:
        return setSynthLFOAmount(params);
    case CommandID::SetSynthDistortion:
        return setSynthDistortion(params);
    case CommandID::SetSynthChorus:
        return setSynthChorus(params);
    case CommandID::SetSynthReverb:
        return setSynthReverb(params);
    case CommandID::SetSynthDelay:
        return setSynthDelay(params);
    case CommandID::RandomizeSynthPatch:
        return randomizeSynthPatch(params);
    case CommandID::AnalyzeSynthPatch:
        return analyzeSynthPatch(params);
    case CommandID::SetSynthUnisonVoices:
        return setSynthUnisonVoices(params);
    case CommandID::SetSynthUnisonDetune:
        return setSynthUnisonDetune(params);
    case CommandID::SetSynthUnisonSpread:
        return setSynthUnisonSpread(params);
    case CommandID::SetSynthUnisonPanRandom:
        return setSynthUnisonPanRandom(params);
    case CommandID::SetSynthArpEnable:
        return setSynthArpEnable(params);
    case CommandID::SetSynthArpMode:
        return setSynthArpMode(params);
    case CommandID::SetSynthArpRate:
        return setSynthArpRate(params);
    case CommandID::SetSynthArpGate:
        return setSynthArpGate(params);
    case CommandID::SetSynthArpSwing:
        return setSynthArpSwing(params);
    case CommandID::SetSynthArpHold:
        return setSynthArpHold(params);
    case CommandID::SetSynthStepLFO1Enable:
        return setSynthStepLFO1Enable(params);
    case CommandID::SetSynthStepLFO1Steps:
        return setSynthStepLFO1Steps(params);
    case CommandID::SetSynthStepLFO1Rate:
        return setSynthStepLFO1Rate(params);
    case CommandID::SetSynthStepLFO1Smoothing:
        return setSynthStepLFO1Smoothing(params);
    case CommandID::SetSynthStepLFO2Enable:
        return setSynthStepLFO2Enable(params);
    case CommandID::SetSynthStepLFO2Steps:
        return setSynthStepLFO2Steps(params);
    case CommandID::SetSynthStepLFO2Rate:
        return setSynthStepLFO2Rate(params);
    case CommandID::SetSynthStepLFO2Smoothing:
        return setSynthStepLFO2Smoothing(params);
    case CommandID::SetSynthStepLFO3Enable:
        return setSynthStepLFO3Enable(params);
    case CommandID::SetSynthStepLFO3Steps:
        return setSynthStepLFO3Steps(params);
    case CommandID::SetSynthStepLFO3Rate:
        return setSynthStepLFO3Rate(params);
    case CommandID::SetSynthStepLFO3Smoothing:
        return setSynthStepLFO3Smoothing(params);
    case CommandID::SetSynthStepLFO4Enable:
        return setSynthStepLFO4Enable(params);
    case CommandID::SetSynthStepLFO4Steps:
        return setSynthStepLFO4Steps(params);
    case CommandID::SetSynthStepLFO4Rate:
        return setSynthStepLFO4Rate(params);
    case CommandID::SetSynthStepLFO4Smoothing:
        return setSynthStepLFO4Smoothing(params);
*/

//==============================================================================
// Add method declarations to CommandAPI.h:
/*
    // Synth Control Commands
    juce::var setSynthOscillatorWave(const juce::var& params);
    juce::var setSynthOscillatorDetune(const juce::var& params);
    juce::var setSynthOscillatorMix(const juce::var& params);
    juce::var setSynthFilterCutoff(const juce::var& params);
    juce::var setSynthFilterResonance(const juce::var& params);
    juce::var setSynthFilterDrive(const juce::var& params);
    juce::var setSynthAmpEnvelope(const juce::var& params);
    juce::var setSynthFilterEnvelope(const juce::var& params);
    juce::var setSynthLFORate(const juce::var& params);
    juce::var setSynthLFOAmount(const juce::var& params);
    juce::var setSynthDistortion(const juce::var& params);
    juce::var setSynthChorus(const juce::var& params);
    juce::var setSynthReverb(const juce::var& params);
    juce::var setSynthDelay(const juce::var& params);
    juce::var randomizeSynthPatch(const juce::var& params);
    juce::var analyzeSynthPatch(const juce::var& params);
    
private:
    WingmanSynthBridge* getSynthBridgeForActiveTrack();
*/

} // namespace zenith