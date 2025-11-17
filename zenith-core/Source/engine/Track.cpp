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
    midiBuffer.clear();

    // Prepare all plugins
    {
        const juce::ScopedLock sl(pluginLock);
        for (auto& plugin : plugins)
        {
            if (plugin != nullptr)
            {
                plugin->prepareToPlay(sampleRate, samplesPerBlockExpected);
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

    // Clear and prepare MIDI buffer for this block
    midiBuffer.clear();

    // TODO: Generate MIDI events from clips here
    // This will be implemented when we wire up the MIDI scheduler

    // Process through plugin chain
    processPluginChain(localBuffer, midiBuffer, bufferToFill.numSamples);

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

void Track::addPlugin(std::unique_ptr<juce::AudioPluginInstance> plugin)
{
    if (plugin != nullptr)
    {
        const juce::ScopedLock sl(pluginLock);

        // Prepare the plugin if we're already initialized
        if (currentSampleRate > 0)
        {
            plugin->prepareToPlay(currentSampleRate, currentBlockSize);
        }

        plugins.push_back(std::move(plugin));
        sendChangeMessage();
    }
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
void Track::processPluginChain(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, int numSamples)
{
    const juce::ScopedLock sl(pluginLock);

    // Process each plugin in the chain
    for (auto& plugin : plugins)
    {
        if (plugin != nullptr)
        {
            // Ensure buffer has correct size for plugin
            int numChannels = juce::jmin(buffer.getNumChannels(), plugin->getTotalNumInputChannels());

            // Create a wrapped buffer view for the plugin
            juce::AudioBuffer<float> pluginView(buffer.getArrayOfWritePointers(),
                                                 numChannels,
                                                 0,
                                                 numSamples);

            // Process the plugin
            plugin->processBlock(pluginView, midi);

            // Note: MIDI buffer is passed through the chain.
            // Instrument plugins consume note-on/off events,
            // effect plugins typically ignore MIDI (but some use it for modulation).
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

//==============================================================================
// MIDI Scheduling
//==============================================================================

void Track::generateMidiForBlock(const juce::ValueTree& trackState,
                                   double tempo,
                                   double sampleRate,
                                   juce::int64 blockStartSample,
                                   int blockSize,
                                   juce::MidiBuffer& midiOut)
{
    // Calculate block boundaries in samples
    juce::int64 blockEndSample = blockStartSample + blockSize;

    // Convert to beats
    // Formula: beats = (samples / sampleRate) * (tempo / 60)
    double beatsPerSample = (tempo / 60.0) / sampleRate;
    double blockStartBeats = blockStartSample * beatsPerSample;
    double blockEndBeats = blockEndSample * beatsPerSample;

    // Get CLIPS node from track state
    auto clipsNode = trackState.getChildWithName(juce::Identifier("CLIPS"));
    if (!clipsNode.isValid())
        return;

    // Process each clip
    for (auto clip : clipsNode)
    {
        // Get clip position (in beats or samples - need to check)
        // For now, assume clip.start is in beats
        double clipStartBeats = clip.getProperty("start", 0.0);
        double clipLengthBeats = clip.getProperty("length", 0.0);

        // Skip clips that don't overlap this block
        if (clipStartBeats + clipLengthBeats < blockStartBeats || clipStartBeats > blockEndBeats)
            continue;

        // Get NOTES node from clip
        auto notesNode = clip.getChildWithName(juce::Identifier("NOTES"));
        if (!notesNode.isValid())
            continue;

        // Process each note in the clip
        for (auto note : notesNode)
        {
            // Get note properties
            double noteStartBeats = note.getProperty("startBeats", 0.0);
            double noteLengthBeats = note.getProperty("lengthBeats", 0.0);
            int pitch = note.getProperty("pitch", 60);
            int velocity = note.getProperty("velocity", 100);
            juce::String noteId = note.getProperty("id", "");

            // Convert note times to absolute timeline beats (relative to clip)
            double absNoteStartBeats = clipStartBeats + noteStartBeats;
            double absNoteEndBeats = absNoteStartBeats + noteLengthBeats;

            // Convert to samples
            double samplesPerBeat = sampleRate * 60.0 / tempo;
            juce::int64 noteStartSample = static_cast<juce::int64>(absNoteStartBeats * samplesPerBeat);
            juce::int64 noteEndSample = static_cast<juce::int64>(absNoteEndBeats * samplesPerBeat);

            // Check if note-on happens in this block
            if (noteStartSample >= blockStartSample && noteStartSample < blockEndSample)
            {
                // Calculate offset within this block
                int sampleOffset = static_cast<int>(noteStartSample - blockStartSample);

                // Create note-on event
                juce::MidiMessage noteOn = juce::MidiMessage::noteOn(1, pitch, static_cast<juce::uint8>(velocity));
                midiOut.addEvent(noteOn, sampleOffset);

                // Track this note as active
                ActiveNote activeNote;
                activeNote.pitch = pitch;
                activeNote.channel = 1;
                activeNote.noteId = noteId;
                activeNotes.push_back(activeNote);
            }

            // Check if note-off happens in this block
            if (noteEndSample >= blockStartSample && noteEndSample < blockEndSample)
            {
                // Calculate offset within this block
                int sampleOffset = static_cast<int>(noteEndSample - blockStartSample);

                // Create note-off event
                juce::MidiMessage noteOff = juce::MidiMessage::noteOff(1, pitch, static_cast<juce::uint8>(0));
                midiOut.addEvent(noteOff, sampleOffset);

                // Remove from active notes
                activeNotes.erase(
                    std::remove_if(activeNotes.begin(), activeNotes.end(),
                        [&](const ActiveNote& n) { return n.noteId == noteId; }),
                    activeNotes.end());
            }
        }
    }
}

} // namespace zenith
