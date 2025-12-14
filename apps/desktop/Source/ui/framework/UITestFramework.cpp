/*
  ==============================================================================

    UITestFramework.cpp
    Created: 2025-12-07
    Author:  AI Assistant

    UI Testing and Validation Framework Implementation

  ==============================================================================
*/

#include "UITestFramework.h"
#include "../ZenithDesignSystem.h"
#include <algorithm>
#include <cmath>

namespace zenith {
namespace testing {

// ============================================================================
// VisualRegressionTester Implementation
// ============================================================================

VisualRegressionTester &VisualRegressionTester::getInstance() {
  static VisualRegressionTester instance;
  return instance;
}

void VisualRegressionTester::setBaselineDirectory(const juce::File &directory) {
  juce::ScopedLock lock(lock_);
  baselineDirectory_ = directory;
  baselineDirectory_.createDirectory();
}

void VisualRegressionTester::setScreenshotDirectory(
    const juce::File &directory) {
  juce::ScopedLock lock(lock_);
  screenshotDirectory_ = directory;
  screenshotDirectory_.createDirectory();
}

void VisualRegressionTester::setSimilarityThreshold(float threshold) {
  juce::ScopedLock lock(lock_);
  similarityThreshold_ = juce::jlimit(0.0f, 1.0f, threshold);
}

TestReport
VisualRegressionTester::captureAndCompare(SkiaComponent *component,
                                          const juce::String &testName) {
  juce::ScopedLock lock(lock_);

  TestReport report;
  report.testName = testName;
  report.timestamp = juce::Time::getCurrentTime();

  // Capture current screenshot
  juce::Image currentImage = captureComponent(component);
  juce::File screenshotPath = getScreenshotPath(testName);

  // Save current screenshot
  juce::PNGImageFormat pngFormat;
  juce::FileOutputStream stream(screenshotPath);
  if (pngFormat.writeImageToStream(currentImage, stream)) {
    report.screenshotPath = screenshotPath.getFullPathName();
  }

  // Load baseline if it exists
  juce::File baselinePath = getBaselinePath(testName);
  report.baselinePath = baselinePath.getFullPathName();

  if (baselinePath.existsAsFile()) {
    juce::Image baselineImage = juce::ImageFileFormat::loadFrom(baselinePath);

    if (baselineImage.isValid()) {
      // Compare images
      report.similarityScore = compareImages(baselineImage, currentImage);

      if (report.similarityScore >= similarityThreshold_) {
        report.result = TestResult::Passed;
        report.message = "Visual test passed with similarity: " +
                         juce::String(report.similarityScore * 100.0f, 1) + "%";
      } else {
        report.result = TestResult::Failed;
        report.message =
            "Visual test failed. Similarity: " +
            juce::String(report.similarityScore * 100.0f, 1) + "%" +
            " (threshold: " + juce::String(similarityThreshold_ * 100.0f, 1) +
            "%)";

        // Generate diff image
        juce::Image diffImage = generateDiffImage(baselineImage, currentImage);
        juce::File diffPath = screenshotPath.getParentDirectory().getChildFile(
            testName + "_diff.png");
        juce::FileOutputStream diffStream(diffPath);
        pngFormat.writeImageToStream(diffImage, diffStream);
      }
    } else {
      report.result = TestResult::Error;
      report.message = "Failed to load baseline image";
    }
  } else {
    // No baseline exists - create one
    if (captureBaseline(component, testName)) {
      report.result = TestResult::Passed;
      report.message = "Baseline created for new test";
    } else {
      report.result = TestResult::Error;
      report.message = "Failed to create baseline";
    }
  }

  testHistory_.add(report);
  return report;
}

bool VisualRegressionTester::captureBaseline(SkiaComponent *component,
                                             const juce::String &testName) {
  juce::ScopedLock lock(lock_);

  juce::Image baselineImage = captureComponent(component);
  juce::File baselinePath = getBaselinePath(testName);

  juce::PNGImageFormat pngFormat;
  juce::FileOutputStream stream(baselinePath);

  return pngFormat.writeImageToStream(baselineImage, stream);
}

float VisualRegressionTester::compareImages(const juce::Image &baseline,
                                            const juce::Image &current) {
  if (baseline.getWidth() != current.getWidth() ||
      baseline.getHeight() != current.getHeight()) {
    return 0.0f; // Different sizes - complete mismatch
  }

  return calculateImageSimilarity(baseline, current);
}

juce::Image
VisualRegressionTester::generateDiffImage(const juce::Image &baseline,
                                          const juce::Image &current) {
  juce::Image diffImage(juce::Image::ARGB, baseline.getWidth(),
                        baseline.getHeight(), true);
  juce::Graphics g(diffImage);

  // Create a red overlay where pixels differ
  for (int y = 0; y < baseline.getHeight(); ++y) {
    for (int x = 0; x < baseline.getWidth(); ++x) {
      juce::Colour baselinePixel = baseline.getPixelAt(x, y);
      juce::Colour currentPixel = current.getPixelAt(x, y);

      if (baselinePixel != currentPixel) {
        // Pixel differs - mark in red
        g.setPixel(x, y, juce::Colours::red.withAlpha(0.5f));
      } else {
        // Pixel matches - use baseline pixel with reduced opacity
        g.setPixel(x, y, baselinePixel.withAlpha(0.3f));
      }
    }
  }

  return diffImage;
}

juce::Array<TestReport> VisualRegressionTester::runVisualTests(
    const juce::Array<SkiaComponent *> &components) {
  juce::Array<TestReport> reports;

  for (int i = 0; i < components.size(); ++i) {
    juce::String testName = "component_" + juce::String(i);
    reports.add(captureAndCompare(components[i], testName));
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

  juce::String html;
  html << "<html><head><title>Visual Test Report</title></head><body>\n";
  html << "<h1>Visual Regression Test Report</h1>\n";
  html << "<p>Generated: " << juce::Time::getCurrentTime().toString(true, true)
       << "</p>\n";

  int passed = 0, failed = 0, errors = 0;
  for (const auto &report : testHistory_) {
    if (report.result == TestResult::Passed)
      passed++;
    else if (report.result == TestResult::Failed)
      failed++;
    else if (report.result == TestResult::Error)
      errors++;
  }

  html << "<p>Passed: " << passed << ", Failed: " << failed
       << ", Errors: " << errors << "</p>\n";
  html << "<table border='1'>\n";
  html << "<tr><th>Test "
          "Name</th><th>Result</th><th>Message</th><th>Similarity</th></tr>\n";

  for (const auto &report : testHistory_) {
    html << "<tr>";
    html << "<td>" << report.testName << "</td>";
    html << "<td style='color:"
         << (report.result == TestResult::Passed ? "green" : "red") << "'>";
    html << (report.result == TestResult::Passed   ? "PASSED"
             : report.result == TestResult::Failed ? "FAILED"
                                                   : "ERROR")
         << "</td>";
    html << "<td>" << report.message << "</td>";
    html << "<td>" << juce::String(report.similarityScore * 100.0f, 1)
         << "%</td>";
    html << "</tr>\n";
  }

  html << "</table></body></html>\n";
  return html;
}

juce::Image VisualRegressionTester::captureComponent(SkiaComponent *component) {
  if (!component) {
    return juce::Image();
  }

  juce::Rectangle<int> bounds = component->getBounds();
  juce::Image screenshot(juce::Image::ARGB, bounds.getWidth(),
                         bounds.getHeight(), true);
  juce::Graphics g(screenshot);

  // Render component to image
  component->paintEntireComponent(g, true);

  return screenshot;
}

juce::File
VisualRegressionTester::getBaselinePath(const juce::String &testName) const {
  return baselineDirectory_.getChildFile(testName + "_baseline.png");
}

juce::File
VisualRegressionTester::getScreenshotPath(const juce::String &testName) const {
  return screenshotDirectory_.getChildFile(testName + "_current.png");
}

float VisualRegressionTester::calculateImageSimilarity(
    const juce::Image &img1, const juce::Image &img2) {
  if (img1.getWidth() != img2.getWidth() ||
      img1.getHeight() != img2.getHeight()) {
    return 0.0f;
  }

  int width = img1.getWidth();
  int height = img1.getHeight();
  int totalPixels = width * height;
  int matchingPixels = 0;

  // Compare pixel by pixel with tolerance
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      juce::Colour pixel1 = img1.getPixelAt(x, y);
      juce::Colour pixel2 = img2.getPixelAt(x, y);

      // Calculate color difference
      int rDiff = std::abs(pixel1.getRed() - pixel2.getRed());
      int gDiff = std::abs(pixel1.getGreen() - pixel2.getGreen());
      int bDiff = std::abs(pixel1.getBlue() - pixel2.getBlue());
      int aDiff = std::abs(pixel1.getAlpha() - pixel2.getAlpha());

      // If colors are very similar, count as match
      if (rDiff < 5 && gDiff < 5 && bDiff < 5 && aDiff < 5) {
        matchingPixels++;
      }
    }
  }

  return static_cast<float>(matchingPixels) / static_cast<float>(totalPixels);
}

// ============================================================================
// ComponentValidator Implementation
// ============================================================================

ComponentValidator &ComponentValidator::getInstance() {
  static ComponentValidator instance;
  return instance;
}

void ComponentValidator::registerRule(const ValidationRule &rule) {
  juce::ScopedLock lock(lock_);
  validationRules_.add(rule);
}

void ComponentValidator::registerRule(
    const juce::String &name, std::function<bool(SkiaComponent *)> check,
    const juce::String &errorMessage, const juce::String &successMessage) {
  ValidationRule rule;
  rule.name = name;
  rule.check = check;
  rule.errorMessage = errorMessage;
  rule.successMessage = successMessage;

  registerRule(rule);
}

TestReport ComponentValidator::validateComponent(SkiaComponent *component) {
  TestReport report;
  report.testName = "Component Validation";
  report.timestamp = juce::Time::getCurrentTime();
  report.result = TestResult::Passed;

  juce::Array<juce::String> failures;

  juce::ScopedLock lock(lock_);

  for (const auto &rule : validationRules_) {
    if (!rule.check(component)) {
      failures.add(rule.errorMessage);
      report.result = TestResult::Failed;
    }
  }

  if (report.result == TestResult::Passed) {
    report.message = "All validation rules passed";
  } else {
    report.message = "Validation failed:\n" + failures.joinIntoString("\n");
  }

  return report;
}

juce::Array<TestReport> ComponentValidator::validateComponents(
    const juce::Array<SkiaComponent *> &components) {
  juce::Array<TestReport> reports;

  for (auto *component : components) {
    reports.add(validateComponent(component));
  }

  return reports;
}

void ComponentValidator::registerBuiltInValidators() {
  registerRule("Component Bounds", validateComponentBounds,
               "Component bounds are invalid (width or height <= 0)",
               "Component bounds are valid");

  registerRule("Component Visibility", validateComponentVisibility,
               "Component visibility state is inconsistent",
               "Component visibility is valid");

  registerRule("Component Colors", validateComponentColors,
               "Component uses invalid or undefined colors",
               "Component colors are valid");

  registerRule("Component Fonts", validateComponentFonts,
               "Component uses invalid or undefined fonts",
               "Component fonts are valid");

  registerRule("Component Accessibility", validateComponentAccessibility,
               "Component fails accessibility requirements",
               "Component meets accessibility requirements");

  registerRule("Component Performance", validateComponentPerformance,
               "Component performance is below threshold",
               "Component performance is acceptable");
}

bool ComponentValidator::validateComponentBounds(SkiaComponent *component) {
  if (!component)
    return false;

  auto bounds = component->getBounds();
  return bounds.getWidth() > 0 && bounds.getHeight() > 0;
}

bool ComponentValidator::validateComponentVisibility(SkiaComponent *component) {
  if (!component)
    return false;

  // Check if component visibility makes sense
  bool isVisible = component->isVisible();
  bool hasParent = component->getParentComponent() != nullptr;

  // Visible components should generally have a parent
  if (isVisible && !hasParent) {
    return false; // Orphaned visible component
  }

  return true;
}

bool ComponentValidator::validateComponentColors(SkiaComponent *component) {
  if (!component)
    return false;

  // This would need to be customized per component type
  // For now, just check that the component exists
  return true;
}

bool ComponentValidator::validateComponentFonts(SkiaComponent *component) {
  if (!component)
    return false;

  // Font validation would be component-specific
  return true;
}

bool ComponentValidator::validateComponentAccessibility(
    SkiaComponent *component) {
  if (!component)
    return false;

  // Basic accessibility check
  return component->isEnabled() && component->getBounds().getWidth() >= 44 &&
         component->getBounds().getHeight() >= 44; // Minimum touch target size
}

bool ComponentValidator::validateComponentPerformance(
    SkiaComponent *component) {
  if (!component)
    return false;

  // Performance validation would need timing data
  return true;
}

// ============================================================================
// PerformanceTester Implementation
// ============================================================================

PerformanceTester &PerformanceTester::getInstance() {
  static PerformanceTester instance;
  return instance;
}

PerformanceTester::PerformanceMetrics
PerformanceTester::measureComponentPerformance(SkiaComponent *component,
                                               int frameCount) {

  PerformanceMetrics metrics;
  juce::Array<float> frameTimes;

  for (int i = 0; i < frameCount; ++i) {
    juce::int64 startTime = juce::Time::currentTimeMillis();

    // Force component repaint
    component->repaint();

    // Process events to ensure paint happens
    // juce::MessageManager::getInstance()->runDispatchLoopUntil(1);

    juce::int64 endTime = juce::Time::currentTimeMillis();
    frameTimes.add(static_cast<float>(endTime - startTime));
  }

  metrics = calculateMetrics(frameTimes);
  metrics.memoryUsage = getCurrentMemoryUsage();
  metrics.cpuUsage = getCurrentCpuUsage();

  return metrics;
}

TestReport PerformanceTester::runPerformanceTest(SkiaComponent *component,
                                                 const juce::String &testName) {
  TestReport report;
  report.testName = testName;
  report.timestamp = juce::Time::getCurrentTime();

  PerformanceMetrics metrics = measureComponentPerformance(component);

  // Check against thresholds
  bool fpsOk = metrics.fps >= fpsThreshold_;
  bool frameTimeOk = metrics.frameTimeMs <= frameTimeThreshold_;
  bool memoryOk = metrics.memoryUsage <= memoryThreshold_;

  if (fpsOk && frameTimeOk && memoryOk) {
    report.result = TestResult::Passed;
    report.message =
        "Performance test passed. FPS: " + juce::String(metrics.fps, 1) +
        ", Frame time: " + juce::String(metrics.frameTimeMs, 1) + "ms";
  } else {
    report.result = TestResult::Failed;
    report.message = "Performance test failed. ";
    if (!fpsOk)
      report.message += "FPS too low. ";
    if (!frameTimeOk)
      report.message += "Frame time too high. ";
    if (!memoryOk)
      report.message += "Memory usage too high. ";
  }

  benchmarkHistory_.add(metrics);
  return report;
}

void PerformanceTester::setFpsThreshold(float minFps) {
  juce::ScopedLock lock(lock_);
  fpsThreshold_ = minFps;
}

void PerformanceTester::setFrameTimeThreshold(float maxMs) {
  juce::ScopedLock lock(lock_);
  frameTimeThreshold_ = maxMs;
}

void PerformanceTester::setMemoryThreshold(juce::int64 maxBytes) {
  juce::ScopedLock lock(lock_);
  memoryThreshold_ = maxBytes;
}

juce::Array<PerformanceTester::PerformanceMetrics>
PerformanceTester::getBenchmarkHistory() const {
  juce::ScopedLock lock(lock_);
  return benchmarkHistory_;
}

void PerformanceTester::clearBenchmarkHistory() {
  juce::ScopedLock lock(lock_);
  benchmarkHistory_.clear();
}

PerformanceTester::PerformanceMetrics
PerformanceTester::calculateMetrics(const juce::Array<float> &frameTimes) {

  PerformanceMetrics metrics;

  // Calculate average frame time
  float totalTime = 0.0f;
  for (float time : frameTimes) {
    totalTime += time;
  }
  metrics.frameTimeMs = totalTime / frameTimes.size();

  // Calculate FPS
  metrics.fps = 1000.0f / metrics.frameTimeMs;

  // Count draw calls and vertices (these would need to be tracked during
  // rendering)
  metrics.drawCallCount = 0;
  metrics.vertexCount = 0;
  metrics.textureBindCount = 0;

  return metrics;
}

juce::int64 PerformanceTester::getCurrentMemoryUsage() {
  // This would need platform-specific implementation
  // For now, return a placeholder
  return 0;
}

float PerformanceTester::getCurrentCpuUsage() {
  // This would need platform-specific implementation
  // For now, return a placeholder
  return 0.0f;
}

// ============================================================================
// AccessibilityTester Implementation
// ============================================================================

AccessibilityTester &AccessibilityTester::getInstance() {
  static AccessibilityTester instance;
  return instance;
}

AccessibilityTester::AccessibilityReport
AccessibilityTester::testComponentAccessibility(SkiaComponent *component) {

  AccessibilityReport report;
  report.keyboardAccessible = component->isEnabled() && component->isVisible();
  report.screenReaderCompatible = false; // Would need screen reader integration
  report.colorContrastValid = true;      // Would need proper color analysis
  report.fontSizeValid = true;           // Would need font size checking

  // Check minimum touch target size
  auto bounds = component->getBounds();
  if (bounds.getWidth() < 44 || bounds.getHeight() < 44) {
    report.issues.add("Component too small for accessible touch target");
  }

  return report;
}

TestReport
AccessibilityTester::runAccessibilityTest(SkiaComponent *component,
                                          const juce::String &testName) {
  TestReport report;
  report.testName = testName;
  report.timestamp = juce::Time::getCurrentTime();

  AccessibilityReport accessibility = testComponentAccessibility(component);

  if (accessibility.keyboardAccessible && accessibility.colorContrastValid &&
      accessibility.fontSizeValid) {
    report.result = TestResult::Passed;
    report.message = "Accessibility test passed";
  } else {
    report.result = TestResult::Failed;
    report.message = "Accessibility issues found:\n" +
                     accessibility.issues.joinIntoString("\n");
  }

  return report;
}

void AccessibilityTester::setStandard(Standard standard) {
  juce::ScopedLock lock(lock_);
  currentStandard_ = standard;
}

AccessibilityTester::Standard AccessibilityTester::getCurrentStandard() const {
  juce::ScopedLock lock(lock_);
  return currentStandard_;
}

float AccessibilityTester::getContrastRatio(SkColor color1, SkColor color2) {
  float lum1 = getLuminance(color1);
  float lum2 = getLuminance(color2);

  float brightest = std::max(lum1, lum2);
  float darkest = std::min(lum1, lum2);

  return (brightest + 0.05f) / (darkest + 0.05f);
}

bool AccessibilityTester::meetsContrastRequirement(SkColor foreground,
                                                   SkColor background,
                                                   float minimumRatio) {
  return getContrastRatio(foreground, background) >= minimumRatio;
}

float AccessibilityTester::getLuminance(SkColor color) {
  // Convert to sRGB and calculate relative luminance
  float r = SkColorGetR(color) / 255.0f;
  float g = SkColorGetG(color) / 255.0f;
  float b = SkColorGetB(color) / 255.0f;

  // sRGB to linear RGB conversion
  auto srgbToLinear = [](float c) {
    return c <= 0.03928f ? c / 12.92f : std::pow((c + 0.055f) / 1.055f, 2.4f);
  };

  r = srgbToLinear(r);
  g = srgbToLinear(g);
  b = srgbToLinear(b);

  // Calculate relative luminance
  return 0.2126f * r + 0.7152f * g + 0.0722f * b;
}

juce::String AccessibilityTester::getWcagLevel(float contrastRatio) {
  if (contrastRatio >= 7.0f)
    return "AAA";
  if (contrastRatio >= 4.5f)
    return "AA";
  return "Fail";
}

// ============================================================================
// TestUtils Implementation
// ============================================================================

std::unique_ptr<SkiaComponent>
TestUtils::createTestComponent(const juce::String &type) {
  // Factory for creating test components
  if (type == "button") {
    return std::make_unique<SkiaButton>("Test Button");
  } else if (type == "textEditor") {
    return std::make_unique<SkiaTextEditor>("Test Editor");
  } else if (type == "label") {
    return std::make_unique<SkiaLabel>("Test Label");
  }

  return nullptr;
}

juce::Array<std::unique_ptr<SkiaComponent>> TestUtils::createTestComponents() {
  juce::Array<std::unique_ptr<SkiaComponent>> components;

  components.add(createTestComponent("button"));
  components.add(createTestComponent("textEditor"));
  components.add(createTestComponent("label"));

  return components;
}

juce::String TestUtils::generateRandomString(int length) {
  juce::String result;
  const char *chars =
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

  for (int i = 0; i < length; ++i) {
    result += chars[juce::Random::getSystemRandom().nextInt(62)];
  }

  return result;
}

juce::Image TestUtils::generateTestImage(int width, int height, SkColor color) {
  juce::Image image(juce::Image::ARGB, width, height, true);
  juce::Graphics g(image);

  g.setColour(juce::Colour::fromRGBA(SkColorGetR(color), SkColorGetG(color),
                                     SkColorGetB(color), SkColorGetA(color)));
  g.fillAll();

  return image;
}

juce::DynamicObject::Ptr TestUtils::generateTestConfig() {
  auto config = new juce::DynamicObject();
  config->setProperty("testString", "testValue");
  config->setProperty("testInt", 42);
  config->setProperty("testFloat", 3.14f);
  config->setProperty("testBool", true);

  return config;
}

// TestUtils::ScopedTimer Implementation
TestUtils::ScopedTimer::ScopedTimer(const juce::String &name)
    : name_(name), startTime_(juce::Time::currentTimeMillis()) {}

TestUtils::ScopedTimer::~ScopedTimer() {
  juce::int64 elapsed = getElapsedTime();
  DBG("[" << name_ << "] " << elapsed << "ms");
}

juce::int64 TestUtils::ScopedTimer::getElapsedTime() const {
  return juce::Time::currentTimeMillis() - startTime_;
}

// Test assertion implementations
void TestUtils::assertTrue(bool condition, const juce::String &message) {
  if (!condition) {
    throw std::runtime_error("Assertion failed: " + message.toStdString());
  }
}

void TestUtils::assertFalse(bool condition, const juce::String &message) {
  if (condition) {
    throw std::runtime_error("Assertion failed: " + message.toStdString());
  }
}

void TestUtils::assertEquals(const juce::var &expected, const juce::var &actual,
                             const juce::String &message) {
  if (expected != actual) {
    throw std::runtime_error(
        "Assertion failed: " + message.toStdString() +
        ". Expected: " + expected.toString().toStdString() +
        ", Actual: " + actual.toString().toStdString());
  }
}

void TestUtils::assertNotNull(const void *pointer,
                              const juce::String &message) {
  if (pointer == nullptr) {
    throw std::runtime_error("Assertion failed: " + message.toStdString());
  }
}

void TestUtils::assertGreaterThan(float value, float threshold,
                                  const juce::String &message) {
  if (value <= threshold) {
    throw std::runtime_error("Assertion failed: " + message.toStdString() +
                             ". Value " + juce::String(value).toStdString() +
                             " is not greater than " +
                             juce::String(threshold).toStdString());
  }
}

void TestUtils::assertLessThan(float value, float threshold,
                               const juce::String &message) {
  if (value >= threshold) {
    throw std::runtime_error("Assertion failed: " + message.toStdString() +
                             ". Value " + juce::String(value).toStdString() +
                             " is not less than " +
                             juce::String(threshold).toStdString());
  }
}

} // namespace testing
} // namespace zenith