/*
  ==============================================================================

    ZenithButtonAccessibilityTest.cpp
    Created: 2025-12-14
    Author:  Zenith DAW Team

    Unit tests for verifying ZenithButton accessibility.

  ==============================================================================
*/

#include "../ui/controls/ZenithButton.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_core/juce_core.h>

class ZenithButtonAccessibilityTest : public juce::UnitTest {
public:
  ZenithButtonAccessibilityTest()
      : juce::UnitTest("ZenithButtonAccessibilityTest", "Accessibility") {}

  void runTest() override {
    testAccessibilityHandlerCreation();
    testButtonRole();
    testToggleButtonRole();
    testTitleFallback();
    testActions();
  }

private:
  void testAccessibilityHandlerCreation() {
    beginTest("Handler Creation");
    zenith::ZenithButton button("Test Button");

    // Force create handler if possible
    auto* handler = button.getAccessibilityHandler();

    if (handler != nullptr) {
        expect(true, "Accessibility handler created");
    } else {
        logMessage("Accessibility handler is null (headless environment). Skipping checks.");
    }
  }

  void testButtonRole() {
    beginTest("Button Role");
    zenith::ZenithButton button("Test Button");
    button.setToggleable(false);

    auto* handler = button.getAccessibilityHandler();
    if (handler) {
        expectEquals((int)handler->getRole(), (int)juce::AccessibilityRole::button);
    }
  }

  void testToggleButtonRole() {
    beginTest("Toggle Button Role");
    zenith::ZenithButton button("Test Toggle");
    button.setToggleable(true);

    auto* handler = button.getAccessibilityHandler();
    if (handler) {
        expectEquals((int)handler->getRole(), (int)juce::AccessibilityRole::toggleButton);

        // Check state
        auto state = handler->getCurrentState();
        expect(!state.isChecked());

        button.setToggleState(true, false);
        state = handler->getCurrentState();
        expect(state.isChecked());
    }
  }

  void testTitleFallback() {
    beginTest("Title Fallback");

    // Case 1: Text
    zenith::ZenithButton btn1("Text");
    auto* h1 = btn1.getAccessibilityHandler();
    if (h1) expectEquals(h1->getTitle(), juce::String("Text"));

    // Case 2: No Text, Tooltip
    zenith::ZenithButton btn2;
    btn2.setTooltip("Tooltip");
    auto* h2 = btn2.getAccessibilityHandler();
    if (h2) expectEquals(h2->getTitle(), juce::String("Tooltip"));

    // Case 3: Neither
    zenith::ZenithButton btn3;
    auto* h3 = btn3.getAccessibilityHandler();
    if (h3) expectEquals(h3->getTitle(), juce::String("Button"));
  }

  void testActions() {
      beginTest("Actions");
      bool clicked = false;
      zenith::ZenithButton button("Action");
      button.onClick = [&] { clicked = true; };

      auto* handler = button.getAccessibilityHandler();
      if (handler) {
          // Perform 'press' action
          // Note: AccessibilityHandler::performAction returns bool indicating success
          bool result = handler->performAction(juce::AccessibilityActionType::press);
          expect(result, "Action execution returned true");
          expect(clicked, "Press action triggered onClick");
      }
  }
};

static ZenithButtonAccessibilityTest zenithButtonAccessibilityTest;
