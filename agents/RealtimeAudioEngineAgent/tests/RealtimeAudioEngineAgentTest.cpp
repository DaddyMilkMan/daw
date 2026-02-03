/*
  ==============================================================================
    agents/RealtimeAudioEngineAgent/tests/RealtimeAudioEngineAgentTest.cpp
    Unit tests for RealtimeAudioEngineAgent
  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "../RealtimeAudioEngineAgent.h"

namespace zenith {
namespace agents {

class RealtimeAudioEngineAgentTest : public juce::UnitTest {
public:
  RealtimeAudioEngineAgentTest() : juce::UnitTest("RealtimeAudioEngineAgent", "Agents") {}

  void runTest() override {
    beginTest("Initialization");
    {
      RealtimeAudioEngineAgent engine;
      engine.initialize(44100.0, 512);

      // No crash implies success for now, as we don't have public accessors for graph
      expect(true, "Engine initialized without crashing");
    }

    beginTest("Audio Processing (Pass-through)");
    {
      RealtimeAudioEngineAgent engine;
      engine.initialize(44100.0, 512);

      juce::AudioBuffer<float> buffer(2, 512);
      // Fill with silence
      buffer.clear();
      // Fill channel 0 with DC
      for (int i = 0; i < 512; ++i) buffer.setSample(0, i, 1.0f);

      juce::MidiBuffer midi;

      // Process
      engine.processBlock(buffer, midi);

      // Check output
      // Since it's pass through, channel 0 should still be 1.0f (or close if precision loss, but copy is exact usually)
      float rms = buffer.getRMSLevel(0, 0, 512);
      expect(rms > 0.9f, "Audio passed through (Signal present)");

      // Channel 1 was silent, should remain silent
      float rms1 = buffer.getRMSLevel(1, 0, 512);
      expect(rms1 < 0.001f, "Audio passed through (Silence preserved)");
    }
  }
};

static RealtimeAudioEngineAgentTest realtimeAudioEngineAgentTest;

} // namespace agents
} // namespace zenith
