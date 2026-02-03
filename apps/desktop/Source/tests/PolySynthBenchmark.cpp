/*
  ==============================================================================

    PolySynthBenchmark.cpp
    Created: 2025-02-18
    Author:  Zenith DAW

    Benchmark for ZenithPolySynthVoice rendering performance.

  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "../instruments/ZenithPolySynthVoice.h"

namespace zenith {
namespace tests {

class PolySynthBenchmark : public juce::UnitTest {
public:
  PolySynthBenchmark() : juce::UnitTest("PolySynthBenchmark", "Performance") {}

  void runTest() override {
    runBenchmark("Baseline (No Mod)", false, false);
    runBenchmark("Heavy Mod (LFO->Pitch)", true, false);
    runBenchmark("Unison (7 Voices)", false, true);
  }

  void runBenchmark(const juce::String& name, bool withModulation, bool withUnison) {
    beginTest(name);

    juce::MPESynthesiser synth;
    auto* voice = new ZenithPolySynthVoice();
    synth.addVoice(voice);

    synth.setCurrentPlaybackSampleRate(44100.0);

    // Apply setup
    if (withModulation) {
        // Detune forces the constant calculation
        voice->setOsc1Detune(15.0f);
        // LFO Pitch mod forces the variable calculation
        voice->setLFO1(5.0f, 10.0f, LFOTarget::Osc1Pitch, LFOWaveform::Sine);
    } else {
        voice->setOsc1Detune(0.0f);
        voice->setLFO1(0.0f, 0.0f, LFOTarget::FilterCutoff, LFOWaveform::Sine);
    }

    if (withUnison) {
        voice->setUnisonVoices(7);
        voice->setUnisonDetune(20.0f);
    }

    // Prepare buffer
    const int numChannels = 2;
    const int numSamples = 512;
    juce::AudioBuffer<float> buffer(numChannels, numSamples);
    juce::MidiBuffer midiBuffer;

    // Start a note
    synth.handleMidiEvent(juce::MidiMessage::noteOn(1, 60, 1.0f));

    // Warmup
    for (int i = 0; i < 100; ++i) {
        buffer.clear();
        synth.renderNextBlock(buffer, midiBuffer, 0, numSamples);
    }

    // Benchmark Loop
    double totalTime = 0.0;
    const int numBlocks = 5000;

    double start = juce::Time::getMillisecondCounterHiRes();

    for (int i = 0; i < numBlocks; ++i) {
        buffer.clear();
        synth.renderNextBlock(buffer, midiBuffer, 0, numSamples);
    }

    double end = juce::Time::getMillisecondCounterHiRes();
    totalTime = end - start;

    logMessage(juce::String("Total time for ") + juce::String(numBlocks) + " blocks: " + juce::String(totalTime, 3) + " ms");
    logMessage(juce::String("Average time per block: ") + juce::String(totalTime / numBlocks, 5) + " ms");
  }
};

static PolySynthBenchmark polySynthBenchmark;

} // namespace tests
} // namespace zenith

#ifdef POLYSYNTH_BENCHMARK_STANDALONE
int main() {
    juce::UnitTestRunner runner;
    runner.runAllTests();
    return 0;
}
#endif
