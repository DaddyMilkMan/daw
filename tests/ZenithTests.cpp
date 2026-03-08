/*
    Zenith Synth - Unit Tests
    Copyright (C)2025 Micah Cooley <micahcooley@protonmail.com>
    AGPL-3.0
*/

#include <modules/zenith_core/instruments/ZenithOscillator.h>
#include <modules/zenith_core/instruments/ZenithFilter.h>
#include <modules/zenith_core/instruments/ZenithPolySynthVoice.h>
#include <juce_unit_test/juce_unit_test.h>

namespace zenith {

//==============================================================================
// OSCILLATOR TESTS
//==============================================================================

class OscillatorTests : public juce::UnitTest {
public:
    OscillatorTests() : juce::UnitTest("ZenithOscillator") {}

    void runTest() override {
        beginTest("OscillatorInit");
        ZenithOscillator osc;
        osc.setSampleRate(44100.0);
        expect(osc.getWaveform() == OscillatorWaveform::Saw);

        beginTest("GenerateSaw");
        osc.setWaveform(OscillatorWaveform::Saw);
        float sample = osc.getNextSample(440.0f);
        expect(sample >= -1.0f && sample <= 1.0f);

        beginTest("UnisonVoices");
        osc.setUnisonVoices(8);
        expect(osc.getUnisonVoices() == 8);

        beginTest("AnalogDrift");
        osc.setAnalogDrift(0.5f);
        expect(osc.getAnalogDrift() == 0.5f);
    }
};

//==============================================================================
// FILTER TESTS
//==============================================================================

class FilterTests : public juce::UnitTest {
public:
    FilterTests() : juce::UnitTest("ZenithFilter") {}

    void runTest() override {
        beginTest("FilterInit");
        ZenithFilter filter;
        filter.setSampleRate(44100.0);
        filter.setType(FilterType::LowPass);

        beginTest("ProcessSample");
        float input = 0.5f;
        float output = filter.processSample(input, 60.0f);
        expect(output > -1.0f && output < 1.0f);

        beginTest("CutoffRange");
        filter.setCutoff(1000.0f);
        // Filter should process without crash

        beginTest("Oversampling");
        filter.setOversampling(4);
        // Should handle oversampling
    }
};

//==============================================================================
// VOICE TESTS
//==============================================================================

class VoiceTests : public juce::UnitTest {
public:
    VoiceTests() : juce::UnitTest("ZenithVoice") {}

    void runTest() override {
        beginTest("VoiceInit");
        ZenithPolySynthVoice voice;
        voice.setSampleRate(44100.0);

        beginTest("OscillatorMix");
        voice.setOsc1Mix(0.5f);
        voice.setOsc2Mix(0.3f);
        voice.setOsc3Mix(0.2f);

        beginTest("FilterSetup");
        voice.setFilter1Type(FilterType::LowPass);
        voice.setFilter1Cutoff(1000.0f);
        voice.setFilter1Resonance(0.5f);

        beginTest("UnisonVoices");
        voice.setUnisonVoices(8);
        // Should handle 8 voice unison

        beginTest("Envelopes");
        voice.setAmpEnv(0.01f, 0.1f, 0.7f, 0.3f);
        voice.setModEnv(0.05f, 0.2f, 0.5f, 0.5f);
    }
};

//==============================================================================
// MODULATION TESTS
//==============================================================================

class ModulationTests : public juce::UnitTest {
public:
    ModulationTests() : juce::UnitTest("ModulationMatrix") {}

    void runTest() override {
        beginTest("MatrixSlots");
        ZenithModulationMatrix matrix;
        matrix.setSlot(0, ModulationSource::LFO1,
            ModulationDestination::FilterCutoff, 0.5f);

        beginTest("MacroControls");
        matrix.setMacroValue(0, 0.75f);
        expect(matrix.getMacroValue(0) == 0.75f);
    }
};

} // namespace zenith
