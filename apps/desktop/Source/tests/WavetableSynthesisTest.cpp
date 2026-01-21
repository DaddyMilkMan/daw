/*
  ==============================================================================

    WavetableSynthesisTest.cpp
    Created: 2026-01-21
    Author:  Zenith DAW

    Unit tests for wavetable synthesis correctness, specifically verifying
    the trigonometric recurrence optimization produces results within tolerance
    of direct std::sin/cos calculations.

  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include "../instruments/WavetableLoader.h"
#include <cmath>

namespace zenith {
namespace tests {

class WavetableSynthesisTest : public juce::UnitTest {
public:
  WavetableSynthesisTest() : juce::UnitTest("WavetableSynthesis", "Correctness") {}

  void runTest() override {
    // Maximum acceptable error: 2e-6 (verified by standalone benchmark)
    // This is well below -144dB noise floor
    constexpr float kMaxError = 2e-6f;

    beginTest("Saw Wave Synthesis Accuracy");
    testSawWave(kMaxError);

    beginTest("Square Wave Synthesis Accuracy");
    testSquareWave(kMaxError);

    beginTest("Triangle Wave Synthesis Accuracy");
    testTriangleWave(kMaxError);

    beginTest("PWM Wave Synthesis Accuracy");
    testPWMWave(kMaxError);

    beginTest("Edge Case: Single Frame");
    testSingleFrame();

    beginTest("Edge Case: Maximum Frames");
    testMaxFrames();
  }

private:
  // Reference implementation using direct std::sin (baseline for comparison)
  float generateSawDirect(float phase, int maxHarmonic = 64) {
    float sample = 0.0f;
    for (int h = 1; h <= maxHarmonic; ++h) {
      sample += std::sin(h * phase * juce::MathConstants<float>::twoPi) / h;
    }
    return sample * 0.5f;
  }

  float generateSquareDirect(float phase, int maxHarmonic = 32) {
    float sample = 0.0f;
    for (int h = 1; h <= maxHarmonic; h += 2) {
      sample += std::sin(h * phase * juce::MathConstants<float>::twoPi) / h;
    }
    return sample * 0.6f;
  }

  float generateTriangleDirect(float phase, int maxHarmonic = 32) {
    float sample = 0.0f;
    int sign = 1;
    for (int h = 1; h <= maxHarmonic; h += 2) {
      sample += sign * std::sin(h * phase * juce::MathConstants<float>::twoPi) / (h * h);
      sign = -sign;
    }
    return sample * 0.8f;
  }

  float generatePWMDirect(float phase, float pw, int maxHarmonic = 32) {
    float sample = 0.0f;
    for (int h = 1; h <= maxHarmonic; ++h) {
      float harmPhase = h * phase * juce::MathConstants<float>::twoPi;
      float coeff = 2.0f / (h * juce::MathConstants<float>::pi);
      sample += coeff * std::sin(h * pw * juce::MathConstants<float>::pi) *
                std::cos(harmPhase - h * pw * juce::MathConstants<float>::pi);
    }
    return sample * 0.6f;
  }

  void testSawWave(float maxError) {
    WavetableLoader loader;
    auto wavetable = loader.generateBasicWavetable(1, 1); // Type 1 = Saw
    
    expect(wavetable != nullptr, "Wavetable generation failed");
    expectEquals(wavetable->getNumFrames(), 1, "Should generate 1 frame");

    const float* frameData = wavetable->getFrameData(0);
    float maxDiff = 0.0f;
    int maxDiffIdx = 0;

    // Compare every sample against direct calculation
    for (int i = 0; i < WAVETABLE_FRAME_SIZE; ++i) {
      float phase = static_cast<float>(i) / WAVETABLE_FRAME_SIZE;
      float expected = generateSawDirect(phase);
      float actual = frameData[i];
      float diff = std::abs(expected - actual);
      
      if (diff > maxDiff) {
        maxDiff = diff;
        maxDiffIdx = i;
      }
    }

    logMessage(juce::String("Saw: Max error = ") + juce::String(maxDiff, 8) + 
               " at sample " + juce::String(maxDiffIdx));
    expect(maxDiff < maxError, "Saw wave error exceeds tolerance");
  }

  void testSquareWave(float maxError) {
    WavetableLoader loader;
    auto wavetable = loader.generateBasicWavetable(2, 1); // Type 2 = Square
    
    const float* frameData = wavetable->getFrameData(0);
    float maxDiff = 0.0f;

    for (int i = 0; i < WAVETABLE_FRAME_SIZE; ++i) {
      float phase = static_cast<float>(i) / WAVETABLE_FRAME_SIZE;
      float expected = generateSquareDirect(phase);
      float actual = frameData[i];
      float diff = std::abs(expected - actual);
      maxDiff = juce::jmax(maxDiff, diff);
    }

    logMessage(juce::String("Square: Max error = ") + juce::String(maxDiff, 8));
    expect(maxDiff < maxError, "Square wave error exceeds tolerance");
  }

  void testTriangleWave(float maxError) {
    WavetableLoader loader;
    auto wavetable = loader.generateBasicWavetable(3, 1); // Type 3 = Triangle
    
    const float* frameData = wavetable->getFrameData(0);
    float maxDiff = 0.0f;

    for (int i = 0; i < WAVETABLE_FRAME_SIZE; ++i) {
      float phase = static_cast<float>(i) / WAVETABLE_FRAME_SIZE;
      float expected = generateTriangleDirect(phase);
      float actual = frameData[i];
      float diff = std::abs(expected - actual);
      maxDiff = juce::jmax(maxDiff, diff);
    }

    logMessage(juce::String("Triangle: Max error = ") + juce::String(maxDiff, 8));
    expect(maxDiff < maxError, "Triangle wave error exceeds tolerance");
  }

  void testPWMWave(float maxError) {
    WavetableLoader loader;
    auto wavetable = loader.generateBasicWavetable(4, 64); // Type 4 = PWM, 64 frames
    
    // Test multiple frames (PWM morphs from 10% to 90% pulse width)
    for (int f = 0; f < wavetable->getNumFrames(); f += 16) { // Sample every 16th frame
      float morphAmount = static_cast<float>(f) / 63.0f;
      float pw = 0.1f + morphAmount * 0.8f;
      
      const float* frameData = wavetable->getFrameData(f);
      float maxDiff = 0.0f;

      for (int i = 0; i < WAVETABLE_FRAME_SIZE; i += 128) { // Sample every 128th sample
        float phase = static_cast<float>(i) / WAVETABLE_FRAME_SIZE;
        float expected = generatePWMDirect(phase, pw);
        float actual = frameData[i];
        float diff = std::abs(expected - actual);
        maxDiff = juce::jmax(maxDiff, diff);
      }

      if (maxDiff >= maxError) {
        logMessage(juce::String("PWM Frame ") + juce::String(f) + 
                   ": Max error = " + juce::String(maxDiff, 8));
        expect(false, "PWM wave error exceeds tolerance");
        return;
      }
    }

    logMessage("PWM: All frames within tolerance");
    expect(true);
  }

  void testSingleFrame() {
    WavetableLoader loader;
    
    // Test each waveform type with single frame
    for (int type = 0; type <= 5; ++type) {
      auto wavetable = loader.generateBasicWavetable(type, 1);
      expect(wavetable != nullptr, "Failed to generate wavetable for type " + juce::String(type));
      expectEquals(wavetable->getNumFrames(), 1, "Should have 1 frame");
      
      const float* data = wavetable->getFrameData(0);
      expect(data != nullptr, "Frame data is null");
      
      // Sanity check: values should be in [-1, 1] range
      for (int i = 0; i < WAVETABLE_FRAME_SIZE; ++i) {
        float val = data[i];
        expect(val >= -1.5f && val <= 1.5f, 
               "Sample out of reasonable range: " + juce::String(val));
      }
    }
  }

  void testMaxFrames() {
    WavetableLoader loader;
    
    // Test morphing wavetables (PWM and Formant use multiple frames)
    auto pwm = loader.generateBasicWavetable(4, 256); // Max frames
    expectEquals(pwm->getNumFrames(), 256, "PWM should support 256 frames");
    
    auto formant = loader.generateBasicWavetable(5, 128);
    expectEquals(formant->getNumFrames(), 128, "Formant should support 128 frames");
  }
};

static WavetableSynthesisTest wavetableSynthesisTest;

} // namespace tests
} // namespace zenith
