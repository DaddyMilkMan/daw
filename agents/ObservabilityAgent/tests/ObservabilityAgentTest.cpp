/*
  ==============================================================================
    agents/ObservabilityAgent/tests/ObservabilityAgentTest.cpp
    Unit tests for ObservabilityAgent periodic export.
  ==============================================================================
*/

#include "../ObservabilityAgent.h"
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>

namespace zenith {
namespace agents {

class ObservabilityAgentTest : public juce::UnitTest {
public:
  ObservabilityAgentTest() : juce::UnitTest("ObservabilityAgent Tests", "ObservabilityAgent") {}

  void runTest() override {
    testPeriodicExport();
  }

private:
  void testPeriodicExport() {
    beginTest("Periodic Export");

    ObservabilityAgent agent;

    // Initial state
    expectEquals(agent.getExportCount(), (uint64_t)0);

    // Set interval to 50ms
    agent.setExportInterval(std::chrono::milliseconds(50));

    // Wait for at least one export (allow 150ms to be safe)
    // We must pump the message loop to allow the Timer to fire

    // Ensure message manager is initialized
    if (auto* mm = juce::MessageManager::getInstance()) {
        juce::Timer::callAfterDelay(150, [mm] { mm->stopDispatchLoop(); });
        mm->runDispatchLoop();
    } else {
        // Fallback if no message manager (shouldn't happen with correct runner)
        // But for Timer to work, MessageManager MUST be present.
        expect(false, "MessageManager not initialized, Timer cannot run");
    }

    // Check that exports happened
    expect(agent.getExportCount() > 0, "Metrics should have been exported at least once");

    // Stop export
    agent.setExportInterval(std::chrono::milliseconds(0));
    uint64_t countAfterStop = agent.getExportCount();

    // Wait again
    if (auto* mm = juce::MessageManager::getInstance()) {
        juce::Timer::callAfterDelay(150, [mm] { mm->stopDispatchLoop(); });
        mm->runDispatchLoop();
    }

    expectEquals(agent.getExportCount(), countAfterStop, "Metrics should not be exported after stopping");
  }
};

static ObservabilityAgentTest observabilityAgentTest;

} // namespace agents
} // namespace zenith
