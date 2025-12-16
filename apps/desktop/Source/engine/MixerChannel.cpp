/*
  ==============================================================================

    MixerChannel.cpp
    Ported from: ZenithDAW-Native/Source/Audio/MixerChannel.cpp (2025-11-11)
    Author:  Zenith DAW → Zenith DAW

    Mixer channel strip implementation with professional compressor

    JUCE 8 / C++20 adaptations:
    - Wrapped in namespace zenith
    - Professional ProCompressor with RMS/Lookahead
    - Lock-free coefficient swapping

  ==============================================================================
*/

#include "MixerChannel.h"
#include "EngineConstants.h"
#include "../dsp/SIMDHelpers.h"
#include "../dsp/AudioFifo.h"

namespace zenith {

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
  coeffsA_ = std::make_unique<FilterCoefficients>();
  coeffsB_ = std::make_unique<FilterCoefficients>();
  activeCoeffs_.store(coeffsA_.get());
}

MixerChannel::~MixerChannel() {}

//==============================================================================
void MixerChannel::prepareToPlay(int samplesPerBlockExpected,
                                 double sampleRate) {
  currentSampleRate = sampleRate;
  currentBlockSize = samplesPerBlockExpected;

  // Prepare ProCompressor
  compressor_.prepare(sampleRate, samplesPerBlockExpected);
  compressor_.setThreshold(compThreshold.load());
  compressor_.setRatio(compRatio.load());
  compressor_.setAttack(compAttack.load());
  compressor_.setRelease(compRelease.load());
  compressor_.setMakeup(compMakeup.load());
  
  // Pre-calculate filter coefficients on message thread
  recalculateCoefficients();
  applyCoefficients();
}

void MixerChannel::releaseResources() {
  // Reset filter states
  for (int i = 0; i < numEQBands; ++i) {
    eqFiltersL[i].reset();
    eqFiltersR[i].reset();
  }

  hpfFilterL.reset();
  hpfFilterR.reset();
  
  compressor_.reset();
}

void MixerChannel::recalculateCoefficients() {
  // Called from message thread - pre-calculate all coefficients
  if (currentSampleRate <= 0) return;
  
  // Get the inactive buffer to write to
  FilterCoefficients* target = useCoeffsA_.load() ? coeffsB_.get() : coeffsA_.get();
  
  // Calculate HPF coefficients
  auto hpfCoeffs = juce::IIRCoefficients::makeHighPass(currentSampleRate, hpfFrequency.load());
  target->hpf = {hpfCoeffs.coefficients[0], hpfCoeffs.coefficients[1], hpfCoeffs.coefficients[2],
                 1.0, hpfCoeffs.coefficients[3], hpfCoeffs.coefficients[4]};
  
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
    
    target->eq[i] = {coeffs.coefficients[0], coeffs.coefficients[1], coeffs.coefficients[2],
                     1.0, coeffs.coefficients[3], coeffs.coefficients[4]};
  }
  
  // Mark for swap
  coeffsDirty_.store(true);
}

void MixerChannel::applyCoefficients() {
  // Called from audio thread - apply pre-calculated coefficients
  if (!coeffsDirty_.load()) return;
  
  // Swap to the newly calculated coefficients
  bool useA = useCoeffsA_.load();
  FilterCoefficients* newCoeffs = useA ? coeffsB_.get() : coeffsA_.get();
  
  // Apply HPF
  juce::IIRCoefficients hpf(newCoeffs->hpf[0], newCoeffs->hpf[1], newCoeffs->hpf[2],
                            newCoeffs->hpf[3], newCoeffs->hpf[4], newCoeffs->hpf[5]);
  hpfFilterL.setCoefficients(hpf);
  hpfFilterR.setCoefficients(hpf);
  
  // Apply EQ bands
  for (int i = 0; i < numEQBands; ++i) {
    juce::IIRCoefficients eq(newCoeffs->eq[i][0], newCoeffs->eq[i][1], newCoeffs->eq[i][2],
                             newCoeffs->eq[i][3], newCoeffs->eq[i][4], newCoeffs->eq[i][5]);
    eqFiltersL[i].setCoefficients(eq);
    eqFiltersR[i].setCoefficients(eq);
  }
  
  // Swap active buffer
  activeCoeffs_.store(newCoeffs);
  useCoeffsA_.store(!useA);
  coeffsDirty_.store(false);
}

void MixerChannel::getNextAudioBlock(
    const juce::AudioSourceChannelInfo &bufferToFill) {
  getNextAudioBlock(bufferToFill, {});
}

void MixerChannel::getNextAudioBlock(
    const juce::AudioSourceChannelInfo &bufferToFill,
    const std::vector<juce::AudioBuffer<float>*>& auxBuffers) {
    
  if (muted.load() || silencedBySolo.load()) {
    bufferToFill.clearActiveBufferRegion();
    inputLevel.store(0.0f);
    outputLevel.store(0.0f);
    return;
  }

  // Apply any pending coefficient changes (RT-safe: just pointer swap)
  applyCoefficients();

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
  compAttack.store(juce::jlimit(constants::kMinCompAttackMs, constants::kMaxCompAttackMs, attackMs));
  compressor_.setAttack(attackMs);
  sendChangeMessage();
}

void MixerChannel::setCompressorRelease(float releaseMs) {
  compRelease.store(juce::jlimit(constants::kMinCompReleaseMs, constants::kMaxCompReleaseMs, releaseMs));
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
void MixerChannel::resetPeaks() {
  inputPeak.store(0.0f);
  outputPeak.store(0.0f);
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
  compThreshold.store(state.getProperty("compThreshold", zenith::constants::kDefaultCompThresholdDb));
  compRatio.store(state.getProperty("compRatio", zenith::constants::kDefaultCompRatio));
  compAttack.store(state.getProperty("compAttack", zenith::constants::kDefaultCompAttackMs));
  compRelease.store(state.getProperty("compRelease", zenith::constants::kDefaultCompReleaseMs));
  compMakeup.store(state.getProperty("compMakeup", 0.0f));
  
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
}

void MixerChannel::processHighPass(juce::AudioBuffer<float> &buffer) {
  if (buffer.getNumChannels() >= 1)
    hpfFilterL.processSamples(buffer.getWritePointer(0),
                              buffer.getNumSamples());

  if (buffer.getNumChannels() >= 2)
    hpfFilterR.processSamples(buffer.getWritePointer(1),
                              buffer.getNumSamples());
}

void MixerChannel::processEQ(juce::AudioBuffer<float> &buffer) {
  for (int band = 0; band < numEQBands; ++band) {
    if (eqBands[band].enabled.load()) {
      if (buffer.getNumChannels() >= 1)
        eqFiltersL[band].processSamples(buffer.getWritePointer(0),
                                        buffer.getNumSamples());

      if (buffer.getNumChannels() >= 2)
        eqFiltersR[band].processSamples(buffer.getWritePointer(1),
                                        buffer.getNumSamples());
    }
  }
}

void MixerChannel::processCompressor(juce::AudioBuffer<float> &buffer) {
  // Use the professional ProCompressor
  compressor_.process(buffer);
}

void MixerChannel::processOutput(juce::AudioBuffer<float> &buffer) {
  const float vol = volume.load();
  const float panValue = pan.load();

  // Calculate left and right gains from pan (-3dB center, constant power)
  const float piOver4 = juce::MathConstants<float>::pi / 4.0f;
  const float leftGain = vol * std::cos(piOver4 * (1.0f + panValue));
  const float rightGain = vol * std::sin(piOver4 * (1.0f + panValue));

  if (buffer.getNumChannels() >= 2) {
    buffer.applyGain(0, 0, buffer.getNumSamples(), leftGain);
    buffer.applyGain(1, 0, buffer.getNumSamples(), rightGain);
  } else if (buffer.getNumChannels() == 1) {
    buffer.applyGain(0, 0, buffer.getNumSamples(), vol);
  }
  
  // Push to visualizer (post-fader, post-pan)
  if (auto* fifo = spectrumFifo_.load(std::memory_order_relaxed)) {
      fifo->pushStereoAsMono(buffer, buffer.getNumSamples());
  }
}

void MixerChannel::updateMeters(const juce::AudioBuffer<float> &buffer,
                                bool isInput) {
  float maxLevel = 0.0f;

  for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
    const float *channelData = buffer.getReadPointer(ch);
    for (int i = 0; i < buffer.getNumSamples(); ++i) {
      const float absValue = std::abs(channelData[i]);
      if (absValue > maxLevel)
        maxLevel = absValue;
    }
  }

  if (isInput) {
    const float currentLevelValue = inputLevel.load();
    const float smoothingFactor = 0.3f;
    const float newLevel = currentLevelValue * (1.0f - smoothingFactor) +
                           maxLevel * smoothingFactor;
    inputLevel.store(newLevel);

    if (maxLevel > inputPeak.load())
      inputPeak.store(maxLevel);
  } else {
    const float currentLevelValue = outputLevel.load();
    const float smoothingFactor = 0.3f;
    const float newLevel = currentLevelValue * (1.0f - smoothingFactor) +
                           maxLevel * smoothingFactor;
    outputLevel.store(newLevel);

    if (maxLevel > outputPeak.load())
      outputPeak.store(maxLevel);
  }
}

} // namespace zenith
