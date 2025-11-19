/*
  ==============================================================================

    Track.cpp
    Created: 2025-11-11
    Author:  Zenith DAW

    Audio/MIDI track implementation

  ==============================================================================
*/

#include "Track.h"
#include "Clip.h"

//==============================================================================
Track::Track(const juce::String& name, Type type)
    : trackName(name), trackType(type)
{
}

Track::~Track()
{
    // Ensure we're not in the middle of audio processing
    const juce::ScopedLock sl1(pluginLock);
    const juce::ScopedLock sl2(clipsLock);

    plugins.clear();
    clips.clear();
}

//==============================================================================
void Track::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlockExpected;

    // Prepare plugin buffer
    pluginBuffer.setSize(2, samplesPerBlockExpected);

    // Prepare all plugins
    {
        const juce::ScopedLock sl(pluginLock);
        for (auto* plugin : plugins)
        {
            if (plugin != nullptr)
            {
                plugin->prepareToPlay(sampleRate, samplesPerBlockExpected);
                plugin->setNonRealtime(false);
            }
        }
    }

    // Prepare all clips
    {
        const juce::ScopedLock sl(clipsLock);
        for (auto* clip : clips)
        {
            if (clip != nullptr)
            {
                clip->prepareToPlay(samplesPerBlockExpected, sampleRate);
            }
        }
    }
}

void Track::releaseResources()
{
    // Release all plugins
    {
        const juce::ScopedLock sl(pluginLock);
        for (auto* plugin : plugins)
        {
            if (plugin != nullptr)
            {
                plugin->releaseResources();
            }
        }
    }

    // Release all clips
    {
        const juce::ScopedLock sl(clipsLock);
        for (auto* clip : clips)
        {
            if (clip != nullptr)
            {
                clip->releaseResources();
            }
        }
    }
}

void Track::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    // Clear the buffer first
    bufferToFill.clearActiveBufferRegion();

    // If track is disabled or muted, return silence
    if (!enabled.load() || muted.load())
    {
        currentLevel.store(0.0f);
        return;
    }

    // Get audio from all clips and mix them together
    {
        const juce::ScopedLock sl(clipsLock);

        for (auto* clip : clips)
        {
            if (clip != nullptr && clip->isActive())
            {
                // Create a temporary buffer for this clip
                juce::AudioBuffer<float> clipBuffer(
                    bufferToFill.buffer->getNumChannels(),
                    bufferToFill.numSamples);
                clipBuffer.clear();

                juce::AudioSourceChannelInfo clipInfo(&clipBuffer, 0, bufferToFill.numSamples);
                clip->getNextAudioBlock(clipInfo);

                // Mix clip into main buffer
                for (int ch = 0; ch < bufferToFill.buffer->getNumChannels(); ++ch)
                {
                    bufferToFill.buffer->addFrom(
                        ch,
                        bufferToFill.startSample,
                        clipBuffer,
                        ch,
                        0,
                        bufferToFill.numSamples);
                }
            }
        }
    }

    // Create a local buffer reference for processing
    juce::AudioBuffer<float> localBuffer(
        bufferToFill.buffer->getArrayOfWritePointers(),
        bufferToFill.buffer->getNumChannels(),
        bufferToFill.startSample,
        bufferToFill.numSamples);

    // Process through plugin chain
    processPluginChain(localBuffer, bufferToFill.numSamples);

    // Apply volume and pan
    applyGainAndPan(localBuffer, bufferToFill.numSamples);

    // Update level meters
    updateLevelMeters(localBuffer, bufferToFill.numSamples);
}

//==============================================================================
void Track::setName(const juce::String& newName)
{
    trackName = newName;
    sendChangeMessage();
}

juce::String Track::getTypeString() const
{
    switch (trackType)
    {
        case Type::Audio:       return "Audio";
        case Type::MIDI:        return "MIDI";
        case Type::Instrument:  return "Instrument";
        default:                return "Unknown";
    }
}

//==============================================================================
void Track::setVolume(float newVolume)
{
    volume.store(juce::jlimit(0.0f, 1.0f, newVolume));
    sendChangeMessage();
}

void Track::setPan(float newPan)
{
    pan.store(juce::jlimit(-1.0f, 1.0f, newPan));
    sendChangeMessage();
}

void Track::setMuted(bool shouldBeMuted)
{
    muted.store(shouldBeMuted);
    sendChangeMessage();
}

void Track::setSolo(bool shouldBeSolo)
{
    solo.store(shouldBeSolo);
    sendChangeMessage();
}

void Track::setArmed(bool shouldBeArmed)
{
    armed.store(shouldBeArmed);
    sendChangeMessage();
}

void Track::setEnabled(bool shouldBeEnabled)
{
    enabled.store(shouldBeEnabled);
    sendChangeMessage();
}

//==============================================================================
void Track::addPlugin(juce::AudioPluginInstance* plugin)
{
    if (plugin != nullptr)
    {
        const juce::ScopedLock sl(pluginLock);

        // Prepare the plugin if we're already initialized
        if (currentSampleRate > 0)
        {
            plugin->prepareToPlay(currentSampleRate, currentBlockSize);
            plugin->setNonRealtime(false);
        }

        plugins.add(plugin);
        sendChangeMessage();
    }
}

void Track::removePlugin(int pluginIndex)
{
    const juce::ScopedLock sl(pluginLock);

    if (juce::isPositiveAndBelow(pluginIndex, plugins.size()))
    {
        auto* plugin = plugins[pluginIndex];
        if (plugin != nullptr)
        {
            plugin->releaseResources();
        }
        plugins.remove(pluginIndex);
        sendChangeMessage();
    }
}

void Track::clearPlugins()
{
    const juce::ScopedLock sl(pluginLock);

    for (auto* plugin : plugins)
    {
        if (plugin != nullptr)
        {
            plugin->releaseResources();
        }
    }

    plugins.clear();
    sendChangeMessage();
}

int Track::getNumPlugins() const
{
    const juce::ScopedLock sl(pluginLock);
    return plugins.size();
}

juce::AudioPluginInstance* Track::getPlugin(int index) const
{
    const juce::ScopedLock sl(pluginLock);
    return plugins[index];
}

//==============================================================================
void Track::addClip(Clip* clip)
{
    if (clip != nullptr)
    {
        const juce::ScopedLock sl(clipsLock);

        // Prepare the clip if we're already initialized
        if (currentSampleRate > 0)
        {
            clip->prepareToPlay(currentBlockSize, currentSampleRate);
        }

        clips.add(clip);
        sendChangeMessage();
    }
}

void Track::removeClip(int clipIndex)
{
    const juce::ScopedLock sl(clipsLock);

    if (juce::isPositiveAndBelow(clipIndex, clips.size()))
    {
        auto* clip = clips[clipIndex];
        if (clip != nullptr)
        {
            clip->releaseResources();
        }
        clips.remove(clipIndex);
        sendChangeMessage();
    }
}

void Track::removeClip(Clip* clip)
{
    const juce::ScopedLock sl(clipsLock);

    int index = clips.indexOf(clip);
    if (index >= 0)
    {
        removeClip(index);
    }
}

void Track::clearClips()
{
    const juce::ScopedLock sl(clipsLock);

    for (auto* clip : clips)
    {
        if (clip != nullptr)
        {
            clip->releaseResources();
        }
    }

    clips.clear();
    sendChangeMessage();
}

int Track::getNumClips() const
{
    const juce::ScopedLock sl(clipsLock);
    return clips.size();
}

Track::Clip* Track::getClip(int index) const
{
    const juce::ScopedLock sl(clipsLock);
    return clips[index];
}

//==============================================================================
void Track::resetPeakLevel()
{
    peakLevel.store(0.0f);
}

//==============================================================================
juce::ValueTree Track::getState() const
{
    juce::ValueTree state("Track");

    state.setProperty("name", trackName, nullptr);
    state.setProperty("type", static_cast<int>(trackType), nullptr);
    state.setProperty("volume", volume.load(), nullptr);
    state.setProperty("pan", pan.load(), nullptr);
    state.setProperty("muted", muted.load(), nullptr);
    state.setProperty("solo", solo.load(), nullptr);
    state.setProperty("armed", armed.load(), nullptr);
    state.setProperty("enabled", enabled.load(), nullptr);

    // Save plugin states
    juce::ValueTree pluginsState("Plugins");
    {
        const juce::ScopedLock sl(pluginLock);
        for (auto* plugin : plugins)
        {
            if (plugin != nullptr)
            {
                juce::MemoryBlock pluginState;
                plugin->getStateInformation(pluginState);

                juce::ValueTree pluginTree("Plugin");
                pluginTree.setProperty("name", plugin->getName(), nullptr);
                pluginTree.setProperty("state", pluginState.toBase64Encoding(), nullptr);
                pluginsState.appendChild(pluginTree, nullptr);
            }
        }
    }
    state.appendChild(pluginsState, nullptr);

    // Save clip states
    juce::ValueTree clipsState("Clips");
    {
        const juce::ScopedLock sl(clipsLock);
        for (auto* clip : clips)
        {
            if (clip != nullptr)
            {
                clipsState.appendChild(clip->getState(), nullptr);
            }
        }
    }
    state.appendChild(clipsState, nullptr);

    return state;
}

void Track::loadState(const juce::ValueTree& state)
{
    if (!state.hasType("Track"))
        return;

    trackName = state.getProperty("name", "Untitled Track");
    trackType = static_cast<Type>(static_cast<int>(state.getProperty("type", 0)));
    volume.store(state.getProperty("volume", 0.8f));
    pan.store(state.getProperty("pan", 0.0f));
    muted.store(state.getProperty("muted", false));
    solo.store(state.getProperty("solo", false));
    armed.store(state.getProperty("armed", false));
    enabled.store(state.getProperty("enabled", true));

    // TODO: Load plugin states (requires plugin manager/scanner)

    // Load clip states
    auto clipsState = state.getChildWithName("Clips");
    if (clipsState.isValid())
    {
        clearClips();
        for (int i = 0; i < clipsState.getNumChildren(); ++i)
        {
            auto clipState = clipsState.getChild(i);
            auto* clip = new Clip();
            clip->loadState(clipState);
            addClip(clip);
        }
    }

    sendChangeMessage();
}

//==============================================================================
void Track::processPluginChain(juce::AudioBuffer<float>& buffer, int numSamples)
{
    const juce::ScopedLock sl(pluginLock);

    for (auto* plugin : plugins)
    {
        if (plugin != nullptr && !plugin->isSuspended())
        {
            juce::MidiBuffer emptyMidi;

            // Process the plugin
            plugin->processBlock(buffer, emptyMidi);
        }
    }
}

void Track::applyGainAndPan(juce::AudioBuffer<float>& buffer, int numSamples)
{
    const float vol = volume.load();
    const float panValue = pan.load();

    // Calculate left and right gains from pan
    // Pan law: -3dB center, constant power
    const float piOver4 = juce::MathConstants<float>::pi / 4.0f;
    const float leftGain = vol * std::cos(piOver4 * (1.0f + panValue));
    const float rightGain = vol * std::sin(piOver4 * (1.0f + panValue));

    if (buffer.getNumChannels() >= 2)
    {
        // Stereo: apply pan
        buffer.applyGain(0, 0, numSamples, leftGain);
        buffer.applyGain(1, 0, numSamples, rightGain);
    }
    else if (buffer.getNumChannels() == 1)
    {
        // Mono: apply volume only
        buffer.applyGain(0, 0, numSamples, vol);
    }
}

void Track::updateLevelMeters(const juce::AudioBuffer<float>& buffer, int numSamples)
{
    float maxLevel = 0.0f;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        const float* channelData = buffer.getReadPointer(ch);
        for (int i = 0; i < numSamples; ++i)
        {
            const float absValue = std::abs(channelData[i]);
            if (absValue > maxLevel)
            {
                maxLevel = absValue;
            }
        }
    }

    // Update current level (RMS-style with smoothing)
    const float currentLevelValue = currentLevel.load();
    const float smoothingFactor = 0.3f;
    const float newLevel = currentLevelValue * (1.0f - smoothingFactor) + maxLevel * smoothingFactor;
    currentLevel.store(newLevel);

    // Update peak level
    if (maxLevel > peakLevel.load())
    {
        peakLevel.store(maxLevel);
    }
}
