/*
  ==============================================================================
    AIMasteringAgent.cpp
  ==============================================================================
*/

#include "AIMasteringAgent.h"
#include "../engine/Engine.h"
#include "../engine/Track.h"

namespace zenith {
namespace ai {

//==============================================================================
// MasteringLimiter Implementation
//==============================================================================

void MasteringLimiter::prepare(const juce::dsp::ProcessSpec &spec) {
  sampleRate_ = spec.sampleRate;
  lookaheadSamples_ = static_cast<int>(sampleRate_ * 0.005);
  lookaheadBuffer_.setSize(static_cast<int>(spec.numChannels),
                           lookaheadSamples_ + 1);
  lookaheadBuffer_.clear();
  lookaheadPos_ = 0;
  envelope_ = 0.0f;
  attackCoeff_ = std::exp(-1.0f / static_cast<float>(sampleRate_ * 0.001f));
  releaseCoeff_ = std::exp(-1.0f / static_cast<float>(sampleRate_ * 0.100f));
}

void MasteringLimiter::reset() {
  lookaheadBuffer_.clear();
  lookaheadPos_ = 0;
  envelope_ = 0.0f;
}

void MasteringLimiter::setCeiling(float ceilingDb) {
  ceiling_ = juce::Decibels::decibelsToGain(ceilingDb);
}

//==============================================================================
// MasteringEQ Implementation
//==============================================================================

void MasteringEQ::prepare(const juce::dsp::ProcessSpec &spec) {
  highPass_.prepare(spec);
  highPass_.setType(juce::dsp::StateVariableTPTFilterType::highpass);
  highPass_.setCutoffFrequency(30.0f);

  lowShelf_.prepare(spec);
  *lowShelf_.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(
      spec.sampleRate, 100.0f, 0.7f, juce::Decibels::decibelsToGain(1.0f));

  midCut_.prepare(spec);
  *midCut_.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
      spec.sampleRate, 300.0f, 1.0f, juce::Decibels::decibelsToGain(-1.0f));

  presence_.prepare(spec);
  *presence_.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
      spec.sampleRate, 3000.0f, 1.0f, juce::Decibels::decibelsToGain(1.5f));

  airBand_.prepare(spec);
  *airBand_.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
      spec.sampleRate, 10000.0f, 0.7f, juce::Decibels::decibelsToGain(2.0f));
}

void MasteringEQ::reset() {
  highPass_.reset();
  lowShelf_.reset();
  midCut_.reset();
  presence_.reset();
  airBand_.reset();
}

//==============================================================================
// GlueCompressor Implementation
//==============================================================================

void GlueCompressor::prepare(const juce::dsp::ProcessSpec &spec) {
  compressor_.prepare(spec);
  compressor_.setThreshold(-12.0f);
  compressor_.setRatio(2.0f);
  compressor_.setAttack(30.0f);
  compressor_.setRelease(200.0f);
}

void GlueCompressor::reset() { compressor_.reset(); }

void GlueCompressor::setAmount(float amount) {
  compressor_.setThreshold(-6.0f - (amount * 12.0f));
  compressor_.setRatio(1.5f + (amount * 2.5f));
}

//==============================================================================
// AIMasteringAgent Implementation
//==============================================================================

AIMasteringAgent::AIMasteringAgent(Engine &engine) : engine_(engine) {}

AIMasteringAgent::~AIMasteringAgent() {}

void AIMasteringAgent::prepare(double sampleRate, int samplesPerBlock,
                               int numChannels) {
  juce::dsp::ProcessSpec spec;
  spec.sampleRate = sampleRate;
  spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
  spec.numChannels = static_cast<juce::uint32>(numChannels);

  eq_.prepare(spec);
  compressor_.prepare(spec);
  limiter_.prepare(spec);

  isPrepared_ = true;
  DBG("AIMasteringAgent: DSP chain prepared");
}

void AIMasteringAgent::runMasteringPass(const MasteringOptions &options) {
  DBG("AIMasteringAgent: Starting mastering pass...");
  if (options.autoLevelMix) {
    performAutoMixing();
  }
  currentOptions_ = options;
  if (options.applyCompression) {
    compressor_.setAmount(options.compressionAmount);
  }
  if (options.applyLimiter) {
    float ceiling = options.target8Bit ? -0.5f : -0.1f;
    limiter_.setCeiling(ceiling);
  }
  masteredSuccessfully_ = true;
  DBG("AIMasteringAgent: Mastering pass complete");
}

void AIMasteringAgent::processBlock(juce::AudioBuffer<float> &buffer) {
  if (!isPrepared_ || !masteredSuccessfully_)
    return;
  juce::dsp::AudioBlock<float> block(buffer);
  juce::dsp::ProcessContextReplacing<float> context(block);
  if (currentOptions_.applyEq)
    eq_.process(context);
  if (currentOptions_.applyCompression)
    compressor_.process(context);
  if (currentOptions_.applyLimiter)
    limiter_.process(context);
}

juce::AudioBuffer<float>
AIMasteringAgent::masterOffline(const juce::AudioBuffer<float> &input,
                                double sampleRate,
                                const MasteringOptions &options) {
  juce::AudioBuffer<float> output(input);
  if (!isPrepared_)
    prepare(sampleRate, input.getNumSamples(), input.getNumChannels());
  runMasteringPass(options);
  processBlock(output);
  if (options.targetLufs != 0.0f)
    normalizeLoudness(output, options.targetLufs);
  return output;
}

void AIMasteringAgent::reset() {
  eq_.reset();
  compressor_.reset();
  limiter_.reset();
}

void AIMasteringAgent::performAutoMixing() {
  DBG("AIMasteringAgent: Analyzing track levels...");
  auto &tracks = engine_.tracks(); // Now we have full Engine definition
  if (tracks.empty())
    return;

  for (int i = 0; i < static_cast<int>(tracks.size()); ++i) {
    auto track = tracks[i];
    if (!track)
      continue;
    float currentLevel = engine_.getTrackLevel(i);
    if (currentLevel > 0.5f) {
      engine_.setTrackVolume(i, 0.5f);
      DBG("AIMasteringAgent: Reduced track " + juce::String(i) + " to -6dB");
    } else if (currentLevel < 0.1f && currentLevel > 0.0f) {
      engine_.setTrackVolume(i, 0.7f);
      DBG("AIMasteringAgent: Boosted track " + juce::String(i));
    }
  }
  DBG("AIMasteringAgent: Auto-mix complete");
}

void AIMasteringAgent::normalizeLoudness(juce::AudioBuffer<float> &buffer,
                                         float targetLufs) {
  float rms = 0.0f;
  int numSamples = buffer.getNumSamples();
  int numChannels = buffer.getNumChannels();
  for (int ch = 0; ch < numChannels; ++ch) {
    const float *data = buffer.getReadPointer(ch);
    for (int i = 0; i < numSamples; ++i) {
      rms += data[i] * data[i];
    }
  }
  rms = std::sqrt(rms / (numSamples * numChannels));
  float currentLufs = 20.0f * std::log10(rms) - 10.0f;
  float gainDb = targetLufs - currentLufs;
  gainDb = juce::jlimit(-12.0f, 12.0f, gainDb);
  float gainLinear = juce::Decibels::decibelsToGain(gainDb);
  buffer.applyGain(gainLinear);
  DBG("AIMasteringAgent: Normalized loudness by " + juce::String(gainDb, 1) +
      "dB");
}

} // namespace ai
} // namespace zenith
