/*
  ==============================================================================

    Clip.cpp
    Ported from: ZenithDAW-Native/Source/Audio/Clip.cpp (2025-11-11)
    Author:  Zenith DAW → Zenith DAW

    Audio/MIDI clip implementation

    JUCE 8 / C++20 adaptations:
    - Wrapped in namespace zenith
    - No container changes needed

  ==============================================================================
*/

#include "Track.h"
#include "Clip.h"
#include "AudioFilePool.h"

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

// Phase 1.3: Check if active at given playhead position
bool Track::Clip::isActiveAt(int64_t playheadSamples) const
{
    const int64_t start = startPosition.load();
    const int64_t end = start + clipLength.load();

    return playheadSamples >= start && playheadSamples < end;
}

// Legacy: Check if active at stored transport position
bool Track::Clip::isActive() const
{
    const int64_t pos = transportPosition.load();
    const int64_t start = startPosition.load();
    const int64_t end = start + clipLength.load();

    return pos >= start && pos < end;
}

//==============================================================================
// Phase 1.2: Set audio file using AudioFilePool (preferred method)
void Track::Clip::setAudioFileFromPool(const juce::File& file, AudioFilePool& pool)
{
    const juce::ScopedLock sl(audioLock);

    audioFile = file;

    // Load file through pool (message thread, does I/O)
    juce::String error;
    auto handle = pool.loadFile(file, error);

    if (handle)
    {
        // Store handle (shared_ptr is RT-safe for read)
        audioFileHandle_ = handle;

        // Set clip length to match audio file
        clipLength.store(handle->lengthInSamples);

        DBG("Clip: Loaded " + file.getFileName() +
            " (" + juce::String(handle->lengthInSamples) + " samples)");
    }
    else
    {
        DBG("Clip: Failed to load " + file.getFileName() + ": " + error);
        audioFileHandle_ = nullptr;
    }

    // Clear legacy audioSource (unused)
    audioSource.reset();
}

// Legacy method (kept for backward compatibility, but not recommended)
void Track::Clip::setAudioFile(const juce::File& file)
{
    const juce::ScopedLock sl(audioLock);

    audioFile = file;

    // Load the audio file (legacy path - loads directly without pool)
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

void Track::Clip::buildMidiSequenceFromNotes(const juce::Array<MidiNoteSpec>& notes,
                                              double clipStartBeats,
                                              double tempo)
{
    // This method must be called from the message thread only
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    // Create a new MIDI sequence
    juce::MidiMessageSequence newSequence;

    // Convert tempo to seconds per beat
    const double secondsPerBeat = 60.0 / tempo;

    // Build note-on and note-off events for each note
    for (const auto& note : notes)
    {
        // Skip muted notes
        if (note.muted)
            continue;

        // Convert beat times to seconds (relative to clip start)
        const double noteStartSeconds = note.startBeats * secondsPerBeat;
        const double noteEndSeconds = (note.startBeats + note.lengthBeats) * secondsPerBeat;

        // Validate pitch and velocity
        const int pitch = juce::jlimit(0, 127, note.pitch);
        const int velocity = juce::jlimit(0, 127, note.velocity);

        // Create note-on message
        juce::MidiMessage noteOn = juce::MidiMessage::noteOn(1, pitch, static_cast<juce::uint8>(velocity));
        noteOn.setTimeStamp(noteStartSeconds);
        newSequence.addEvent(noteOn);

        // Create note-off message
        juce::MidiMessage noteOff = juce::MidiMessage::noteOff(1, pitch, static_cast<juce::uint8>(0));
        noteOff.setTimeStamp(noteEndSeconds);
        newSequence.addEvent(noteOff);
    }

    // Sort events by timestamp
    newSequence.updateMatchedPairs();

    // Replace the current sequence (thread-safe via lock)
    {
        const juce::ScopedLock sl(midiLock);
        midiSequence = newSequence;

        // Update clip length based on the last MIDI event
        if (midiSequence.getNumEvents() > 0)
        {
            const double lastEventTime = midiSequence.getEndTime();
            const int64_t lengthInSamples = static_cast<int64_t>(lastEventTime * currentSampleRate);
            clipLength.store(lengthInSamples);
        }
    }

    DBG("Clip: Rebuilt MIDI sequence with " + juce::String(notes.size()) + " notes");
}

void Track::Clip::getMidiEvents(juce::MidiBuffer& midiBuffer, int numSamples)
{
    if (clipType != Type::MIDI)
        return;

    if (!playing.load() || midiSequence.getNumEvents() == 0)
        return;

    const juce::ScopedLock sl(midiLock);

    // Calculate time range for this block
    const int64_t currentPos = transportPosition.load();
    const int64_t clipOffsetSamples = clipOffset.load();

    // Position within the clip's MIDI sequence (accounting for offset)
    const int64_t posInClip = currentPos - clipOffsetSamples;

    if (posInClip < 0)
        return; // Clip hasn't started yet

    const double startTime = static_cast<double>(posInClip) / currentSampleRate;
    const double endTime = static_cast<double>(posInClip + numSamples) / currentSampleRate;

    // Find and add all MIDI events in this time range
    for (int i = 0; i < midiSequence.getNumEvents(); ++i)
    {
        auto* event = midiSequence.getEventPointer(i);
        const double eventTime = event->message.getTimeStamp();

        if (eventTime >= startTime && eventTime < endTime)
        {
            // Calculate sample offset within this block
            const int sampleOffset = static_cast<int>((eventTime - startTime) * currentSampleRate);

            // Add the MIDI message to the buffer
            midiBuffer.addEvent(event->message, sampleOffset);
        }
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
        juce::MidiFile midiFile;
        midiFile.addTrack(midiSequence);
        juce::MemoryOutputStream stream;
        midiFile.writeTo(stream);
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

    // Convert var to String before passing to Colour::fromString
    juce::var colorVar = state.getProperty("color", juce::Colours::blue.toString());
    clipColor = juce::Colour::fromString(colorVar.toString());

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
            juce::MemoryInputStream inputStream(midiData, false);
            midiFile.readFrom(inputStream);

            if (midiFile.getNumTracks() > 0)
            {
                setMidiSequence(*midiFile.getTrack(0));
            }
        }
    }
}

//==============================================================================
// Phase 1.3: Process audio clip with explicit playhead position
void Track::Clip::processAudioClip(const juce::AudioSourceChannelInfo& bufferToFill, int64_t playheadSamples)
{
    // Phase 1.2: Use AudioFilePool handle if available (RT-safe)
    // Otherwise fall back to legacy audioBuffer

    // Get audio source buffer (RT-safe - no locks needed for shared_ptr read)
    const juce::AudioBuffer<float>* sourceBuffer = nullptr;

    if (audioFileHandle_)
    {
        // Cast type-erased handle back to AudioFileHandle
        auto handle = std::static_pointer_cast<const AudioFilePool::AudioFileHandle>(audioFileHandle_);
        sourceBuffer = &handle->buffer;
    }
    else
    {
        // Fall back to legacy buffer (for setAudioBuffer() method)
        const juce::ScopedLock sl(audioLock);
        if (audioBuffer.getNumSamples() > 0)
        {
            sourceBuffer = &audioBuffer;
        }
    }

    if (!sourceBuffer || sourceBuffer->getNumSamples() == 0)
        return;

    const int64_t transportPos = playheadSamples;  // Use passed-in playhead
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
    if (looping.load() && sourcePosition >= sourceBuffer->getNumSamples())
    {
        sourcePosition = sourcePosition % sourceBuffer->getNumSamples();
    }

    // Copy audio from buffer
    const int numSamplesToCopy = juce::jmin(
        bufferToFill.numSamples,
        static_cast<int>(sourceBuffer->getNumSamples() - sourcePosition),
        static_cast<int>(clipLen - positionInClip));

    if (numSamplesToCopy <= 0)
        return;

    const float clipGain = gain.load();

    for (int ch = 0; ch < juce::jmin(bufferToFill.buffer->getNumChannels(), sourceBuffer->getNumChannels()); ++ch)
    {
        bufferToFill.buffer->copyFrom(
            ch,
            bufferToFill.startSample,
            *sourceBuffer,
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

// Legacy overload: uses internal transportPosition
void Track::Clip::processAudioClip(const juce::AudioSourceChannelInfo& bufferToFill)
{
    processAudioClip(bufferToFill, transportPosition.load());
}

// Phase 2A: Process MIDI clip - schedule MIDI events into MidiBuffer
void Track::Clip::processMidiClip(juce::MidiBuffer& midiBuffer, int64_t playheadSamples, int numSamples)
{
    // Get clip parameters (atomic loads)
    const int64_t clipStart = startPosition.load();
    const int64_t clipLen = clipLength.load();
    const int64_t clipOff = clipOffset.load();

    // Calculate position within clip
    const int64_t positionInClip = playheadSamples - clipStart;

    // Check if playhead is within clip bounds
    if (positionInClip < 0 || positionInClip >= clipLen)
        return;

    // Calculate the range of samples we're rendering: [blockStart, blockEnd)
    const int64_t blockStart = positionInClip;
    const int64_t blockEnd = positionInClip + numSamples;

    // Lock MIDI sequence for reading (RT-safe if sequence isn't being modified)
    const juce::ScopedLock sl(midiLock);

    // Iterate through MIDI events and schedule those that fall within this block
    for (int i = 0; i < midiSequence.getNumEvents(); ++i)
    {
        auto* event = midiSequence.getEventPointer(i);
        if (event == nullptr)
            continue;

        // Get event time in clip-relative samples
        // MidiMessageSequence stores times in seconds, convert to samples
        const double eventTimeSeconds = event->message.getTimeStamp();
        const int64_t eventSamples = static_cast<int64_t>(eventTimeSeconds * currentSampleRate);

        // Apply clip offset (trimming)
        const int64_t eventInClip = eventSamples - clipOff;

        // Handle looping
        int64_t adjustedEventSamples = eventInClip;
        if (looping.load() && clipLen > 0)
        {
            // Wrap event position into clip length
            if (adjustedEventSamples < 0)
                adjustedEventSamples = clipLen - ((-adjustedEventSamples) % clipLen);
            else if (adjustedEventSamples >= clipLen)
                adjustedEventSamples = adjustedEventSamples % clipLen;
        }

        // Check if event falls within current block
        if (adjustedEventSamples >= blockStart && adjustedEventSamples < blockEnd)
        {
            // Calculate sample offset within the buffer (0 to numSamples-1)
            const int sampleOffset = static_cast<int>(adjustedEventSamples - blockStart);

            // Add event to MIDI buffer
            midiBuffer.addEvent(event->message, sampleOffset);
        }
    }
}

// Legacy overload: uses internal transportPosition (for AudioSourceChannelInfo)
void Track::Clip::processMidiClip(const juce::AudioSourceChannelInfo& bufferToFill, int64_t playheadSamples)
{
    juce::ignoreUnused(bufferToFill, playheadSamples);
    // Legacy path - MIDI clips don't produce audio directly
    // They need to be processed by instrument plugins
    bufferToFill.clearActiveBufferRegion();
}

// Legacy overload: uses internal transportPosition
void Track::Clip::processMidiClip(const juce::AudioSourceChannelInfo& bufferToFill)
{
    processMidiClip(bufferToFill, transportPosition.load());
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

