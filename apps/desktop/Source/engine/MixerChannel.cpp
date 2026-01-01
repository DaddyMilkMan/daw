/*
  ==============================================================================

    MixerChannel.cpp
    Ported from: ZenithDAW-Native/Source/Audio/MixerChannel.cpp (2025-11-11)
    Author:  Zenith DAW → Zenith DAW

    Mixer channel strip implementation with professional compressor

    JUCE 8 / C++20 adaptations:
    - Wrapped in namespace zenith
    - Professional ProCompressor with RMS/Lookahead
    - Lock-free coefficient swapping via ReferenceCountedObject

  ==============================================================================
*/

#include "MixerChannel.h"
#include "../dsp/AudioFifo.h"
#include "../dsp/SIMDHelpers.h"
#include "EngineConstants.h"
#include "RealTimeGarbageCollector.h"

namespace zenith {

//==============================================================================
// ProCompressor Implementation
//==============================================================================

void ProCompressor::prepare(double sampleRate, int maxBlockSize) {
  sampleRate_ = sampleRate;

  // Lookahead buffer (5ms - use constant)
  lookaheadSamples_ =
      static_cast<int>(sampleRate * ::zenith::constants::kCompLookaheadMs / 1000.0);
  lookaheadBuffer_.setSize(2, std::max(1, lookaheadSamples_ + maxBlockSize));
  lookaheadBuffer_.clear();
  lookaheadWritePos_ = 0;

  // RMS buffer (10ms window - use constant)
  rmsWindowSamples_ =
      std::max(1, static_cast<int>(sampleRate * ::zenith::constants::kCompRmsWindowMs / 1000.0));
  rmsBuffer_.assign(rmsWindowSamples_, 0.0f);
  rmsWritePos_ = 0;
  rmsSum_ = 0.0f;

  // Initialize envelopes
  envL_ = 0.0f;
  envR_ = 0.0f;
  gainSmooth_ = 1.0f;

  updateCoefficients();
}

void ProCompressor::reset() {
  lookaheadBuffer_.clear();
  lookaheadWritePos_ = 0;
  std::fill(rmsBuffer_.begin(), rmsBuffer_.end(), 0.0f);
  rmsWritePos_ = 0;
  rmsSum_ = 0.0f;
  envL_ = 0.0f;
  envR_ = 0.0f;
  gainSmooth_ = 1.0f;
}

void ProCompressor::setThreshold(float thresholdDb) {
  threshold_ = thresholdDb;
  updateAutoMakeup();
}
void ProCompressor::setRatio(float ratio) {
  ratio_ = ratio;
  updateAutoMakeup();
}
void ProCompressor::setAttack(float attackMs) {
  attackMs_ = attackMs;
  updateCoefficients();
}
void ProCompressor::setRelease(float releaseMs) {
  releaseMs_ = releaseMs;
  updateCoefficients();
}
void ProCompressor::setMakeup(float makeupDb) {
  makeup_ = makeupDb;
  autoMakeupEnabled_ = false;
}
void ProCompressor::setKnee(float kneeDb) { knee_ = kneeDb; }
void ProCompressor::setAutoMakeup(bool enabled) {
  autoMakeupEnabled_ = enabled;
  if (enabled)
    updateAutoMakeup();
}
void ProCompressor::setLookaheadEnabled(bool enabled) { lookaheadEnabled_ = enabled; }
void ProCompressor::setRmsEnabled(bool enabled) { useRms_ = enabled; }

float ProCompressor::getGainReduction() const {
  return gainReduction_.load();
}

void ProCompressor::process(juce::AudioBuffer<float> &buffer) {
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
    float levelDb = zenith::simd::fastGainToDb(level);

    // Compute gain reduction with soft knee
    float gr = computeGainReduction(levelDb);

    // Smooth the gain change (envelope follower)
    if (gr < gainSmooth_) {
      gainSmooth_ += attackCoeff_ * (gr - gainSmooth_);
    } else {
      gainSmooth_ += releaseCoeff_ * (gr - gainSmooth_);
    }

    // Track max gain reduction for metering
    float grDb = zenith::simd::fastGainToDb(gainSmooth_);
    if (-grDb > maxGR)
      maxGR = -grDb;

    // Apply makeup gain
    float totalGain =
        gainSmooth_ * zenith::simd::fastDbToGain(
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

void ProCompressor::updateCoefficients() {
  if (sampleRate_ <= 0)
    return;
  attackCoeff_ = 1.0f - std::exp(-1.0f / (attackMs_ * 0.001f *
                                          static_cast<float>(sampleRate_)));
  releaseCoeff_ = 1.0f - std::exp(-1.0f / (releaseMs_ * 0.001f *
                                           static_cast<float>(sampleRate_)));
}

void ProCompressor::updateAutoMakeup() {
  float overshoot = -20.0f - threshold_;
  if (overshoot > 0.0f && ratio_ > 1.0f) {
    autoMakeup_ = overshoot * (1.0f - 1.0f / ratio_) * 0.5f;
  } else {
    autoMakeup_ = 0.0f;
  }
}

float ProCompressor::computeGainReduction(float inputDb) const {
  float halfKnee = knee_ * 0.5f;
  float output;

  if (inputDb < threshold_ - halfKnee) {
    output = inputDb;
  } else if (inputDb > threshold_ + halfKnee) {
    output = threshold_ + (inputDb - threshold_) / ratio_;
  } else {
    float safeKnee = std::max(0.1f, knee_);
    float x = inputDb - threshold_ + safeKnee * 0.5f;
    float kneeGain = (1.0f / ratio_ - 1.0f) / (2.0f * safeKnee);
    output = inputDb + kneeGain * x * x;
  }

  float gr = output - inputDb;
  return zenith::simd::fastDbToGain(gr);
}

//==============================================================================
MixerChannel::MixerChannel() {
  // Initialize send levels to 0 (off)
  for (int i = 0; i < numSends; ++i) {
    sendLevels[i].store(0.0f);
    sendPreFader[i].store(false);
  }

  // Initialize EQ bands with sensible defaults
  // Band 0: Low shelf at 100Hz
  eqBands[0].type = EQBand::Type::LowShelf;
  eqBands[0].frequency.store(100.0f);

  // Band 1: Low-mid peak at 500Hz
  eqBands[1].type = EQBand::Type::Peak;
  eqBands[1].frequency.store(500.0f);

  // Band 2: High-mid peak at 2kHz
  eqBands[2].type = EQBand::Type::Peak;
  eqBands[2].frequency.store(2000.0f);

  // Band 3: High shelf at 8kHz
  eqBands[3].type = EQBand::Type::HighShelf;
  eqBands[3].frequency.store(8000.0f);

  // Initialize coefficient buffers
  auto* coeffs = new FilterCoefficients();
  coeffs->incReferenceCount(); // Held by activeCoeffs_
  activeCoeffs_.store(coeffs);

  consoleEmulation = std::make_unique<zenith::effects::ConsoleEmulation>();
}

MixerChannel::~MixerChannel() {}

//==============================================================================
void MixerChannel::prepareToPlay(int samplesPerBlockExpected,
                                 double sampleRate) {
  currentSampleRate = sampleRate;
  currentBlockSize = samplesPerBlockExpected;

  juce::dsp::ProcessSpec spec;
  spec.sampleRate = sampleRate;
  spec.maximumBlockSize = samplesPerBlockExpected;
  spec.numChannels = 2;

  // Prepare Console Emulation
  if (consoleEmulation) {
    consoleEmulation->prepare(spec);
    consoleEmulation->setMode(consoleMode.load());
    consoleEmulation->setDrive(consoleDrive.load());
    consoleEmulation->setCharacter(consoleCharacter.load());
  }

  // Prepare Gain
  gain.prepare(spec);
  gain.setRampDurationSeconds(0.05); // Smooth volume changes

  // Prepare ProCompressor
  compressor_.prepare(sampleRate, samplesPerBlockExpected);
  compressor_.setThreshold(compThreshold.load());
  compressor_.setRatio(compRatio.load());
  compressor_.setAttack(compAttack.load());
  compressor_.setRelease(compRelease.load());
  compressor_.setMakeup(compMakeup.load());

  // Reset filters
  hpfFilterL.reset();
  hpfFilterR.reset();
  for (int i = 0; i < numEQBands; ++i) {
    eqFiltersL[i].reset();
    eqFiltersR[i].reset();
  }

  // Pre-calculate filter coefficients on message thread
  recalculateCoefficients();
  // Ensure filters are updated before playback
  updateFiltersFromCoefficients();
}

void MixerChannel::releaseResources() {
  // Reset filter states
  for (int i = 0; i < numEQBands; ++i) {
    eqFiltersL[i].reset();
    eqFiltersR[i].reset();
  }

  hpfFilterL.reset();
  hpfFilterR.reset();

  hpfFilterR.reset();

  compressor_.reset();
  if (consoleEmulation)
    consoleEmulation->reset();
}

void MixerChannel::recalculateCoefficients() {
  // Called from message thread - pre-calculate all coefficients
  if (currentSampleRate <= 0)
    return;

  // Create NEW coefficients object
  auto newCoeffs = new FilterCoefficients();

  // Calculate HPF coefficients
  auto hpfK = juce::IIRCoefficients::makeHighPass(currentSampleRate,
                                                  hpfFrequency.load());
  newCoeffs->hpf = {hpfK.coefficients[0], hpfK.coefficients[1],
                    hpfK.coefficients[2], 1.0,
                    hpfK.coefficients[3], hpfK.coefficients[4]};

  // Calculate EQ coefficients
  for (int i = 0; i < numEQBands; ++i) {
    const float freq = eqBands[i].frequency.load();
    const float gain = eqBands[i].gain.load();
    const float q = eqBands[i].q.load();

    juce::IIRCoefficients coeffs;

    switch (eqBands[i].type) {
    case EQBand::Type::LowShelf:
      coeffs = juce::IIRCoefficients::makeLowShelf(currentSampleRate, freq, q,
                                                   dbToGain(gain));
      break;

    case EQBand::Type::Peak:
      coeffs = juce::IIRCoefficients::makePeakFilter(currentSampleRate, freq, q,
                                                     dbToGain(gain));
      break;

    case EQBand::Type::HighShelf:
      coeffs = juce::IIRCoefficients::makeHighShelf(currentSampleRate, freq, q,
                                                    dbToGain(gain));
      break;
    }

    newCoeffs->eq[i] = {coeffs.coefficients[0], coeffs.coefficients[1],
                        coeffs.coefficients[2], 1.0,
                        coeffs.coefficients[3], coeffs.coefficients[4]};
  }

  // Atomically swap pointer - old coefficients will be released by GC
  // FilterCoefficients uses juce::ReferenceCountedObject for lifetime management.
  newCoeffs->incReferenceCount(); // Held by activeCoeffs_
  auto* oldCoeffs = activeCoeffs_.exchange(newCoeffs);
  
  if (oldCoeffs)
  {
      // Pass the old pointer to GC, it will decrement the refcount when safe
      juce::ReferenceCountedObjectPtr<FilterCoefficients> ptr(oldCoeffs);
      oldCoeffs->decReferenceCount(); // Transfer ownership to smart pointer
      RealTimeGarbageCollector::getInstance().deferDelete(ptr);
  }
}

void MixerChannel::updateFiltersFromCoefficients() {
  // Safe atomic retrieval of current coefficients
  // We don't increment refcount here as we assume the pointer is valid 
  // for the duration of this call (GC has 1s safety buffer)
  auto* localCoeffs = activeCoeffs_.load(std::memory_order_acquire);

  if (localCoeffs == nullptr)
    return;

  // Apply HPF
  juce::IIRCoefficients hpf(localCoeffs->hpf[0], localCoeffs->hpf[1],
                            localCoeffs->hpf[2], localCoeffs->hpf[3],
                            localCoeffs->hpf[4], localCoeffs->hpf[5]);
  hpfFilterL.setCoefficients(hpf);
  hpfFilterR.setCoefficients(hpf);

  // Apply EQ bands
  for (int i = 0; i < numEQBands; ++i) {
    juce::IIRCoefficients eq(localCoeffs->eq[i][0], localCoeffs->eq[i][1],
                             localCoeffs->eq[i][2], localCoeffs->eq[i][3],
                             localCoeffs->eq[i][4], localCoeffs->eq[i][5]);
    eqFiltersL[i].setCoefficients(eq);
    eqFiltersR[i].setCoefficients(eq);
  }
}

void MixerChannel::getNextAudioBlock(
    const juce::AudioSourceChannelInfo &bufferToFill) {
  getNextAudioBlock(bufferToFill, {});
}

void MixerChannel::getNextAudioBlock(
    const juce::AudioSourceChannelInfo &bufferToFill,
    const std::vector<juce::AudioBuffer<float> *> &auxBuffers) {

  if (muted.load() || silencedBySolo.load()) {
    bufferToFill.clearActiveBufferRegion();
    inputLevel.store(0.0f);
    outputLevel.store(0.0f);
    return;
  }

  // Update filter coefficients safely before processing block
  updateFiltersFromCoefficients();

  // Create a local buffer for processing
  juce::AudioBuffer<float> localBuffer(
      bufferToFill.buffer->getArrayOfWritePointers(),
      bufferToFill.buffer->getNumChannels(), bufferToFill.startSample,
      bufferToFill.numSamples);

  // Update input meters
  updateMeters(localBuffer, true);

  // Process input section (gain, phase invert)
  processInput(localBuffer);

  // Process high-pass filter
  if (hpfEnabled.load()) {
    processHighPass(localBuffer);
  }

  // Process EQ
  processEQ(localBuffer);

  // Process compressor
  if (compressorEnabled.load()) {
    processCompressor(localBuffer);
  }

  // Process PRE-FADER Sends
  processSends(localBuffer, auxBuffers, true);

  // Process output section (pan, volume)
  processOutput(localBuffer);

  // Process POST-FADER Sends
  processSends(localBuffer, auxBuffers, false);

  // Update output meters
  updateMeters(localBuffer, false);
}

void MixerChannel::processSends(
    const juce::AudioBuffer<float> &sourceBuffer,
    const std::vector<juce::AudioBuffer<float> *> &sendBuffers,
    bool matchPreFader) {
  for (int i = 0; i < numSends && i < static_cast<int>(sendBuffers.size());
       ++i) {
    // Check if this send matches the requested pre/post mode
    if (sendPreFader[i].load() != matchPreFader)
      continue;

    auto *sendBuffer = sendBuffers[i];
    if (sendBuffer == nullptr)
      continue;

    const float level = sendLevels[i].load();
    if (level <= 0.001f)
      continue;

    // Apply send level
    const int numChannels =
        juce::jmin(sourceBuffer.getNumChannels(), sendBuffer->getNumChannels());
    const int numSamples = sourceBuffer.getNumSamples();

    for (int ch = 0; ch < numChannels; ++ch) {
      sendBuffer->addFrom(ch, 0, sourceBuffer, ch, 0, numSamples, level);
    }
  }
}

//==============================================================================
void MixerChannel::setInputGain(float gainInDb) {
  inputGain.store(juce::jlimit(-60.0f, 24.0f, gainInDb));
  sendChangeMessage();
}

void MixerChannel::setPhaseInvert(bool shouldInvert) {
  phaseInvert.store(shouldInvert);
  sendChangeMessage();
}

//==============================================================================
void MixerChannel::setHighPassEnabled(bool enabled) {
  hpfEnabled.store(enabled);
  sendChangeMessage();
}

void MixerChannel::setHighPassFrequency(float frequency) {
  hpfFrequency.store(juce::jlimit(20.0f, 500.0f, frequency));
  // Pre-calculate coefficients on message thread (RT-safe)
  recalculateCoefficients();
  sendChangeMessage();
}

//==============================================================================
MixerChannel::EQBand &MixerChannel::getEQBand(int bandIndex) {
  jassert(juce::isPositiveAndBelow(bandIndex, numEQBands));
  return eqBands[bandIndex];
}

const MixerChannel::EQBand &MixerChannel::getEQBand(int bandIndex) const {
  jassert(juce::isPositiveAndBelow(bandIndex, numEQBands));
  return eqBands[bandIndex];
}

void MixerChannel::markEQDirty(int bandIndex) {
  juce::ignoreUnused(bandIndex);
  // Pre-calculate coefficients on message thread (RT-safe)
  recalculateCoefficients();
  sendChangeMessage();
}

//==============================================================================
void MixerChannel::setCompressorEnabled(bool enabled) {
  compressorEnabled.store(enabled);
  sendChangeMessage();
}

void MixerChannel::setCompressorThreshold(float thresholdDb) {
  compThreshold.store(juce::jlimit(-60.0f, 0.0f, thresholdDb));
  compressor_.setThreshold(thresholdDb);
  sendChangeMessage();
}

void MixerChannel::setCompressorRatio(float ratio) {
  compRatio.store(juce::jlimit(1.0f, 20.0f, ratio));
  compressor_.setRatio(ratio);
  sendChangeMessage();
}

void MixerChannel::setCompressorAttack(float attackMs) {
  compAttack.store(juce::jlimit<float>(::zenith::constants::kMinCompAttackMs,
                                ::zenith::constants::kMaxCompAttackMs, attackMs));
  compressor_.setAttack(attackMs);
  sendChangeMessage();
}

void MixerChannel::setCompressorRelease(float releaseMs) {
  compRelease.store(juce::jlimit<float>(::zenith::constants::kMinCompReleaseMs,
                                 ::zenith::constants::kMaxCompReleaseMs, releaseMs));
  compressor_.setRelease(releaseMs);
  sendChangeMessage();
}

void MixerChannel::setCompressorMakeup(float makeupDb) {
  compMakeup.store(juce::jlimit(0.0f, 24.0f, makeupDb));
  compressor_.setMakeup(makeupDb);
  sendChangeMessage();
}

void MixerChannel::setCompressorKnee(float kneeDb) {
  compressor_.setKnee(juce::jlimit(0.0f, 24.0f, kneeDb));
  sendChangeMessage();
}

void MixerChannel::setCompressorLookahead(bool enabled) {
  compressor_.setLookaheadEnabled(enabled);
  sendChangeMessage();
}

void MixerChannel::setCompressorRmsMode(bool useRms) {
  compressor_.setRmsEnabled(useRms);
  sendChangeMessage();
}

void MixerChannel::setCompressorAutoMakeup(bool enabled) {
  compressor_.setAutoMakeup(enabled);
  sendChangeMessage();
}

//==============================================================================
void MixerChannel::setSendLevel(int sendIndex, float level) {
  if (juce::isPositiveAndBelow(sendIndex, numSends)) {
    sendLevels[sendIndex].store(juce::jlimit(0.0f, 1.0f, level));
    sendChangeMessage();
  }
}

float MixerChannel::getSendLevel(int sendIndex) const {
  if (juce::isPositiveAndBelow(sendIndex, numSends)) {
    return sendLevels[sendIndex].load();
  }
  return 0.0f;
}

void MixerChannel::setSendPreFader(int sendIndex, bool preFader) {
  if (juce::isPositiveAndBelow(sendIndex, numSends)) {
    sendPreFader[sendIndex].store(preFader);
    sendChangeMessage();
  }
}

bool MixerChannel::isSendPreFader(int sendIndex) const {
  if (juce::isPositiveAndBelow(sendIndex, numSends)) {
    return sendPreFader[sendIndex].load();
  }
  return false;
}

//==============================================================================
void MixerChannel::setVolume(float newVolume) {
  volume.store(juce::jlimit(0.0f, 1.0f, newVolume));
  sendChangeMessage();
}

void MixerChannel::setPan(float newPan) {
  pan.store(juce::jlimit(-1.0f, 1.0f, newPan));
  sendChangeMessage();
}

void MixerChannel::setMuted(bool shouldBeMuted) {
  muted.store(shouldBeMuted);
  sendChangeMessage();
}

void MixerChannel::setSolo(bool shouldBeSolo) {
  solo.store(shouldBeSolo);
  sendChangeMessage();
}

//==============================================================================
void MixerChannel::setConsoleMode(
    zenith::effects::ConsoleEmulation::Mode mode) {
  consoleMode.store(mode);
  if (consoleEmulation) {
    consoleEmulation->setMode(mode);
  }
  sendChangeMessage();
}

zenith::effects::ConsoleEmulation::Mode MixerChannel::getConsoleMode() const {
  return consoleMode.load();
}

void MixerChannel::setConsoleDrive(float drive) {
  consoleDrive.store(juce::jlimit(0.0f, 1.0f, drive));
  if (consoleEmulation) {
    consoleEmulation->setDrive(drive);
  }
  sendChangeMessage();
}

float MixerChannel::getConsoleDrive() const { return consoleDrive.load(); }

void MixerChannel::setConsoleCharacter(float character) {
  consoleCharacter.store(juce::jlimit(0.0f, 1.0f, character));
  if (consoleEmulation) {
    consoleEmulation->setCharacter(character);
  }
  sendChangeMessage();
}

float MixerChannel::getConsoleCharacter() const {
  return consoleCharacter.load();
}

//==============================================================================
juce::ValueTree MixerChannel::getState() const {
  juce::ValueTree state("MixerChannel");

  // Input section
  state.setProperty("inputGain", inputGain.load(), nullptr);
  state.setProperty("phaseInvert", phaseInvert.load(), nullptr);

  // High-pass filter
  state.setProperty("hpfEnabled", hpfEnabled.load(), nullptr);
  state.setProperty("hpfFrequency", hpfFrequency.load(), nullptr);

  // EQ bands
  for (int i = 0; i < numEQBands; ++i) {
    juce::ValueTree bandState("EQBand");
    bandState.setProperty("enabled", eqBands[i].enabled.load(), nullptr);
    bandState.setProperty("frequency", eqBands[i].frequency.load(), nullptr);
    bandState.setProperty("gain", eqBands[i].gain.load(), nullptr);
    bandState.setProperty("q", eqBands[i].q.load(), nullptr);
    bandState.setProperty("type", static_cast<int>(eqBands[i].type), nullptr);
    state.appendChild(bandState, nullptr);
  }

  // Compressor
  state.setProperty("compressorEnabled", compressorEnabled.load(), nullptr);
  state.setProperty("compThreshold", compThreshold.load(), nullptr);
  state.setProperty("compRatio", compRatio.load(), nullptr);
  state.setProperty("compAttack", compAttack.load(), nullptr);
  state.setProperty("compRelease", compRelease.load(), nullptr);
  state.setProperty("compMakeup", compMakeup.load(), nullptr);

  // Console Emulation
  state.setProperty("consoleMode", static_cast<int>(consoleMode.load()),
                    nullptr);
  state.setProperty("consoleDrive", consoleDrive.load(), nullptr);
  state.setProperty("consoleCharacter", consoleCharacter.load(), nullptr);

  // Sends
  for (int i = 0; i < numSends; ++i) {
    juce::ValueTree sendState("Send");
    sendState.setProperty("level", sendLevels[i].load(), nullptr);
    sendState.setProperty("preFader", sendPreFader[i].load(), nullptr);
    state.appendChild(sendState, nullptr);
  }

  // Output
  state.setProperty("volume", volume.load(), nullptr);
  state.setProperty("pan", pan.load(), nullptr);
  state.setProperty("muted", muted.load(), nullptr);
  state.setProperty("solo", solo.load(), nullptr);

  return state;
}

void MixerChannel::loadState(const juce::ValueTree &state) {
  if (!state.hasType("MixerChannel"))
    return;

  // Input section
  inputGain.store(state.getProperty("inputGain", 0.0f));
  phaseInvert.store(state.getProperty("phaseInvert", false));

  // High-pass filter
  hpfEnabled.store(state.getProperty("hpfEnabled", false));
  hpfFrequency.store(state.getProperty("hpfFrequency", 20.0f));

  // EQ bands
  int bandIndex = 0;
  for (const auto &child : state) {
    if (child.hasType("EQBand") && bandIndex < numEQBands) {
      eqBands[bandIndex].enabled.store(child.getProperty("enabled", false));
      eqBands[bandIndex].frequency.store(
          child.getProperty("frequency", 1000.0f));
      eqBands[bandIndex].gain.store(child.getProperty("gain", 0.0f));
      eqBands[bandIndex].q.store(child.getProperty("q", 0.707f));
      eqBands[bandIndex].type = static_cast<EQBand::Type>(
          static_cast<int>(child.getProperty("type", 1)));
      ++bandIndex;
    }
  }

  // Compressor
  compressorEnabled.store(state.getProperty("compressorEnabled", false));
  compThreshold.store(
      state.getProperty("compThreshold", constants::kDefaultCompThresholdDb));
  compRatio.store(state.getProperty("compRatio", constants::kDefaultCompRatio));
  compAttack.store(
      state.getProperty("compAttack", constants::kDefaultCompAttackMs));
  compRelease.store(
      state.getProperty("compRelease", constants::kDefaultCompReleaseMs));
  compMakeup.store(state.getProperty("compMakeup", 0.0f));

  // Console Emulation
  consoleMode.store(static_cast<zenith::effects::ConsoleEmulation::Mode>(
      static_cast<int>(state.getProperty("consoleMode", 0))));
  consoleDrive.store(state.getProperty("consoleDrive", 0.1f));
  consoleCharacter.store(state.getProperty("consoleCharacter", 0.0f));

  if (consoleEmulation) {
    consoleEmulation->setMode(consoleMode.load());
    consoleEmulation->setDrive(consoleDrive.load());
    consoleEmulation->setCharacter(consoleCharacter.load());
  }

  // Update ProCompressor with loaded values
  compressor_.setThreshold(compThreshold.load());
  compressor_.setRatio(compRatio.load());
  compressor_.setAttack(compAttack.load());
  compressor_.setRelease(compRelease.load());
  compressor_.setMakeup(compMakeup.load());

  // Sends
  int sendIndex = 0;
  for (const auto &child : state) {
    if (child.hasType("Send") && sendIndex < numSends) {
      sendLevels[sendIndex].store(child.getProperty("level", 0.0f));
      sendPreFader[sendIndex].store(child.getProperty("preFader", false));
      ++sendIndex;
    }
  }

  // Output
  volume.store(state.getProperty("volume", 0.8f));
  pan.store(state.getProperty("pan", 0.0f));
  muted.store(state.getProperty("muted", false));
  solo.store(state.getProperty("solo", false));

  recalculateCoefficients();
  sendChangeMessage();
}

//==============================================================================
void MixerChannel::processInput(juce::AudioBuffer<float> &buffer) {
  const float gainLinear = dbToGain(inputGain.load());
  const bool invertPhase = phaseInvert.load();

  buffer.applyGain(gainLinear);

  if (invertPhase) {
    buffer.applyGain(-1.0f);
  }

  // Apply Console Emulation (Saturation/Color)
  if (consoleEmulation) {
    consoleEmulation->process(buffer);
  }
}

void MixerChannel::processHighPass(juce::AudioBuffer<float> &buffer) {
  if (buffer.getNumChannels() == 0 || buffer.getNumSamples() == 0)
    return;

  hpfFilterL.processSamples(buffer.getWritePointer(0), buffer.getNumSamples());
  if (buffer.getNumChannels() > 1)
    hpfFilterR.processSamples(buffer.getWritePointer(1),
                              buffer.getNumSamples());
}

void MixerChannel::processEQ(juce::AudioBuffer<float> &buffer) {
  for (int i = 0; i < numEQBands; ++i) {
    if (eqBands[i].enabled.load()) {
      eqFiltersL[i].processSamples(buffer.getWritePointer(0),
                                   buffer.getNumSamples());
      if (buffer.getNumChannels() > 1)
        eqFiltersR[i].processSamples(buffer.getWritePointer(1),
                                     buffer.getNumSamples());
    }
  }
}

void MixerChannel::processCompressor(juce::AudioBuffer<float> &buffer) {
  compressor_.process(buffer);
}

void MixerChannel::processOutput(juce::AudioBuffer<float> &buffer) {
  if (buffer.getNumChannels() == 0 || buffer.getNumSamples() == 0)
    return;

  gain.setGainLinear(volume.load());

  // Apply gain using SIMD helper for safety (avoids AudioBlock assertions)
  buffer.applyGain(volume.load());
}

void MixerChannel::updateMeters(const juce::AudioBuffer<float> &buffer,
                                bool isInput) {
  if (isInput)
    inputMeter.process(buffer);
  else
    outputMeter.process(buffer);
}

} // namespace zenith
