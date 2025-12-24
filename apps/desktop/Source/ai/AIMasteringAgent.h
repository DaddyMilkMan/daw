/*
  ==============================================================================
    apps/desktop/Source/ai/AIMasteringAgent.h
    Automated mixing and mastering agent with real DSP.
  ==============================================================================
*/

#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_dsp/juce_dsp.h>


// Forward declaration to break circular dependency
namespace zenith {
class Engine;
}

namespace zenith {
namespace ai {

//==============================================================================
/**
    Real-time Brickwall Limiter for mastering
*/
class MasteringLimiter {
public:
  MasteringLimiter() = default;

  void prepare(const juce::dsp::ProcessSpec &spec);
  void reset();
  void setCeiling(float ceilingDb);

  template <typename ProcessContext>
  void process(const ProcessContext &context) {
    auto &inputBlock = context.getInputBlock();
    auto &outputBlock = context.getOutputBlock();
    auto numChannels = inputBlock.getNumChannels();
    auto numSamples = inputBlock.getNumSamples();

    for (size_t sample = 0; sample < numSamples; ++sample) {
      float peak = 0.0f;
      for (size_t ch = 0; ch < numChannels; ++ch) {
        float absVal = std::abs(inputBlock.getSample(static_cast<int>(ch),
                                                     static_cast<int>(sample)));
        peak = std::max(peak, absVal);
      }

      float targetGain = (peak > ceiling_) ? ceiling_ / peak : 1.0f;

      if (targetGain < envelope_) {
        envelope_ =
            attackCoeff_ * envelope_ + (1.0f - attackCoeff_) * targetGain;
      } else {
        envelope_ =
            releaseCoeff_ * envelope_ + (1.0f - releaseCoeff_) * targetGain;
      }

      for (size_t ch = 0; ch < numChannels; ++ch) {
        float currentSample = inputBlock.getSample(static_cast<int>(ch),
                                                   static_cast<int>(sample));
        int readPos = (lookaheadPos_ + 1) % (lookaheadSamples_ + 1);
        float delayedSample =
            lookaheadBuffer_.getSample(static_cast<int>(ch), readPos);
        lookaheadBuffer_.setSample(static_cast<int>(ch), lookaheadPos_,
                                   currentSample);
        outputBlock.setSample(static_cast<int>(ch), static_cast<int>(sample),
                              delayedSample * envelope_);
      }

      lookaheadPos_ = (lookaheadPos_ + 1) % (lookaheadSamples_ + 1);
    }
  }

private:
  double sampleRate_ = 44100.0;
  float ceiling_ = 0.99f;
  float envelope_ = 1.0f;
  float attackCoeff_ = 0.0f;
  float releaseCoeff_ = 0.0f;

  juce::AudioBuffer<float> lookaheadBuffer_;
  int lookaheadSamples_ = 0;
  int lookaheadPos_ = 0;
};

//==============================================================================
/**
    Mastering-grade Parametric EQ
*/
class MasteringEQ {
public:
  MasteringEQ() = default;
  void prepare(const juce::dsp::ProcessSpec &spec);
  void reset();

  template <typename ProcessContext>
  void process(const ProcessContext &context) {
    highPass_.process(context);
    lowShelf_.process(context);
    midCut_.process(context);
    presence_.process(context);
    airBand_.process(context);
  }

private:
  juce::dsp::StateVariableTPTFilter<float> highPass_;
  juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                 juce::dsp::IIR::Coefficients<float>>
      lowShelf_;
  juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                 juce::dsp::IIR::Coefficients<float>>
      midCut_;
  juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                 juce::dsp::IIR::Coefficients<float>>
      presence_;
  juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                 juce::dsp::IIR::Coefficients<float>>
      airBand_;
};

//==============================================================================
/**
    Glue Compressor for master bus
*/
class GlueCompressor {
public:
  GlueCompressor() = default;
  void prepare(const juce::dsp::ProcessSpec &spec);
  void reset();
  void setAmount(float amount);

  template <typename ProcessContext>
  void process(const ProcessContext &context) {
    compressor_.process(context);
  }

private:
  juce::dsp::Compressor<float> compressor_;
};

//==============================================================================
/**
    Main AI Mastering Agent - performs actual DSP processing
*/
class AIMasteringAgent {
public:
  struct MasteringOptions {
    bool autoLevelMix = true;
    bool applyEq = true;
    bool applyCompression = true;
    bool applyLimiter = true;
    bool target8Bit = false;
    float targetLufs = -14.0f;
    float compressionAmount = 0.5f;
  };

  explicit AIMasteringAgent(Engine &engine);
  ~AIMasteringAgent(); // Defined in CPP

  void prepare(double sampleRate, int samplesPerBlock, int numChannels);
  void runMasteringPass(const MasteringOptions &options);
  void processBlock(juce::AudioBuffer<float> &buffer);

  juce::AudioBuffer<float> masterOffline(const juce::AudioBuffer<float> &input,
                                         double sampleRate,
                                         const MasteringOptions &options);

  void reset();

private:
  Engine &engine_;

  MasteringEQ eq_;
  GlueCompressor compressor_;
  MasteringLimiter limiter_;

  bool isPrepared_ = false; // Changed from atomic to bool to match usage? OR
                            // keep atomic? Step 520 used bool.
  bool masteredSuccessfully_ = false;
  MasteringOptions currentOptions_;

  void performAutoMixing();
  void normalizeLoudness(juce::AudioBuffer<float> &buffer, float targetLufs);
};

} // namespace ai
} // namespace zenith
