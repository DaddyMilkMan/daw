/*
  ==============================================================================

    ZenithButtonAccessibilityTest.cpp
    Created: 2025-12-14
    Author:  Zenith DAW Team

    Unit tests for ZenithButton accessibility.

  ==============================================================================
*/

#include "../ui/controls/ZenithButton.h"
#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>

class ZenithButtonAccessibilityTest : public juce::UnitTest {
public:
  ZenithButtonAccessibilityTest()
      : juce::UnitTest("ZenithButtonAccessibilityTest", "Accessibility") {}

  void runTest() override {
    testRoleAndTitle();
    testIconOnlyFallback();
    testActions();
  }

private:
  void testRoleAndTitle() {
    beginTest("Role and Title");

    zenith::ZenithButton button("Click Me");
    // Ensure handler is created
    auto* handler = button.getAccessibilityHandler();

    if (handler != nullptr) {
        expectEquals((int)handler->getRole(), (int)juce::AccessibilityRole::button);
        expectEquals(handler->getTitle(), juce::String("Click Me"));
    } else {
        logMessage("Accessibility handler not available (headless)");
    }

    zenith::ZenithButton toggleButton("Toggle");
    toggleButton.setToggleable(true);

    auto* toggleHandler = toggleButton.getAccessibilityHandler();
    if (toggleHandler != nullptr) {
        expectEquals((int)toggleHandler->getRole(), (int)juce::AccessibilityRole::toggleButton);
    }
  }

  void testIconOnlyFallback() {
    beginTest("Icon Only Fallback");

    zenith::ZenithButton button;
    button.setTooltip("Save Project");

    auto* handler = button.getAccessibilityHandler();
    if (handler != nullptr) {
        expectEquals(handler->getTitle(), juce::String("Save Project"), "Should use tooltip as title when text is empty");

        // Check priority
        button.setButtonText("Explicit Text");
        expectEquals(handler->getTitle(), juce::String("Explicit Text"), "Explicit text should take priority");
    }
  }

  void testActions() {
    beginTest("Actions");

    zenith::ZenithButton button("Action");
    bool clicked = false;
    button.onClick = [&] { clicked = true; };

    auto* handler = button.getAccessibilityHandler();
    if (handler != nullptr) {
        // Attempt to perform press action
        // Note: In some JUCE versions/environments, getAction/performAction availability might vary.
        // Assuming JUCE 6+ API.

        // We can check if the action exists
        // const auto& actions = handler->getActions(); // protected?

        // Try performAction directly
        handler->performAction(juce::AccessibilityActionType::press);

        expect(clicked, "onClick should be triggered by accessibility press");

        // Test toggle action
        button.setToggleable(true);
        // Note: handler might be cached with old actions if already created?
        // AccessibilityHandler is usually immutable regarding supported actions once created?
        // Or we need to invalidate it?
        // JUCE components re-create handler if needed?
        // But here we modify the EXISTING button.
        // Our createAccessibilityHandler creates actions based on CURRENT state.
        // If we change state, we might need to recreate handler.
        // Component::accessibilityHandler is a unique_ptr managed by component.
        // Changing toggleable_ doesn't automatically recreate it in standard JUCE.
        // So let's create a NEW button for toggle test to be safe.
    }

    zenith::ZenithButton toggleBtn("Toggle");
    toggleBtn.setToggleable(true);
    bool toggled = false;
    // Current state is false.

    auto* tHandler = toggleBtn.getAccessibilityHandler();
    if (tHandler != nullptr) {
        tHandler->performAction(juce::AccessibilityActionType::toggle);
        expect(toggleBtn.getToggleState() == true, "Toggle state should change after accessibility toggle");
    }
  }
};

static ZenithButtonAccessibilityTest zenithButtonAccessibilityTest;
