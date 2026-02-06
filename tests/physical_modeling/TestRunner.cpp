#include "PhysicalModelingTest.h"
#include "../../../JuceLibraryCode/JuceHeader.h"

using namespace Zenith;

int main(int argc, char* argv[])
{
    // Initialize JUCE
    juce::initialiseJuce_GUI();
    
    // Create test runner
    PhysicalModelingTest testRunner;
    
    DBG("ZenithUltraSynth Physical Modeling Test Runner");
    DBG("=============================================");
    
    // Run all tests
    bool allPassed = testRunner.runAllTests();
    
    // Print results
    testRunner.printTestResults();
    
    // Generate test report
    testRunner.generateTestReport();
    
    // Export test audio
    testRunner.exportTestAudio();
    
    // Shutdown JUCE
    juce::shutdownJuce_GUI();
    
    // Return exit code
    return allPassed ? 0 : 1;
}
