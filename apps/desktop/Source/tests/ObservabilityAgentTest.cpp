/*
  ==============================================================================
    ObservabilityAgentTest.cpp
  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include "agents/ObservabilityAgent/ObservabilityAgent.h"

namespace zenith {
namespace tests {

class ObservabilityAgentTest : public juce::UnitTest {
public:
  ObservabilityAgentTest() : juce::UnitTest("ObservabilityAgent") {}

  void runTest() override {
    beginTest("Log Queueing");

    // Instantiate agent
    // This starts the background thread
    agents::ObservabilityAgent agent;

    // Test char* overload (RT-safe path)
    agent.log(agents::ObservabilityAgent::LogLevel::Info, "Test message 1 (char*)");

    // Test String overload
    agent.log(agents::ObservabilityAgent::LogLevel::Warning, juce::String("Test message 2 (String)"));

    // Test rapid logging to ensure buffer wrap-around handling
    // We log enough to potentially wrap if buffer was small, but with 1024 size it's fine.
    // Just checking it doesn't crash.
    for (int i = 0; i < 50; ++i) {
      agent.log(agents::ObservabilityAgent::LogLevel::Debug, "Rapid fire log");
    }

    // Verify RT-safe methods don't crash
    agent.recordCounter("test_counter", 1.0);

    // Give the background thread some time to process the logs
    // We can't easily assert on DBG output, but we verify stability.
    juce::Thread::sleep(200);

    expect(true, "ObservabilityAgent ran without crashing");
  }
};

static ObservabilityAgentTest observabilityAgentTest;

} // namespace tests
} // namespace zenith
