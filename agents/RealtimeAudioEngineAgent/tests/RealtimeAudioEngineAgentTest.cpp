/*
  ==============================================================================
    agents/RealtimeAudioEngineAgent/tests/RealtimeAudioEngineAgentTest.cpp
    Unit tests for RealtimeAudioEngineAgent.
  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "../RealtimeAudioEngineAgent.h"

namespace zenith {
namespace agents {
namespace tests {

class RealtimeAudioEngineAgentTest : public juce::UnitTest {
public:
  RealtimeAudioEngineAgentTest() : juce::UnitTest("RealtimeAudioEngineAgent", "Agents") {}

  void runTest() override {
    beginTest("Initialization and Graph Setup");
    {
      RealtimeAudioEngineAgent engine;
      engine.initialize(44100.0, 512);

      // We can't inspect the graph directly as it's private, but we can verify behavior.
      expect(!engine.isRunning(), "Engine should not be running after init");

      engine.start();
      expect(engine.isRunning(), "Engine should be running after start");

      engine.stop();
      expect(!engine.isRunning(), "Engine should not be running after stop");
    }

    beginTest("Audio Pass-through");
    {
      RealtimeAudioEngineAgent engine;
      double sampleRate = 44100.0;
      int bufferSize = 512;
      engine.initialize(sampleRate, bufferSize);
      engine.start();

      juce::AudioBuffer<float> buffer(2, bufferSize);
      juce::MidiBuffer midi;

      // Create a test signal in the input
      for (int ch = 0; ch < 2; ++ch) {
        auto* data = buffer.getWritePointer(ch);
        for (int i = 0; i < bufferSize; ++i) {
          data[i] = 1.0f; // DC offset for simplicity
        }
      }

      // Process
      engine.processBlock(buffer, midi);

      // Verify output matches input (pass-through)
      // Since it's a direct connection, it should be exactly the same.

      float magnitude = buffer.getMagnitude(0, bufferSize);
      expect(magnitude > 0.0f, "Output should not be silent");

      for (int ch = 0; ch < 2; ++ch) {
          float chMag = buffer.getMagnitude(ch, 0, bufferSize);
          expectEquals(chMag, 1.0f);
      }
    }
  }
};

static RealtimeAudioEngineAgentTest realtimeAudioEngineAgentTest;

} // namespace tests
} // namespace agents
} // namespace zenith
