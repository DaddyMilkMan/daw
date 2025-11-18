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

    // Prepare plugin buffer
    pluginBuffer.setSize(2, samplesPerBlockExpected);

    // TODO(Phase 2: plugin hosting) - Prepare all plugins

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

    // TODO(Phase 2: plugin hosting) - Release all plugins

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
// Plugin management - TODO(Phase 2: plugin hosting)
// Stubbed for now; will implement VST3/AU support in Phase 2

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

    // TODO(Phase 2: plugin hosting) - Save plugin states

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

    // TODO(Phase 2: plugin hosting) - Load plugin states

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

//==============================================================================
void Track::processPluginChain(juce::AudioBuffer<float>& buffer, int numSamples)
{
    (void)numSamples;
    // TODO(Phase 2: plugin hosting) - Process plugin chain
    // For now, this is a placeholder that will iterate over plugins when implemented

    // Example of correct buffer sizing for when plugins are added:
    // When processing plugins, especially instrument plugins (0 inputs, >0 outputs),
    // we must size the buffer view using max(inputs, outputs) to ensure synths
    // have writable channels instead of receiving an empty buffer.
    //
    // for (auto& plugin : pluginChain)
    // {
    //     const int bufferChannels = buffer.getNumChannels();
    //     const int pluginInputs = plugin->getTotalNumInputChannels();
    //     const int pluginOutputs = plugin->getTotalNumOutputChannels();
    //
    //     // Use max(inputs, outputs) so instrument plugins (0 in, >0 out) get actual audio
    //     const int numChannels = juce::jmin(bufferChannels, juce::jmax(pluginInputs, pluginOutputs));
    //
    //     juce::AudioBuffer<float> pluginView(
    //         buffer.getArrayOfWritePointers(),
    //         numChannels,
    //         0,
    //         numSamples);
    //
    //     plugin->processBlock(pluginView, midiBuffer_);
    // }
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
