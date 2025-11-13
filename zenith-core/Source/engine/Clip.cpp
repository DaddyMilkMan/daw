/*
  ==============================================================================

    Clip.cpp
    Ported from: VexelDAW-Native/Source/Audio/Clip.cpp (2025-11-11)
    Author:  Vexel DAW → Zenith DAW

    Audio/MIDI clip implementation

    JUCE 8 / C++20 adaptations:
    - Wrapped in namespace zenith
    - No container changes needed

  ==============================================================================
*/

#include "Track.h"
#include "Clip.h"

namespace zenith {

//==============================================================================
Track::Clip::Clip()
{
}

Track::Clip::~Clip()
{
    releaseResources();
}

//==============================================================================
void Track::Clip::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlockExpected;

    if (audioSource != nullptr)
    {
        audioSource->prepareToPlay(samplesPerBlockExpected, sampleRate);
    }
}

void Track::Clip::releaseResources()
{
    if (audioSource != nullptr)
    {
        audioSource->releaseResources();
    }
}

void Track::Clip::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    bufferToFill.clearActiveBufferRegion();

    if (!playing.load() || !isActive())
    {
        return;
    }

    if (clipType == Type::Audio)
    {
        processAudioClip(bufferToFill);
    }
    else if (clipType == Type::MIDI)
    {
        processMidiClip(bufferToFill);
    }
}

//==============================================================================
void Track::Clip::setStartPosition(int64_t position)
{
    startPosition.store(juce::jmax(int64_t(0), position));
}

void Track::Clip::setLength(int64_t lengthInSamples)
{
    clipLength.store(juce::jmax(int64_t(0), lengthInSamples));
}

void Track::Clip::setOffset(int64_t offsetInSamples)
{
    clipOffset.store(juce::jmax(int64_t(0), offsetInSamples));
}

//==============================================================================
void Track::Clip::setTransportPosition(int64_t position)
{
    transportPosition.store(position);
}

void Track::Clip::setPlaying(bool shouldPlay)
{
    playing.store(shouldPlay);
}

bool Track::Clip::isActive() const
{
    const int64_t pos = transportPosition.load();
    const int64_t start = startPosition.load();
    const int64_t end = start + clipLength.load();

    return pos >= start && pos < end;
}

//==============================================================================
void Track::Clip::setAudioFile(const juce::File& file)
{
    const juce::ScopedLock sl(audioLock);

    audioFile = file;

    // Load the audio file
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    auto* reader = formatManager.createReaderFor(file);

    if (reader != nullptr)
    {
        // Read the entire file into memory
        audioBuffer.setSize(static_cast<int>(reader->numChannels),
                           static_cast<int>(reader->lengthInSamples));

        reader->read(&audioBuffer,
                    0,
                    static_cast<int>(reader->lengthInSamples),
                    0,
                    true,
                    true);

        // Set clip length to match audio file length
        clipLength.store(reader->lengthInSamples);

        // Create audio source for playback
        audioSource.reset(new juce::AudioFormatReaderSource(reader, true));

        if (currentSampleRate > 0)
        {
            audioSource->prepareToPlay(currentBlockSize, currentSampleRate);
        }
    }
}

void Track::Clip::setAudioBuffer(const juce::AudioBuffer<float>& buffer)
{
    const juce::ScopedLock sl(audioLock);

    audioBuffer.makeCopyOf(buffer);
    clipLength.store(buffer.getNumSamples());
}

//==============================================================================
void Track::Clip::setMidiSequence(const juce::MidiMessageSequence& sequence)
{
    const juce::ScopedLock sl(midiLock);

    midiSequence = sequence;

    // Calculate clip length from MIDI sequence
    if (midiSequence.getNumEvents() > 0)
    {
        const double lastEventTime = midiSequence.getEndTime();
        const int64_t lengthInSamples = static_cast<int64_t>(lastEventTime * currentSampleRate);
        clipLength.store(lengthInSamples);
    }
}

//==============================================================================
void Track::Clip::setFadeIn(int64_t fadeInSamples)
{
    fadeInLength.store(juce::jmax(int64_t(0), fadeInSamples));
}

void Track::Clip::setFadeOut(int64_t fadeOutSamples)
{
    fadeOutLength.store(juce::jmax(int64_t(0), fadeOutSamples));
}

//==============================================================================
void Track::Clip::setGain(float newGain)
{
    gain.store(juce::jlimit(0.0f, 2.0f, newGain));
}

//==============================================================================
void Track::Clip::setLooping(bool shouldLoop)
{
    looping.store(shouldLoop);
}

//==============================================================================
void Track::Clip::setColor(juce::Colour color)
{
    clipColor = color;
}

//==============================================================================
juce::ValueTree Track::Clip::getState() const
{
    juce::ValueTree state("Clip");

    state.setProperty("name", clipName, nullptr);
    state.setProperty("type", static_cast<int>(clipType), nullptr);
    state.setProperty("startPosition", static_cast<int>(startPosition.load()), nullptr);
    state.setProperty("length", static_cast<int>(clipLength.load()), nullptr);
    state.setProperty("offset", static_cast<int>(clipOffset.load()), nullptr);
    state.setProperty("fadeIn", static_cast<int>(fadeInLength.load()), nullptr);
    state.setProperty("fadeOut", static_cast<int>(fadeOutLength.load()), nullptr);
    state.setProperty("gain", gain.load(), nullptr);
    state.setProperty("looping", looping.load(), nullptr);
    state.setProperty("color", clipColor.toString(), nullptr);

    if (clipType == Type::Audio && audioFile.existsAsFile())
    {
        state.setProperty("audioFile", audioFile.getFullPathName(), nullptr);
    }
    else if (clipType == Type::MIDI)
    {
        const juce::ScopedLock sl(midiLock);

        // Save MIDI sequence as base64
        juce::MemoryOutputStream stream;
        midiSequence.createMidiFile(stream, 0);
        state.setProperty("midiData", stream.getMemoryBlock().toBase64Encoding(), nullptr);
    }

    return state;
}

void Track::Clip::loadState(const juce::ValueTree& state)
{
    if (!state.hasType("Clip"))
        return;

    clipName = state.getProperty("name", "Clip");
    clipType = static_cast<Type>(static_cast<int>(state.getProperty("type", 0)));
    startPosition.store(state.getProperty("startPosition", 0));
    clipLength.store(state.getProperty("length", 0));
    clipOffset.store(state.getProperty("offset", 0));
    fadeInLength.store(state.getProperty("fadeIn", 0));
    fadeOutLength.store(state.getProperty("fadeOut", 0));
    gain.store(state.getProperty("gain", 1.0f));
    looping.store(state.getProperty("looping", false));
    clipColor = juce::Colour::fromString(state.getProperty("color", juce::Colours::blue.toString()));

    if (clipType == Type::Audio)
    {
        juce::String audioFilePath = state.getProperty("audioFile", "");
        if (audioFilePath.isNotEmpty())
        {
            setAudioFile(juce::File(audioFilePath));
        }
    }
    else if (clipType == Type::MIDI)
    {
        juce::String midiDataBase64 = state.getProperty("midiData", "");
        if (midiDataBase64.isNotEmpty())
        {
            juce::MemoryBlock midiData;
            midiData.fromBase64Encoding(midiDataBase64);

            juce::MidiFile midiFile;
            midiFile.readFrom(midiData.begin(), midiData.getSize());

            if (midiFile.getNumTracks() > 0)
            {
                setMidiSequence(*midiFile.getTrack(0));
            }
        }
    }
}

//==============================================================================
void Track::Clip::processAudioClip(const juce::AudioSourceChannelInfo& bufferToFill)
{
    const juce::ScopedLock sl(audioLock);

    if (audioBuffer.getNumSamples() == 0)
        return;

    const int64_t transportPos = transportPosition.load();
    const int64_t clipStart = startPosition.load();
    const int64_t clipLen = clipLength.load();
    const int64_t clipOff = clipOffset.load();

    // Calculate position within clip
    int64_t positionInClip = transportPos - clipStart;

    if (positionInClip < 0 || positionInClip >= clipLen)
        return;

    // Add offset for trimming
    int64_t sourcePosition = positionInClip + clipOff;

    // Handle looping
    if (looping.load() && sourcePosition >= audioBuffer.getNumSamples())
    {
        sourcePosition = sourcePosition % audioBuffer.getNumSamples();
    }

    // Copy audio from buffer
    const int numSamplesToCopy = juce::jmin(
        bufferToFill.numSamples,
        static_cast<int>(audioBuffer.getNumSamples() - sourcePosition),
        static_cast<int>(clipLen - positionInClip));

    if (numSamplesToCopy <= 0)
        return;

    const float clipGain = gain.load();

    for (int ch = 0; ch < juce::jmin(bufferToFill.buffer->getNumChannels(), audioBuffer.getNumChannels()); ++ch)
    {
        bufferToFill.buffer->copyFrom(
            ch,
            bufferToFill.startSample,
            audioBuffer,
            ch,
            static_cast<int>(sourcePosition),
            numSamplesToCopy);

        // Apply gain
        bufferToFill.buffer->applyGain(ch, bufferToFill.startSample, numSamplesToCopy, clipGain);

        // Apply fades
        for (int i = 0; i < numSamplesToCopy; ++i)
        {
            const float fadeMultiplier = calculateFadeMultiplier(positionInClip + i);
            const float sample = bufferToFill.buffer->getSample(ch, bufferToFill.startSample + i);
            bufferToFill.buffer->setSample(ch, bufferToFill.startSample + i, sample * fadeMultiplier);
        }
    }
}

void Track::Clip::processMidiClip(const juce::AudioSourceChannelInfo& bufferToFill)
{
    // MIDI clips don't produce audio directly
    // They would need to be processed by an instrument plugin
    // For now, just clear the buffer
    bufferToFill.clearActiveBufferRegion();

    // TODO(Phase 2: plugin hosting) - Send MIDI events to parent track's instrument plugins
}

float Track::Clip::calculateFadeMultiplier(int64_t positionInClip) const
{
    const int64_t fadeIn = fadeInLength.load();
    const int64_t fadeOut = fadeOutLength.load();
    const int64_t clipLen = clipLength.load();

    float multiplier = 1.0f;

    // Fade in
    if (fadeIn > 0 && positionInClip < fadeIn)
    {
        multiplier *= static_cast<float>(positionInClip) / static_cast<float>(fadeIn);
    }

    // Fade out
    if (fadeOut > 0 && positionInClip > clipLen - fadeOut)
    {
        const int64_t positionInFadeOut = clipLen - positionInClip;
        multiplier *= static_cast<float>(positionInFadeOut) / static_cast<float>(fadeOut);
    }

    return multiplier;
}

} // namespace zenith
