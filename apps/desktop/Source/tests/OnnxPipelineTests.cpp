/**
 * @file OnnxPipelineTests.cpp
 * @brief Unit tests for ONNX stem separation pipeline
 * 
 * Tests cover:
 * 1. Model loading with valid/invalid files
 * 2. Corruption/size validation handling
 * 3. Inference pipeline verification (single pass)
 * 4. DSP fallback behavior verification
 */

#include "../dsp/ONNXStemSeparator.h"
#include "../dsp/DSPStemSeparator.h"
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

namespace zenith {
namespace tests {

class OnnxPipelineTests : public juce::UnitTest {
public:
    OnnxPipelineTests() : juce::UnitTest("ONNX Pipeline", "DSP") {}

    void runTest() override {
        beginTest("Missing Model File Handling");
        runMissingModelTest();
        
        beginTest("Corrupt/Small Model File Handling");
        runCorruptModelTest();
        
        beginTest("DSP Fallback Verification");
        runDSPFallbackTest();
        
        beginTest("Single Inference Pass (No Crash)");
        runInferencePipelineTest();
        
        beginTest("Model Discovery");
        runModelDiscoveryTest();
    }

private:
    /**
     * Test that attempting to load a missing model file returns false
     * and doesn't crash.
     */
    void runMissingModelTest() {
        ONNXStemSeparator separator;
        
        // Try to initialize with a non-existent file
        juce::File nonExistent("/this/path/does/not/exist/model.onnx");
        bool result = separator.initialize(nonExistent);
        
        expect(!result, "Initialize should return false for missing file");
        expect(!separator.isAvailable() || result == false, 
               "Separator should handle missing file gracefully");
        
        // Try with empty file path
        juce::File emptyPath;
        result = separator.initialize(emptyPath);
        // May return true if it finds a default model, or false if not
        // Either way, shouldn't crash
        DBG("OnnxPipelineTests: Empty path init result = " + juce::String(result ? "true" : "false"));
    }
    
    /**
     * Test that corrupt or too-small files are rejected.
     */
    void runCorruptModelTest() {
        ONNXStemSeparator separator;
        
        // Create a temp directory for test files
        juce::File tempDir = juce::File::getSpecialLocation(
            juce::File::tempDirectory).getChildFile("zenith_onnx_test");
        tempDir.createDirectory();
        
        // Test 1: Empty file (0 bytes)
        juce::File emptyFile = tempDir.getChildFile("empty.onnx");
        emptyFile.replaceWithText("");
        
        bool result = separator.initialize(emptyFile);
        expect(!result, "Initialize should reject empty file");
        
        // Test 2: File too small (under 100KB threshold)
        juce::File smallFile = tempDir.getChildFile("small.onnx");
        {
            juce::MemoryBlock data(1024); // 1KB
            data.fillWith(0x08); // Fill with valid-ish protobuf byte
            smallFile.replaceWithData(data.getData(), data.getSize());
        }
        
        result = separator.initialize(smallFile);
        expect(!result, "Initialize should reject file under 100KB");
        
        // Test 3: File with wrong header (not ONNX format)
        juce::File wrongFormat = tempDir.getChildFile("notonnx.onnx");
        {
            juce::MemoryBlock data(150 * 1024); // 150KB (passes size check)
            // Fill with zeros - invalid protobuf header
            data.fillWith(0x00);
            wrongFormat.replaceWithData(data.getData(), data.getSize());
        }
        
        result = separator.initialize(wrongFormat);
        // This may or may not fail depending on ONNX Runtime being compiled in
        // At minimum it should not crash
        DBG("OnnxPipelineTests: Wrong format file result = " + juce::String(result ? "true" : "false"));
        
        // Cleanup
        emptyFile.deleteFile();
        smallFile.deleteFile();
        wrongFormat.deleteFile();
        tempDir.deleteRecursively();
    }
    
    /**
     * Verify that DSP fallback produces valid output when ONNX is unavailable.
     */
    void runDSPFallbackTest() {
        DSPStemSeparator dsp;
        
        // Prepare the DSP processor
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = 44100.0;
        spec.maximumBlockSize = 4410; // 100ms
        spec.numChannels = 2;
        dsp.prepare(spec);
        
        // Create stereo test signal (sine wave)
        juce::AudioBuffer<float> input(2, 4410);
        juce::AudioBuffer<float> output(2, 4410);
        
        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < 4410; ++i) {
                // 440Hz sine wave with some stereo offset
                float sample = std::sin(2.0f * juce::MathConstants<float>::pi * 440.0f * i / 44100.0f);
                if (ch == 1) sample *= 0.8f; // Slight stereo difference
                input.setSample(ch, i, sample * 0.5f);
            }
        }
        
        juce::dsp::AudioBlock<const float> inputBlock(input);
        juce::dsp::AudioBlock<float> outputBlock(output);
        
        // Test each stem type
        for (auto stemType : {DSPStemSeparator::StemType::Vocals,
                              DSPStemSeparator::StemType::Drums,
                              DSPStemSeparator::StemType::Bass,
                              DSPStemSeparator::StemType::Other}) {
            output.clear();
            dsp.process(inputBlock, outputBlock, stemType);
            
            // Verify output is not all zeros (separation produced something)
            float maxVal = output.getMagnitude(0, output.getNumSamples());
            expect(maxVal > 0.0f, "DSP stem separation should produce non-zero output");
            
            // Verify no NaN or Inf values
            bool hasInvalid = false;
            for (int ch = 0; ch < 2; ++ch) {
                for (int i = 0; i < output.getNumSamples(); ++i) {
                    float sample = output.getSample(ch, i);
                    if (std::isnan(sample) || std::isinf(sample)) {
                        hasInvalid = true;
                        break;
                    }
                }
            }
            expect(!hasInvalid, "DSP output should not contain NaN or Inf values");
        }
    }
    
    /**
     * Run a complete separation pass through ONNXStemSeparator.
     * This tests the full pipeline doesn't crash, regardless of whether
     * ONNX Runtime is available or DSP fallback is used.
     */
    void runInferencePipelineTest() {
        ONNXStemSeparator separator;
        
        // Try to find and load default model (may not exist)
        juce::File modelFile = ONNXStemSeparator::findDefaultModel();
        if (modelFile.existsAsFile()) {
            DBG("OnnxPipelineTests: Found model at " + modelFile.getFullPathName());
            separator.initialize(modelFile);
        } else {
            DBG("OnnxPipelineTests: No default model found, will use DSP fallback");
        }
        
        // Create test audio (1 second of stereo audio)
        juce::AudioBuffer<float> input(2, 44100);
        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < 44100; ++i) {
                // Mix of frequencies to simulate real audio
                float sample = std::sin(2.0f * juce::MathConstants<float>::pi * 440.0f * i / 44100.0f) * 0.3f;
                sample += std::sin(2.0f * juce::MathConstants<float>::pi * 100.0f * i / 44100.0f) * 0.4f; // Bass
                sample += std::sin(2.0f * juce::MathConstants<float>::pi * 2000.0f * i / 44100.0f) * 0.2f; // Treble
                input.setSample(ch, i, sample);
            }
        }
        
        // Run separation - this is the critical test
        auto result = separator.separate(input, 44100.0);
        
        // Verify result
        expect(result.success, "Separation should succeed (ONNX or DSP fallback)");
        
        // Check that all 4 stems have correct buffer sizes
        expectEquals(result.vocals.getNumSamples(), 44100);
        expectEquals(result.drums.getNumSamples(), 44100);
        expectEquals(result.bass.getNumSamples(), 44100);
        expectEquals(result.other.getNumSamples(), 44100);
        
        // Verify at least one stem has non-zero content
        bool anyNonZero = false;
        for (auto* buffer : {&result.vocals, &result.drums, &result.bass, &result.other}) {
            if (buffer->getMagnitude(0, buffer->getNumSamples()) > 0.001f) {
                anyNonZero = true;
                break;
            }
        }
        expect(anyNonZero, "At least one stem should have non-zero content");
        
        // Log which method was used
        DBG("OnnxPipelineTests: Separation used " + 
            juce::String(result.usedONNX ? "ONNX Runtime" : "DSP Fallback"));
    }
    
    /**
     * Test the model discovery mechanism.
     */
    void runModelDiscoveryTest() {
        // This test just verifies findDefaultModel() doesn't crash
        juce::File modelFile = ONNXStemSeparator::findDefaultModel();
        
        if (modelFile.existsAsFile()) {
            expect(modelFile.getFileName().endsWithIgnoreCase(".onnx"),
                   "Found model file should have .onnx extension");
            expect(modelFile.getSize() > 0, "Model file should not be empty");
            
            DBG("OnnxPipelineTests: Default model found: " + modelFile.getFullPathName());
            DBG("OnnxPipelineTests: Model size: " + juce::String(modelFile.getSize() / 1024) + " KB");
        } else {
            DBG("OnnxPipelineTests: No default model found (this is OK for testing)");
        }
        
        // Test getModelInfo on uninitialized separator
        ONNXStemSeparator separator;
        juce::String info = separator.getModelInfo();
        expect(info.isNotEmpty(), "getModelInfo should return something even when uninitialized");
    }
};

static OnnxPipelineTests onnxPipelineTests;

} // namespace tests
} // namespace zenith
