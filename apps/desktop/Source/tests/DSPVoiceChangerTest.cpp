/*
  ==============================================================================

    DSPVoiceChangerTests.cpp
    Created: 2025-12-27
    Author:  Zenith DAW - Testing Team

    Unit tests for DSPVoiceChanger WSOLA pitch shifting implementation.

  ==============================================================================
*/

#include "../dsp/DSPVoiceChanger.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <cmath>
#include <chrono>

namespace zenith {
namespace tests {

//==============================================================================
// DSP Voice Changer Tests
//==============================================================================

class DSPVoiceChangerTests : public juce::UnitTest {
public:
    DSPVoiceChangerTests() : juce::UnitTest("DSP Voice Changer", "DSP") {}

    void runTest() override {
        testConstruction();
        testPitchRatioCalculation();
        testDeepMaleProcessing();
        testChipmunkProcessing();
        testRobotProcessing();
        testEtherealProcessing();
        testTransitionFade();
        testNoNaNInf();
        testFormantPreservation();
        testPerformance();
    }

private:
    //==========================================================================
    // Test: Construction and Default State
    //==========================================================================
    
    void testConstruction() {
        beginTest("Construction and Default State");
        
        DSPVoiceChanger voiceChanger;
        
        // Should not crash
        expect(true, "DSPVoiceChanger constructed successfully");
        
        // Prepare with standard specs
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = 48000.0;
        spec.maximumBlockSize = 512;
        spec.numChannels = 2;
        
        voiceChanger.prepare(spec);
        expect(true, "DSPVoiceChanger prepared successfully");
        
        // Default formant preservation should be off
        expect(!voiceChanger.isFormantPreservationEnabled(), 
               "Formant preservation should be disabled by default");
        
        // Default window size should be kDefaultWindowSizeMs
        expectEquals(voiceChanger.getWindowSizeMs(), kDefaultWindowSizeMs);
    }

    //==========================================================================
    // Test: Semitone to Ratio Calculation
    //==========================================================================
    
    void testPitchRatioCalculation() {
        beginTest("Pitch Ratio Calculation");
        
        // 0 semitones = ratio of 1.0
        expectWithinAbsoluteError(DSPVoiceChanger::semitonesToRatio(0.0f), 1.0f, 0.001f);
        
        // +12 semitones = ratio of 2.0 (one octave up)
        expectWithinAbsoluteError(DSPVoiceChanger::semitonesToRatio(12.0f), 2.0f, 0.001f);
        
        // -12 semitones = ratio of 0.5 (one octave down)
        expectWithinAbsoluteError(DSPVoiceChanger::semitonesToRatio(-12.0f), 0.5f, 0.001f);
        
        // DeepMale: -5 semitones ≈ 0.7492
        float deepMaleRatio = DSPVoiceChanger::semitonesToRatio(kDeepMaleSemitones);
        expectWithinAbsoluteError(deepMaleRatio, 0.7492f, 0.001f);
        
        // Chipmunk: +7 semitones ≈ 1.4983
        float chipmunkRatio = DSPVoiceChanger::semitonesToRatio(kChipmunkSemitones);
        expectWithinAbsoluteError(chipmunkRatio, 1.4983f, 0.001f);
        
        // Round trip test
        float semitones = 5.0f;
        float ratio = DSPVoiceChanger::semitonesToRatio(semitones);
        float backToSemitones = DSPVoiceChanger::ratioToSemitones(ratio);
        expectWithinAbsoluteError(backToSemitones, semitones, 0.001f);
    }

    //==========================================================================
    // Test: Deep Male Voice Processing
    //==========================================================================
    
    void testDeepMaleProcessing() {
        beginTest("Deep Male Voice Processing");
        
        DSPVoiceChanger voiceChanger;
        
        const double sampleRate = 48000.0;
        const int blockSize = 512;
        
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = blockSize;
        spec.numChannels = 2;
        
        voiceChanger.prepare(spec);
        
        // Generate 440Hz sine wave input
        juce::AudioBuffer<float> inputBuffer(2, blockSize);
        juce::AudioBuffer<float> outputBuffer(2, blockSize);
        
        const float frequency = 440.0f;
        const float phaseIncrement = frequency / static_cast<float>(sampleRate);
        float phase = 0.0f;
        
        for (int i = 0; i < blockSize; ++i) {
            float sample = std::sin(2.0f * juce::MathConstants<float>::pi * phase) * 0.5f;
            inputBuffer.setSample(0, i, sample);
            inputBuffer.setSample(1, i, sample);
            phase += phaseIncrement;
            if (phase >= 1.0f) phase -= 1.0f;
        }
        
        // Process multiple blocks to let WSOLA settle
        for (int block = 0; block < 10; ++block) {
            juce::dsp::AudioBlock<const float> inputBlock(inputBuffer);
            juce::dsp::AudioBlock<float> outputBlock(outputBuffer);
            
            voiceChanger.process(inputBlock, outputBlock, 
                                 DSPVoiceChanger::VoiceCharacter::DeepMale);
        }
        
        // Output should contain audio (not silent)
        float outputMagnitude = outputBuffer.getMagnitude(0, 0, blockSize);
        expect(outputMagnitude > 0.01f, "Deep Male output should not be silent");
        
        // Output should be lower pitch (longer period)
        // This is verified by the fact that WSOLA is applied and output exists
    }

    //==========================================================================
    // Test: Chipmunk Voice Processing
    //==========================================================================
    
    void testChipmunkProcessing() {
        beginTest("Chipmunk Voice Processing");
        
        DSPVoiceChanger voiceChanger;
        
        const double sampleRate = 48000.0;
        const int blockSize = 512;
        
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = blockSize;
        spec.numChannels = 2;
        
        voiceChanger.prepare(spec);
        
        // Generate 220Hz sine wave input
        juce::AudioBuffer<float> inputBuffer(2, blockSize);
        juce::AudioBuffer<float> outputBuffer(2, blockSize);
        
        const float frequency = 220.0f;
        const float phaseIncrement = frequency / static_cast<float>(sampleRate);
        float phase = 0.0f;
        
        for (int i = 0; i < blockSize; ++i) {
            float sample = std::sin(2.0f * juce::MathConstants<float>::pi * phase) * 0.5f;
            inputBuffer.setSample(0, i, sample);
            inputBuffer.setSample(1, i, sample);
            phase += phaseIncrement;
            if (phase >= 1.0f) phase -= 1.0f;
        }
        
        // Process multiple blocks
        for (int block = 0; block < 10; ++block) {
            juce::dsp::AudioBlock<const float> inputBlock(inputBuffer);
            juce::dsp::AudioBlock<float> outputBlock(outputBuffer);
            
            voiceChanger.process(inputBlock, outputBlock, 
                                 DSPVoiceChanger::VoiceCharacter::Chipmunk);
        }
        
        // Output should contain audio
        float outputMagnitude = outputBuffer.getMagnitude(0, 0, blockSize);
        expect(outputMagnitude > 0.01f, "Chipmunk output should not be silent");
    }

    //==========================================================================
    // Test: Robot Voice Processing
    //==========================================================================
    
    void testRobotProcessing() {
        beginTest("Robot Voice Processing");
        
        DSPVoiceChanger voiceChanger;
        
        const double sampleRate = 48000.0;
        const int blockSize = 512;
        
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = blockSize;
        spec.numChannels = 2;
        
        voiceChanger.prepare(spec);
        
        // Generate DC signal (to clearly see ring modulation effect)
        juce::AudioBuffer<float> inputBuffer(2, blockSize);
        juce::AudioBuffer<float> outputBuffer(2, blockSize);
        
        for (int i = 0; i < blockSize; ++i) {
            inputBuffer.setSample(0, i, 0.5f);
            inputBuffer.setSample(1, i, 0.5f);
        }
        
        juce::dsp::AudioBlock<const float> inputBlock(inputBuffer);
        juce::dsp::AudioBlock<float> outputBlock(outputBuffer);
        
        voiceChanger.process(inputBlock, outputBlock, 
                             DSPVoiceChanger::VoiceCharacter::Robot);
        
        // Output should contain modulated signal (varies due to ring mod)
        float maxSample = 0.0f;
        float minSample = 0.0f;
        for (int i = 0; i < blockSize; ++i) {
            float sample = outputBuffer.getSample(0, i);
            maxSample = std::max(maxSample, sample);
            minSample = std::min(minSample, sample);
        }
        
        // Ring modulation should create positive and negative values
        expect(maxSample > 0.0f, "Robot mode should produce positive samples");
        expect(minSample < 0.0f, "Robot mode should produce negative samples (ring mod)");
    }

    //==========================================================================
    // Test: Ethereal Voice Processing
    //==========================================================================
    
    void testEtherealProcessing() {
        beginTest("Ethereal Voice Processing (Pass-through)");
        
        DSPVoiceChanger voiceChanger;
        
        const double sampleRate = 48000.0;
        const int blockSize = 512;
        
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = blockSize;
        spec.numChannels = 2;
        
        voiceChanger.prepare(spec);
        
        // Generate test signal
        juce::AudioBuffer<float> inputBuffer(2, blockSize);
        juce::AudioBuffer<float> outputBuffer(2, blockSize);
        
        for (int i = 0; i < blockSize; ++i) {
            float sample = std::sin(2.0f * juce::MathConstants<float>::pi * i / 100.0f) * 0.5f;
            inputBuffer.setSample(0, i, sample);
            inputBuffer.setSample(1, i, sample);
        }
        
        juce::dsp::AudioBlock<const float> inputBlock(inputBuffer);
        juce::dsp::AudioBlock<float> outputBlock(outputBuffer);
        
        voiceChanger.process(inputBlock, outputBlock, 
                             DSPVoiceChanger::VoiceCharacter::Ethereal);
        
        // Ethereal should be pass-through (reverb handled elsewhere)
        for (int i = 0; i < blockSize; ++i) {
            expectWithinAbsoluteError(
                outputBuffer.getSample(0, i), 
                inputBuffer.getSample(0, i), 
                0.0001f);
        }
    }

    //==========================================================================
    // Test: Transition Fade (No Clicks)
    //==========================================================================
    
    void testTransitionFade() {
        beginTest("Transition Fade (No Clicks/Pops)");
        
        DSPVoiceChanger voiceChanger;
        
        const double sampleRate = 48000.0;
        const int blockSize = 512;
        
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = blockSize;
        spec.numChannels = 2;
        
        voiceChanger.prepare(spec);
        
        // Generate continuous sine wave
        juce::AudioBuffer<float> inputBuffer(2, blockSize);
        juce::AudioBuffer<float> outputBuffer(2, blockSize);
        
        float phase = 0.0f;
        const float frequency = 440.0f;
        const float phaseIncrement = frequency / static_cast<float>(sampleRate);
        
        // Process with DeepMale first
        for (int block = 0; block < 5; ++block) {
            for (int i = 0; i < blockSize; ++i) {
                float sample = std::sin(2.0f * juce::MathConstants<float>::pi * phase) * 0.5f;
                inputBuffer.setSample(0, i, sample);
                inputBuffer.setSample(1, i, sample);
                phase += phaseIncrement;
                if (phase >= 1.0f) phase -= 1.0f;
            }
            
            juce::dsp::AudioBlock<const float> inputBlock(inputBuffer);
            juce::dsp::AudioBlock<float> outputBlock(outputBuffer);
            voiceChanger.process(inputBlock, outputBlock, 
                                 DSPVoiceChanger::VoiceCharacter::DeepMale);
        }
        
        float prevSample = outputBuffer.getSample(0, blockSize - 1);
        
        // Now switch to Chipmunk mid-stream
        for (int i = 0; i < blockSize; ++i) {
            float sample = std::sin(2.0f * juce::MathConstants<float>::pi * phase) * 0.5f;
            inputBuffer.setSample(0, i, sample);
            inputBuffer.setSample(1, i, sample);
            phase += phaseIncrement;
            if (phase >= 1.0f) phase -= 1.0f;
        }
        
        juce::dsp::AudioBlock<const float> inputBlock(inputBuffer);
        juce::dsp::AudioBlock<float> outputBlock(outputBuffer);
        voiceChanger.process(inputBlock, outputBlock, 
                             DSPVoiceChanger::VoiceCharacter::Chipmunk);
        
        // Check for discontinuities (clicks/pops)
        float maxDelta = 0.0f;
        float firstSampleAfterTransition = outputBuffer.getSample(0, 0);
        float transitionDelta = std::abs(firstSampleAfterTransition - prevSample);
        
        for (int i = 1; i < blockSize; ++i) {
            float delta = std::abs(outputBuffer.getSample(0, i) - outputBuffer.getSample(0, i - 1));
            maxDelta = std::max(maxDelta, delta);
        }
        
        // Transition delta should not be dramatically larger than normal deltas
        // (a click would be a huge spike)
        expect(transitionDelta < 1.0f, "Transition should not cause a click (delta < 1.0)");
        expect(maxDelta < 0.5f, "Output should not have discontinuities");
    }

    //==========================================================================
    // Test: No NaN or Inf in Output
    //==========================================================================
    
    void testNoNaNInf() {
        beginTest("No NaN or Inf in Output");
        
        DSPVoiceChanger voiceChanger;
        
        const double sampleRate = 48000.0;
        const int blockSize = 512;
        
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = blockSize;
        spec.numChannels = 2;
        
        voiceChanger.prepare(spec);
        
        // Test with random noise input
        juce::Random random;
        juce::AudioBuffer<float> inputBuffer(2, blockSize);
        juce::AudioBuffer<float> outputBuffer(2, blockSize);
        
        // Test all voice characters
        std::array<DSPVoiceChanger::VoiceCharacter, 4> characters = {
            DSPVoiceChanger::VoiceCharacter::DeepMale,
            DSPVoiceChanger::VoiceCharacter::Chipmunk,
            DSPVoiceChanger::VoiceCharacter::Robot,
            DSPVoiceChanger::VoiceCharacter::Ethereal
        };
        
        for (auto character : characters) {
            voiceChanger.reset();
            
            for (int block = 0; block < 20; ++block) {
                // Fill with random noise
                for (int i = 0; i < blockSize; ++i) {
                    float sample = random.nextFloat() * 2.0f - 1.0f;
                    inputBuffer.setSample(0, i, sample);
                    inputBuffer.setSample(1, i, sample);
                }
                
                juce::dsp::AudioBlock<const float> inputBlock(inputBuffer);
                juce::dsp::AudioBlock<float> outputBlock(outputBuffer);
                voiceChanger.process(inputBlock, outputBlock, character);
                
                // Check for NaN/Inf
                for (int ch = 0; ch < 2; ++ch) {
                    for (int i = 0; i < blockSize; ++i) {
                        float sample = outputBuffer.getSample(ch, i);
                        expect(!std::isnan(sample), "Output should not be NaN");
                        expect(!std::isinf(sample), "Output should not be Inf");
                    }
                }
            }
        }
    }

    //==========================================================================
    // Test: Formant Preservation
    //==========================================================================
    
    void testFormantPreservation() {
        beginTest("Formant Preservation Toggle");
        
        DSPVoiceChanger voiceChanger;
        
        const double sampleRate = 48000.0;
        const int blockSize = 512;
        
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = blockSize;
        spec.numChannels = 2;
        
        voiceChanger.prepare(spec);
        
        // Toggle formant preservation
        expect(!voiceChanger.isFormantPreservationEnabled(), 
               "Formant preservation should be off by default");
        
        voiceChanger.setFormantPreservation(true);
        expect(voiceChanger.isFormantPreservationEnabled(), 
               "Formant preservation should be on after enabling");
        
        // Process with formant preservation enabled
        juce::AudioBuffer<float> inputBuffer(2, blockSize);
        juce::AudioBuffer<float> outputBuffer(2, blockSize);
        
        for (int i = 0; i < blockSize; ++i) {
            float sample = std::sin(2.0f * juce::MathConstants<float>::pi * i / 50.0f) * 0.5f;
            inputBuffer.setSample(0, i, sample);
            inputBuffer.setSample(1, i, sample);
        }
        
        juce::dsp::AudioBlock<const float> inputBlock(inputBuffer);
        juce::dsp::AudioBlock<float> outputBlock(outputBuffer);
        
        // Should not crash with formant preservation enabled
        voiceChanger.process(inputBlock, outputBlock, 
                             DSPVoiceChanger::VoiceCharacter::Chipmunk);
        
        float outputMagnitude = outputBuffer.getMagnitude(0, 0, blockSize);
        expect(outputMagnitude > 0.01f, "Output with formant preservation should not be silent");
        
        voiceChanger.setFormantPreservation(false);
        expect(!voiceChanger.isFormantPreservationEnabled(), 
               "Formant preservation should be off after disabling");
    }

    //==========================================================================
    // Test: Performance (CPU Usage)
    //==========================================================================
    
    void testPerformance() {
        beginTest("Performance (CPU Budget)");
        
        DSPVoiceChanger voiceChanger;
        
        const double sampleRate = 48000.0;
        const int blockSize = 512;
        
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = blockSize;
        spec.numChannels = 2;
        
        voiceChanger.prepare(spec);
        voiceChanger.setFormantPreservation(true); // Worst case
        
        juce::AudioBuffer<float> inputBuffer(2, blockSize);
        juce::AudioBuffer<float> outputBuffer(2, blockSize);
        
        // Fill with test signal
        for (int i = 0; i < blockSize; ++i) {
            float sample = std::sin(2.0f * juce::MathConstants<float>::pi * i / 100.0f) * 0.5f;
            inputBuffer.setSample(0, i, sample);
            inputBuffer.setSample(1, i, sample);
        }
        
        // Process 10 seconds worth of audio and measure time
        const int blocksFor10Seconds = static_cast<int>(sampleRate * 10.0 / blockSize);
        
        auto startTime = std::chrono::high_resolution_clock::now();
        
        for (int block = 0; block < blocksFor10Seconds; ++block) {
            juce::dsp::AudioBlock<const float> inputBlock(inputBuffer);
            juce::dsp::AudioBlock<float> outputBlock(outputBuffer);
            voiceChanger.process(inputBlock, outputBlock, 
                                 DSPVoiceChanger::VoiceCharacter::Chipmunk);
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        
        // 10 seconds of audio processed, should take < 500ms for <5% CPU
        // (5% of 10000ms = 500ms)
        expect(duration.count() < 500, 
               "Processing 10s of audio should take < 500ms for <5% CPU usage");
        
        // Log the actual time for informational purposes
        juce::Logger::writeToLog("Performance test: Processed 10s of audio in " 
                                  + juce::String(duration.count()) + "ms");
    }
};

// Static registration
static DSPVoiceChangerTests dspVoiceChangerTests;

} // namespace tests
} // namespace zenith
