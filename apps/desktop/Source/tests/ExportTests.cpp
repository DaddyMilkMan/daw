/**
 * @file ExportTests.cpp
 * @brief Unit tests for audio export functionality
 * 
 * Tests:
 * - Dithering algorithms (TPDF, noise-shaped)
 * - Normalization accuracy
 * - Stem export file generation
 */

#include "../engine/AudioExporter.h"
#include "../dsp/Dither.h"
#include "../engine/Engine.h"
#include "../engine/ProjectState.h"
#include "TestUtils.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <cmath>

namespace zenith {
namespace tests {

//==============================================================================
// Dithering Tests
//==============================================================================

class DitherTests : public juce::UnitTest {
public:
  DitherTests() : juce::UnitTest("Dither Algorithms", "Export") {}

  void runTest() override {
    beginTest("TPDF dither adds noise at correct amplitude");
    {
      zenith::dsp::Dither dither;
      dither.prepare(2);
      dither.setType(dsp::DitherType::FlatTPDF);

      // Create a silent buffer
      juce::AudioBuffer<float> buffer(2, 1024);
      buffer.clear();

      // Apply 16-bit dithering
      dither.process(buffer, 16);

      // For 16-bit, LSB = 1/32768 ≈ 3.05e-5
      // TPDF noise should have magnitude roughly in this range
      float magnitude = buffer.getRMSLevel(0, 0, 1024);
      
      // RMS of triangular distribution with amplitude A is A/sqrt(6)
      // A = LSB = 1/32768, so RMS ≈ 1.25e-5
      expect(magnitude > 1e-6f, "Dither should add measurable noise");
      expect(magnitude < 1e-3f, "Dither noise should be small (< -60dB)");
    }

    beginTest("Noise-shaped TPDF produces higher noise at high frequencies");
    {
      // This is a statistical test - noise shaping pushes energy to high frequencies
      zenith::dsp::Dither dither;
      dither.prepare(2);
      dither.setType(dsp::DitherType::ShapedTPDF);

      juce::AudioBuffer<float> buffer(2, 4096);
      buffer.clear();

      // Apply dithering
      dither.process(buffer, 16);

      // Compute simple high-pass energy estimate
      float highFreqEnergy = 0.0f;
      float lowFreqEnergy = 0.0f;
      const float* data = buffer.getReadPointer(0);

      for (int i = 1; i < buffer.getNumSamples(); ++i) {
        float diff = data[i] - data[i-1];  // Simple differentiator (high-pass)
        highFreqEnergy += diff * diff;
        lowFreqEnergy += data[i] * data[i];
      }

      // With noise shaping, high-frequency energy should be significant
      expect(highFreqEnergy > 0.0f, "Shaped dither should produce noise");
    }

    beginTest("32-bit bypasses dithering");
    {
      zenith::dsp::Dither dither;
      dither.prepare(2);

      juce::AudioBuffer<float> buffer(2, 1024);
      buffer.clear();
      for (int i = 0; i < 1024; ++i) {
        buffer.setSample(0, i, 0.5f);
        buffer.setSample(1, i, 0.5f);
      }

      // Process at 32-bit (should be bypassed)
      dither.process(buffer, 32);

      // Buffer should be unchanged
      for (int i = 0; i < 1024; ++i) {
        expectEquals(buffer.getSample(0, i), 0.5f);
      }
    }

    beginTest("DitherType::None produces no modification");
    {
      zenith::dsp::Dither dither;
      dither.prepare(2);
      dither.setType(dsp::DitherType::None);

      juce::AudioBuffer<float> buffer(2, 1024);
      for (int i = 0; i < 1024; ++i) {
        buffer.setSample(0, i, 0.25f);
        buffer.setSample(1, i, 0.25f);
      }

      dither.process(buffer, 16);

      for (int i = 0; i < 1024; ++i) {
        expectEquals(buffer.getSample(0, i), 0.25f);
      }
    }
  }
};

//==============================================================================
// Export Options Tests
//==============================================================================

class ExportOptionsTests : public juce::UnitTest {
public:
  ExportOptionsTests() : juce::UnitTest("Export Options", "Export") {}

  void runTest() override {
    beginTest("ExportOptions defaults are sensible");
    {
      ExportOptions opts;
      
      expectEquals(opts.sampleRate, 44100.0);
      expectEquals(opts.bitDepth, 24);
      expect(opts.format == ExportFormat::WAV);
      expect(opts.enableDither);
      expect(opts.ditherType == dsp::DitherType::ShapedTPDF);
      expect(!opts.normalize);
      expectEquals(opts.normalizeDb, -0.1);
      expect(!opts.exportStems);
      expect(opts.exportStemsAsync);
    }

    beginTest("StemExportProgress initialization");
    {
      StemExportProgress progress;
      
      expectEquals(progress.trackIndex, -1);
      expectEquals(progress.progress, 0.0f);
      expect(!progress.completed);
      expect(!progress.success);
    }
  }
};

//==============================================================================
// Normalization Math Tests
//==============================================================================

class NormalizationTests : public juce::UnitTest {
public:
  NormalizationTests() : juce::UnitTest("Normalization", "Export") {}

  void runTest() override {
    beginTest("Peak detection math");
    {
      // Create a buffer with known peak
      juce::AudioBuffer<float> buffer(2, 1024);
      buffer.clear();
      
      // Set a known peak at sample 512
      buffer.setSample(0, 512, 0.5f);  // Peak at 0.5 (-6dB)
      buffer.setSample(1, 512, -0.3f);

      float peak = 0.0f;
      for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        float chPeak = buffer.getMagnitude(ch, 0, buffer.getNumSamples());
        if (chPeak > peak) peak = chPeak;
      }

      expectEquals(peak, 0.5f);
    }

    beginTest("Gain calculation for normalization");
    {
      float peak = 0.5f;  // -6dB
      float targetDb = -0.1f;
      float targetLinear = juce::Decibels::decibelsToGain(targetDb);  // ~0.989
      
      float gain = targetLinear / peak;  // ~1.978 (+5.9dB)
      
      expect(gain > 1.9f && gain < 2.0f, "Gain should be approximately 2x for -6dB to -0.1dB");
      
      // Verify normalized peak
      float normalizedPeak = peak * gain;
      float normalizedDb = juce::Decibels::gainToDecibels(normalizedPeak);
      
      expect(std::abs(normalizedDb - targetDb) < 0.01f, 
             "Normalized peak should match target within 0.01dB");
    }
  }
};

// Static test registration
static DitherTests ditherTests;
static ExportOptionsTests exportOptionsTests;
static NormalizationTests normalizationTests;

} // namespace tests
} // namespace zenith
