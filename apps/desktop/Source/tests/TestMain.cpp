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
  juce::String categoryFilter;
  juce::File jsonOutputFile;

  // Parse arguments
  for (int i = 1; i < argc; ++i) {
      juce::String arg(argv[i]);
      if (arg.startsWith("--gtest_output=json:")) {
          jsonOutputFile = juce::File::getCurrentWorkingDirectory().getChildFile(arg.substring(20));
      } else if (!arg.startsWith("-")) {
          categoryFilter = arg;
      }
  }

  // List all available tests first
  std::cout << "Available Unit Tests:" << std::endl;
  auto& allTests = juce::UnitTest::getAllTests();
  for (auto* t : allTests) {
      std::cout << " - [" << t->getCategory().toStdString() << "] " << t->getName().toStdString() << std::endl;
  }
  std::cout << "-----------------------------------" << std::endl;

  if (categoryFilter.isNotEmpty()) {
      std::cout << "Running tests in category: " << categoryFilter << std::endl;
      runner.runTestsInCategory(categoryFilter);
  } else {
      runner.runAllTests();
  }

  // Generate JSON Report if requested
  if (jsonOutputFile != juce::File()) {
      juce::var root;
      juce::var testsuites; // Array of suites

      // Group results by category (treating category as testsuite)
      std::map<juce::String, juce::Array<const juce::UnitTestRunner::TestResult*>> suites;
      for (int i = 0; i < runner.getNumResults(); ++i) {
          const auto* result = runner.getResult(i);
          suites[result->unitTestName].add(result); // Usually category is not in result, check documentation.
          // Actually runner.getResult(i) returns TestResult.
          // TestResult has: unitTestName, subcategoryName, passes, failures, messages.
          // We'll treat unitTestName as the suite.
      }

      for (int i = 0; i < runner.getNumResults(); ++i) {
          const auto* result = runner.getResult(i);
          
          juce::var suiteObj(new juce::DynamicObject());
          suiteObj.getDynamicObject()->setProperty("name", result->unitTestName);
          suiteObj.getDynamicObject()->setProperty("tests", 1);
          suiteObj.getDynamicObject()->setProperty("failures", result->failures);
          suiteObj.getDynamicObject()->setProperty("errors", 0);
          suiteObj.getDynamicObject()->setProperty("time", "0"); // Time not tracked per test in standard runner easily without custom printer

          juce::var testcases;
          
          juce::var testcase(new juce::DynamicObject());
          testcase.getDynamicObject()->setProperty("classname", result->unitTestName);
          testcase.getDynamicObject()->setProperty("name", result->subcategoryName.isNotEmpty() ? result->subcategoryName : "Main");
          testcase.getDynamicObject()->setProperty("time", "0");

          if (result->failures > 0) {
              juce::var failures;
              for (const auto& msg : result->messages) {
                  // Heuristic: messages usually contain failure info
                  juce::var failure(new juce::DynamicObject());
                  failure.getDynamicObject()->setProperty("message", msg);
                  failures.append(failure);
              }
              testcase.getDynamicObject()->setProperty("failures", failures);
          }

          testcases.append(testcase);
          suiteObj.getDynamicObject()->setProperty("testsuite", testcases); // Agent expects "testsuite" array inside
          
          testsuites.append(suiteObj);
      }

      // Root object structure: { "testsuites": [ ... ] }
      // But Agent expects GTest JSON format:
      // { "testsuites": [ { "name": "...", "testsuite": [ ... ] } ] }
      // Actually GTest format is slightly different but Agent parses:
      // data.get("testsuites", []) -> suite -> suite.get("testsuite", [])
      
      juce::DynamicObject* rootObj = new juce::DynamicObject();
      rootObj->setProperty("testsuites", testsuites);
      
      juce::String jsonString = juce::JSON::toString(juce::var(rootObj));
      jsonOutputFile.replaceWithText(jsonString);
      std::cout << "Wrote JSON report to: " << jsonOutputFile.getFullPathName() << std::endl;
  }

  // C5: Flush garbage collector to avoid false positive leaks from deferred deletion
  zenith::RealTimeGarbageCollector::getInstance().ensureClean();
  zenith::RealTimeGarbageCollector::deleteInstance();
  
  // C6: Shutdown ONNX environment
  zenith::ONNXStemSeparator::shutdown();

  // Print final status
  std::cout << "TestMain: Shutdown complete." << std::endl;


  return 0;
}
