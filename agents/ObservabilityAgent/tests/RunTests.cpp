/*
  ==============================================================================
    agents/ObservabilityAgent/tests/RunTests.cpp
    Simple test runner for ObservabilityAgent
  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include <iostream>

namespace juce {
    extern const char* const juce_compilationDate = __DATE__;
    extern const char* const juce_compilationTime = __TIME__;
}

int main(int argc, char* argv[]) {
  juce::UnitTestRunner runner;
  runner.runAllTests();
  return 0;
}
