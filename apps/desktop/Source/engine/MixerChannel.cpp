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

  // Prepare Gain - volume is applied directly in processOutput, so no separate
  // gain processor needed If gain refers to a specific processor not in header,
  // we remove it. MixerChannel.h has no 'gain' member of type GainProcessor.

  // Prepare ProCompressor
  compressor_.prepare(sampleRate, samplesPerBlockExpected);
  compressor_.setThreshold(compThreshold.load());
  compressor_.setRatio(compRatio.load());
  compressor_.setAttack(compAttack.load());
  compressor_.setRelease(compRelease.load());
  compressor_.setMakeup(compMakeup.load());

  // Pre-calculate filter coefficients on message thread
  recalculateCoefficients();
  // Ensure filters are updated before playback
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
  if (currentSampleRate <= 0)
    return;

  // Write to BACK buffer (inverse of current)
  // If useCoeffsA is true, active is A, we write to B.
  FilterCoefficients *targetCoeffs =
      useCoeffsA_.load() ? coeffsB_.get() : coeffsA_.get();

  // Calculate HPF coefficients
  auto hpfK = juce::IIRCoefficients::makeHighPass(currentSampleRate,
                                                  hpfFrequency.load());
  targetCoeffs->hpf = {hpfK.coefficients[0], hpfK.coefficients[1],
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

    targetCoeffs->eq[i] = {coeffs.coefficients[0], coeffs.coefficients[1],
                           coeffs.coefficients[2], 1.0,
                           coeffs.coefficients[3], coeffs.coefficients[4]};
  }

  // Swap buffers
  bool usingA = useCoeffsA_.load();
  useCoeffsA_.store(!usingA);
  activeCoeffs_.store(targetCoeffs);
  coeffsDirty_.store(true);
}

void MixerChannel::applyCoefficients() {
  if (!coeffsDirty_.load())
    return;

  // Safe atomic retrieval of current coefficients
  FilterCoefficients *localCoeffs = activeCoeffs_.load();

  if (!localCoeffs)
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

  coeffsDirty_.store(false);
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
  compAttack.store(juce::jlimit(constants::kMinCompAttackMs,
                                constants::kMaxCompAttackMs, attackMs));
  compressor_.setAttack(attackMs);
  sendChangeMessage();
}

void MixerChannel::setCompressorRelease(float releaseMs) {
  compRelease.store(juce::jlimit(constants::kMinCompReleaseMs,
                                 constants::kMaxCompReleaseMs, releaseMs));
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
// Console methods removed

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

  // Console Emulation removed

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

  // Console Emulation removed (not in header)

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
  // Console Emulation removed
}

void MixerChannel::processHighPass(juce::AudioBuffer<float> &buffer) {
  juce::dsp::AudioBlock<float> block(buffer);
  juce::dsp::ProcessContextReplacing<float> context(block);
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
  // Apply volume directly
  buffer.applyGain(volume.load());
}

void MixerChannel::updateMeters(const juce::AudioBuffer<float> &buffer,
                                bool isInput) {
  float rms = buffer.getRMSLevel(0, 0, buffer.getNumSamples());
  if (buffer.getNumChannels() > 1) {
    rms = std::max(rms, buffer.getRMSLevel(1, 0, buffer.getNumSamples()));
  }

  // Simple decay for peak
  float magnitude = buffer.getMagnitude(0, buffer.getNumSamples());

  if (isInput) {
    inputLevel.store(rms);
    float currentPeak = inputPeak.load();
    if (magnitude > currentPeak) {
      inputPeak.store(magnitude);
    } else {
      inputPeak.store(currentPeak * 0.95f); // Simple decay
    }
  } else {
    outputLevel.store(rms);
    float currentPeak = outputPeak.load();
    if (magnitude > currentPeak) {
      outputPeak.store(magnitude);
    } else {
      outputPeak.store(currentPeak * 0.95f); // Simple decay
    }
  }
}

} // namespace zenith
