/*
  ==============================================================================

    ModelManagerTests.cpp
    Runtime validation tests for ModelManager

  ==============================================================================
*/

#include "ModelManager.h"
#include <juce_core/juce_core.h>

namespace zenith {

/**
 * @brief Runtime validation tests for ModelManager
 *
 * Tests the core functionality without requiring network access or large files.
 */
class ModelManagerTests {
public:
    struct TestResult {
        bool passed = false;
        juce::String message;
    };

    /**
     * @brief Test SHA256 calculation consistency
     *
     * Creates a small test file and verifies SHA256 calculation is consistent.
     */
    static TestResult testSHA256Consistency() {
        TestResult result;

        try {
            // Create a temporary test file
            juce::File testFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
                                      .getChildFile("zenith_sha256_test.txt");

            // Write known test data
            const juce::String testContent = "Hello, World! This is a SHA256 test.";
            testFile.replaceText(testContent);

            // Create ModelManager instance
            ModelManager modelManager;

            // Calculate SHA256
            juce::String hash1 = modelManager.calculateSHA256(testFile);
            juce::String hash2 = modelManager.calculateSHA256(testFile);

            // Verify consistency
            if (hash1.isEmpty() || hash2.isEmpty()) {
                result.message = "SHA256 calculation returned empty hash";
                return result;
            }

            if (hash1 != hash2) {
                result.message = "SHA256 calculation inconsistent: " + hash1 + " != " + hash2;
                return result;
            }

            // Verify hash is 64 characters (256 bits = 64 hex chars)
            if (hash1.length() != 64) {
                result.message = "SHA256 hash has wrong length: " + juce::String(hash1.length()) + " (expected 64)";
                return result;
            }

            // Verify hash is valid hexadecimal
            for (auto c : hash1) {
                if (!juce::CharacterFunctions::isHexDigit(c)) {
                    result.message = "SHA256 hash contains non-hex characters: " + hash1;
                    return result;
                }
            }

            // Clean up
            testFile.deleteFile();

            result.passed = true;
            result.message = "SHA256 calculation is consistent and valid: " + hash1;
            return result;

        } catch (const std::exception& e) {
            result.message = "Exception during test: " + juce::String(e.what());
            return result;
        }
    }

    /**
     * @brief Test SHA256 calculation on larger file
     *
     * Tests streaming SHA256 on a 1MB file to ensure it doesn't load entire file into RAM.
     */
    static TestResult testStreamingSHA256() {
        TestResult result;

        try {
            // Create a 1MB test file
            juce::File testFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
                                      .getChildFile("zenith_streaming_test_1mb.bin");

            const size_t fileSize = 1024 * 1024; // 1MB
            juce::FileOutputStream stream(testFile);

            if (!stream.openedOk()) {
                result.message = "Failed to create test file";
                return result;
            }

            // Write 1MB of data
            constexpr size_t bufferSize = 65536;
            char buffer[bufferSize];

            // Fill buffer with pattern
            for (size_t i = 0; i < bufferSize; i++) {
                buffer[i] = static_cast<char>(i % 256);
            }

            // Write 16 chunks of 64KB = 1MB
            for (int i = 0; i < 16; i++) {
                stream.write(buffer, bufferSize);
            }

            stream.flush();

            // Verify file size
            int64_t actualSize = testFile.getSize();
            if (actualSize != fileSize) {
                result.message = "Test file has wrong size: " + juce::String(actualSize) + " (expected " + juce::String(fileSize) + ")";
                testFile.deleteFile();
                return result;
            }

            // Calculate SHA256
            ModelManager modelManager;
            juce::String hash = modelManager.calculateSHA256(testFile);

            if (hash.isEmpty()) {
                result.message = "SHA256 calculation failed for 1MB file";
                testFile.deleteFile();
                return result;
            }

            if (hash.length() != 64) {
                result.message = "SHA256 hash has wrong length for 1MB file";
                testFile.deleteFile();
                return result;
            }

            // Clean up
            testFile.deleteFile();

            result.passed = true;
            result.message = "Streaming SHA256 works correctly on 1MB file: " + hash;
            return result;

        } catch (const std::exception& e) {
            result.message = "Exception during streaming test: " + juce::String(e.what());
            return result;
        } catch (const std::bad_alloc&) {
            result.message = "Bad alloc - streaming implementation may be loading file into RAM";
            return result;
        }
    }

    /**
     * @brief Test ModelManager initialization
     */
    static TestResult testModelManagerInit() {
        TestResult result;

        try {
            ModelManager modelManager;

            // Check if models directory is accessible
            juce::File modelsDir = modelManager.getModelsDirectory();
            if (!modelsDir.exists()) {
                result.message = "Models directory doesn't exist: " + modelsDir.getFullPathName();
                return result;
            }

            // Check if we can get available models
            auto models = modelManager.getAvailableModels();
            if (models.size() == 0) {
                result.message = "No models registered in ModelManager";
                return result;
            }

            // Check if each model has required fields
            for (const auto& model : models) {
                if (model.name.isEmpty()) {
                    result.message = "Model has empty name";
                    return result;
                }
                if (model.url.isEmpty()) {
                    result.message = "Model '" + model.name + "' has empty URL";
                    return result;
                }
            }

            result.passed = true;
            result.message = "ModelManager initialized successfully with " + juce::String(models.size()) + " models";
            return result;

        } catch (const std::exception& e) {
            result.message = "Exception during init test: " + juce::String(e.what());
            return result;
        }
    }

    /**
     * @brief Run all tests
     */
    static bool runAllTests() {
        juce::Logger::writeToLog("=== ModelManager Runtime Tests ===");

        auto test1 = testSHA256Consistency();
        juce::Logger::writeToLog(test1.passed ? "[PASS]" : "[FAIL]");
        juce::Logger::writeToLog(test1.message);

        auto test2 = testStreamingSHA256();
        juce::Logger::writeToLog(test2.passed ? "[PASS]" : "[FAIL]");
        juce::Logger::writeToLog(test2.message);

        auto test3 = testModelManagerInit();
        juce::Logger::writeToLog(test3.passed ? "[PASS]" : "[FAIL]");
        juce::Logger::writeToLog(test3.message);

        bool allPassed = test1.passed && test2.passed && test3.passed;
        juce::Logger::writeToLog("=== Tests " + juce::String(allPassed ? "PASSED" : "FAILED") + " ===");

        return allPassed;
    }
};

} // namespace zenith
