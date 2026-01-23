/*
  ==============================================================================
    agents/ObservabilityAgent/tests/RunTests.cpp
    Test runner for ObservabilityAgent
  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <iostream>

namespace juce {
    extern const char* juce_compilationDate = __DATE__;
    extern const char* juce_compilationTime = __TIME__;
}

int main(int argc, char* argv[]) {
  // Initialize MessageManager for Timer support
  auto* mm = juce::MessageManager::getInstance();

  juce::UnitTestRunner runner;
  runner.runAllTests();

  juce::MessageManager::deleteInstance();
  return 0;
}