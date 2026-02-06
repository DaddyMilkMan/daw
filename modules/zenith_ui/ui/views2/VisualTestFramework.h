/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of
    the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

// VisualTestFramework.h


#include "../ViewTheme.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <vector>
#include <string>
#include <map>

namespace zenith::ui::testing {

/**
 * @brief Test result structure for visual tests
 */
struct TestResult {
    std::string testName;
    bool passed;
    std::string message;
    float differenceScore; // 0.0 = identical, 1.0 = completely different
    juce::Rectangle<int> differenceArea; // Region that differs
    juce::Image baselineImage;
    juce::Image currentImage;
    juce::Image diffImage;
    juce::String timestamp;

    TestResult()
        : passed(false)
        , differenceScore(1.0f)
        , differenceArea(0, 0, 0, 0)
        , timestamp(juce::Time::getCurrentTime().toString(true, true, true, true))
    {}
};

/**
 * @brief Visual test configuration
 */
struct VisualTestConfig {
    // Test settings
    float toleranceThreshold = 0.05f; // Maximum allowed difference (0.0-1.0)
    int maxImageSize = 1920; // Maximum dimension for test images
    bool saveDifferences = true; // Save difference images
    bool updateBaselines = false; // Update baseline images if tests fail
    bool verboseOutput = true; // Detailed output

    // Performance settings
    int warmupFrames = 10; // Number of frames to warm up before capture
    int captureFrames = 3; // Number of frames to average
    int cooldownFrames = 5; // Frames to wait between tests

    // Theme settings for testing
    ViewTheme::Theme testTheme;
    ViewTheme::Layout::Preset layoutPreset = ViewTheme::Layout::Preset::Standard;
    ViewTheme::Colors::Preset colorPreset = ViewTheme::Colors::Preset::NeonNoir;
    ViewTheme::Animation::Preset animationPreset = ViewTheme::Animation::Preset::Normal;

    // Output settings
    juce::String baselineDirectory = "test_baselines";
    juce::String resultsDirectory = "test_results";
    juce::String diffDirectory = "test_differences";

    // Test categories to run
    bool runLayoutTests = true;
    bool runColorTests = true;
    bool runAnimationTests = true;
    bool runErrorHandlingTests = true;
    bool runPerformanceTests = true;

    // Test case selection
    std::vector<std::string> selectedTests;
    std::vector<std::string> excludedTests;

    static VisualTestConfig createDefault();
    void validate();
};

/**
 * @brief Visual test runner for SkiaSessionView
 */
class VisualTestRunner {
public:
    VisualTestRunner();
    ~VisualTestRunner();

    // Test configuration
    void setConfig(const VisualTestConfig& config);
    const VisualTestConfig& getConfig() const;

    // Test execution
    std::vector<TestResult> runAllTests();
    TestResult runSingleTest(const std::string& testName);
    void runTestSuite(const std::vector<std::string>& testNames);

    // Test management
    void addTest(const std::string& name, std::function<TestResult()> testFunction);
    void removeTest(const std::string& name);
    std::vector<std::string> getAvailableTests() const;
    std::vector<std::string> getTestCategories() const;

    // Results
    const std::vector<TestResult>& getResults() const;
    juce::String generateReport() const;
    void saveReport(const juce::String& filePath) const;
    void saveResults() const;

    // Baseline management
    bool createBaseline(const std::string& testName);
    bool updateBaseline(const std::string& testName);
    bool restoreBaseline(const std::string& testName);
    bool deleteBaseline(const std::string& testName);
    std::vector<std::string> getAvailableBaselines() const;

    // Utilities
    static juce::Image captureComponentImage(juce::Component& component, int width, int height);
    static juce::Image compareImages(const juce::Image& baseline, const juce::Image& current);
    static float calculateDifferenceScore(const juce::Image& baseline, const juce::Image& current);
    static juce::Image highlightDifferences(const juce::Image& baseline, const juce::Image& current);

private:
    VisualTestConfig config_;
    std::map<std::string, std::function<TestResult()>> tests_;
    std::vector<TestResult> results_;
    juce::File baselineDir_;
    juce::File resultsDir_;
    juce::File diffDir_;

    // Test categories
    enum class TestCategory {
        Layout,
        Color,
        Animation,
        ErrorHandling,
        Performance
    };

    // Test categorization
    std::map<std::string, TestCategory> testCategories_;

    // Utility methods
    void setupDirectories();
    void categorizeTest(const std::string& name, TestCategory category);
    TestResult runTimedTest(const std::string& name, std::function<TestResult()> test);
    void waitForStability(juce::Component& component, int frames);
    void captureTestImage(juce::Component& component, juce::Image& outImage);

    // Test helpers
    TestResult testBasicLayout();
    TestResult testColorScheme();
    TestResult testAnimationEffects();
    TestResult testErrorHandling();
    TestResult testPerformance();
    TestResult testResponsiveLayout();
    TestResult testThemeSwitching();

    // Static test instances
    static SkiaSessionView* createTestSessionView();
    static void populateTestSessionView(SkiaSessionView& view);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VisualTestRunner)
};

/**
 * @brief Test console application for running visual tests
 */
class VisualTestConsole {
public:
    static void run(int argc, char* argv[]);
    static void showHelp();
    static bool parseArguments(int argc, char* argv[], VisualTestConfig& config);

private:
    static juce::String generateHelpText();
};

} // namespace zenith::ui::testing