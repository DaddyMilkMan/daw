/*
  ==============================================================================

    MPEHelpersTest.cpp
    Created: 2026-01-29
    Author:  Zenith DAW

    Unit tests for MPE expression helper functions.

  ==============================================================================
*/

#include "../Source/ui/piano-roll/MPEExpressionHelpers.h"
#include <juce_core/juce_core.h>

namespace zenith {
namespace test {

class MPEHelpersTest : public juce::UnitTest {
public:
  MPEHelpersTest() : juce::UnitTest("MPE Helper Functions", "MPE") {}

  void runTest() override {
    testExpressionLabels();
    testExpressionColors();
    testClampedLaneHeight();
  }

private:
  void testExpressionLabels() {
    beginTest("Expression type labels");

    expectEquals(getExpressionLabel(ExpressionType::PitchBend),
                 juce::String("PITCH"), "PitchBend label");
    expectEquals(getExpressionLabel(ExpressionType::Pressure),
                 juce::String("PRESSURE"), "Pressure label");
    expectEquals(getExpressionLabel(ExpressionType::Slide),
                 juce::String("SLIDE (MPE)"), "Slide label");
    expectEquals(getExpressionLabel(ExpressionType::Expression),
                 juce::String("EXPRESSION"), "Expression label");
  }

  void testExpressionColors() {
    beginTest("Expression type colors");

    SkColor pitchColor = getExpressionColor(ExpressionType::PitchBend);
    SkColor pressureColor = getExpressionColor(ExpressionType::Pressure);
    SkColor slideColor = getExpressionColor(ExpressionType::Slide);
    SkColor exprColor = getExpressionColor(ExpressionType::Expression);

    // Verify colors are distinct (not equal)
    expect(pitchColor != pressureColor, "Pitch and Pressure colors differ");
    expect(pressureColor != slideColor, "Pressure and Slide colors differ");
    expect(slideColor != exprColor, "Slide and Expression colors differ");
  }

  void testClampedLaneHeight() {
    beginTest("Lane height clamping");

    float minHeight = 40.0f;
    float maxHeight = 200.0f;

    float belowMin = 20.0f;
    float aboveMax = 300.0f;
    float inRange = 100.0f;

    float clampedBelow = getClampedLaneHeight(belowMin);
    float clampedAbove = getClampedLaneHeight(aboveMax);
    float clampedInRange = getClampedLaneHeight(inRange);

    expect(clampedBelow >= minHeight, "Below minimum is clamped up");
    expect(clampedAbove <= maxHeight, "Above maximum is clamped down");
    expect(clampedInRange == inRange, "In-range value is unchanged");
  }
};

static MPEHelpersTest mpeHelpersTest;

} // namespace test
} // namespace zenith
