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
#include <iostream>

#include "../ui/controls/SkiaAlertWindow.h"
#include "../engine/RealTimeGarbageCollector.h"
#include "../dsp/ONNXStemSeparator.h"

extern "C" int main(int argc, char *argv[]) {
  juce::ScopedJuceInitialiser_GUI initialiser;
  juce::MessageManager::getInstance(); // Ensure instance is created
  zenith::SkiaAlertWindow::setTestMode(true);

  juce::UnitTestRunner runner;
  
  // List all available tests first
  std::cout << "Available Unit Tests:" << std::endl;
  auto& allTests = juce::UnitTest::getAllTests();
  for (auto* t : allTests) {
      std::cout << " - [" << t->getCategory().toStdString() << "] " << t->getName().toStdString() << std::endl;
  }
  std::cout << "-----------------------------------" << std::endl;

  if (argc > 1) {
      juce::String filter (argv[1]);
      std::cout << "Running tests in category: " << filter << std::endl;
      runner.runTestsInCategory(filter);
  } else {
      runner.runAllTests();
  }

  // C5: Flush garbage collector to avoid false positive leaks from deferred deletion
  // Note: With Meyer's singleton, the instance is automatically destroyed at program exit
  zenith::RealTimeGarbageCollector::getInstance().ensureClean();
  
  // C6: Shutdown ONNX environment
  zenith::ONNXStemSeparator::shutdown();

  // Print final status
  std::cout << "TestMain: Shutdown complete." << std::endl;


  return 0;
}
