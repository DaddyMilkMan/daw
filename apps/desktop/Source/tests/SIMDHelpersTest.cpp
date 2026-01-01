/*
  ==============================================================================

    SIMDHelpersTest.cpp
    Created: 2025
    Author:  Zenith DAW

  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h> // For UnitTestRunner
#include "../dsp/SIMDHelpers.h"

namespace zenith {

class SIMDHelpersTest : public juce::UnitTest {
public:
  SIMDHelpersTest() : juce::UnitTest("SIMD Fast Math Precision", "DSP") {}

  void runTest() override {
    beginTest("fastLog10 Precision");

    float maxError = 0.0f;
    float avgError = 0.0f;
    int count = 0;

    // Test range from -100dB (1e-5) to +20dB (10.0)
    for (float val = 0.00001f; val <= 10.0f; val *= 1.01f) {
      float standard = std::log10(val);
      float fast = zenith::simd::fastLog10(val);
      float error = std::abs(standard - fast);
      
      maxError = std::max(maxError, error);
      avgError += error;
      count++;
    }
    avgError /= (float)count;

    // Log the error
    logMessage("fastLog10 Max Error: " + juce::String(maxError, 6));
    logMessage("fastLog10 Avg Error: " + juce::String(avgError, 6));

    // Expect reasonable precision for a dynamics processor (approx 0.06 log units ~= 1.2 dB error worst case?)
    // 0.06 log10 units. 20 * 0.06 = 1.2 dB.
    expect(maxError < 0.1f, "fastLog10 error too high (> 0.1)");
    
    beginTest("fastPow10 Precision");
    
    maxError = 0.0f;
    avgError = 0.0f;
    count = 0;
    
    // Test range -5 (0.00001) to 1 (10.0)
    for (float val = -5.0f; val <= 1.0f; val += 0.01f) {
        float standard = std::pow(10.0f, val);
        float fast = zenith::simd::fastPow10(val);
        // Error relative to value (percentage) for pow?
        // Or absolute? For small values absolute is tiny.
        // Let's check dbToGain roundtrip error.
        
        float error = std::abs(standard - fast);
        // Normalized error
        if (standard > 1e-6f)
            error /= standard;
            
        maxError = std::max(maxError, error);
        avgError += error;
        count++;
    }
    
    logMessage("fastPow10 Max Relative Error: " + juce::String(maxError * 100.0f, 4) + "%");
    expect(maxError < 0.1f, "fastPow10 relative error too high (> 10%)");
  }
};

static SIMDHelpersTest simdHelpersTest;

} // namespace zenith
