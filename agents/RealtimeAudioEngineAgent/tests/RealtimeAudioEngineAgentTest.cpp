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
namespace tests {

class RealtimeAudioEngineAgentTest : public juce::UnitTest {
public:
  RealtimeAudioEngineAgentTest() : juce::UnitTest("RealtimeAudioEngineAgent", "Agents") {}

  void runTest() override {
    beginTest("Initialization and Defaults");
    {
      RealtimeAudioEngineAgent agent;
      const auto& metrics = agent.getMetrics();

      expect(metrics.cpuUsage.load() == 0.0f);
      expect(metrics.bufferUnderruns.load() == 0);
      expect(metrics.samplesProcessed.load() == 0);
      expect(!metrics.overloadDetected.load());
      expect(!agent.isRunning());
    }

    beginTest("ProcessBlock updates metrics");
    {
      RealtimeAudioEngineAgent agent;
      agent.initialize(44100.0, 512);
      agent.start();

      juce::AudioBuffer<float> buffer(2, 512);
      buffer.clear();
      juce::MidiBuffer midi;

      // Process a few blocks
      for (int i = 0; i < 10; ++i) {
        agent.processBlock(buffer, midi);
      }

      const auto& metrics = agent.getMetrics();

      // Check samples processed
      expectEquals(metrics.samplesProcessed.load(), (int64_t)(512 * 10));

      // Check CPU usage (should be non-negative)
      expect(metrics.cpuUsage.load() >= 0.0f);
      expect(metrics.cpuUsage.load() <= 4.0f); // Guardrail check

      // Check underruns (should be 0 for this simple test)
      expectEquals(metrics.bufferUnderruns.load(), 0);
      expect(!metrics.overloadDetected.load());
    }
  }
};

static RealtimeAudioEngineAgentTest realtimeAudioEngineAgentTest;

} // namespace tests
} // namespace agents
} // namespace zenith
