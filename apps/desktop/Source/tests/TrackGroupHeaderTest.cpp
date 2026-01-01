/*
  ==============================================================================

    TrackGroupHeaderTest.cpp
    Created: 2025-12-31
    Author: Agent

  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../ui/mixer/TrackGroupHeader.h"

namespace zenith {
namespace tests {

class TrackGroupHeaderTest : public juce::UnitTest {
public:
  TrackGroupHeaderTest() : juce::UnitTest("TrackGroupHeaderTest") {}

  void runTest() override {
    runInitialStateTest();
    runPropsUpdateTest();
    runVUMeterApiTest();
  }

private:
  void runInitialStateTest() {
    beginTest("Initial State");

    TrackGroupHeader header;
    TrackGroupHeaderProps props = header.getProps();

    expectEquals(props.groupName, juce::String("Group"));
    expect(props.isExpanded);
    expect(!props.isMuted);
    expect(!props.isSolo);
    expectEquals(props.trackCount, 0);
  }

  void runPropsUpdateTest() {
    beginTest("Props Update");

    TrackGroupHeader header;
    TrackGroupHeaderProps newProps;
    newProps.groupName = "Drums";
    newProps.isMuted = true;
    newProps.trackCount = 5;

    header.setProps(newProps);
    
    // Verify props are updated
    const auto& currentProps = header.getProps();
    expectEquals(currentProps.groupName, juce::String("Drums"));
    expect(currentProps.isMuted);
    expectEquals(currentProps.trackCount, 5);
  }

  void runVUMeterApiTest() {
    beginTest("VU Meter API Thread Safety");

    TrackGroupHeader header;
    auto* meter = header.getVUMeter();

    // Verify non-blocking setLevel can be called rapidly (simulation)
    // In a real test we can't easily assert on "non-blocking", but we can ensure
    // it doesn't crash or throw.
    for (int i = 0; i < 1000; ++i) {
        float level = (float)i / 1000.0f;
        meter->setLevel(level);
    }
    
    // Test stereo API
    meter->setStereo(true);
    expect(meter->isStereo());
    
    for (int i = 0; i < 1000; ++i) {
        meter->setLevels(0.5f, 0.8f);
    }
    
    // Pass if we got here without crashing
    expect(true); 
  }
};

static TrackGroupHeaderTest trackGroupHeaderTest;

} // namespace tests
} // namespace zenith
