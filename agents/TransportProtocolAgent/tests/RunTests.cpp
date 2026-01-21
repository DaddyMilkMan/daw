/*
  ==============================================================================
    agents/TransportProtocolAgent/tests/RunTests.cpp
    Simple test runner for TransportProtocolAgent
  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include <iostream>

int main(int argc, char* argv[]) {
  juce::UnitTestRunner runner;
  runner.runAllTests();

  return 0;
}
