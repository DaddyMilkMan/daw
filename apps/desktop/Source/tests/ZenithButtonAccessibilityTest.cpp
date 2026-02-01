/*
  ==============================================================================

    ZenithButtonAccessibilityTest.cpp
    Created: 2025-12-14
    Author:  Palette (UX Agent)

    Unit tests for verifying accessibility handlers in ZenithButton.

  ==============================================================================
*/

#include "../ui/controls/ZenithButton.h"
#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

class ZenithButtonAccessibilityTest : public juce::UnitTest {
public:
  ZenithButtonAccessibilityTest()
      : juce::UnitTest("ZenithButtonAccessibilityTest", "Accessibility") {}

  void runTest() override {
    testButtonRoleAndTitle();
    testIconOnlyButtonFallback();
    testToggleButtonRole();
    testActions();
    testMouseCursor();
  }

private:
  void testButtonRoleAndTitle() {
    beginTest("Standard Button Accessibility");
    {
      zenith::ZenithButton button("Click Me");
      button.setSize(100, 30);
      button.setVisible(true);

      auto *handler = button.getAccessibilityHandler();

      if (handler != nullptr) {
        expectEquals(static_cast<int>(handler->getRole()),
                     static_cast<int>(juce::AccessibilityRole::button),
                     "Role must be 'button'");

        expectEquals(handler->getTitle(), juce::String("Click Me"),
                     "Title must match button text");
      } else {
        logMessage("Note: Accessibility handler not available");
      }
    }
  }

  void testIconOnlyButtonFallback() {
    beginTest("Icon-Only Button Accessibility Fallback");
    {
      zenith::ZenithButton button;
      button.setTooltip("Play");
      button.setSize(30, 30);
      button.setVisible(true);
      // Simulate icon setting if needed, but text is empty.

      auto *handler = button.getAccessibilityHandler();

      if (handler != nullptr) {
        expectEquals(handler->getTitle(), juce::String("Play"),
                     "Title must fallback to tooltip for icon-only button");

        expectEquals(handler->getHelp(), juce::String("Play"),
                     "Help text must match tooltip");
      }
    }
  }

  void testToggleButtonRole() {
    beginTest("Toggle Button Accessibility");
    {
      zenith::ZenithButton button("Mute");
      button.setToggleable(true);
      button.setSize(100, 30);
      button.setVisible(true);

      auto *handler = button.getAccessibilityHandler();

      if (handler != nullptr) {
        expectEquals(static_cast<int>(handler->getRole()),
                     static_cast<int>(juce::AccessibilityRole::toggleButton),
                     "Role must be 'toggleButton'");
      }
    }
  }

  void testActions() {
      beginTest("Button Actions");
      {
          zenith::ZenithButton button("Action");
          button.setSize(100, 30);

          auto* handler = button.getAccessibilityHandler();
          if (handler != nullptr) {
              auto actions = handler->getActions();
              bool hasPress = false;
              for (auto& action : actions) {
                  if (action.type == juce::AccessibilityActionType::press)
                      hasPress = true;
              }
              expect(hasPress, "Button must have 'press' action");
          }
      }
  }

  void testMouseCursor() {
    beginTest("Mouse Cursor");
    {
      zenith::ZenithButton button;
      // Component::getMouseCursor() returns the cursor set for this component.
      // StandardCursorType PointingHandCursor

      expect(button.getMouseCursor() == juce::MouseCursor::PointingHandCursor,
             "Mouse cursor should be PointingHandCursor");
    }
  }
};

static ZenithButtonAccessibilityTest zenithButtonAccessibilityTest;
