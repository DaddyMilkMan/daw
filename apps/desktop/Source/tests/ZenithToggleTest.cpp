/*
  ==============================================================================

    ZenithToggleTest.cpp
    Created: 2025-12-10
    Author:  Zenith DAW

    Unit tests for ZenithToggle control.

  ==============================================================================
*/

#include "../../../../modules/zenith_ui/ui/controls/ZenithToggle.h"
#include <gtest/gtest.h>
#include <juce_gui_basics/juce_gui_basics.h>

class ZenithToggleTest : public ::testing::Test {
protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(ZenithToggleTest, InitialStateIsFalse) {
  zenith::ZenithToggle toggle("TestToggle");
  EXPECT_FALSE(toggle.getToggleState());
}

TEST_F(ZenithToggleTest, KeyPressSpaceTogglesState) {
  zenith::ZenithToggle toggle("TestToggle");
  toggle.setToggleState(false, false);

  // Simulate space key press
  juce::KeyPress spaceKey(juce::KeyPress::spaceKey);
  toggle.keyPressed(spaceKey);

  EXPECT_TRUE(toggle.getToggleState());

  // Toggle back
  toggle.keyPressed(spaceKey);
  EXPECT_FALSE(toggle.getToggleState());
}

TEST_F(ZenithToggleTest, KeyPressReturnTogglesState) {
  zenith::ZenithToggle toggle("TestToggle");
  toggle.setToggleState(false, false);

  // Simulate return key press
  juce::KeyPress returnKey(juce::KeyPress::returnKey);
  toggle.keyPressed(returnKey);

  EXPECT_TRUE(toggle.getToggleState());
}

TEST_F(ZenithToggleTest, ClickCallbackIsCalledOnKey) {
  zenith::ZenithToggle toggle("TestToggle");
  bool clicked = false;
  toggle.onClick = [&]() { clicked = true; };

  juce::KeyPress spaceKey(juce::KeyPress::spaceKey);
  toggle.keyPressed(spaceKey);

  EXPECT_TRUE(clicked);
}
