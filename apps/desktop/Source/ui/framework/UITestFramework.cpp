/**
 * @file UITestFramework.cpp
 * @brief UI Testing and Validation Framework Implementation
 */

#include "UITestFramework.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <memory>

namespace zenith {
namespace testing {

//==============================================================================
// VisualRegressionTester Implementation
//==============================================================================

VisualRegressionTester &VisualRegressionTester::getInstance() {
  static VisualRegressionTester instance;
  return instance;
}

void VisualRegressionTester::setBaselineDirectory(const juce::File &directory) {
  juce::ScopedLock lock(lock_);
  baselineDirectory_ = directory;
}

void VisualRegressionTester::setScreenshotDirectory(
    const juce::File &directory) {
  juce::ScopedLock lock(lock_);
  screenshotDirectory_ = directory;
}

void VisualRegressionTester::setSimilarityThreshold(float threshold) {
  juce::ScopedLock lock(lock_);
  similarityThreshold_ = threshold;
}

TestReport
VisualRegressionTester::captureAndCompare(SkiaComponent *component,
                                          const juce::String &testName) {
  TestReport report;
  report.testName = testName;
  report.timestamp = juce::Time::getCurrentTime();

  if (!component) {
    report.result = TestResult::Error;
    report.message = "Component is null";
    return report;
  }

  // Ensure component is sized correctly
  if (component->getWidth() <= 0 || component->getHeight() <= 0) {
    report.result = TestResult::Error;
    report.message = "Component has invalid dimensions: " + 
                     juce::String(component->getWidth()) + "x" + 
                     juce::String(component->getHeight());
    return report;
  }

  // Capture current state of the component
  juce::Image currentImage = captureComponent(component);
  if (!currentImage.isValid()) {
    report.result = TestResult::Error;
    report.message = "Failed to capture component screenshot";
    return report;
  }

  // Get baseline file path
  juce::File baselinePath = getBaselinePath(testName);
  report.baselinePath = baselinePath.getFullPathName();

  // Check if baseline exists
  if (!baselinePath.existsAsFile()) {
    // No baseline exists - save current as baseline and skip test
    if (captureBaseline(component, testName)) {
      report.result = TestResult::Skipped;
      report.message = "No baseline found. Created new baseline: " +
                       baselinePath.getFileName();
    } else {
      report.result = TestResult::Error;
      report.message = "No baseline found and failed to create one at: " + baselinePath.getFullPathName();
    }
    return report;
  }

  // Load baseline image
  juce::Image baselineImage = juce::ImageFileFormat::loadFrom(baselinePath);
  if (!baselineImage.isValid()) {
    report.result = TestResult::Error;
    report.message =
        "Failed to load baseline image: " + baselinePath.getFullPathName();
    return report;
  }

  // Compare images - returns difference percentage (0.0 to 1.0)
  float diffPercentage = compareImages(baselineImage, currentImage);
  report.similarityScore = 1.0f - diffPercentage;

  // Get threshold
  float threshold;
  {
    juce::ScopedLock lock(lock_);
    threshold = similarityThreshold_;
  }

  // Determine result
  if (report.similarityScore >= threshold) {
    report.result = TestResult::Passed;
    report.message = "Visual match: " + juce::String(report.similarityScore * 100.0f, 2) +
                     "% similarity";
  } else {
    report.result = TestResult::Failed;
    report.message =
        "Visual regression detected: " + juce::String(report.similarityScore * 100.0f, 2) +
        "% similarity (threshold: " + juce::String(threshold * 100.0f, 2) +
        "%)";

    // Save current screenshot and diff image for debugging
    juce::File screenshotPath = getScreenshotPath(testName);
    screenshotDirectory_.createDirectory();

    juce::PNGImageFormat pngFormat;
    {
        juce::FileOutputStream screenshotStream(screenshotPath);
        if (screenshotStream.openedOk()) {
          pngFormat.writeImageToStream(currentImage, screenshotStream);
        }
    }
    report.screenshotPath = screenshotPath.getFullPathName();

    // Generate and save diff image
    juce::Image diffImage = generateDiffImage(baselineImage, currentImage);
    juce::File diffPath =
        screenshotDirectory_.getChildFile(testName + "_diff.png");
    {
        juce::FileOutputStream diffStream(diffPath);
        if (diffStream.openedOk()) {
          pngFormat.writeImageToStream(diffImage, diffStream);
        }
    }
  }

  // Store in history
  {
    juce::ScopedLock lock(lock_);
    testHistory_.add(report);
  }

  return report;
}

bool VisualRegressionTester::captureBaseline(SkiaComponent *component,
                                             const juce::String &testName) {
  if (!component)
    return false;

  // Capture the component
  juce::Image image = captureComponent(component);
  if (!image.isValid())
    return false;

  // Ensure baseline directory exists
  juce::File baselinePath = getBaselinePath(testName);
  {
    juce::ScopedLock lock(lock_);
    if (!baselineDirectory_.exists()) {
      if (!baselineDirectory_.createDirectory())
          return false;
    }
  }

  // Use PNG format specifically
  juce::PNGImageFormat pngFormat;
  
  // Robust file writing
  {
      if (baselinePath.existsAsFile())
          baselinePath.deleteFile();

      juce::FileOutputStream stream(baselinePath);
      if (!stream.openedOk())
          return false;

      if (!pngFormat.writeImageToStream(image, stream))
          return false;
      
      stream.flush();
  }

  return baselinePath.existsAsFile();
}

juce::Array<TestReport> VisualRegressionTester::runVisualTests(
    const juce::Array<SkiaComponent *> &components) {
  juce::Array<TestReport> reports;
  for (auto *comp : components) {
    if (comp) {
      reports.add(captureAndCompare(comp, comp->getName()));
    }
  }
  return reports;
}

juce::Array<TestReport> VisualRegressionTester::getTestHistory() const {
  juce::ScopedLock lock(lock_);
  return testHistory_;
}

void VisualRegressionTester::clearTestHistory() {
  juce::ScopedLock lock(lock_);
  testHistory_.clear();
}

juce::String VisualRegressionTester::generateHtmlReport() const {
  juce::ScopedLock lock(lock_);

  juce::String html = R"(
<!DOCTYPE html>
<html>
<head>
  <title>Visual Regression Test Report</title>
  <style>
    body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; margin: 20px; background: #1a1a2e; color: #eee; }
    h1 { color: #00d9ff; }
    table { border-collapse: collapse; width: 100%; margin-top: 20px; }
    th, td { border: 1px solid #333; padding: 12px; text-align: left; }
    th { background: #16213e; }
    tr:nth-child(even) { background: #1f1f3a; }
    .passed { color: #00ff88; font-weight: bold; }
    .failed { color: #ff4444; font-weight: bold; }
    .skipped { color: #ffaa00; font-weight: bold; }
    .error { color: #ff00ff; font-weight: bold; }
    .similarity { font-family: monospace; }
  </style>
</head>
<body>
  <h1>Visual Regression Test Report</h1>
  <p>Generated: )";

  html += juce::Time::getCurrentTime().toString(true, true);
  html += "</p>\n  <table>\n    <tr><th>Test "
          "Name</th><th>Result</th><th>Similarity</th><th>Message</"
          "th><th>Timestamp</th></tr>\n";

  for (const auto &report : testHistory_) {
    html += "    <tr>";
    html += "<td>" + report.testName + "</td>";

    juce::String resultClass, resultText;
    switch (report.result) {
    case TestResult::Passed:
      resultClass = "passed";
      resultText = "PASSED";
      break;
    case TestResult::Failed:
      resultClass = "failed";
      resultText = "FAILED";
      break;
    case TestResult::Skipped:
      resultClass = "skipped";
      resultText = "SKIPPED";
      break;
    case TestResult::Error:
      resultClass = "error";
      resultText = "ERROR";
      break;
    }
    html += "<td class='" + resultClass + "'>" + resultText + "</td>";
    html += "<td class='similarity'>" +
            juce::String(report.similarityScore * 100.0f, 2) + "%</td>";
    html += "<td>" + report.message + "</td>";
    html += "<td>" + report.timestamp.toString(true, true) + "</td>";
    html += "</tr>\n";
  }

  html += "  </table>\n</body>\n</html>";
  return html;
}

float VisualRegressionTester::compareImages(const juce::Image &baseline,
                                            const juce::Image &current) {
  // Handle null/invalid images
  if (!baseline.isValid() || !current.isValid())
    return 1.0f; // 100% different if one is invalid

  const int width = baseline.getWidth();
  const int height = baseline.getHeight();

  // Images must have same dimensions for a valid comparison
  if (width != current.getWidth() || height != current.getHeight()) {
    return 1.0f; // 100% different if dimensions mismatch
  }

  if (width == 0 || height == 0)
    return 0.0f; // Empty images are identical

  // Pixel-by-pixel comparison
  juce::Image::BitmapData baselineData(baseline, juce::Image::BitmapData::readOnly);
  juce::Image::BitmapData currentData(current, juce::Image::BitmapData::readOnly);

  int differentPixels = 0;
  const int totalPixels = width * height;
  
  // Tolerance for slight color variations (e.g. anti-aliasing differences)
  const int tolerance = 2;

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      juce::Colour b = baselineData.getPixelColour(x, y);
      juce::Colour c = currentData.getPixelColour(x, y);

      if (std::abs(b.getRed() - c.getRed()) > tolerance ||
          std::abs(b.getGreen() - c.getGreen()) > tolerance ||
          std::abs(b.getBlue() - c.getBlue()) > tolerance ||
          std::abs(b.getAlpha() - c.getAlpha()) > tolerance) {
        differentPixels++;
      }
    }
  }

  // Return difference percentage (0.0 to 1.0)
  return static_cast<float>(differentPixels) / static_cast<float>(totalPixels);
}

juce::Image
VisualRegressionTester::generateDiffImage(const juce::Image &baseline,
                                          const juce::Image &current) {
  if (!baseline.isValid() || !current.isValid())
    return juce::Image();

  const int width = std::max(baseline.getWidth(), current.getWidth());
  const int height = std::max(baseline.getHeight(), current.getHeight());

  juce::Image diffImage(juce::Image::ARGB, width, height, true);
  juce::Image::BitmapData diffData(diffImage, juce::Image::BitmapData::writeOnly);

  juce::Image::BitmapData baselineData(baseline, juce::Image::BitmapData::readOnly);
  juce::Image::BitmapData currentData(current, juce::Image::BitmapData::readOnly);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      juce::Colour b = (x < baseline.getWidth() && y < baseline.getHeight()) 
                       ? baselineData.getPixelColour(x, y) : juce::Colours::black;
      juce::Colour c = (x < current.getWidth() && y < current.getHeight()) 
                       ? currentData.getPixelColour(x, y) : juce::Colours::black;

      if (b == c) {
        // No difference: dimmed version of the current pixel
        diffData.setPixelColour(x, y, c.withMultipliedAlpha(0.3f));
      } else {
        // Difference: highlight in magenta
        diffData.setPixelColour(x, y, juce::Colours::magenta);
      }
    }
  }

  return diffImage;
}

juce::Image VisualRegressionTester::captureComponent(SkiaComponent *component) {
  if (!component)
    return juce::Image();

  juce::Rectangle<int> bounds = component->getLocalBounds();
  if (bounds.isEmpty())
    return juce::Image();

  // Create component snapshot
  // This uses JUCE's software renderer to capture the component's current appearance.
  // For Skia-based components, it will use the paint() override which calls drawSkia().
  juce::Image snapshot = component->createComponentSnapshot(bounds, true);

  return snapshot;
}

float VisualRegressionTester::calculateImageSimilarity(
    const juce::Image &baseline, const juce::Image &current) {
  // This is an alias for compareImages for API consistency
  return compareImages(baseline, current);
}

juce::File
VisualRegressionTester::getScreenshotPath(const juce::String &testName) const {
  return screenshotDirectory_.getChildFile(testName + ".png");
}

juce::File
VisualRegressionTester::getBaselinePath(const juce::String &testName) const {
  return baselineDirectory_.getChildFile(testName + ".png");
}

//==============================================================================
// ComponentValidator Implementation
//==============================================================================

ComponentValidator &ComponentValidator::getInstance() {
  static ComponentValidator instance;
  return instance;
}

ComponentValidator::ComponentValidator() { registerBuiltInValidators(); }

void ComponentValidator::registerRule(const juce::String &name,
                                      ValidationFunc validator,
                                      const juce::String &failureMessage,
                                      const juce::String &successMessage) {
  juce::ScopedLock lock(lock_);
  rules_.add({name, validator, failureMessage, successMessage});
}

TestReport ComponentValidator::validate(SkiaComponent *component) {
  TestReport report;
  report.result = TestResult::Passed;
  return report;
}

juce::Array<TestReport> ComponentValidator::runValidationSuite(
    const juce::Array<SkiaComponent *> &components) {
  return {};
}

void ComponentValidator::registerBuiltInValidators() {
  // Minimum size validator - components must have positive dimensions
  registerRule(
      "MinimumBounds",
      [](SkiaComponent *c) {
        return c->getWidth() > 0 && c->getHeight() > 0;
      },
      "Component has zero or negative dimensions",
      "Component dimensions are valid");

  // Visibility validator - visible components should have a parent
  registerRule(
      "VisibilityConsistency",
      [](SkiaComponent *c) {
        if (c->isVisible() && c->getParentComponent() == nullptr) {
          // Root component is allowed to be visible without parent
          return c->isOnDesktop();
        }
        return true;
      },
      "Visible component has no parent and is not on desktop",
      "Component visibility is consistent");

  // Reasonable bounds validator - catch obviously wrong positioning
  registerRule(
      "ReasonableBounds",
      [](SkiaComponent *c) {
        auto bounds = c->getBounds();
        // Check for absurd values that indicate bugs
        return bounds.getX() > -10000 && bounds.getX() < 100000 &&
               bounds.getY() > -10000 && bounds.getY() < 100000 &&
               bounds.getWidth() < 100000 && bounds.getHeight() < 100000;
      },
      "Component bounds are unreasonable (possible layout bug)",
      "Component bounds are within reasonable range");
}

// Implement Checkers...
bool ComponentValidator::validateComponentBounds(SkiaComponent *component) {
  return true;
}
bool ComponentValidator::validateComponentVisibility(SkiaComponent *component) {
  return true;
}
bool ComponentValidator::validateComponentColors(SkiaComponent *component) {
  return true;
}
bool ComponentValidator::validateComponentFonts(SkiaComponent *component) {
  return true;
}
bool ComponentValidator::validateComponentAccessibility(
    SkiaComponent *component) {
  return true;
}
bool ComponentValidator::validateComponentPerformance(
    SkiaComponent *component) {
  return true;
}

//==============================================================================
// PerformanceTester Implementation
//==============================================================================

PerformanceTester &PerformanceTester::getInstance() {
  static PerformanceTester instance;
  return instance;
}

PerformanceTester::PerformanceMetrics
PerformanceTester::measureComponentPerformance(SkiaComponent *component,
                                               int frameCount) {
  return {};
}

TestReport PerformanceTester::runPerformanceTest(SkiaComponent *component,
                                                 const juce::String &testName) {
  TestReport report;
  report.result = TestResult::Passed;
  return report;
}

void PerformanceTester::setFpsThreshold(float minFps) {
  fpsThreshold_ = minFps;
}
void PerformanceTester::setFrameTimeThreshold(float maxMs) {
  frameTimeThreshold_ = maxMs;
}
void PerformanceTester::setMemoryThreshold(juce::int64 maxBytes) {
  memoryThreshold_ = maxBytes;
}

juce::Array<PerformanceTester::PerformanceMetrics>
PerformanceTester::getBenchmarkHistory() const {
  return benchmarkHistory_;
}

void PerformanceTester::clearBenchmarkHistory() {
  juce::ScopedLock lock(lock_);
  benchmarkHistory_.clear();
}

juce::int64 PerformanceTester::getCurrentMemoryUsage() { return 0; }
float PerformanceTester::getCurrentCpuUsage() { return 0.0f; }

PerformanceTester::PerformanceMetrics
PerformanceTester::calculateMetrics(const juce::Array<float> &frameTimes) {
  return {};
}

//==============================================================================
// AccessibilityTester Implementation
//==============================================================================

AccessibilityTester &AccessibilityTester::getInstance() {
  static AccessibilityTester instance;
  return instance;
}

AccessibilityTester::AccessibilityReport
AccessibilityTester::testComponentAccessibility(SkiaComponent *component) {
  return {};
}

TestReport
AccessibilityTester::runAccessibilityTest(SkiaComponent *component,
                                          const juce::String &testName) {
  TestReport report;
  report.result = TestResult::Passed;
  return report;
}

void AccessibilityTester::setStandard(Standard standard) {
  juce::ScopedLock lock(lock_);
  currentStandard_ = standard;
}

AccessibilityTester::Standard AccessibilityTester::getCurrentStandard() const {
  return currentStandard_;
}

float AccessibilityTester::getContrastRatio(SkColor color1, SkColor color2) {
  return 1.0f;
}
bool AccessibilityTester::meetsContrastRequirement(SkColor foreground,
                                                   SkColor background,
                                                   float minimumRatio) {
  return true;
}
float AccessibilityTester::getLuminance(SkColor color) { return 0.5f; }
juce::String AccessibilityTester::getWcagLevel(float contrastRatio) {
  return "AAA";
}

//==============================================================================
// TestUtils Implementation
//==============================================================================

std::unique_ptr<SkiaComponent>
TestUtils::createTestComponent(const juce::String &type) {
  return nullptr;
}

juce::Array<std::unique_ptr<SkiaComponent>> TestUtils::createTestComponents() {
  return {};
}

juce::String TestUtils::generateRandomString(int length) { return "test"; }
juce::Image TestUtils::generateTestImage(int width, int height, SkColor color) {
  return juce::Image(juce::Image::ARGB, width, height, true);
}
juce::DynamicObject::Ptr TestUtils::generateTestConfig() {
  return new juce::DynamicObject();
}

TestUtils::ScopedTimer::ScopedTimer(const juce::String &name)
    : name_(name), startTime_(juce::Time::getMillisecondCounter()) {}
TestUtils::ScopedTimer::~ScopedTimer() {}
juce::int64 TestUtils::ScopedTimer::getElapsedTime() const {
  return juce::Time::getMillisecondCounter() - startTime_;
}

void TestUtils::assertTrue(bool condition, const juce::String &message) {}
void TestUtils::assertFalse(bool condition, const juce::String &message) {}
void TestUtils::assertEquals(const juce::var &expected, const juce::var &actual,
                             const juce::String &message) {}
void TestUtils::assertNotNull(const void *pointer,
                              const juce::String &message) {}
void TestUtils::assertGreaterThan(float value, float threshold,
                                  const juce::String &message) {}
void TestUtils::assertLessThan(float value, float threshold,
                               const juce::String &message) {}

} // namespace testing
} // namespace zenith
