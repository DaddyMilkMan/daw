/*
  ==============================================================================
    TestRunner.cpp
    Simple runner for JUCE unit tests
  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <juce_gui_basics/juce_gui_basics.h>

int main(int argc, char* argv[]) {
    juce::ScopedJuceInitialiser_GUI guiInit;
    juce::UnitTestRunner runner;
    runner.runAllTests();
    return 0;
}
