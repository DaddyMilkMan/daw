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

// VisualTestFramework.cpp

#include "VisualTestFramework.h"
#include "../session/SkiaSessionView.h"
#include "../../design-system/ZenithTheme.h"
#include "../../design-system/ZenithDesignSystem.h"
#include <algorithm>
#include <chrono>
#include <thread>

namespace zenith::ui::testing {

//==============================================================================
// VisualTestConfig Implementation
//==============================================================================

VisualTestConfig VisualTestConfig::createDefault() {
    VisualTestConfig config;

    // Default settings
    config.toleranceThreshold = 0.05f;
    config.maxImageSize = 1920;
    config.saveDifferences = true;
    config.updateBaselines = false;
    config.verboseOutput = true;

    // Performance settings
    config.warmupFrames = 10;
    config.captureFrames = 3;
    config.cooldownFrames = 5;

    // Default theme settings
    config.testTheme = ViewTheme::Theme::createBuiltIn(ViewTheme::Theme::BuiltIn::NeonNoir);
    config.layoutPreset = ViewTheme::Layout::Preset::Standard;
    config.colorPreset = ViewTheme::Colors::Preset::NeonNoir;
    config.animationPreset = ViewTheme::Animation::Preset::Normal;

    // Run all test categories by default
    config.runLayoutTests = true;
    config.runColorTests = true;
    config.runAnimationTests = true;
    config.runErrorHandlingTests = true;
    config.runPerformanceTests = true;

    return config;
}

void VisualTestConfig::validate() {
    // Ensure valid ranges
    toleranceThreshold = std::clamp(toleranceThreshold, 0.0f, 1.0f);
    maxImageSize = std::clamp(maxImageSize, 100, 8192);
    warmupFrames = std::max(0, warmupFrames);
    captureFrames = std::max(1, captureFrames);
    cooldownFrames = std::max(0, cooldownFrames);

    // Ensure directories exist
    juce::File baselineDir(baselineDirectory);
    juce::File resultsDir(resultsDirectory);
    juce::File diffDir(diffDirectory);

    baselineDir.createDirectory();
    resultsDir.createDirectory();
    diffDir.createDirectory();
}

//==============================================================================
// VisualTestRunner Implementation
//==============================================================================

VisualTestRunner::VisualTestRunner() {
    setupDirectories();

    // Register default tests
    addTest("basic_layout", [this]() { return testBasicLayout(); });
    addTest("color_scheme", [this]() { return testColorScheme(); });
    addTest("animation_effects", [this]() { return testAnimationEffects(); });
    addTest("error_handling", [this]() { return testErrorHandling(); });
    addTest("performance", [this]() { return testPerformance(); });
    addTest("responsive_layout", [this]() { return testResponsiveLayout(); });
    addTest("theme_switching", [this]() { return testThemeSwitching(); });

    // Categorize tests
    categorizeTest("basic_layout", TestCategory::Layout);
    categorizeTest("color_scheme", TestCategory::Color);
    categorizeTest("animation_effects", TestCategory::Animation);
    categorizeTest("error_handling", TestCategory::ErrorHandling);
    categorizeTest("performance", TestCategory::Performance);
    categorizeTest("responsive_layout", TestCategory::Layout);
    categorizeTest("theme_switching", TestCategory::Color);
}

VisualTestRunner::~VisualTestRunner() {
    // Save results on destruction
    saveResults();
}

void VisualTestRunner::setConfig(const VisualTestConfig& config) {
    config_ = config;
    config_.validate();
    setupDirectories();
}

const VisualTestConfig& VisualTestRunner::getConfig() const {
    return config_;
}

std::vector<TestResult> VisualTestRunner::runAllTests() {
    results_.clear();

    if (config_.verboseOutput) {
        juce::Logger::writeToLog("Running visual regression tests...");
    }

    // Filter tests based on configuration
    std::vector<std::string> testsToRun;
    for (const auto& test : tests_) {
        bool shouldRun = true;

        // Check if test is in exclusion list
        if (config_.excludedTests.size() > 0) {
            shouldRun = std::find(config_.excludedTests.begin(), config_.excludedTests.end(), test.first) == config_.excludedTests.end();
        }

        // Check if test is in selection list (if specified)
        if (!config_.selectedTests.empty()) {
            shouldRun = std::find(config_.selectedTests.begin(), config_.selectedTests.end(), test.first) != config_.selectedTests.end();
        }

        // Check category selection
        if (shouldRun) {
            auto categoryIt = testCategories_.find(test.first);
            if (categoryIt != testCategories_.end()) {
                switch (categoryIt->second) {
                    case TestCategory::Layout:
                        shouldRun = config_.runLayoutTests;
                        break;
                    case TestCategory::Color:
                        shouldRun = config_.runColorTests;
                        break;
                    case TestCategory::Animation:
                        shouldRun = config_.runAnimationTests;
                        break;
                    case TestCategory::ErrorHandling:
                        shouldRun = config_.runErrorHandlingTests;
                        break;
                    case TestCategory::Performance:
                        shouldRun = config_.runPerformanceTests;
                        break;
                }
            }
        }

        if (shouldRun) {
            testsToRun.push_back(test.first);
        }
    }

    // Run tests
    for (const auto& testName : testsToRun) {
        if (config_.verboseOutput) {
            juce::Logger::writeToLog("Running test: " + testName);
        }

        auto result = runSingleTest(testName);
        results_.push_back(result);

        // Cooldown between tests
        if (config_.cooldownFrames > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(config_.cooldownFrames * 16));
        }
    }

    if (config_.verboseOutput) {
        juce::Logger::writeToLog("Visual regression tests completed.");
    }

    return results_;
}

TestResult VisualTestRunner::runSingleTest(const std::string& testName) {
    auto testIt = tests_.find(testName);
    if (testIt == tests_.end()) {
        TestResult result;
        result.testName = testName;
        result.passed = false;
        result.message = "Test not found: " + testName;
        return result;
    }

    return runTimedTest(testName, testIt->second);
}

void VisualTestRunner::runTestSuite(const std::vector<std::string>& testNames) {
    for (const auto& name : testNames) {
        if (tests_.find(name) != tests_.end()) {
            runSingleTest(name);
        }
    }
}

void VisualTestRunner::addTest(const std::string& name, std::function<TestResult()> testFunction) {
    tests_[name] = testFunction;
}

void VisualTestRunner::removeTest(const std::string& name) {
    tests_.erase(name);
    testCategories_.erase(name);
}

std::vector<std::string> VisualTestRunner::getAvailableTests() const {
    std::vector<std::string> names;
    for (const auto& test : tests_) {
        names.push_back(test.first);
    }
    return names;
}

std::vector<std::string> VisualTestRunner::getTestCategories() const {
    std::vector<std::string> categories;
    categories.push_back("Layout");
    categories.push_back("Color");
    categories.push_back("Animation");
    categories.push_back("ErrorHandling");
    categories.push_back("Performance");
    return categories;
}

const std::vector<TestResult>& VisualTestRunner::getResults() const {
    return results_;
}

juce::String VisualTestRunner::generateReport() const {
    juce::String report;
    report << "Zenith DAW - Visual Regression Test Report\n";
    report << "==========================================\n\n";
    report << "Generated: " << juce::Time::getCurrentTime().toString(true, true, true, true) << "\n";
    report << "Config Tolerance: " << config_.toleranceThreshold << "\n";
    report << "Total Tests: " << results_.size() << "\n\n";

    int passedCount = 0;
    int failedCount = 0;

    for (const auto& result : results_) {
        if (result.passed) {
            passedCount++;
        } else {
            failedCount++;
        }
    }

    report << "Results: " << passedCount << " passed, " << failedCount << " failed\n";
    report << "Pass Rate: " << (float)passedCount / results_.size() * 100 << "%\n\n";

    report << "Test Details:\n";
    report << "-------------\n";

    for (const auto& result : results_) {
        report << result.testName << ": ";
        if (result.passed) {
            report << "PASS";
        } else {
            report << "FAIL (Score: " << juce::String(result.differenceScore, 3) << ")";
        }
        report << "\n";
        if (!result.message.isEmpty()) {
            report << "  Message: " << result.message << "\n";
        }
        report << "\n";
    }

    return report;
}

void VisualTestRunner::saveReport(const juce::String& filePath) const {
    juce::File reportFile(filePath);
    reportFile.replaceText(generateReport());
}

void VisualTestRunner::saveResults() const {
    for (const auto& result : results_) {
        // Save difference image if requested and test failed
        if (config_.saveDifferences && !result.passed && !result.diffImage.isNull()) {
            juce::File diffFile(diffDir_.getChildFile(result.testName + "_diff.png"));
            juce::PNGImageFormat pngFormat;
            pngFormat.writeToFile(result.diffImage, diffFile);
        }

        // Save test result data
        juce::File resultFile(resultsDir_.getChildFile(result.testName + ".xml"));
        juce::XmlDocument doc;

        XmlElement* root = doc.createNewElement("TestResult");
        root->setAttribute("name", result.testName);
        root->setAttribute("passed", result.passed ? "true" : "false");
        root->setAttribute("differenceScore", juce::String(result.differenceScore));
        root->setAttribute("timestamp", result.timestamp);
        root->setAttribute("message", result.message);

        // Add difference area if applicable
        if (!result.differenceArea.isEmpty()) {
            XmlElement* diffArea = root->createNewChildElement("DifferenceArea");
            diffArea->setAttribute("x", result.differenceArea.getX());
            diffArea->setAttribute("y", result.differenceArea.getY());
            diffArea->setAttribute("width", result.differenceArea.getWidth());
            diffArea->setAttribute("height", result.differenceArea.getHeight());
        }

        doc.writeTo(resultFile, juce::XmlDocument::writeEveryNode);
    }
}

bool VisualTestRunner::createBaseline(const std::string& testName) {
    auto sessionView = createTestSessionView();
    if (!sessionView) return false;

    // Wait for initialization
    waitForStability(*sessionView, config_.warmupFrames);

    // Capture image
    juce::Image baselineImage = captureComponentImage(*sessionView, config_.maxImageSize, config_.maxImageSize);
    if (baselineImage.isNull()) return false;

    // Save baseline
    juce::File baselineFile(baselineDir_.getChildFile(testName + "_baseline.png"));
    juce::PNGImageFormat pngFormat;
    bool success = pngFormat.writeToFile(baselineImage, baselineFile);

    delete sessionView;
    return success;
}

bool VisualTestRunner::updateBaseline(const std::string& testName) {
    // Run the test to get current image
    auto result = runSingleTest(testName);
    if (!result.currentImage.isValid()) return false;

    // Save as new baseline
    juce::File baselineFile(baselineDir_.getChildFile(testName + "_baseline.png"));
    juce::PNGImageFormat pngFormat;
    bool success = pngFormat.writeToFile(result.currentImage, baselineFile);

    return success;
}

bool VisualTestRunner::restoreBaseline(const std::string& testName) {
    // Remove current baseline to force regeneration
    juce::File baselineFile(baselineDir_.getChildFile(testName + "_baseline.png"));
    if (baselineFile.exists()) {
        baselineFile.deleteFile();
    }

    // Create new baseline
    return createBaseline(testName);
}

bool VisualTestRunner::deleteBaseline(const std::string& testName) {
    juce::File baselineFile(baselineDir_.getChildFile(testName + "_baseline.png"));
    return baselineFile.deleteFile();
}

std::vector<std::string> VisualTestRunner::getAvailableBaselines() const {
    std::vector<std::string> baselines;
    for (const auto& file : baselineDir_.findChildFiles(juce::File::findFiles, false, "*.png")) {
        juce::String name = file.getFileNameWithoutExtension();
        if (name.endsWith("_baseline")) {
            name = name.substring(0, name.length() - 9); // Remove "_baseline"
            baselines.push_back(name.toStdString());
        }
    }
    return baselines;
}

//==============================================================================
// Static Utility Methods
//==============================================================================

juce::Image VisualTestRunner::captureComponentImage(juce::Component& component, int width, int height) {
    // Calculate scaled dimensions
    int scaledWidth = std::min(width, component.getWidth());
    int scaledHeight = std::min(height, component.getHeight());

    if (scaledWidth <= 0 || scaledHeight <= 0) {
        return juce::Image();
    }

    // Create image
    juce::Image image(juce::Image::ARGB, scaledWidth, scaledHeight, true);
    juce::Graphics g(image);

    // Render component
    g.reduceClipRegion(juce::Rectangle<int>(0, 0, scaledWidth, scaledHeight));
    component.paintEntireComponent(g, true);

    return image;
}

juce::Image VisualTestRunner::compareImages(const juce::Image& baseline, const juce::Image& current) {
    if (baseline.isNull() || current.isNull()) {
        return juce::Image();
    }

    // Create difference image
    int width = std::min(baseline.getWidth(), current.getWidth());
    int height = std::min(baseline.getHeight(), current.getHeight());

    juce::Image diffImage(juce::Image::ARGB, width, height, true);
    juce::Graphics g(diffImage);

    // Compare pixels
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            juce::Colour baselinePixel = baseline.getPixelAt(x, y);
            juce::Colour currentPixel = current.getPixelAt(x, y);

            if (baselinePixel != currentPixel) {
                // Highlight differences in red
                g.setColour(juce::Colours::red);
                g.fillEllipse((float)x - 1, (float)y - 1, 3, 3);
            }
        }
    }

    return diffImage;
}

float VisualTestRunner::calculateDifferenceScore(const juce::Image& baseline, const juce::Image& current) {
    if (baseline.isNull() || current.isNull()) {
        return 1.0f;
    }

    int width = std::min(baseline.getWidth(), current.getWidth());
    int height = std::min(baseline.getHeight(), current.getHeight());
    int totalPixels = width * height;
    int differingPixels = 0;

    // Count differing pixels
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            juce::Colour baselinePixel = baseline.getPixelAt(x, y);
            juce::Colour currentPixel = current.getPixelAt(x, y);

            if (baselinePixel != currentPixel) {
                differingPixels++;
            }
        }
    }

    // Return normalized difference score
    return (float)differingPixels / totalPixels;
}

juce::Image VisualTestRunner::highlightDifferences(const juce::Image& baseline, const juce::Image& current) {
    if (baseline.isNull() || current.isNull()) {
        return juce::Image();
    }

    // Create comparison image with baseline and current side by side
    int width = baseline.getWidth() + current.getWidth() + 10;
    int height = std::max(baseline.getHeight(), current.getHeight());

    juce::Image resultImage(juce::Image::ARGB, width, height, true);
    juce::Graphics g(resultImage);

    // Draw baseline (left)
    g.setColour(juce::Colours::white);
    g.drawText("Baseline", 5, 5, 100, 20, juce::Justification::left);
    g.drawImage(baseline, 5, 30, baseline.getWidth(), baseline.getHeight(), 0, 0, baseline.getWidth(), baseline.getHeight());

    // Draw current (right)
    g.setColour(juce::Colours::white);
    g.drawText("Current", baseline.getWidth() + 15, 5, 100, 20, juce::Justification::left);
    g.drawImage(current, baseline.getWidth() + 15, 30, current.getWidth(), current.getHeight(), 0, 0, current.getWidth(), current.getHeight());

    // Draw divider
    g.setColour(juce::Colours::grey);
    g.drawLine(baseline.getWidth() + 5, 25, baseline.getWidth() + 5, height - 5, 2);

    return resultImage;
}

//==============================================================================
// Private Test Implementations
//==============================================================================

TestResult VisualTestRunner::testBasicLayout() {
    TestResult result;
    result.testName = "basic_layout";

    auto sessionView = createTestSessionView();
    if (!sessionView) {
        result.passed = false;
        result.message = "Failed to create test session view";
        return result;
    }

    // Test with minimal data
    std::vector<SessionTrackData> tracks;
    std::vector<SceneData> scenes;

    SessionTrackData track;
    track.name = "Test Track";
    track.color = juce::Colours::orange;
    track.clips.resize(4);
    for (int i = 0; i < 4; ++i) {
        track.clips[i].state = ClipSlotState::Empty;
        track.clips[i].name = "Clip " + juce::String(i + 1);
    }
    tracks.push_back(track);

    SceneData scene;
    scene.name = "Scene 1";
    scenes.push_back(scene);

    sessionView->setTracks(tracks);
    sessionView->setScenes(scenes);

    // Wait for rendering
    waitForStability(*sessionView, config_.warmupFrames);

    // Capture current image
    juce::Image currentImage = captureComponentImage(*sessionView, config_.maxImageSize, config_.maxImageSize);
    if (currentImage.isNull()) {
        result.passed = false;
        result.message = "Failed to capture test image";
        delete sessionView;
        return result;
    }

    // Load or create baseline
    juce::File baselineFile(baselineDir_.getChildFile("basic_layout_baseline.png"));
    juce::Image baselineImage;

    if (baselineFile.exists()) {
        baselineImage = juce::ImageCache::getFromFile(baselineFile);
    } else {
        baselineImage = currentImage;
        juce::PNGImageFormat pngFormat;
        pngFormat.writeToFile(baselineImage, baselineFile);
    }

    // Compare images
    result.differenceScore = calculateDifferenceScore(baselineImage, currentImage);
    result.baselineImage = baselineImage;
    result.currentImage = currentImage;
    result.diffImage = compareImages(baselineImage, currentImage);

    if (result.differenceScore <= config_.toleranceThreshold) {
        result.passed = true;
        result.message = "Layout test passed with difference score: " + juce::String(result.differenceScore, 4);
    } else {
        result.passed = false;
        result.message = "Layout test failed - difference score exceeds threshold: " + juce::String(result.differenceScore, 4);
    }

    delete sessionView;
    return result;
}

TestResult VisualTestRunner::testColorScheme() {
    TestResult result;
    result.testName = "color_scheme";

    auto sessionView = createTestSessionView();
    if (!sessionView) {
        result.passed = false;
        result.message = "Failed to create test session view";
        return result;
    }

    // Test with color variations
    std::vector<SessionTrackData> tracks;
    std::vector<SceneData> scenes;

    // Create tracks with different colors
    juce::Array<juce::Colour> testColors = {
        juce::Colours::red,
        juce::Colours::green,
        juce::Colours::blue,
        juce::Colours::yellow,
        juce::Colours::purple,
        juce::Colours::cyan
    };

    for (int i = 0; i < testColors.size(); ++i) {
        SessionTrackData track;
        track.name = "Track " + juce::String(i + 1);
        track.color = testColors[i];
        track.clips.resize(2);
        for (int j = 0; j < 2; ++j) {
            track.clips[j].state = ClipSlotState::Stopped;
            track.clips[j].name = "Clip " + juce::String(j + 1);
            track.clips[j].color = testColors[i];
        }
        tracks.push_back(track);
    }

    SceneData scene;
    scene.name = "Test Scene";
    scenes.push_back(scene);

    sessionView->setTracks(tracks);
    sessionView->setScenes(scenes);

    // Wait for rendering
    waitForStability(*sessionView, config_.warmupFrames);

    // Capture current image
    juce::Image currentImage = captureComponentImage(*sessionView, config_.maxImageSize, config_.maxImageSize);
    if (currentImage.isNull()) {
        result.passed = false;
        result.message = "Failed to capture test image";
        delete sessionView;
        return result;
    }

    // Load or create baseline
    juce::File baselineFile(baselineDir_.getChildFile("color_scheme_baseline.png"));
    juce::Image baselineImage;

    if (baselineFile.exists()) {
        baselineImage = juce::ImageCache::getFromFile(baselineFile);
    } else {
        baselineImage = currentImage;
        juce::PNGImageFormat pngFormat;
        pngFormat.writeToFile(baselineImage, baselineFile);
    }

    // Compare images
    result.differenceScore = calculateDifferenceScore(baselineImage, currentImage);
    result.baselineImage = baselineImage;
    result.currentImage = currentImage;
    result.diffImage = compareImages(baselineImage, currentImage);

    if (result.differenceScore <= config_.toleranceThreshold) {
        result.passed = true;
        result.message = "Color scheme test passed with difference score: " + juce::String(result.differenceScore, 4);
    } else {
        result.passed = false;
        result.message = "Color scheme test failed - difference score exceeds threshold: " + juce::String(result.differenceScore, 4);
    }

    delete sessionView;
    return result;
}

TestResult VisualTestRunner::testAnimationEffects() {
    TestResult result;
    result.testName = "animation_effects";

    auto sessionView = createTestSessionView();
    if (!sessionView) {
        result.passed = false;
        result.message = "Failed to create test session view";
        return result;
    }

    // Test with animated states
    std::vector<SessionTrackData> tracks;
    std::vector<SceneData> scenes;

    SessionTrackData track;
    track.name = "Animation Track";
    track.color = juce::Colours::orange;
    track.clips.resize(3);

    // Different animation states
    track.clips[0].state = ClipSlotState::Playing;
    track.clips[0].name = "Playing";
    track.clips[0].playProgress = 0.5f;

    track.clips[1].state = ClipSlotState::Queued;
    track.clips[1].name = "Queued";

    track.clips[2].state = ClipSlotState::Recording;
    track.clips[2].name = "Recording";

    tracks.push_back(track);

    SceneData scene;
    scene.name = "Animation Scene";
    scene.isPlaying = true;
    scenes.push_back(scene);

    sessionView->setTracks(tracks);
    sessionView->setScenes(scenes);

    // Wait longer for animation effects
    waitForStability(*sessionView, config_.warmupFrames * 2);

    // Capture multiple frames and average
    juce::Image totalImage;
    for (int i = 0; i < config_.captureFrames; ++i) {
        auto frameImage = captureComponentImage(*sessionView, config_.maxImageSize, config_.maxImageSize);
        if (!frameImage.isNull()) {
            if (totalImage.isNull()) {
                totalImage = frameImage;
            } else {
                // Simple averaging - in production use more sophisticated methods
                totalImage = totalImage.createCopy();
                for (int y = 0; y < frameImage.getHeight(); ++y) {
                    for (int x = 0; x < frameImage.getWidth(); ++x) {
                        juce::Colour current = frameImage.getPixelAt(x, y);
                        juce::Colour total = totalImage.getPixelAt(x, y);
                        juce::Colour averaged(
                            (current.getRed() + total.getRed()) / 2,
                            (current.getGreen() + total.getGreen()) / 2,
                            (current.getBlue() + total.getBlue()) / 2,
                            (current.getAlpha() + total.getAlpha()) / 2
                        );
                        totalImage.setPixelAt(x, y, averaged);
                    }
                }
            }
        }

        // Wait for next frame
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60fps
    }

    if (totalImage.isNull()) {
        result.passed = false;
        result.message = "Failed to capture animation frames";
        delete sessionView;
        return result;
    }

    // Load or create baseline
    juce::File baselineFile(baselineDir_.getChildFile("animation_effects_baseline.png"));
    juce::Image baselineImage;

    if (baselineFile.exists()) {
        baselineImage = juce::ImageCache::getFromFile(baselineFile);
    } else {
        baselineImage = totalImage;
        juce::PNGImageFormat pngFormat;
        pngFormat.writeToFile(baselineImage, baselineFile);
    }

    // Compare images
    result.differenceScore = calculateDifferenceScore(baselineImage, totalImage);
    result.baselineImage = baselineImage;
    result.currentImage = totalImage;
    result.diffImage = compareImages(baselineImage, totalImage);

    if (result.differenceScore <= config_.toleranceThreshold) {
        result.passed = true;
        result.message = "Animation effects test passed with difference score: " + juce::String(result.differenceScore, 4);
    } else {
        result.passed = false;
        result.message = "Animation effects test failed - difference score exceeds threshold: " + juce::String(result.differenceScore, 4);
    }

    delete sessionView;
    return result;
}

TestResult VisualTestRunner::testErrorHandling() {
    TestResult result;
    result.testName = "error_handling";

    auto sessionView = createTestSessionView();
    if (!sessionView) {
        result.passed = false;
        result.message = "Failed to create test session view";
        return result;
    }

    // Test error state
    std::vector<SessionTrackData> tracks;
    std::vector<SceneData> scenes;

    SessionTrackData track;
    track.name = "Error Test Track";
    track.color = juce::Colours::red;
    track.clips.resize(1);
    track.clips[0].state = ClipSlotState::Recording; // Error-prone state
    track.clips[0].name = "Error Clip";
    tracks.push_back(track);

    SceneData scene;
    scene.name = "Error Scene";
    scenes.push_back(scene);

    sessionView->setTracks(tracks);
    sessionView->setScenes(scenes);

    // Wait for rendering
    waitForStability(*sessionView, config_.warmupFrames);

    // Capture image
    juce::Image currentImage = captureComponentImage(*sessionView, config_.maxImageSize, config_.maxImageSize);
    if (currentImage.isNull()) {
        result.passed = false;
        result.message = "Failed to capture test image";
        delete sessionView;
        return result;
    }

    // Load or create baseline
    juce::File baselineFile(baselineDir_.getChildFile("error_handling_baseline.png"));
    juce::Image baselineImage;

    if (baselineFile.exists()) {
        baselineImage = juce::ImageCache::getFromFile(baselineFile);
    } else {
        baselineImage = currentImage;
        juce::PNGImageFormat pngFormat;
        pngFormat.writeToFile(baselineImage, baselineFile);
    }

    // Compare images
    result.differenceScore = calculateDifferenceScore(baselineImage, currentImage);
    result.baselineImage = baselineImage;
    result.currentImage = currentImage;
    result.diffImage = compareImages(baselineImage, currentImage);

    if (result.differenceScore <= config_.toleranceThreshold) {
        result.passed = true;
        result.message = "Error handling test passed with difference score: " + juce::String(result.differenceScore, 4);
    } else {
        result.passed = false;
        result.message = "Error handling test failed - difference score exceeds threshold: " + juce::String(result.differenceScore, 4);
    }

    delete sessionView;
    return result;
}

TestResult VisualTestRunner::testPerformance() {
    TestResult result;
    result.testName = "performance";

    auto sessionView = createTestSessionView();
    if (!sessionView) {
        result.passed = false;
        result.message = "Failed to create test session view";
        return result;
    }

    // Populate with test data
    populateTestSessionView(*sessionView);

    // Test rendering performance
    auto startTime = std::chrono::high_resolution_clock::now();

    // Trigger multiple redraws
    for (int i = 0; i < 10; ++i) {
        sessionView->repaint();
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // Wait for frame
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    // Performance score (lower is better)
    float performanceScore = static_cast<float>(duration.count()) / 10.0f; // ms per frame

    // Test passes if rendering takes less than 16.67ms (60fps)
    result.passed = performanceScore <= 16.67f;
    result.message = "Average frame time: " + juce::String(performanceScore, 2) + "ms";

    // Capture final state for visual inspection
    juce::Image currentImage = captureComponentImage(*sessionView, config_.maxImageSize, config_.maxImageSize);
    if (!currentImage.isNull()) {
        juce::File baselineFile(baselineDir_.getChildFile("performance_baseline.png"));
        if (baselineFile.exists()) {
            auto baselineImage = juce::ImageCache::getFromFile(baselineFile);
            result.differenceScore = calculateDifferenceScore(baselineImage, currentImage);
        } else {
            result.differenceScore = 0.0f; // No baseline for performance test
        }
        result.currentImage = currentImage;
    }

    delete sessionView;
    return result;
}

TestResult VisualTestRunner::testResponsiveLayout() {
    TestResult result;
    result.testName = "responsive_layout";

    auto sessionView = createTestSessionView();
    if (!sessionView) {
        result.passed = false;
        result.message = "Failed to create test session view";
        return result;
    }

    // Test with different window sizes
    std::vector<std::pair<int, int>> testSizes = {
        {800, 600},   // Small
        {1024, 768},  // Medium
        {1920, 1080}, // Large
        {3840, 2160}  // Very large
    };

    int passedSizes = 0;
    std::vector<juce::Image> capturedImages;

    for (const auto& size : testSizes) {
        // Resize component
        sessionView->setSize(size.first, size.second);

        // Populate test data
        populateTestSessionView(*sessionView);

        // Wait for rendering
        waitForStability(*sessionView, config_.warmupFrames);

        // Capture image
        auto image = captureComponentImage(*sessionView, config_.maxImageSize, config_.maxImageSize);
        if (!image.isNull()) {
            capturedImages.push_back(image);
            passedSizes++;
        }
    }

    result.passed = (passedSizes == testSizes.size());
    result.message = juce::String(passedSizes) + " / " + juce::String(testSizes.size()) + " sizes passed";

    // Create comparison montage
    if (capturedImages.size() > 1) {
        int totalWidth = 0;
        int maxHeight = 0;
        for (const auto& image : capturedImages) {
            totalWidth += image.getWidth() + 10;
            maxHeight = std::max(maxHeight, image.getHeight());
        }

        juce::Image montage(juce::Image::ARGB, totalWidth, maxHeight, true);
        juce::Graphics g(montage);

        int x = 0;
        for (size_t i = 0; i < capturedImages.size(); ++i) {
            g.drawImage(capturedImages[i], x, 0, capturedImages[i].getWidth(), capturedImages[i].getHeight(), 0, 0, capturedImages[i].getWidth(), capturedImages[i].getHeight());
            x += capturedImages[i].getWidth() + 10;

            // Add size label
            g.setColour(juce::Colours::white);
            g.drawText(std::to_string(i + 1), x - 15, 5, 10, 20, juce::Justification::centred);
        }

        result.diffImage = montage;
    }

    delete sessionView;
    return result;
}

TestResult VisualTestRunner::testThemeSwitching() {
    TestResult result;
    result.testName = "theme_switching";

    auto sessionView = createTestSessionView();
    if (!sessionView) {
        result.passed = false;
        result.message = "Failed to create test session view";
        return result;
    }

    // Test multiple themes
    std::vector<ViewTheme::Theme::BuiltIn> themes = {
        ViewTheme::Theme::BuiltIn::NeonNoir,
        ViewTheme::Theme::BuiltIn::DarkModern,
        ViewTheme::Theme::BuiltIn::OLED
    };

    int passedThemes = 0;
    std::vector<juce::Image> themeImages;

    for (const auto& theme : themes) {
        // Apply theme
        auto& themeManager = ViewTheme::getThemeManager();
        themeManager.applyBuiltInTheme(theme);

        // Populate test data
        populateTestSessionView(*sessionView);

        // Wait for rendering
        waitForStability(*sessionView, config_.warmupFrames);

        // Capture image
        auto image = captureComponentImage(*sessionView, config_.maxImageSize, config_.maxImageSize);
        if (!image.isNull()) {
            themeImages.push_back(image);
            passedThemes++;
        }
    }

    result.passed = (passedThemes == themes.size());
    result.message = juce::String(passedThemes) + " / " + juce::String(themes.size()) + " themes passed";

    // Create comparison montage
    if (themeImages.size() > 1) {
        int totalWidth = 0;
        int maxHeight = 0;
        for (const auto& image : themeImages) {
            totalWidth += image.getWidth() + 10;
            maxHeight = std::max(maxHeight, image.getHeight());
        }

        juce::Image montage(juce::Image::ARGB, totalWidth, maxHeight, true);
        juce::Graphics g(montage);

        int x = 0;
        for (size_t i = 0; i < themeImages.size(); ++i) {
            g.drawImage(themeImages[i], x, 0, themeImages[i].getWidth(), themeImages[i].getHeight(), 0, 0, themeImages[i].getWidth(), themeImages[i].getHeight());
            x += themeImages[i].getWidth() + 10;

            // Add theme label
            g.setColour(juce::Colours::white);
            g.drawText(std::to_string(i + 1), x - 15, 5, 10, 20, juce::Justification::centred);
        }

        result.diffImage = montage;
    }

    delete sessionView;
    return result;
}

//==============================================================================
// Private Helper Methods
//==============================================================================

void VisualTestRunner::setupDirectories() {
    baselineDir_ = juce::File::getCurrentWorkingDirectory().getChildFile(config_.baselineDirectory);
    resultsDir_ = juce::File::getCurrentWorkingDirectory().getChildFile(config_.resultsDirectory);
    diffDir_ = juce::File::getCurrentWorkingDirectory().getChildFile(config_.diffDirectory);

    baselineDir_.createDirectory();
    resultsDir_.createDirectory();
    diffDir_.createDirectory();
}

void VisualTestRunner::categorizeTest(const std::string& name, TestCategory category) {
    testCategories_[name] = category;
}

TestResult VisualTestRunner::runTimedTest(const std::string& name, std::function<TestResult()> test) {
    auto startTime = std::chrono::high_resolution_clock::now();

    TestResult result = test();

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    result.message += " (Execution time: " + juce::String(duration.count()) + "ms)";

    return result;
}

void VisualTestRunner::waitForStability(juce::Component& component, int frames) {
    for (int i = 0; i < frames; ++i) {
        component.repaint();
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // Wait for frame
    }
}

void VisualTestRunner::captureTestImage(juce::Component& component, juce::Image& outImage) {
    outImage = captureComponentImage(component, config_.maxImageSize, config_.maxImageSize);
}

SkiaSessionView* VisualTestRunner::createTestSessionView() {
    try {
        auto* view = new SkiaSessionView();
        view->setSize(800, 600); // Default size
        view->setVisible(true);
        return view;
    } catch (const std::exception& e) {
        juce::Logger::writeToLog("Error creating test session view: " + juce::String(e.what()));
        return nullptr;
    }
}

void VisualTestRunner::populateTestSessionView(SkiaSessionView& view) {
    std::vector<SessionTrackData> tracks;
    std::vector<SceneData> scenes;

    // Create test tracks
    for (int i = 0; i < 8; ++i) {
        SessionTrackData track;
        track.name = "Track " + juce::String(i + 1);
        track.color = juce::Colours::fromHSV(i * 45.0f, 0.7f, 0.9f);
        track.clips.resize(4);

        for (int j = 0; j < 4; ++j) {
            track.clips[j].name = "Clip " + juce::String(j + 1);
            track.clips[j].color = track.color;

            // Mix states
            if (i == 0 && j == 0) track.clips[j].state = ClipSlotState::Playing;
            else if (i == 1 && j == 0) track.clips[j].state = ClipSlotState::Queued;
            else if (i == 2 && j == 0) track.clips[j].state = ClipSlotState::Recording;
            else track.clips[j].state = ClipSlotState::Stopped;
        }

        tracks.push_back(track);
    }

    // Create test scenes
    for (int i = 0; i < 4; ++i) {
        SceneData scene;
        scene.name = "Scene " + juce::String(i + 1);
        scenes.push_back(scene);
    }

    view.setTracks(tracks);
    view.setScenes(scenes);
}

//==============================================================================
// Console Application
//==============================================================================

void VisualTestConsole::run(int argc, char* argv[]) {
    VisualTestConfig config = VisualTestConfig::createDefault();

    if (!parseArguments(argc, argv, config)) {
        showHelp();
        return;
    }

    VisualTestRunner runner;
    runner.setConfig(config);

    // Run tests
    auto results = runner.runAllTests();

    // Generate and display report
    juce::String report = runner.generateReport();
    juce::Logger::writeToLog(report);

    // Save results
    runner.saveResults();
}

void VisualTestConsole::showHelp() {
    juce::Logger::writeToLog(generateHelpText());
}

bool VisualTestConsole::parseArguments(int argc, char* argv[], VisualTestConfig& config) {
    for (int i = 1; i < argc; ++i) {
        juce::String arg(argv[i]);

        if (arg == "--help" || arg == "-h") {
            return false;
        }
        else if (arg == "--update-baselines") {
            config.updateBaselines = true;
        }
        else if (arg == "--verbose") {
            config.verboseOutput = true;
        }
        else if (arg.startsWith("--tolerance=")) {
            float tolerance = arg.fromFirstOccurrenceOf("=", false, false).getFloatValue();
            config.toleranceThreshold = tolerance;
        }
        else if (arg.startsWith("--baseline-dir=")) {
            config.baselineDirectory = arg.fromFirstOccurrenceOf("=", false, false);
        }
        else if (arg.startsWith("--results-dir=")) {
            config.resultsDirectory = arg.fromFirstOccurrenceOf("=", false, false);
        }
        else if (arg.startsWith("--test=")) {
            config.selectedTests.push_back(arg.fromFirstOccurrenceOf("=", false, false));
        }
        else if (arg.startsWith("--exclude=")) {
            config.excludedTests.push_back(arg.fromFirstOccurrenceOf("=", false, false));
        }
        else if (arg == "--no-layout") {
            config.runLayoutTests = false;
        }
        else if (arg == "--no-color") {
            config.runColorTests = false;
        }
        else if (arg == "--no-animation") {
            config.runAnimationTests = false;
        }
        else if (arg == "--no-error-handling") {
            config.runErrorHandlingTests = false;
        }
        else if (arg == "--no-performance") {
            config.runPerformanceTests = false;
        }
        else {
            // Unknown argument
            juce::Logger::writeToLog("Warning: Unknown argument: " + arg);
        }
    }

    return true;
}

juce::String VisualTestConsole::generateHelpText() {
    juce::String help;
    help << "Zenith DAW - Visual Regression Test Tool\n\n";
    help << "Usage: visual_test [options]\n\n";
    help << "Options:\n";
    help << "  --help, -h                Show this help message\n";
    help << "  --update-baselines        Update baseline images (requires manual review)\n";
    help << "  --verbose                 Enable verbose output\n";
    help << "  --tolerance=<value>       Set tolerance threshold (0.0-1.0, default: 0.05)\n";
    help << "  --baseline-dir=<path>     Set baseline directory\n";
    help << "  --results-dir=<path>      Set results directory\n";
    help << "  --test=<name>             Run specific test (can be used multiple times)\n";
    help << "  --exclude=<name>          Exclude specific test\n";
    help << "  --no-layout               Skip layout tests\n";
    help << "  --no-color                Skip color tests\n";
    help << "  --no-animation            Skip animation tests\n";
    help << "  --no-error-handling       Skip error handling tests\n";
    help << "  --no-performance          Skip performance tests\n\n";
    help << "Examples:\n";
    help << "  visual_test --verbose\n";
    help << "  visual_test --test=basic_layout --test=color_scheme\n";
    help << "  visual_test --tolerance=0.1 --no-performance\n";
    help << "  visual_test --update-baselines --baseline-dir=custom_baselines\n";

    return help;
}

} // namespace zenith::ui::testing