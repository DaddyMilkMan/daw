#include <juce_gui_basics/juce_gui_basics.h>
#include <gtest/gtest.h>
#include "../ui/controls/ZenithButton.h"

class ZenithButtonTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(ZenithButtonTest, InitialStateIsVisible) {
    zenith::ZenithButton button("TestButton");
    EXPECT_TRUE(button.isVisible());
}

TEST_F(ZenithButtonTest, TriggerClickCallsOnClick) {
    zenith::ZenithButton button("TestButton");
    bool clicked = false;
    button.onClick = [&]() { clicked = true; };
    button.setEnabled(true);

    button.triggerClick();

    EXPECT_TRUE(clicked);
}

TEST_F(ZenithButtonTest, SpaceKeyTriggersClick) {
    zenith::ZenithButton button("TestButton");
    bool clicked = false;
    button.onClick = [&]() { clicked = true; };
    button.setEnabled(true);

    button.keyPressed(juce::KeyPress(juce::KeyPress::spaceKey));

    EXPECT_TRUE(clicked);
}

TEST_F(ZenithButtonTest, ReturnKeyTriggersClick) {
    zenith::ZenithButton button("TestButton");
    bool clicked = false;
    button.onClick = [&]() { clicked = true; };
    button.setEnabled(true);

    button.keyPressed(juce::KeyPress(juce::KeyPress::returnKey));

    EXPECT_TRUE(clicked);
}

TEST_F(ZenithButtonTest, AccessibilityHandlerCreated) {
    zenith::ZenithButton button("TestButton");
    auto handler = button.createAccessibilityHandler();
    ASSERT_NE(handler, nullptr);
    EXPECT_EQ(handler->getTitle(), "TestButton");
    EXPECT_EQ(handler->getRole(), juce::AccessibilityRole::button);
}

TEST_F(ZenithButtonTest, AccessibilityHandlerToggleRole) {
    zenith::ZenithButton button("ToggleButton");
    button.setToggleable(true);
    auto handler = button.createAccessibilityHandler();
    ASSERT_NE(handler, nullptr);
    EXPECT_EQ(handler->getRole(), juce::AccessibilityRole::toggleButton);
}
