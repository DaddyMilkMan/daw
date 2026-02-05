/*
  ==============================================================================
    ScheduledTransportAgentTest.cpp
    Unit tests for ScheduledTransportAgent.
  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include "agents/ScheduledTransportAgent/ScheduledTransportAgent.h"

namespace zenith {
namespace tests {

class ScheduledTransportAgentTest : public juce::UnitTest {
public:
  ScheduledTransportAgentTest() : juce::UnitTest("ScheduledTransportAgent") {}

  class MockListener : public agents::ScheduledTransportAgent::Listener {
  public:
    void transportStateChanged(agents::ScheduledTransportAgent::TransportState newState) override {
      lastState = newState;
      callCount++;
    }

    agents::ScheduledTransportAgent::TransportState lastState{agents::ScheduledTransportAgent::TransportState::Stopped};
    int callCount{0};
  };

  void runTest() override {
    beginTest("Transport State Notifications");

    agents::ScheduledTransportAgent agent;
    MockListener listener;

    agent.addListener(&listener);

    // Test Play
    agent.play();
    expectEquals(listener.callCount, 1, "Listener should be called once for play");
    expect(listener.lastState == agents::ScheduledTransportAgent::TransportState::Playing, "State should be Playing");

    // Test Stop
    agent.stop();
    expectEquals(listener.callCount, 2, "Listener should be called once for stop");
    expect(listener.lastState == agents::ScheduledTransportAgent::TransportState::Stopped, "State should be Stopped");

    // Test Record
    agent.record();
    expectEquals(listener.callCount, 3, "Listener should be called once for record");
    expect(listener.lastState == agents::ScheduledTransportAgent::TransportState::Recording, "State should be Recording");

    // Test Pause
    agent.pause();
    expectEquals(listener.callCount, 4, "Listener should be called once for pause");
    expect(listener.lastState == agents::ScheduledTransportAgent::TransportState::Paused, "State should be Paused");

    // Test Remove Listener
    agent.removeListener(&listener);
    agent.play();
    expectEquals(listener.callCount, 4, "Listener should not be called after removal");

    // Check internal state matches
    expect(agent.getState() == agents::ScheduledTransportAgent::TransportState::Playing, "Internal state should be Playing");
  }
};

static ScheduledTransportAgentTest scheduledTransportAgentTest;

} // namespace tests
} // namespace zenith
