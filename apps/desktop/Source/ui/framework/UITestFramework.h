/*
  ==============================================================================

    UITestFramework.h
    Created: 2025-12-07
    Author:  AI Assistant

    UI Testing and Validation Framework

  ==============================================================================
*/

#pragma once

#include "../SkiaComponent.h"
#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>

namespace zenith {
namespace testing {

// ============================================================================
// Test Result Types
// ============================================================================

enum class TestResult { Passed, Failed, Skipped, Error };

struct TestReport {
  juce::String testName;
  TestResult result;
  juce::String message;
  juce::Time timestamp;
  juce::String screenshotPath;
  juce::String baselinePath;
  float similarityScore = 0.0f;
};

// ============================================================================
// Component Validation
// ============================================================================

class ComponentValidator {
public:
  static ComponentValidator &getInstance();

  using ValidationFunc = std::function<bool(SkiaComponent *)>;

  struct ValidationRule {
    juce::String name;
    ValidationFunc validator;
    juce::String failureMessage;
    juce::String successMessage;
  };

  void registerRule(const juce::String &name, ValidationFunc validator,
                    const juce::String &failureMessage,
                    const juce::String &successMessage);

  TestReport validate(SkiaComponent *component);
  juce::Array<TestReport>
  runValidationSuite(const juce::Array<SkiaComponent *> &components);

  // Built-in validators
  static bool validateComponentBounds(SkiaComponent *component);
  static bool validateComponentVisibility(SkiaComponent *component);
  static bool validateComponentColors(SkiaComponent *component);
  static bool validateComponentFonts(SkiaComponent *component);
  static bool validateComponentAccessibility(SkiaComponent *component);
  static bool validateComponentPerformance(SkiaComponent *component);

private:
  ComponentValidator();
  void registerBuiltInValidators();

  juce::Array<ValidationRule> rules_;
  juce::CriticalSection lock_;
};

// ============================================================================
// Visual Regression Testing
// ============================================================================

class VisualRegressionTester {
public:
  static VisualRegressionTester &getInstance();

  // Configuration
  void setBaselineDirectory(const juce::File &directory);
  void setScreenshotDirectory(const juce::File &directory);
  void setSimilarityThreshold(float threshold); // 0.0 to 1.0

  // Capture and compare
  TestReport captureAndCompare(SkiaComponent *component,
                               const juce::String &testName);
  bool captureBaseline(SkiaComponent *component, const juce::String &testName);

  // Batch testing
  juce::Array<TestReport>
  runVisualTests(const juce::Array<SkiaComponent *> &components);
  // Compatibility signature for std::vector if needed, but implementation uses
  // juce::Array

  // History and Reporting
  juce::Array<TestReport> getTestHistory() const;
  void clearTestHistory();
  juce::String generateHtmlReport() const;

  // Comparison util
  float compareImages(const juce::Image &baseline, const juce::Image &current);
  juce::Image generateDiffImage(const juce::Image &baseline,
                                const juce::Image &current);

private:
  VisualRegressionTester() = default;

  juce::Image captureComponent(SkiaComponent *component);
  float calculateImageSimilarity(const juce::Image &baseline,
                                 const juce::Image &current);
  juce::File getScreenshotPath(const juce::String &testName);
  juce::File getBaselinePath(const juce::String &testName);

  juce::File baselineDirectory_;
  juce::File screenshotDirectory_;
  float similarityThreshold_ = 0.99f;

  juce::Array<TestReport> testHistory_;
  mutable juce::CriticalSection lock_;
};

// ============================================================================
// Performance Testing
// ============================================================================

class PerformanceTester {
public:
  static PerformanceTester &getInstance();

  struct PerformanceMetrics {
    float fps = 0.0f;
    float frameTimeMs = 0.0f;
    int drawCallCount = 0;
    int vertexCount = 0;
    int textureBindCount = 0;
    juce::int64 memoryUsage = 0;
    float cpuUsage = 0.0f;
  };

  PerformanceMetrics measureComponentPerformance(SkiaComponent *component,
                                                 int frameCount = 60);
  TestReport runPerformanceTest(SkiaComponent *component,
                                const juce::String &testName);

  juce::int64 getCurrentMemoryUsage();
  float getCurrentCpuUsage();

private:
  PerformanceTester() = default;

  PerformanceMetrics calculateMetrics(const juce::Array<float> &frameTimes);
};

// ============================================================================
// Accessibility Testing
// ============================================================================

class AccessibilityTester {
public:
  static AccessibilityTester &getInstance();

  enum class Standard { WCAG_AA, WCAG_AAA };

  struct AccessibilityReport {
    bool keyboardAccessible = false;
    bool screenReaderCompatible = false;
    bool colorContrastValid = false;
    bool fontSizeValid = false;
    juce::StringArray issues; // Changed from Array<String> to StringArray based
                              // on joinIntoString usage
  };

  AccessibilityReport testComponentAccessibility(SkiaComponent *component);
  TestReport runAccessibilityTest(SkiaComponent *component,
                                  const juce::String &testName);

  void setStandard(Standard standard);
  Standard getCurrentStandard() const;

  // Utils
  float getContrastRatio(SkColor color1, SkColor color2);
  bool meetsContrastRequirement(SkColor foreground, SkColor background,
                                float minimumRatio);
  float getLuminance(SkColor color);
  juce::String getWcagLevel(float contrastRatio);

private:
  AccessibilityTester() = default;

  Standard currentStandard_ = Standard::WCAG_AA;
  mutable juce::CriticalSection lock_;
};

// ============================================================================
// Test Utilities
// ============================================================================

class TestUtils {
public:
  static std::unique_ptr<SkiaComponent>
  createTestComponent(const juce::String &type);
  static juce::Array<std::unique_ptr<SkiaComponent>> createTestComponents();

  static juce::String generateRandomString(int length);
  static juce::Image generateTestImage(int width, int height, SkColor color);
  static juce::DynamicObject::Ptr generateTestConfig();

  class ScopedTimer {
  public:
    ScopedTimer(const juce::String &name);
    ~ScopedTimer();
    juce::int64 getElapsedTime() const;

  private:
    juce::String name_;
    juce::int64 startTime_;
  };

  // Assertions
  static void assertTrue(bool condition, const juce::String &message = {});
  static void assertFalse(bool condition, const juce::String &message = {});
  static void assertEquals(const juce::var &expected, const juce::var &actual,
                           const juce::String &message = {});
  static void assertNotNull(const void *pointer,
                            const juce::String &message = {});
  static void assertGreaterThan(float value, float threshold,
                                const juce::String &message = {});
  static void assertLessThan(float value, float threshold,
                             const juce::String &message = {});
};

} // namespace testing
} // namespace zenith