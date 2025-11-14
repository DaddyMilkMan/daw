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
    // Initialize empty clip snapshot
    clipsSnapshot_.store(std::make_shared<const ClipSnapshot>());
}

Track::~Track()
{
    // Ensure we're not in the middle of audio processing
    const juce::ScopedLock sl1(pluginLock);

    // Phase 2A: Clips are automatically destroyed via std::unique_ptr
    // No lock needed - destructor is called from message thread only
    clipsOwned_.clear();
    clipsSnapshot_.store(std::make_shared<const ClipSnapshot>());
}

//==============================================================================
void Track::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlockExpected;

    // Prepare plugin buffer
    pluginBuffer.setSize(2, samplesPerBlockExpected);

    // Phase 1: Pre-allocate clip buffer to avoid RT allocations
    clipBuffer_.setSize(2, samplesPerBlockExpected, false, true, false);

    // TODO(Phase 2: plugin hosting) - Prepare all plugins

    // Phase 2A: Prepare all clips (message thread only, no lock needed)
    for (auto& clip : clipsOwned_)
    {
        if (clip != nullptr)
        {
            clip->prepareToPlay(samplesPerBlockExpected, sampleRate);
        }
    }
}

void Track::releaseResources()
{
    // TODO(Phase 2: plugin hosting) - Release all plugins

    // Phase 2A: Release all clips (message thread only, no lock needed)
    for (auto& clip : clipsOwned_)
    {
        if (clip != nullptr)
        {
            clip->releaseResources();
        }
    }
}

// Phase 1.3 / 2A: Process with explicit playhead position (lock-free)
void Track::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill, int64_t playheadSamples)
{
    // Clear the buffer first
    bufferToFill.clearActiveBufferRegion();

    // If track is disabled or muted, return silence
    if (!enabled.load() || muted.load())
    {
        currentLevel.store(0.0f);
        midiBuffer_.clear();  // Phase 2A: Clear MIDI buffer too
        return;
    }

    // Phase 2A: Clear MIDI buffer for this block
    midiBuffer_.clear();

    // Phase 2A: Get current clip snapshot (RT-safe atomic load)
    auto currentSnapshot = clipsSnapshot_.load(std::memory_order_acquire);

    if (currentSnapshot)
    {
        // Iterate clips from snapshot (no lock needed!)
        for (auto* clip : currentSnapshot->clips)
        {
            if (clip != nullptr && clip->isPlaying() && clip->isActiveAt(playheadSamples))
            {
                if (clip->getType() == Clip::Type::Audio)
                {
                    // Phase 1: Use pre-allocated clipBuffer_ to avoid RT allocations
                    clipBuffer_.clear();

                    juce::AudioSourceChannelInfo clipInfo(&clipBuffer_, 0, bufferToFill.numSamples);

                    // Phase 1.3: Pass playhead to clip for timing
                    clip->processAudioClip(clipInfo, playheadSamples);

                    // Mix clip into main buffer
                    const int channelsToMix = juce::jmin(bufferToFill.buffer->getNumChannels(),
                                                          clipBuffer_.getNumChannels());

                    for (int ch = 0; ch < channelsToMix; ++ch)
                    {
                        bufferToFill.buffer->addFrom(
                            ch,
                            bufferToFill.startSample,
                            clipBuffer_,
                            ch,
                            0,
                            bufferToFill.numSamples);
                    }
                }
                else if (clip->getType() == Clip::Type::MIDI)
                {
                    // Phase 2A: Process MIDI clip into track's MIDI buffer
                    clip->processMidiClip(midiBuffer_, playheadSamples, bufferToFill.numSamples);
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
    // TODO(Phase 2B: plugin hosting) - Pass midiBuffer_ to plugins
    processPluginChain(localBuffer, bufferToFill.numSamples);

    // Apply volume and pan
    applyGainAndPan(localBuffer, bufferToFill.numSamples);

    // Update level meters
    updateLevelMeters(localBuffer, bufferToFill.numSamples);
}

// Legacy overload: uses default playhead of 0
void Track::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    getNextAudioBlock(bufferToFill, 0);
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
// Phase 2A: Lock-free clip management (message thread only)
//==============================================================================

void Track::updateClipSnapshot()
{
    // Called from message thread only - no lock needed
    // Create new snapshot from current ownership
    auto newSnapshot = std::make_shared<const ClipSnapshot>(clipsOwned_);

    // Atomically swap snapshot (audio thread will see new snapshot on next load)
    clipsSnapshot_.store(newSnapshot, std::memory_order_release);
}

void Track::addClip(std::unique_ptr<Clip> clip)
{
    if (clip != nullptr)
    {
        // Prepare the clip if we're already initialized
        if (currentSampleRate > 0)
        {
            clip->prepareToPlay(currentBlockSize, currentSampleRate);
        }

        // Add to ownership vector
        clipsOwned_.push_back(std::move(clip));

        // Update snapshot for audio thread
        updateClipSnapshot();

        sendChangeMessage();
    }
}

void Track::removeClip(int clipIndex)
{
    if (clipIndex >= 0 && clipIndex < static_cast<int>(clipsOwned_.size()))
    {
        auto& clip = clipsOwned_[clipIndex];
        if (clip != nullptr)
        {
            clip->releaseResources();
        }

        // Remove from ownership vector
        clipsOwned_.erase(clipsOwned_.begin() + clipIndex);

        // Update snapshot for audio thread
        updateClipSnapshot();

        sendChangeMessage();
    }
}

void Track::removeClip(Clip* clip)
{
    auto it = std::find_if(clipsOwned_.begin(), clipsOwned_.end(),
        [clip](const std::unique_ptr<Clip>& c) { return c.get() == clip; });

    if (it != clipsOwned_.end())
    {
        (*it)->releaseResources();

        // Remove from ownership vector
        clipsOwned_.erase(it);

        // Update snapshot for audio thread
        updateClipSnapshot();

        sendChangeMessage();
    }
}

void Track::clearClips()
{
    for (auto& clip : clipsOwned_)
    {
        if (clip != nullptr)
        {
            clip->releaseResources();
        }
    }

    // Clear ownership vector
    clipsOwned_.clear();

    // Update snapshot for audio thread
    updateClipSnapshot();

    sendChangeMessage();
}

int Track::getNumClips() const
{
    // Message thread access - read ownership vector directly
    return static_cast<int>(clipsOwned_.size());
}

Track::Clip* Track::getClip(int index) const
{
    // Message thread access - read ownership vector directly
    if (index >= 0 && index < static_cast<int>(clipsOwned_.size()))
        return clipsOwned_[index].get();
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

    // Phase 2A: Save clip states (message thread, no lock needed)
    juce::ValueTree clipsState("Clips");
    for (auto& clip : clipsOwned_)
    {
        if (clip != nullptr)
        {
            clipsState.appendChild(clip->getState(), nullptr);
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
    (void)buffer;
    (void)numSamples;
    // TODO(Phase 2: plugin hosting) - Process plugin chain
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
