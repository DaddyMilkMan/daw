/*
  ==============================================================================

    MixerChannel.h
    Ported from: ZenithDAW-Native/Source/Audio/MixerChannel.h (2025-11-11)
    Author:  Zenith DAW → Zenith DAW

    Mixer channel strip with EQ, dynamics, and send/return processing

    JUCE 8 / C++20 adaptations:
    - Wrapped in namespace zenith
    - Professional-grade compressor with RMS detection & lookahead
    - Pre-calculated filter coefficients (RT-safe)

  ==============================================================================
*/

#pragma once

#include <array>
#include <cmath>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace zenith {

//==============================================================================
/**
    Professional-grade compressor with RMS detection, lookahead, and soft knee.

    Features:
    - RMS envelope detection (more musical than peak)
    - Lookahead for transparent limiting
    - Soft knee option
    - Auto makeup gain
*/
class ProCompressor {
public:
  ProCompressor() = default;

  void prepare(double sampleRate, int maxBlockSize) {
    sampleRate_ = sampleRate;

    // Lookahead buffer (5ms)
    lookaheadSamples_ = static_cast<int>(sampleRate * 0.005);
    lookaheadBuffer_.setSize(2, lookaheadSamples_ + maxBlockSize);
    lookaheadBuffer_.clear();
    lookaheadWritePos_ = 0;

    // RMS buffer (10ms window)
    rmsWindowSamples_ = static_cast<int>(sampleRate * 0.010);
    rmsBuffer_.resize(rmsWindowSamples_, 0.0f);
    rmsWritePos_ = 0;
    rmsSum_ = 0.0f;

    // Initialize envelopes
    envL_ = 0.0f;
    envR_ = 0.0f;
    gainSmooth_ = 1.0f;

    updateCoefficients();
  }

  void reset() {
    lookaheadBuffer_.clear();
    lookaheadWritePos_ = 0;
    std::fill(rmsBuffer_.begin(), rmsBuffer_.end(), 0.0f);
    rmsWritePos_ = 0;
    rmsSum_ = 0.0f;
    envL_ = 0.0f;
    envR_ = 0.0f;
    gainSmooth_ = 1.0f;
  }

  void setThreshold(float thresholdDb) {
    threshold_ = thresholdDb;
    updateAutoMakeup();
  }
  void setRatio(float ratio) {
    ratio_ = ratio;
    updateAutoMakeup();
  }
  void setAttack(float attackMs) {
    attackMs_ = attackMs;
    updateCoefficients();
  }
  void setRelease(float releaseMs) {
    releaseMs_ = releaseMs;
    updateCoefficients();
  }
  void setMakeup(float makeupDb) {
    makeup_ = makeupDb;
    autoMakeupEnabled_ = false;
  }
  void setKnee(float kneeDb) { knee_ = kneeDb; }
  void setAutoMakeup(bool enabled) {
    autoMakeupEnabled_ = enabled;
    if (enabled)
      updateAutoMakeup();
  }
  void setLookaheadEnabled(bool enabled) { lookaheadEnabled_ = enabled; }
  void setRmsEnabled(bool enabled) { useRms_ = enabled; }

  float getGainReduction() const { return gainReduction_.load(); }

  void process(juce::AudioBuffer<float> &buffer) {
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if (numChannels < 1)
      return;

    float maxGR = 0.0f;

    for (int i = 0; i < numSamples; ++i) {
      // Get input samples
      float inputL = buffer.getSample(0, i);
      float inputR = (numChannels >= 2) ? buffer.getSample(1, i) : inputL;

      // Compute detector signal (summed mono)
      float detector = (std::abs(inputL) + std::abs(inputR)) * 0.5f;

      // RMS or Peak detection
      float level;
      if (useRms_) {
        // Update RMS buffer
        float oldValue = rmsBuffer_[rmsWritePos_];
        rmsSum_ -= oldValue * oldValue;
        rmsSum_ += detector * detector;
        rmsBuffer_[rmsWritePos_] = detector;
        rmsWritePos_ = (rmsWritePos_ + 1) % rmsWindowSamples_;

        // RMS value
        level = std::sqrt(rmsSum_ / static_cast<float>(rmsWindowSamples_));
      } else {
        level = detector;
      }

      // Convert to dB
      float levelDb = juce::Decibels::gainToDecibels(level, -100.0f);

      // Compute gain reduction with soft knee
      float gr = computeGainReduction(levelDb);

      // Smooth the gain change (envelope follower)
      if (gr < gainSmooth_) {
        gainSmooth_ += attackCoeff_ * (gr - gainSmooth_);
      } else {
        gainSmooth_ += releaseCoeff_ * (gr - gainSmooth_);
      }

      // Track max gain reduction for metering
      float grDb = juce::Decibels::gainToDecibels(gainSmooth_, -100.0f);
      if (-grDb > maxGR)
        maxGR = -grDb;

      // Apply makeup gain
      float totalGain =
          gainSmooth_ * juce::Decibels::decibelsToGain(
                            autoMakeupEnabled_ ? autoMakeup_ : makeup_);

      // Apply with lookahead
      if (lookaheadEnabled_ && lookaheadSamples_ > 0) {
        // Write current samples to lookahead buffer
        lookaheadBuffer_.setSample(0, lookaheadWritePos_, inputL);
        if (numChannels >= 2) {
          lookaheadBuffer_.setSample(1, lookaheadWritePos_, inputR);
        }

        // Read delayed samples
        int readPos = (lookaheadWritePos_ - lookaheadSamples_ +
                       lookaheadBuffer_.getNumSamples()) %
                      lookaheadBuffer_.getNumSamples();
        float delayedL = lookaheadBuffer_.getSample(0, readPos);
        float delayedR = (numChannels >= 2)
                             ? lookaheadBuffer_.getSample(1, readPos)
                             : delayedL;

        lookaheadWritePos_ =
            (lookaheadWritePos_ + 1) % lookaheadBuffer_.getNumSamples();

        buffer.setSample(0, i, delayedL * totalGain);
        if (numChannels >= 2) {
          buffer.setSample(1, i, delayedR * totalGain);
        }
      } else {
        buffer.setSample(0, i, inputL * totalGain);
        if (numChannels >= 2) {
          buffer.setSample(1, i, inputR * totalGain);
        }
      }
    }

    gainReduction_.store(maxGR);
  }

private:
  double sampleRate_ = 44100.0;

  // Parameters
  float threshold_ = -10.0f;
  float ratio_ = 4.0f;
  float attackMs_ = 10.0f;
  float releaseMs_ = 100.0f;
  float makeup_ = 0.0f;
  float knee_ = 6.0f; // Soft knee width in dB
  float autoMakeup_ = 0.0f;
  bool autoMakeupEnabled_ = false;
  bool lookaheadEnabled_ = true;
  bool useRms_ = true;

  // State
  float attackCoeff_ = 0.0f;
  float releaseCoeff_ = 0.0f;
  float envL_ = 0.0f;
  float envR_ = 0.0f;
  float gainSmooth_ = 1.0f;
  std::atomic<float> gainReduction_{0.0f};

  // Lookahead
  juce::AudioBuffer<float> lookaheadBuffer_;
  int lookaheadSamples_ = 0;
  int lookaheadWritePos_ = 0;

  // RMS detection
  std::vector<float> rmsBuffer_;
  int rmsWindowSamples_ = 0;
  int rmsWritePos_ = 0;
  float rmsSum_ = 0.0f;

  void updateCoefficients() {
    if (sampleRate_ <= 0)
      return;
    attackCoeff_ = 1.0f - std::exp(-1.0f / (attackMs_ * 0.001f *
                                            static_cast<float>(sampleRate_)));
    releaseCoeff_ = 1.0f - std::exp(-1.0f / (releaseMs_ * 0.001f *
                                             static_cast<float>(sampleRate_)));
  }

  void updateAutoMakeup() {
    // Calculate auto makeup based on threshold and ratio
    // Approximates the average gain reduction at -20dB input
    float overshoot = -20.0f - threshold_;
    if (overshoot > 0.0f && ratio_ > 1.0f) {
      autoMakeup_ = overshoot * (1.0f - 1.0f / ratio_) * 0.5f;
    } else {
      autoMakeup_ = 0.0f;
    }
  }

  float computeGainReduction(float inputDb) const {
    // Soft knee implementation
    float halfKnee = knee_ * 0.5f;
    float output;

    if (inputDb < threshold_ - halfKnee) {
      // Below knee - no compression
      output = inputDb;
    } else if (inputDb > threshold_ + halfKnee) {
      // Above knee - full compression
      output = threshold_ + (inputDb - threshold_) / ratio_;
    } else {
      // In knee region - smooth transition
      float x = inputDb - threshold_ + halfKnee;
      float kneeGain = (1.0f / ratio_ - 1.0f) / (2.0f * knee_);
      output = inputDb + kneeGain * x * x;
    }

    float gr = output - inputDb;
    return juce::Decibels::decibelsToGain(gr);
  }
};

//==============================================================================
/**
    Pre-calculated filter coefficients for RT-safe coefficient swapping.
*/
struct FilterCoefficients {
  std::array<double, 6> hpf = {1.0, 0.0, 0.0,
                               1.0, 0.0, 0.0}; // b0,b1,b2,a0,a1,a2
  std::array<std::array<double, 6>, 4> eq;     // 4 EQ bands

  FilterCoefficients() {
    for (auto &band : eq) {
      band = {1.0, 0.0, 0.0, 1.0, 0.0, 0.0};
    }
  }
};

//==============================================================================
/**
    Represents a mixer channel strip with professional signal processing.

    Each mixer channel provides:
    - Input gain
    - High-pass filter
    - 4-band parametric EQ
    - Professional Compressor with RMS/Lookahead
    - Send effects (up to 4 sends)
    - Pan and volume
    - Metering (input, output, gain reduction)

    All processing is lock-free and real-time safe.
*/
class MixerChannel : public juce::AudioSource, public juce::ChangeBroadcaster {
public:
  //==============================================================================
  MixerChannel();
  ~MixerChannel() override;

  //==============================================================================
  // AudioSource interface
  void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
  void releaseResources() override;
  void
  getNextAudioBlock(const juce::AudioSourceChannelInfo &bufferToFill) override;

  // Overload with Aux Sends support
  void
  getNextAudioBlock(const juce::AudioSourceChannelInfo &bufferToFill,
                    const std::vector<juce::AudioBuffer<float> *> &auxBuffers);

  //==============================================================================
  // Input section
  void setInputGain(float gainInDb);
  float getInputGain() const { return inputGain.load(); }

  void setPhaseInvert(bool shouldInvert);
  bool isPhaseInverted() const { return phaseInvert.load(); }

  //==============================================================================
  // High-pass filter
  void setHighPassEnabled(bool enabled);
  bool isHighPassEnabled() const { return hpfEnabled.load(); }

  void setHighPassFrequency(float frequency);
  float getHighPassFrequency() const { return hpfFrequency.load(); }

  //==============================================================================
  // 4-Band Parametric EQ
  struct EQBand {
    std::atomic<bool> enabled{false};
    std::atomic<float> frequency{1000.0f};
    std::atomic<float> gain{0.0f}; // In dB
    std::atomic<float> q{0.707f};

    enum class Type { LowShelf, Peak, HighShelf };
    Type type = Type::Peak;
  };

  EQBand &getEQBand(int bandIndex);
  const EQBand &getEQBand(int bandIndex) const;

  // Mark EQ as needing coefficient recalculation (message thread safe)
  void markEQDirty(int bandIndex);

  //==============================================================================
  // Dynamics (Compressor) - Now uses ProCompressor
  void setCompressorEnabled(bool enabled);
  bool isCompressorEnabled() const { return compressorEnabled.load(); }

  void setCompressorThreshold(float thresholdDb);
  float getCompressorThreshold() const { return compThreshold.load(); }

  void setCompressorRatio(float ratio);
  float getCompressorRatio() const { return compRatio.load(); }

  void setCompressorAttack(float attackMs);
  float getCompressorAttack() const { return compAttack.load(); }

  void setCompressorRelease(float releaseMs);
  float getCompressorRelease() const { return compRelease.load(); }

  void setCompressorMakeup(float makeupDb);
  float getCompressorMakeup() const { return compMakeup.load(); }

  void setCompressorKnee(float kneeDb);
  void setCompressorLookahead(bool enabled);
  void setCompressorRmsMode(bool useRms);
  void setCompressorAutoMakeup(bool enabled);

  float getGainReduction() const { return compressor_.getGainReduction(); }

  //==============================================================================
  // Send effects (4 aux sends)
  void setSendLevel(int sendIndex, float level);
  float getSendLevel(int sendIndex) const;

  void setSendPreFader(int sendIndex, bool preFader);
  bool isSendPreFader(int sendIndex) const;

  //==============================================================================
  // Output section
  void setVolume(float volume);
  float getVolume() const { return volume.load(); }

  void setPan(float pan);
  float getPan() const { return this->pan.load(); }

  void setMuted(bool shouldBeMuted);
  bool isMuted() const { return muted.load(); }

  void setSolo(bool shouldBeSolo);
  bool isSolo() const { return solo.load(); }

  // Sol-In-Place Logic
  void setSilencedBySolo(bool silenced) { silencedBySolo.store(silenced); }
  bool isSilencedBySolo() const { return silencedBySolo.load(); }

  //==============================================================================
  // Metering
  float getInputLevel() const { return inputLevel.load(); }
  float getOutputLevel() const { return outputLevel.load(); }
  float getInputPeak() const { return inputPeak.load(); }
  float getOutputPeak() const { return outputPeak.load(); }

  void resetPeaks();

  //==============================================================================
  // State management
  juce::ValueTree getState() const;
  void loadState(const juce::ValueTree &state);

private:
  //==============================================================================
  // Input section
  std::atomic<float> inputGain{0.0f}; // In dB
  std::atomic<bool> phaseInvert{false};

  //==============================================================================
  // High-pass filter
  std::atomic<bool> hpfEnabled{false};
  std::atomic<float> hpfFrequency{20.0f};
  juce::IIRFilter hpfFilterL, hpfFilterR;

  //==============================================================================
  // EQ section
  static constexpr int numEQBands = 4;
  EQBand eqBands[numEQBands];
  juce::IIRFilter eqFiltersL[numEQBands];
  juce::IIRFilter eqFiltersR[numEQBands];

  // RT-safe coefficient swapping
  std::atomic<FilterCoefficients *> activeCoeffs_{nullptr};
  std::unique_ptr<FilterCoefficients> coeffsA_;
  std::unique_ptr<FilterCoefficients> coeffsB_;
  std::atomic<bool> useCoeffsA_{true};
  std::atomic<bool> coeffsDirty_{true};

  // Pre-calculate coefficients on message thread
  void recalculateCoefficients();
  void applyCoefficients(); // Called from audio thread

  //==============================================================================
  // Dynamics section - Now using ProCompressor
  ProCompressor compressor_;
  std::atomic<bool> compressorEnabled{false};
  std::atomic<float> compThreshold{-10.0f};
  std::atomic<float> compRatio{4.0f};
  std::atomic<float> compAttack{10.0f};
  std::atomic<float> compRelease{100.0f};
  std::atomic<float> compMakeup{0.0f};

  //==============================================================================
  // Send effects
  static constexpr int numSends = 4;
  std::atomic<float> sendLevels[numSends];
  std::atomic<bool> sendPreFader[numSends];

  //==============================================================================
  // Output section
  std::atomic<float> volume{0.8f};
  std::atomic<float> pan{0.0f};
  std::atomic<bool> muted{false};
  std::atomic<bool> solo{false};
  std::atomic<bool> silencedBySolo{false};

  // Smoothing for de-zippering automation (Audio Thread)
  juce::LinearSmoothedValue<float> smoothedVolume;
  juce::LinearSmoothedValue<float> smoothedPan;

  //==============================================================================
  // Metering
  std::atomic<float> inputLevel{0.0f};
  std::atomic<float> outputLevel{0.0f};
  std::atomic<float> inputPeak{0.0f};
  std::atomic<float> outputPeak{0.0f};

  //==============================================================================
  // Processing state
  double currentSampleRate = 44100.0;
  int currentBlockSize = 512;

  //==============================================================================
  // Helper methods
  void processInput(juce::AudioBuffer<float> &buffer);
  void processHighPass(juce::AudioBuffer<float> &buffer);
  void processEQ(juce::AudioBuffer<float> &buffer);
  void processCompressor(juce::AudioBuffer<float> &buffer);
  void processSends(const juce::AudioBuffer<float> &sourceBuffer,
                    const std::vector<juce::AudioBuffer<float> *> &sendBuffers,
                    bool matchPreFader);
  void processOutput(juce::AudioBuffer<float> &buffer);
  void updateMeters(const juce::AudioBuffer<float> &buffer, bool isInput);

  float dbToGain(float db) const { return juce::Decibels::decibelsToGain(db); }
  float gainToDb(float gain) const {
    return juce::Decibels::gainToDecibels(gain);
  }

  //==============================================================================
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerChannel)
};

} // namespace zenith
