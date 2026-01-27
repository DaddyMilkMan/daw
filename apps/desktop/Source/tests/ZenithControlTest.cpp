/*
  ==============================================================================

    ZenithControlTest.cpp
    Created: 2025-12-16
    Author:  Zenith DAW

    Unit tests for ZenithControl base class.

  ==============================================================================
*/

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_core/juce_core.h>
#include "../ui/controls/ZenithControl.h"

class ZenithControlTests : public juce::UnitTest {
public:
    ZenithControlTests() : juce::UnitTest("ZenithControlTests") {}

    void runTest() override {
        beginTest("Keyboard Navigation");
        {
            zenith::ZenithControl control("TestControl");
            control.setRange(0.0f, 100.0f);
            control.setValue(50.0f, false);

            // Step size calculation: (100-0) * 0.01 = 1.0

            // Simulate Up key (increase)
            juce::KeyPress upKey(juce::KeyPress::upKey);
            bool handled = control.keyPressed(upKey);
            expect(handled, "Up key should be handled");
            expectGreaterThan(control.getValue(), 50.0f);
            expectEquals(control.getValue(), 51.0f); // Default step 1%

            // Simulate Down key (decrease)
            juce::KeyPress downKey(juce::KeyPress::downKey);
            handled = control.keyPressed(downKey);
            expect(handled, "Down key should be handled");
            expectLessThan(control.getValue(), 51.0f);
            expectEquals(control.getValue(), 50.0f);

            // Simulate Right key (increase)
            juce::KeyPress rightKey(juce::KeyPress::rightKey);
            handled = control.keyPressed(rightKey);
            expect(handled, "Right key should be handled");
            expectEquals(control.getValue(), 51.0f);

            // Simulate Left key (decrease)
            juce::KeyPress leftKey(juce::KeyPress::leftKey);
            handled = control.keyPressed(leftKey);
            expect(handled, "Left key should be handled");
            expectEquals(control.getValue(), 50.0f);
        }

        beginTest("Fine Control (Shift)");
        {
            zenith::ZenithControl control("TestControl");
            control.setRange(0.0f, 100.0f);
            control.setValue(50.0f, false);

            // Default step 1.0
            // Fine step 0.1 * 1.0 = 0.1

            // Simulate Shift + Up key
            juce::KeyPress shiftUpKey(juce::KeyPress::upKey, juce::ModifierKeys::shiftModifier, 0);
            bool handled = control.keyPressed(shiftUpKey);
            expect(handled, "Shift+Up key should be handled");
            expectWithinAbsoluteError(control.getValue(), 50.1f, 0.001f);

            // Simulate Shift + Down key
            juce::KeyPress shiftDownKey(juce::KeyPress::downKey, juce::ModifierKeys::shiftModifier, 0);
            handled = control.keyPressed(shiftDownKey);
            expect(handled, "Shift+Down key should be handled");
            expectWithinAbsoluteError(control.getValue(), 50.0f, 0.001f);
        }

        beginTest("Range Interval");
        {
            zenith::ZenithControl control("TestControl");
            control.setRange(0.0f, 10.0f, 0.5f); // Step 0.5
            control.setValue(5.0f, false);

            // Simulate Up key
            juce::KeyPress upKey(juce::KeyPress::upKey);
            control.keyPressed(upKey);
            expectEquals(control.getValue(), 5.5f);
        }
    }
};

static ZenithControlTests zenithControlTests;
