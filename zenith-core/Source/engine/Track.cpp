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
#include "PluginHost.h"
#include "../instruments/Instrument.h"
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
void Track::setInstrument(std::unique_ptr<Instrument> instrument)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    // Release old instrument if present
    if (instrument_ != nullptr)
    {
        auto* processor = instrument_->getAudioProcessor();
        if (processor != nullptr)
        {
            processor->releaseResources();
        }
    }

    // Set new instrument
    instrument_ = std::move(instrument);

    // Prepare new instrument if audio is running
    if (instrument_ != nullptr && currentSampleRate > 0)
    {
        auto* processor = instrument_->getAudioProcessor();
        if (processor != nullptr)
        {
            processor->setPlayConfigDetails(0, 2, currentSampleRate, currentBlockSize);
            processor->prepareToPlay(currentSampleRate, currentBlockSize);
        }
    }

    sendChangeMessage();
}

//==============================================================================
void Track::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlockExpected;

    // Prepare instrument buffer (fixed size, no reallocation on audio thread)
    instrumentBuffer_.setSize(2, samplesPerBlockExpected, false, true, true);

    // Prepare clip buffer (preallocated for RT-safe clip mixing)
    clipBuffer_.setSize(2, samplesPerBlockExpected, false, true, true);

    // Prepare instrument if present
    if (instrument_ != nullptr)
    {
        auto* processor = instrument_->getAudioProcessor();
        if (processor != nullptr)
        {
            processor->setPlayConfigDetails(0, 2, sampleRate, samplesPerBlockExpected);
            processor->prepareToPlay(sampleRate, samplesPerBlockExpected);
        }
    }

    // Phase 3: Prepare all plugins
    {
        const juce::ScopedLock sl(pluginLock);
        for (auto& plugin : plugins)
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
    // Release instrument if present
    if (instrument_ != nullptr)
    {
        auto* processor = instrument_->getAudioProcessor();
        if (processor != nullptr)
        {
            processor->releaseResources();
        }
    }

    // Phase 3: Release all plugins
    {
        const juce::ScopedLock sl(pluginLock);
        for (auto& plugin : plugins)
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

    // Clear MIDI buffer for this block
    midiBuffer.clear();

    // Get audio/MIDI from all clips and mix them together
    // NOTE: clipsLock is still used here - NOT fully RT-safe yet
    // TODO: Implement lock-free ClipSnapshot pattern for full RT-safety
    {
        const juce::ScopedLock sl(clipsLock);

        for (auto& clip : clips)
        {
            if (clip != nullptr && clip->isActive())
            {
                // For audio clips, use preallocated buffer (RT-safe, no allocation)
                if (clip->getClipType() == Clip::Type::Audio)
                {
                    // Ensure clipBuffer_ has correct dimensions (should already be sized from prepareToPlay)
                    const int numChannels = bufferToFill.buffer->getNumChannels();
                    const int numSamples = bufferToFill.numSamples;

                    // Clear the preallocated buffer for reuse
                    for (int ch = 0; ch < juce::jmin(numChannels, clipBuffer_.getNumChannels()); ++ch)
                    {
                        clipBuffer_.clear(ch, 0, numSamples);
                    }

                    juce::AudioSourceChannelInfo clipInfo(&clipBuffer_, 0, numSamples);
                    clip->getNextAudioBlock(clipInfo);

                    // Mix clip into main buffer
                    for (int ch = 0; ch < numChannels; ++ch)
                    {
                        bufferToFill.buffer->addFrom(
                            ch,
                            bufferToFill.startSample,
                            clipBuffer_,
                            ch,
                            0,
                            numSamples);
                    }
                }
                // For MIDI clips, collect MIDI events
                else if (clip->getClipType() == Clip::Type::MIDI)
                {
                    clip->getMidiEvents(midiBuffer, bufferToFill.numSamples);
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

    // Process instrument if present (for Instrument tracks)
    if (instrument_ != nullptr && trackType == Type::Instrument)
    {
        auto* processor = instrument_->getAudioProcessor();
        if (processor != nullptr)
        {
            // Use preallocated buffer (RT-safe, no reallocation)
            // instrumentBuffer_ was sized in prepareToPlay
            const int numSamples = bufferToFill.numSamples;

            // Clear instrument buffer for this block
            instrumentBuffer_.clear();

            // Process instrument (RT-safe as long as numSamples <= currentBlockSize)
            processor->processBlock(instrumentBuffer_, midiBuffer_);

            // Mix instrument output into track buffer
            for (int ch = 0; ch < juce::jmin(localBuffer.getNumChannels(), instrumentBuffer_.getNumChannels()); ++ch)
            {
                localBuffer.addFrom(ch, 0, instrumentBuffer_, ch, 0, numSamples);
            }

            // Clear MIDI buffer for next block
            midiBuffer_.clear();
        }
    }

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
// Plugin chain management (Phase 3: VST3 hosting MVP)
//==============================================================================

void Track::addPlugin(std::unique_ptr<juce::AudioPluginInstance> plugin)
{
    if (plugin == nullptr)
        return;

    const juce::ScopedLock sl(pluginLock);

    // Prepare the plugin if we're already initialized
    if (currentSampleRate > 0)
    {
        plugin->prepareToPlay(currentSampleRate, currentBlockSize);
        plugin->setNonRealtime(false);
    }

    plugins.push_back(std::move(plugin));
    sendChangeMessage();
}

void Track::removePlugin(int pluginIndex)
{
    const juce::ScopedLock sl(pluginLock);

    if (pluginIndex >= 0 && pluginIndex < static_cast<int>(plugins.size()))
    {
        auto& plugin = plugins[pluginIndex];
        if (plugin != nullptr)
        {
            plugin->releaseResources();
        }
        plugins.erase(plugins.begin() + pluginIndex);
        sendChangeMessage();
    }
}

void Track::clearPlugins()
{
    const juce::ScopedLock sl(pluginLock);

    for (auto& plugin : plugins)
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
    return static_cast<int>(plugins.size());
}

juce::AudioPluginInstance* Track::getPlugin(int index) const
{
    const juce::ScopedLock sl(pluginLock);
    if (index >= 0 && index < static_cast<int>(plugins.size()))
        return plugins[index].get();
    return nullptr;
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

    // Phase 3: Save plugin states
    juce::ValueTree pluginsState("Plugins");
    {
        const juce::ScopedLock sl(pluginLock);
        for (auto& plugin : plugins)
        {
            if (plugin != nullptr)
            {
                juce::ValueTree pluginState("Plugin");

                // Store plugin identifier
                auto description = plugin->getPluginDescription();
                pluginState.setProperty("identifier", description.createIdentifierString(), nullptr);
                pluginState.setProperty("name", description.name, nullptr);

                // Store plugin state as binary data
                juce::MemoryBlock stateData;
                plugin->getStateInformation(stateData);

                if (stateData.getSize() > 0)
                {
                    pluginState.setProperty("state", stateData.toBase64Encoding(), nullptr);
                }

                pluginsState.appendChild(pluginState, nullptr);
            }
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

    // Phase 3: Load plugin states
    // NOTE: Plugin loading requires PluginHost to recreate instances.
    // This should be called from Engine/ProjectState level where PluginHost is available.
    // For now, we just clear plugins and document the requirement.
    // TODO(Phase 3+): Add loadPluginState(ValueTree, PluginHost&) method
    auto pluginsState = state.getChildWithName("Plugins");
    if (pluginsState.isValid())
    {
        clearPlugins();
        // Plugin recreation needs to happen at Engine level with access to PluginHost
        // See ProjectState integration for proper plugin loading
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

void Track::loadPluginStates(const juce::ValueTree& state, PluginHost& pluginHost)
{
    if (!state.hasType("Track"))
        return;

    auto pluginsState = state.getChildWithName("Plugins");
    if (!pluginsState.isValid())
        return;

    DBG("Track: Loading plugin states for " + trackName);

    // Clear existing plugins
    clearPlugins();

    // Recreate each plugin
    for (int i = 0; i < pluginsState.getNumChildren(); ++i)
    {
        auto pluginState = pluginsState.getChild(i);

        if (!pluginState.hasType("Plugin"))
            continue;

        // Get plugin identifier
        juce::String identifier = pluginState.getProperty("identifier", "");
        juce::String name = pluginState.getProperty("name", "Unknown");

        if (identifier.isEmpty())
        {
            DBG("Track: Skipping plugin with no identifier");
            continue;
        }

        // Try to create plugin instance
        juce::String errorMessage;
        auto instance = pluginHost.createInstance(
            identifier,
            currentSampleRate > 0 ? currentSampleRate : 44100.0,
            currentBlockSize > 0 ? currentBlockSize : 512,
            errorMessage);

        if (instance == nullptr)
        {
            DBG("Track: WARNING - Failed to load plugin '" + name + "': " + errorMessage);
            continue;
        }

        // Restore plugin state
        juce::String stateBase64 = pluginState.getProperty("state", "");
        if (stateBase64.isNotEmpty())
        {
            juce::MemoryBlock stateData;
            if (stateData.fromBase64Encoding(stateBase64))
            {
                instance->setStateInformation(stateData.getData(), static_cast<int>(stateData.getSize()));
                DBG("Track: Restored state for plugin '" + name + "'");
            }
        }

        // Add to track
        addPlugin(std::move(instance));
        DBG("Track: Loaded plugin '" + name + "'");
    }

    DBG("Track: Loaded " + juce::String(getNumPlugins()) + " plugins");
}

//==============================================================================
void Track::processPluginChain(juce::AudioBuffer<float>& buffer, int numSamples)
{
    // Phase 3: Process plugin chain
    // RT-SAFE: We only read the plugins vector here, no modifications
    // The pluginLock is only used when adding/removing plugins (message thread)

    if (plugins.empty())
        return;

    // Clear MIDI buffer (for audio tracks, empty MIDI; for instrument tracks, would contain MIDI)
    pluginMidiBuffer.clear();

    // For MVP: Simple linear plugin chain processing
    // Audio tracks: audio in → plugins → audio out
    // Instrument tracks (future): MIDI in → first plugin (synth) → audio → remaining plugins → audio out

    // TODO(Phase 3+): Distinguish instrument vs audio tracks properly
    // For now, we process all plugins as audio FX (assuming audio input)

    for (auto& plugin : plugins)
    {
        if (plugin != nullptr)
        {
            // CODEX FEEDBACK APPLIED: Use max(inputs, outputs) for channel sizing
            // This ensures instrument plugins (0 inputs, >0 outputs) get writable channels
            // instead of receiving an empty buffer.
            const int bufferChannels = buffer.getNumChannels();
            const int pluginInputs = plugin->getTotalNumInputChannels();
            const int pluginOutputs = plugin->getTotalNumOutputChannels();

            // Use max(inputs, outputs) so instrument plugins (0 in, >0 out) get actual audio
            const int numChannels = juce::jmin(bufferChannels, juce::jmax(pluginInputs, pluginOutputs));

            // Create a view of the buffer with the correct number of channels
            juce::AudioBuffer<float> pluginView(
                buffer.getArrayOfWritePointers(),
                numChannels,
                0,
                numSamples);

            // Process this plugin
            // Note: processBlock expects the full buffer, not just a section
            // We're processing in-place
            plugin->processBlock(pluginView, pluginMidiBuffer);
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

} // namespace zenith
