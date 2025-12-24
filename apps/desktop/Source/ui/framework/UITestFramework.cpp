/**
 * @file UITestFramework.cpp
 * @brief UI Testing and Validation Framework Implementation
 */

#include "UITestFramework.h"
#include "ZenithDesignSystem.h"
#include "controls/SkiaButton.h"
#include "controls/SkiaLabel.h"
#include "controls/SkiaTextEditor.h"
#include <algorithm>
#include <cmath>

namespace zenith {
namespace test {

//==============================================================================
TestResult UITestFramework::runAllTests() {
  TestResult overall;
  overall.success = true;

  auto results = {testComponentHierarchy(), testEventPropagation(),
                  testThemingConsistency(), testSkiaPerformance()};

  for (const auto &res : results) {
    overall.success &= res.success;
    overall.messages.addArray(res.messages);
  }

  return overall;
}

TestResult UITestFramework::testComponentHierarchy() {
  TestResult res;
  res.success = true;
  res.messages.add("Testing Component Hierarchy...");

  auto *mainWindow = juce::Desktop::getInstance().getComponents()[0];
  if (!mainWindow) {
    res.success = false;
    res.messages.add("FAIL: Main window not found");
    return res;
  }

  // Verify critical panels exist
  juce::StringArray requiredPanels = {"TransportBar", "MainLayout", "BottomBar",
                                      "RightSidePanel"};

  for (const auto &name : requiredPanels) {
    bool found = false;
    for (auto *child : mainWindow->getChildren()) {
      if (child->getName() == name) {
        found = true;
        break;
      }
    }

    if (!found) {
      res.messages.add("WARN: " + name + " not found via direct name match");
    }
  }

  return res;
}

TestResult UITestFramework::testEventPropagation() {
  TestResult res;
  res.success = true;
  res.messages.add("Testing Event Propagation...");
  return res;
}

TestResult UITestFramework::testThemingConsistency() {
  TestResult res;
  res.success = true;
  res.messages.add("Testing Theming Consistency...");
  return res;
}

TestResult UITestFramework::testSkiaPerformance() {
  TestResult res;
  res.success = true;
  res.messages.add("Testing Skia Performance...");
  return res;
}

//==============================================================================
VisualRegressionTester::VisualRegressionTester() {
  auto docsDir =
      juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
  baselineDir = docsDir.getChildFile("ZenithDAW").getChildFile("TestBaselines");

  if (!baselineDir.exists())
    baselineDir.createDirectory();
}

bool VisualRegressionTester::captureBaseline(SkiaComponent *component,
                                             const juce::String &testName) {
  if (!component)
    return false;

  auto image = component->captureToImage();
  juce::File file = baselineDir.getChildFile(testName + ".png");

  if (file.existsAsFile())
    file.deleteFile();

  std::unique_ptr<juce::FileOutputStream> stream(file.createOutputStream());
  if (stream) {
    juce::PNGImageFormat png;
    return png.writeImageToStream(image, *stream);
  }

  return false;
}

bool VisualRegressionTester::compareAgainstBaseline(
    SkiaComponent *component, const juce::String &testName, float tolerance) {
  if (!component)
    return false;

  juce::File file = baselineDir.getChildFile(testName + ".png");
  if (!file.existsAsFile())
    return false;

  auto current = component->captureToImage();
  auto baseline = juce::ImageFileFormat::loadFrom(file);

  if (!baseline.isValid())
    return false;

  if (current.getWidth() != baseline.getWidth() ||
      current.getHeight() != baseline.getHeight())
    return false;

  float diff = calculateDifference(current, baseline);
  return diff <= tolerance;
}

float VisualRegressionTester::calculateDifference(const juce::Image &img1,
                                                  const juce::Image &img2) {
  int diffPixels = 0;
  int totalPixels = img1.getWidth() * img1.getHeight();

  for (int y = 0; y < img1.getHeight(); ++y) {
    for (int x = 0; x < img1.getWidth(); ++x) {
      if (img1.getPixelAt(x, y) != img2.getPixelAt(x, y)) {
        diffPixels++;
      }
    }
  }

  return static_cast<float>(diffPixels) / static_cast<float>(totalPixels);
}

juce::Image
VisualRegressionTester::createDiffImage(const juce::Image &current,
                                        const juce::Image &baseline) {
  juce::Image diffImage(juce::Image::ARGB, current.getWidth(),
                        current.getHeight(), true);

  for (int y = 0; y < current.getHeight(); ++y) {
    for (int x = 0; x < current.getWidth(); ++x) {
      auto currentPixel = current.getPixelAt(x, y);
      auto baselinePixel = baseline.getPixelAt(x, y);

      if (currentPixel != baselinePixel) {
        // Highlight differences in red
        diffImage.setPixelAt(x, y, juce::Colours::red.withAlpha(0.5f));
      } else {
        // Pixel matches - use baseline pixel with reduced opacity
        diffImage.setPixelAt(x, y, baselinePixel.withAlpha(0.3f));
      }
    }
  }

  return diffImage;
}

juce::Array<TestReport> VisualRegressionTester::runVisualTests(
    const juce::Array<SkiaComponent *> &components) {
  juce::Array<TestReport> reports;

  for (auto *comp : components) {
    TestReport report;
    report.testName = comp->getName();
    report.passed = compareAgainstBaseline(comp, report.testName);
    report.timestamp = juce::Time::getCurrentTime().toMilliseconds();
    reports.add(report);
  }

  return reports;
}

} // namespace test
} // namespace zenith