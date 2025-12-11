/*
  ==============================================================================

    TestMain.cpp
    Created: 2025-12-10
    Author:  Zenith DAW

    Entry point for the console-based unit test runner.

  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>

int main(int argc, char *argv[]) {
  juce::ignoreUnused(argc, argv);

  // Initialise valid JUCE message manager for tests that might rely on it (e.g.
  // timers/events)
  juce::ScopedJuceInitialiser_GUI initialiser;

  juce::UnitTestRunner runner;
  runner.runAllTests();

  return 0;
}
