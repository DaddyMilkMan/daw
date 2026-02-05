/*
  ==============================================================================
    RunTests.cpp
    Test runner for ScheduledTransportAgent
  ==============================================================================
*/

#include <juce_core/juce_core.h>

namespace juce {
    extern const char* const juce_compilationDate = __DATE__;
    extern const char* const juce_compilationTime = __TIME__;
}

int main(int argc, char* argv[]) {
  juce::UnitTestRunner runner;
  runner.runAllTests();

  return 0;
}
