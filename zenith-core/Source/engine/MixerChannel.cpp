/*
  ==============================================================================

    MixerChannel.cpp
    Ported from: ZenithDAW-Native/Source/Audio/MixerChannel.cpp (2025-11-11)
    Author:  Zenith DAW → Zenith DAW

    Mixer channel strip implementation

    JUCE 8 / C++20 adaptations:
    - Wrapped in namespace zenith
    - No container changes needed

  ==============================================================================
*/

#include "MixerChannel.h"

namespace zenith {

//==============================================================================
MixerChannel::MixerChannel()
{
    // Initialize send levels to 0 (off)
    for (int i = 0; i < numSends; ++i)
    {
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
}

MixerChannel::~MixerChannel()
{
}

//==============================================================================
void MixerChannel::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlockExpected;

    // Update all filter coefficients
    updateFilterCoefficients();
    updateCompressorCoefficients();
}

void MixerChannel::releaseResources()
{
    // Reset filter states
    for (int i = 0; i < numEQBands; ++i)
    {
        eqFiltersL[i].reset();
        eqFiltersR[i].reset();
    }

    hpfFilterL.reset();
    hpfFilterR.reset();
}

void MixerChannel::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    if (muted.load())
    {
        bufferToFill.clearActiveBufferRegion();
        inputLevel.store(0.0f);
        outputLevel.store(0.0f);
        return;
    }

    // Create a local buffer for processing
    juce::AudioBuffer<float> localBuffer(
        bufferToFill.buffer->getArrayOfWritePointers(),
        bufferToFill.buffer->getNumChannels(),
        bufferToFill.startSample,
        bufferToFill.numSamples);

    // Update input meters
    updateMeters(localBuffer, true);

    // Process input section (gain, phase invert)
    processInput(localBuffer);

    // Process high-pass filter
    if (hpfEnabled.load())
    {
        processHighPass(localBuffer);
    }

    // Process EQ
    processEQ(localBuffer);

    // Process compressor
    if (compressorEnabled.load())
    {
        processCompressor(localBuffer);
    }

    // TODO(zenith-core#1): Process sends (would need references to send buses)

    // Process output section (pan, volume)
    processOutput(localBuffer);

    // Update output meters
    updateMeters(localBuffer, false);
}

//==============================================================================
void MixerChannel::setInputGain(float gainInDb)
{
    inputGain.store(juce::jlimit(-60.0f, 24.0f, gainInDb));
    sendChangeMessage();
}

void MixerChannel::setPhaseInvert(bool shouldInvert)
{
    phaseInvert.store(shouldInvert);
    sendChangeMessage();
}

//==============================================================================
void MixerChannel::setHighPassEnabled(bool enabled)
{
    hpfEnabled.store(enabled);
    sendChangeMessage();
}

void MixerChannel::setHighPassFrequency(float frequency)
{
    hpfFrequency.store(juce::jlimit(20.0f, 500.0f, frequency));
    updateFilterCoefficients();
    sendChangeMessage();
}

//==============================================================================
MixerChannel::EQBand& MixerChannel::getEQBand(int bandIndex)
{
    jassert(juce::isPositiveAndBelow(bandIndex, numEQBands));
    return eqBands[bandIndex];
}

const MixerChannel::EQBand& MixerChannel::getEQBand(int bandIndex) const
{
    jassert(juce::isPositiveAndBelow(bandIndex, numEQBands));
    return eqBands[bandIndex];
}

//==============================================================================
void MixerChannel::setCompressorEnabled(bool enabled)
{
    compressorEnabled.store(enabled);
    sendChangeMessage();
}

void MixerChannel::setCompressorThreshold(float thresholdDb)
{
    compThreshold.store(juce::jlimit(-60.0f, 0.0f, thresholdDb));
    sendChangeMessage();
}

void MixerChannel::setCompressorRatio(float ratio)
{
    compRatio.store(juce::jlimit(1.0f, 20.0f, ratio));
    sendChangeMessage();
}

void MixerChannel::setCompressorAttack(float attackMs)
{
    compAttack.store(juce::jlimit(0.1f, 100.0f, attackMs));
    updateCompressorCoefficients();
    sendChangeMessage();
}

void MixerChannel::setCompressorRelease(float releaseMs)
{
    compRelease.store(juce::jlimit(10.0f, 1000.0f, releaseMs));
    updateCompressorCoefficients();
    sendChangeMessage();
}

void MixerChannel::setCompressorMakeup(float makeupDb)
{
    compMakeup.store(juce::jlimit(0.0f, 24.0f, makeupDb));
    sendChangeMessage();
}

//==============================================================================
void MixerChannel::setSendLevel(int sendIndex, float level)
{
    if (juce::isPositiveAndBelow(sendIndex, numSends))
    {
        sendLevels[sendIndex].store(juce::jlimit(0.0f, 1.0f, level));
        sendChangeMessage();
    }
}

float MixerChannel::getSendLevel(int sendIndex) const
{
    if (juce::isPositiveAndBelow(sendIndex, numSends))
    {
        return sendLevels[sendIndex].load();
    }
    return 0.0f;
}

void MixerChannel::setSendPreFader(int sendIndex, bool preFader)
{
    if (juce::isPositiveAndBelow(sendIndex, numSends))
    {
        sendPreFader[sendIndex].store(preFader);
        sendChangeMessage();
    }
}

bool MixerChannel::isSendPreFader(int sendIndex) const
{
    if (juce::isPositiveAndBelow(sendIndex, numSends))
    {
        return sendPreFader[sendIndex].load();
    }
    return false;
}

//==============================================================================
void MixerChannel::setVolume(float newVolume)
{
    volume.store(juce::jlimit(0.0f, 1.0f, newVolume));
    sendChangeMessage();
}

void MixerChannel::setPan(float newPan)
{
    pan.store(juce::jlimit(-1.0f, 1.0f, newPan));
    sendChangeMessage();
}

void MixerChannel::setMuted(bool shouldBeMuted)
{
    muted.store(shouldBeMuted);
    sendChangeMessage();
}

void MixerChannel::setSolo(bool shouldBeSolo)
{
    solo.store(shouldBeSolo);
    sendChangeMessage();
}

//==============================================================================
void MixerChannel::resetPeaks()
{
    inputPeak.store(0.0f);
    outputPeak.store(0.0f);
}

//==============================================================================
juce::ValueTree MixerChannel::getState() const
{
    juce::ValueTree state("MixerChannel");

    // Input section
    state.setProperty("inputGain", inputGain.load(), nullptr);
    state.setProperty("phaseInvert", phaseInvert.load(), nullptr);

    // High-pass filter
    state.setProperty("hpfEnabled", hpfEnabled.load(), nullptr);
    state.setProperty("hpfFrequency", hpfFrequency.load(), nullptr);

    // EQ bands
    for (int i = 0; i < numEQBands; ++i)
    {
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
    for (int i = 0; i < numSends; ++i)
    {
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

void MixerChannel::loadState(const juce::ValueTree& state)
{
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
    for (const auto& child : state)
    {
        if (child.hasType("EQBand") && bandIndex < numEQBands)
        {
            eqBands[bandIndex].enabled.store(child.getProperty("enabled", false));
            eqBands[bandIndex].frequency.store(child.getProperty("frequency", 1000.0f));
            eqBands[bandIndex].gain.store(child.getProperty("gain", 0.0f));
            eqBands[bandIndex].q.store(child.getProperty("q", 0.707f));
            eqBands[bandIndex].type = static_cast<EQBand::Type>(static_cast<int>(child.getProperty("type", 1)));
            ++bandIndex;
        }
    }

    // Compressor
    compressorEnabled.store(state.getProperty("compressorEnabled", false));
    compThreshold.store(state.getProperty("compThreshold", -10.0f));
    compRatio.store(state.getProperty("compRatio", 4.0f));
    compAttack.store(state.getProperty("compAttack", 10.0f));
    compRelease.store(state.getProperty("compRelease", 100.0f));
    compMakeup.store(state.getProperty("compMakeup", 0.0f));

    // Sends
    int sendIndex = 0;
    for (const auto& child : state)
    {
        if (child.hasType("Send") && sendIndex < numSends)
        {
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

    updateFilterCoefficients();
    updateCompressorCoefficients();
    sendChangeMessage();
}

//==============================================================================
void MixerChannel::processInput(juce::AudioBuffer<float>& buffer)
{
    const float gainLinear = dbToGain(inputGain.load());
    const bool invertPhase = phaseInvert.load();

    buffer.applyGain(gainLinear);

    if (invertPhase)
    {
        buffer.applyGain(-1.0f);
    }
}

void MixerChannel::processHighPass(juce::AudioBuffer<float>& buffer)
{
    if (buffer.getNumChannels() >= 1)
        hpfFilterL.processSamples(buffer.getWritePointer(0), buffer.getNumSamples());

    if (buffer.getNumChannels() >= 2)
        hpfFilterR.processSamples(buffer.getWritePointer(1), buffer.getNumSamples());
}

void MixerChannel::processEQ(juce::AudioBuffer<float>& buffer)
{
    for (int band = 0; band < numEQBands; ++band)
    {
        if (eqBands[band].enabled.load())
        {
            if (buffer.getNumChannels() >= 1)
                eqFiltersL[band].processSamples(buffer.getWritePointer(0), buffer.getNumSamples());

            if (buffer.getNumChannels() >= 2)
                eqFiltersR[band].processSamples(buffer.getWritePointer(1), buffer.getNumSamples());
        }
    }
}

void MixerChannel::processCompressor(juce::AudioBuffer<float>& buffer)
{
    const float threshold = compThreshold.load();
    const float ratio = compRatio.load();
    const float makeupGain = dbToGain(compMakeup.load());

    float maxGR = 0.0f;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        float* channelData = buffer.getWritePointer(ch);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float inputSample = channelData[i];
            const float inputDb = gainToDb(std::abs(inputSample));

            // Envelope follower
            if (inputDb > envelopeFollower)
                envelopeFollower += attackCoeff * (inputDb - envelopeFollower);
            else
                envelopeFollower += releaseCoeff * (inputDb - envelopeFollower);

            // Calculate gain reduction
            float gr = 0.0f;
            if (envelopeFollower > threshold)
            {
                gr = (envelopeFollower - threshold) * (1.0f - 1.0f / ratio);
            }

            // Track maximum gain reduction
            if (gr > maxGR)
                maxGR = gr;

            // Apply compression
            const float compressionGain = dbToGain(-gr);
            channelData[i] = inputSample * compressionGain * makeupGain;
        }
    }

    gainReduction.store(maxGR);
}

void MixerChannel::processOutput(juce::AudioBuffer<float>& buffer)
{
    const float vol = volume.load();
    const float panValue = pan.load();

    // Calculate left and right gains from pan (-3dB center, constant power)
    const float piOver4 = juce::MathConstants<float>::pi / 4.0f;
    const float leftGain = vol * std::cos(piOver4 * (1.0f + panValue));
    const float rightGain = vol * std::sin(piOver4 * (1.0f + panValue));

    if (buffer.getNumChannels() >= 2)
    {
        buffer.applyGain(0, 0, buffer.getNumSamples(), leftGain);
        buffer.applyGain(1, 0, buffer.getNumSamples(), rightGain);
    }
    else if (buffer.getNumChannels() == 1)
    {
        buffer.applyGain(0, 0, buffer.getNumSamples(), vol);
    }
}

void MixerChannel::updateMeters(const juce::AudioBuffer<float>& buffer, bool isInput)
{
    float maxLevel = 0.0f;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        const float* channelData = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float absValue = std::abs(channelData[i]);
            if (absValue > maxLevel)
                maxLevel = absValue;
        }
    }

    if (isInput)
    {
        const float currentLevelValue = inputLevel.load();
        const float smoothingFactor = 0.3f;
        const float newLevel = currentLevelValue * (1.0f - smoothingFactor) + maxLevel * smoothingFactor;
        inputLevel.store(newLevel);

        if (maxLevel > inputPeak.load())
            inputPeak.store(maxLevel);
    }
    else
    {
        const float currentLevelValue = outputLevel.load();
        const float smoothingFactor = 0.3f;
        const float newLevel = currentLevelValue * (1.0f - smoothingFactor) + maxLevel * smoothingFactor;
        outputLevel.store(newLevel);

        if (maxLevel > outputPeak.load())
            outputPeak.store(maxLevel);
    }
}

void MixerChannel::updateFilterCoefficients()
{
    if (currentSampleRate <= 0)
        return;

    // Update high-pass filter
    auto hpfCoeffs = juce::IIRCoefficients::makeHighPass(
        currentSampleRate,
        hpfFrequency.load());

    hpfFilterL.setCoefficients(hpfCoeffs);
    hpfFilterR.setCoefficients(hpfCoeffs);

    // Update EQ filters
    for (int i = 0; i < numEQBands; ++i)
    {
        const float freq = eqBands[i].frequency.load();
        const float gain = eqBands[i].gain.load();
        const float q = eqBands[i].q.load();

        juce::IIRCoefficients coeffs;

        switch (eqBands[i].type)
        {
            case EQBand::Type::LowShelf:
                coeffs = juce::IIRCoefficients::makeLowShelf(currentSampleRate, freq, q, dbToGain(gain));
                break;

            case EQBand::Type::Peak:
                coeffs = juce::IIRCoefficients::makePeakFilter(currentSampleRate, freq, q, dbToGain(gain));
                break;

            case EQBand::Type::HighShelf:
                coeffs = juce::IIRCoefficients::makeHighShelf(currentSampleRate, freq, q, dbToGain(gain));
                break;
        \n    default: break;\n\n    default: break;\n}

        eqFiltersL[i].setCoefficients(coeffs);
        eqFiltersR[i].setCoefficients(coeffs);
    }
}

void MixerChannel::updateCompressorCoefficients()
{
    if (currentSampleRate <= 0)
        return;

    const float attackMs = compAttack.load();
    const float releaseMs = compRelease.load();

    // Calculate time constants
    attackCoeff = 1.0f - std::exp(-1.0f / (attackMs * 0.001f * currentSampleRate));
    releaseCoeff = 1.0f - std::exp(-1.0f / (releaseMs * 0.001f * currentSampleRate));
}
} // namespace zenith


