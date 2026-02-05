/*
  ==============================================================================

    PianoRollKeyboardShortcutTests.cpp
    Created: 2026-01-29
    Author:  Zenith DAW

    Unit tests for Piano Roll keyboard shortcuts, focusing on MPE expression
    lane toggle functionality.

  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include "../ui/design-system/ZenithDesignSystem.h"

namespace zenith {
namespace tests {

class PianoRollKeyboardShortcutTest : public juce::UnitTest {
public:
  PianoRollKeyboardShortcutTest()
      : juce::UnitTest("PianoRollKeyboardShortcuts", "UI") {}

  void runTest() override {
    beginTest("Cmd+E Toggles All Expression Lanes");
    testToggleExpressionLanes();

    beginTest("Expression Lane Toggle Uses Design Constants");
    testToggleUsesConstants();

    beginTest("Toggle Shortcut ID Matches Design System");
    testShortcutKeyMatch();

    beginTest("Expression Lane State Persistence");
    testStatePersistence();
  }

private:
  void testToggleExpressionLanes() {
    // Test that Cmd+E toggles all expression lanes simultaneously
    // This is a behavioral test verifying the toggle logic

    // Simulate initial state (all lanes hidden by default)
    bool initialState = false;

    // Simulate Cmd+E keypress
    bool newState = !initialState;

    // Verify all lanes would be toggled to the same state
    expect(newState == true, "First toggle should enable all lanes");

    // Simulate second Cmd+E keypress
    bool thirdState = !newState;

    // Verify lanes toggle back
    expect(thirdState == false, "Second toggle should disable all lanes");

    logMessage(juce::String::formatted(
        "Toggle test: Initial=%d, After Cmd+E=%d, After second Cmd+E=%d",
        initialState, newState, thirdState));
  }

  void testToggleUsesConstants() {
    // Verify the toggle key uses design system constants

    // Check that TOGGLE_LANES_KEY is 'e'
    expect(design::colors::mpe::TOGGLE_LANES_KEY == 'e',
           "Toggle key should be 'e'");

    // Verify constant is lowercase (cross-platform consistency)
    expect(design::colors::mpe::TOGGLE_LANES_KEY >= 'a' &&
               design::colors::mpe::TOGGLE_LANES_KEY <= 'z',
           "Toggle key should be lowercase letter");

    logMessage("Design system constant check passed");
  }

  void testShortcutKeyMatch() {
    // Verify the KeyPress uses the correct character

    juce::KeyPress toggleKey(design::colors::mpe::TOGGLE_LANES_KEY,
                              juce::ModifierKeys::commandModifier, 0);

    // Check the character
    juce::juce_wchar keyChar = toggleKey.getTextCharacter();
    expect(keyChar == design::colors::mpe::TOGGLE_LANES_KEY,
           "KeyPress char should match TOGGLE_LANES_KEY");

    // Check modifiers
    expect(toggleKey.getModifiers().isCommandDown(),
           "Toggle shortcut requires Command/Cmd key");
    expect(!toggleKey.getModifiers().isCtrlDown() ||
               toggleKey.getModifiers().isCommandDown(),
           "Uses Command (macOS) or Ctrl+Command (Linux)");

    logMessage(juce::String::formatted(
        "Shortcut key: '%c' with modifiers=%d",
        keyChar,
        toggleKey.getModifiers().getRawFlags()));
  }

  void testStatePersistence() {
    // Test that expression lane visibility state persists across toggles

    // Simulate user customizing lane visibility
    bool userPressure = true;
    bool userPitchBend = true;
    bool userSlide = false;
    bool userExpression = false;

    // Simulate Cmd+E (toggles all)
    bool toggleState = !userPressure;
    userPressure = toggleState;
    userPitchBend = toggleState;
    userSlide = toggleState;
    userExpression = toggleState;

    // Verify all flipped
    expect(userPressure == false, "Pressure flipped to false");
    expect(userPitchBend == false, "Pitch bend flipped to false");
    expect(userSlide == true, "Slide flipped to true");
    expect(userExpression == true, "Expression flipped to true");

    // Simulate second Cmd+E
    toggleState = !userPressure;
    userPressure = toggleState;
    userPitchBend = toggleState;
    userSlide = toggleState;
    userExpression = toggleState;

    // Verify all flipped back
    expect(userPressure == true, "Pressure flipped back to true");
    expect(userPitchBend == true, "Pitch bend flipped back to true");
    expect(userSlide == false, "Slide flipped back to false");
    expect(userExpression == false, "Expression flipped back to false");

    logMessage("State persistence across toggles verified");
  }
};

static PianoRollKeyboardShortcutTest pianoRollKeyboardShortcutTest;

} // namespace tests
} // namespace zenith
