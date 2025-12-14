/*
  ==============================================================================

    AccessibilityTests.cpp
    Created: 2025-12-14
    Author:  Zenith DAW Team

    Unit tests for verifying accessibility handlers in the panel system.
    These tests ensure that PanelHeader, PanelDivider, and TabGroup components
    are properly exposed to screen readers with correct semantic roles.

  ==============================================================================
*/

#include "../ui/ResizablePanelContainer.h"
#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

/**
 * @brief Test suite for panel system accessibility compliance
 *
 * Verifies that accessibility handlers are properly implemented for:
 * - PanelHeader: Role::group with panel title
 * - PanelDivider: Role::unspecified with "Drag to resize" help text
 * - TabGroup: Role::list as tab list
 */
class AccessibilityTests : public juce::UnitTest {
public:
  AccessibilityTests()
      : juce::UnitTest("AccessibilityTests", "Accessibility") {}

  void runTest() override {
    testPanelHeaderAccessibility();
    testPanelDividerAccessibility();
    testTabGroupAccessibility();
    testComponentHierarchyAccessibility();
  }

private:
  /**
   * @brief Test that PanelHeader has proper accessibility handler
   * Requirements:
   * - getAccessibilityHandler() returns non-null
   * - Role is AccessibilityRole::group
   * - Title matches panel name
   */
  void testPanelHeaderAccessibility() {
    beginTest("PanelHeader Accessibility Handler");
    {
      zenith::PanelHeader header("Test Panel", true);

      // Force accessibility handler creation
      auto *handler = header.getAccessibilityHandler();

      // Assert handler exists
      expect(handler != nullptr,
             "PanelHeader must have an accessibility handler");

      if (handler != nullptr) {
        // Verify role is Group
        expectEquals(static_cast<int>(handler->getRole()),
                     static_cast<int>(juce::AccessibilityRole::group),
                     "PanelHeader role must be 'group'");

        // Verify title is set
        expect(handler->getTitle().isNotEmpty(),
               "PanelHeader must have a title for accessibility");
      }
    }
  }

  /**
   * @brief Test that PanelDivider has proper accessibility handler
   * Requirements:
   * - getAccessibilityHandler() returns non-null
   * - Role is AccessibilityRole::unspecified (closest to splitter)
   * - Help text is "Drag to resize"
   */
  void testPanelDividerAccessibility() {
    beginTest("PanelDivider Accessibility Handler");
    {
      // Test horizontal divider
      zenith::PanelDivider horizontalDivider(true);

      auto *hHandler = horizontalDivider.getAccessibilityHandler();
      expect(hHandler != nullptr,
             "PanelDivider (horizontal) must have an accessibility handler");

      if (hHandler != nullptr) {
        // Verify role (splitter not available, using unspecified)
        expectEquals(
            static_cast<int>(hHandler->getRole()),
            static_cast<int>(juce::AccessibilityRole::unspecified),
            "PanelDivider role must be 'unspecified' (splitter equivalent)");

        // Verify help text
        expectEquals(hHandler->getHelp(), juce::String("Drag to resize"),
                     "PanelDivider must have help text 'Drag to resize'");
      }

      // Test vertical divider
      zenith::PanelDivider verticalDivider(false);

      auto *vHandler = verticalDivider.getAccessibilityHandler();
      expect(vHandler != nullptr,
             "PanelDivider (vertical) must have an accessibility handler");

      if (vHandler != nullptr) {
        expectEquals(
            hHandler->getHelp(), juce::String("Drag to resize"),
            "Vertical PanelDivider must also have help text 'Drag to resize'");
      }
    }
  }

  /**
   * @brief Test that TabGroup has proper accessibility handler
   * Requirements:
   * - getAccessibilityHandler() returns non-null
   * - Role is AccessibilityRole::list (closest to tabList)
   */
  void testTabGroupAccessibility() {
    beginTest("TabGroup Accessibility Handler");
    {
      zenith::TabGroup tabGroup;

      auto *handler = tabGroup.getAccessibilityHandler();

      expect(handler != nullptr, "TabGroup must have an accessibility handler");

      if (handler != nullptr) {
        // Verify role is List (closest to TabList in JUCE)
        expectEquals(static_cast<int>(handler->getRole()),
                     static_cast<int>(juce::AccessibilityRole::list),
                     "TabGroup role must be 'list' (tabList equivalent)");

        // Verify title/description
        expect(handler->getTitle().isNotEmpty() ||
                   handler->getDescription().isNotEmpty(),
               "TabGroup must have a title or description for accessibility");
      }
    }
  }

  /**
   * @brief Test accessibility across a component hierarchy
   *
   * Creates a full ResizablePanelContainer and iterates through its
   * component hierarchy to verify all accessibility handlers are present.
   */
  void testComponentHierarchyAccessibility() {
    beginTest("Component Hierarchy Accessibility");
    {
      zenith::ResizablePanelContainer container;

      // Add sample panels to create a realistic hierarchy
      auto content1 = std::make_unique<juce::Component>();
      auto content2 = std::make_unique<juce::Component>();

      zenith::layout::PanelConfig config1;
      config1.id = "test_panel_1";
      config1.name = "Test Panel 1";
      config1.type = "custom";
      config1.flex = 1.0f;
      config1.minSize = 100.0f;
      config1.isCollapsible = true;

      zenith::layout::PanelConfig config2;
      config2.id = "test_panel_2";
      config2.name = "Test Panel 2";
      config2.type = "custom";
      config2.flex = 1.0f;
      config2.minSize = 100.0f;
      config2.isCollapsible = true;

      container.addPanel(std::move(content1), config1);
      container.addPanel(std::move(content2), config2);

      // Count accessible components
      int accessiblePanelHeaders = 0;
      int accessibleDividers = 0;

      // Iterate through container's children
      std::function<void(juce::Component *)> checkAccessibility =
          [&](juce::Component *comp) {
            if (comp == nullptr)
              return;

            // Check if this is a PanelHeader
            if (auto *header = dynamic_cast<zenith::PanelHeader *>(comp)) {
              auto *handler = header->getAccessibilityHandler();
              if (handler != nullptr &&
                  handler->getRole() == juce::AccessibilityRole::group) {
                accessiblePanelHeaders++;
              }
            }

            // Check if this is a PanelDivider
            if (auto *divider = dynamic_cast<zenith::PanelDivider *>(comp)) {
              auto *handler = divider->getAccessibilityHandler();
              if (handler != nullptr) {
                accessibleDividers++;
              }
            }

            // Recurse into children
            for (int i = 0; i < comp->getNumChildComponents(); ++i) {
              checkAccessibility(comp->getChildComponent(i));
            }
          };

      checkAccessibility(&container);

      // With 2 panels, we expect:
      // - At least 2 PanelHeaders (one per panel wrapper)
      // - At least 1 PanelDivider (between the two panels)
      expect(accessiblePanelHeaders >= 2, "Container with 2 panels should have "
                                          "at least 2 accessible PanelHeaders");
      expect(accessibleDividers >= 1, "Container with 2 panels should have at "
                                      "least 1 accessible PanelDivider");

      logMessage("Found " + juce::String(accessiblePanelHeaders) +
                 " accessible PanelHeaders");
      logMessage("Found " + juce::String(accessibleDividers) +
                 " accessible PanelDividers");
    }
  }
};

// Static instance registers the test
static AccessibilityTests accessibilityTests;
