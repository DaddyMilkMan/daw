/*
  ==============================================================================
    ObservabilityAgentTestMain.cpp
    Lightweight console-only test runner for ObservabilityAgent
  ==============================================================================
*/

#include <juce_core/juce_core.h>

// Define missing symbols usually provided by JuceHeader
namespace juce {
    extern const char* const juce_compilationDate = "2024-01-01";
    extern const char* const juce_compilationTime = "12:00:00";
}

int main(int argc, char* argv[]) {
    juce::UnitTestRunner runner;
    runner.runAllTests();
    return 0;
}
