/*
  ==============================================================================

    TestMain.cpp
    Simple runner for JUCE Unit Tests

  ==============================================================================
*/

#include <juce_core/juce_core.h>

int main() {
  juce::UnitTestRunner runner;
  runner.runAllTests();
  return 0;
}
