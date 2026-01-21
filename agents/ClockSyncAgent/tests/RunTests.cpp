/*
  ==============================================================================
    agents/ClockSyncAgent/tests/RunTests.cpp
    Simple test runner for ClockSyncAgent
  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include <iostream>

int main(int argc, char* argv[]) {
  // juce::ScopedJuceInitialiser_GUI initialiser; // Not needed for console app without GUI module

  juce::UnitTestRunner runner;
  runner.runAllTests();

  return 0;
}
