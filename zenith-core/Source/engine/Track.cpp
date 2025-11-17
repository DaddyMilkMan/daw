/*
  ==============================================================================

    Track.cpp
    Ported from: VexelDAW-Native/Source/Audio/Track.cpp (2025-11-11)
    Author:  Vexel DAW → Zenith DAW

    Audio/MIDI track implementation

    JUCE 8 / C++20 adaptations:
    - Wrapped in namespace zenith
    - OwnedArray<Clip> → std::vector<std::unique_ptr<Clip>>
    - Plugin hosting stubbed for Phase 2

  ==============================================================================
*/

#include "Track.h"
#include "Clip.h"
#include <algorithm>

namespace zenith {

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

    // Clips are automatically destroyed via std::unique_ptr
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
        for (auto& slot : pluginChain)
        {
            if (slot.instance != nullptr)
            {
                slot.instance->prepareToPlay(sampleRate, samplesPerBlockExpected);
                slot.instance->setNonRealtime(false);
            }
        }
    }

    // Prepare all clips
    {
        const juce::ScopedLock sl(clipsLock);
        for (auto& clip : clips)
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
        for (auto& slot : pluginChain)
        {
            if (slot.instance != nullptr)
            {
                slot.instance->releaseResources();
            }
        }
    }

    // Release all clips
    {
        const juce::ScopedLock sl(clipsLock);
        for (auto& clip : clips)
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

        for (auto& clip : clips)
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
// Plugin management
//==============================================================================

bool Track::addPlugin(const juce::PluginDescription& pluginDescription,
                      juce::AudioPluginFormatManager& formatManager,
                      double sampleRate,
                      int blockSize,
                      juce::String& errorMessage)
{
    // MESSAGE THREAD ONLY!
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    // Find the appropriate format for this plugin
    auto* format = formatManager.findFormatForDescription(pluginDescription, errorMessage);
    if (format == nullptr)
    {
        errorMessage = "Could not find format for plugin: " + pluginDescription.name;
        return false;
    }

    // Create the plugin instance (this is a blocking call)
    auto instance = format->createInstanceFromDescription(pluginDescription, sampleRate, blockSize);
    if (instance == nullptr)
    {
        errorMessage = "Failed to create plugin instance: " + pluginDescription.name;
        return false;
    }

    // Prepare the plugin for playback
    instance->prepareToPlay(sampleRate, blockSize);
    instance->setNonRealtime(false);

    // Add to plugin chain
    {
        const juce::ScopedLock sl(pluginLock);

        PluginSlot slot;
        slot.instance = std::move(instance);
        slot.description = pluginDescription;
        slot.bypassed = false;

        pluginChain.push_back(std::move(slot));
    }

    sendChangeMessage();
    return true;
}

void Track::removePlugin(int pluginIndex)
{
    // MESSAGE THREAD ONLY!
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    const juce::ScopedLock sl(pluginLock);

    if (pluginIndex >= 0 && pluginIndex < static_cast<int>(pluginChain.size()))
    {
        auto& slot = pluginChain[pluginIndex];
        if (slot.instance != nullptr)
        {
            slot.instance->releaseResources();
        }
        pluginChain.erase(pluginChain.begin() + pluginIndex);
        sendChangeMessage();
    }
}

void Track::clearPlugins()
{
    // MESSAGE THREAD ONLY!
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    const juce::ScopedLock sl(pluginLock);

    for (auto& slot : pluginChain)
    {
        if (slot.instance != nullptr)
        {
            slot.instance->releaseResources();
        }
    }

    pluginChain.clear();
    sendChangeMessage();
}

int Track::getNumPlugins() const
{
    const juce::ScopedLock sl(pluginLock);
    return static_cast<int>(pluginChain.size());
}

void Track::setPluginBypassed(int pluginIndex, bool bypassed)
{
    // MESSAGE THREAD ONLY!
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    const juce::ScopedLock sl(pluginLock);

    if (pluginIndex >= 0 && pluginIndex < static_cast<int>(pluginChain.size()))
    {
        pluginChain[pluginIndex].bypassed = bypassed;
        sendChangeMessage();
    }
}

bool Track::isPluginBypassed(int pluginIndex) const
{
    const juce::ScopedLock sl(pluginLock);

    if (pluginIndex >= 0 && pluginIndex < static_cast<int>(pluginChain.size()))
        return pluginChain[pluginIndex].bypassed;

    return false;
}

juce::PluginDescription Track::getPluginDescription(int pluginIndex) const
{
    const juce::ScopedLock sl(pluginLock);

    if (pluginIndex >= 0 && pluginIndex < static_cast<int>(pluginChain.size()))
        return pluginChain[pluginIndex].description;

    return juce::PluginDescription();
}

//==============================================================================
void Track::addClip(std::unique_ptr<Clip> clip)
{
    if (clip != nullptr)
    {
        const juce::ScopedLock sl(clipsLock);

        // Prepare the clip if we're already initialized
        if (currentSampleRate > 0)
        {
            clip->prepareToPlay(currentBlockSize, currentSampleRate);
        }

        clips.push_back(std::move(clip));
        sendChangeMessage();
    }
}

void Track::removeClip(int clipIndex)
{
    const juce::ScopedLock sl(clipsLock);

    if (clipIndex >= 0 && clipIndex < static_cast<int>(clips.size()))
    {
        auto& clip = clips[clipIndex];
        if (clip != nullptr)
        {
            clip->releaseResources();
        }
        clips.erase(clips.begin() + clipIndex);
        sendChangeMessage();
    }
}

void Track::removeClip(Clip* clip)
{
    const juce::ScopedLock sl(clipsLock);

    auto it = std::find_if(clips.begin(), clips.end(),
        [clip](const std::unique_ptr<Clip>& c) { return c.get() == clip; });

    if (it != clips.end())
    {
        (*it)->releaseResources();
        clips.erase(it);
        sendChangeMessage();
    }
}

void Track::clearClips()
{
    const juce::ScopedLock sl(clipsLock);

    for (auto& clip : clips)
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
    return static_cast<int>(clips.size());
}

Track::Clip* Track::getClip(int index) const
{
    const juce::ScopedLock sl(clipsLock);
    if (index >= 0 && index < static_cast<int>(clips.size()))
        return clips[index].get();
    return nullptr;
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

    // Save plugin chain
    juce::ValueTree pluginsState("Plugins");
    {
        const juce::ScopedLock sl(pluginLock);
        for (size_t i = 0; i < pluginChain.size(); ++i)
        {
            const auto& slot = pluginChain[i];

            juce::ValueTree pluginState("Plugin");
            pluginState.setProperty("name", slot.description.name, nullptr);
            pluginState.setProperty("descriptiveName", slot.description.descriptiveName, nullptr);
            pluginState.setProperty("pluginFormatName", slot.description.pluginFormatName, nullptr);
            pluginState.setProperty("category", slot.description.category, nullptr);
            pluginState.setProperty("manufacturerName", slot.description.manufacturerName, nullptr);
            pluginState.setProperty("version", slot.description.version, nullptr);
            pluginState.setProperty("fileOrIdentifier", slot.description.fileOrIdentifier, nullptr);
            pluginState.setProperty("lastFileModTime", slot.description.lastFileModTime.toMilliseconds(), nullptr);
            pluginState.setProperty("lastInfoUpdateTime", slot.description.lastInfoUpdateTime.toMilliseconds(), nullptr);
            pluginState.setProperty("uid", slot.description.uid, nullptr);
            pluginState.setProperty("isInstrument", slot.description.isInstrument, nullptr);
            pluginState.setProperty("numInputChannels", slot.description.numInputChannels, nullptr);
            pluginState.setProperty("numOutputChannels", slot.description.numOutputChannels, nullptr);
            pluginState.setProperty("hasSharedContainer", slot.description.hasSharedContainer, nullptr);
            pluginState.setProperty("bypassed", slot.bypassed, nullptr);

            // TODO: Save plugin state (preset/parameter values)
            // For now, we only save the plugin identifier for reloading
            // Future: Use slot.instance->getStateInformation() to save full state

            pluginsState.appendChild(pluginState, nullptr);
        }
    }
    state.appendChild(pluginsState, nullptr);

    // Save clip states
    juce::ValueTree clipsState("Clips");
    {
        const juce::ScopedLock sl(clipsLock);
        for (auto& clip : clips)
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

    // Load plugin chain descriptions (but don't instantiate yet)
    // To actually load plugins, call loadPluginsFromState() with format manager
    // TODO: This is a limitation - we save plugin descriptions but need external
    // call to actually instantiate them. Future: Store Engine reference in Track?
    auto pluginsState = state.getChildWithName("Plugins");
    if (pluginsState.isValid())
    {
        // For now, just clear the plugin chain
        // The caller needs to call loadPluginsFromState() with format manager
        clearPlugins();

        // NOTE: Plugin instances will be created by loadPluginsFromState()
        // which must be called separately with the format manager
    }

    // Load clip states
    auto clipsState = state.getChildWithName("Clips");
    if (clipsState.isValid())
    {
        clearClips();
        for (int i = 0; i < clipsState.getNumChildren(); ++i)
        {
            auto clipState = clipsState.getChild(i);
            auto clip = std::make_unique<Clip>();
            clip->loadState(clipState);
            addClip(std::move(clip));
        }
    }

    sendChangeMessage();
}

void Track::loadPluginsFromState(const juce::ValueTree& state,
                                  juce::AudioPluginFormatManager& formatManager,
                                  double sampleRate,
                                  int blockSize)
{
    // MESSAGE THREAD ONLY!
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    auto pluginsState = state.getChildWithName("Plugins");
    if (!pluginsState.isValid())
        return;

    // Clear existing plugins first
    clearPlugins();

    // Load each plugin
    for (int i = 0; i < pluginsState.getNumChildren(); ++i)
    {
        auto pluginState = pluginsState.getChild(i);
        if (!pluginState.hasType("Plugin"))
            continue;

        // Reconstruct plugin description
        juce::PluginDescription desc;
        desc.name = pluginState.getProperty("name", "");
        desc.descriptiveName = pluginState.getProperty("descriptiveName", "");
        desc.pluginFormatName = pluginState.getProperty("pluginFormatName", "");
        desc.category = pluginState.getProperty("category", "");
        desc.manufacturerName = pluginState.getProperty("manufacturerName", "");
        desc.version = pluginState.getProperty("version", "");
        desc.fileOrIdentifier = pluginState.getProperty("fileOrIdentifier", "");
        desc.lastFileModTime = juce::Time(static_cast<juce::int64>(pluginState.getProperty("lastFileModTime", 0)));
        desc.lastInfoUpdateTime = juce::Time(static_cast<juce::int64>(pluginState.getProperty("lastInfoUpdateTime", 0)));
        desc.uid = pluginState.getProperty("uid", 0);
        desc.isInstrument = pluginState.getProperty("isInstrument", false);
        desc.numInputChannels = pluginState.getProperty("numInputChannels", 0);
        desc.numOutputChannels = pluginState.getProperty("numOutputChannels", 0);
        desc.hasSharedContainer = pluginState.getProperty("hasSharedContainer", false);

        bool bypassed = pluginState.getProperty("bypassed", false);

        // Try to add the plugin
        juce::String errorMessage;
        if (addPlugin(desc, formatManager, sampleRate, blockSize, errorMessage))
        {
            // Set bypass state if needed
            if (bypassed)
                setPluginBypassed(getNumPlugins() - 1, true);
        }
        else
        {
            DBG("Failed to load plugin: " + desc.name + " - " + errorMessage);
            // Continue loading other plugins even if one fails
        }

        // TODO: Load plugin state (preset/parameter values)
        // Future: Use slot.instance->setStateInformation()
    }
}

//==============================================================================
void Track::processPluginChain(juce::AudioBuffer<float>& buffer, int numSamples)
{
    // AUDIO THREAD - must be RT-safe!
    // We use a CriticalSection here which is NOT ideal for RT-safety,
    // but acceptable for now since plugin operations are rare.
    // Future enhancement: Use lock-free swap for plugin chain updates.

    const juce::ScopedTryLock stl(pluginLock);

    // If we can't get the lock, skip plugin processing this time
    // (better to have a small glitch than to block the audio thread)
    if (!stl.isLocked())
        return;

    // Process each plugin in the chain
    for (auto& slot : pluginChain)
    {
        if (slot.instance == nullptr || slot.bypassed)
            continue;

        // Create a MIDI buffer (empty for now, will add MIDI support later)
        juce::MidiBuffer midiBuffer;

        // Process the plugin
        // Note: This modifies the buffer in-place
        slot.instance->processBlock(buffer, midiBuffer);
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

} // namespace zenith
