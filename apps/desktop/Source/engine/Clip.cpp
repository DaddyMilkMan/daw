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
#include <cmath>
#include <algorithm>

namespace zenith {

//==============================================================================
Clip::Clip() : midiSequence_(std::make_shared<juce::MidiMessageSequence>())
{
}

Clip::~Clip()
{
    releaseResources();
}

//==============================================================================
void Clip::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlockExpected;

    if (audioSource != nullptr)
    {
        audioSource->prepareToPlay(samplesPerBlockExpected, sampleRate);
    }
}

void Clip::releaseResources()
{
    if (audioSource != nullptr)
    {
        audioSource->releaseResources();
    }
}

void Clip::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
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
void Clip::setStartPosition(int64_t position)
{
    startPosition.store(juce::jmax(int64_t(0), position));
}

void Clip::setLength(int64_t lengthInSamples)
{
    clipLength.store(juce::jmax(int64_t(0), lengthInSamples));
}

void Clip::setOffset(int64_t offsetInSamples)
{
    clipOffset.store(juce::jmax(int64_t(0), offsetInSamples));
}

//==============================================================================
void Clip::setTransportPosition(int64_t position)
{
    transportPosition.store(position);
}

void Clip::setPlaying(bool shouldPlay)
{
    playing.store(shouldPlay);
}

// Phase 1.3: Check if active at given playhead position
bool Clip::isActiveAt(int64_t playheadSamples) const
{
    const int64_t start = startPosition.load();
    const int64_t end = start + clipLength.load();

    return playheadSamples >= start && playheadSamples < end;
}

// Legacy: Check if active at stored transport position
bool Clip::isActive() const
{
    const int64_t pos = transportPosition.load();
    const int64_t start = startPosition.load();
    const int64_t end = start + clipLength.load();

    return pos >= start && pos < end;
}

//==============================================================================
// Phase 1.2: Set audio file using AudioFilePool (preferred method)
void Clip::setAudioFileFromPool(const juce::File& file, AudioFilePool& pool)
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
void Clip::setAudioFile(const juce::File& file)
{
    const juce::ScopedLock sl(audioLock);

    audioFile = file;

    // Load the audio file (legacy path - loads directly without pool)
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    auto* reader = formatManager.createReaderFor(file);

    if (reader != nullptr && reader->numChannels > 0 && reader->lengthInSamples > 0)
    {
        // Read the entire file into memory
        audioBuffer.setSize(static_cast<int>(reader->numChannels),
                           static_cast<int>(reader->lengthInSamples));

        bool readSuccess = reader->read(&audioBuffer,
                    0,
                    static_cast<int>(reader->lengthInSamples),
                    0,
                    true,
                    true);

        if (!readSuccess)
        {
            // Read failed - clear the buffer
            audioBuffer.setSize(0, 0);
            delete reader;
            DBG("Clip: Failed to read audio file - file may be corrupted");
            return;
        }

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

void Clip::setAudioBuffer(const juce::AudioBuffer<float>& buffer)
{
    const juce::ScopedLock sl(audioLock);

    audioBuffer.makeCopyOf(buffer);
    clipLength.store(buffer.getNumSamples());
}

//==============================================================================
void Clip::setMidiSequence(const juce::MidiMessageSequence& sequence)
{
    auto newSequence = std::make_shared<juce::MidiMessageSequence>(sequence);
    
    {
        const juce::ScopedLock sl(midiLock);
        midiSequence_.store(newSequence, std::memory_order_release);
    }

    if (newSequence->getNumEvents() > 0)
    {
        const double lastEventTime = newSequence->getEndTime();
        const int64_t lengthInSamples = static_cast<int64_t>(lastEventTime * currentSampleRate);
        clipLength.store(lengthInSamples);
    }
}

void Clip::buildMidiSequenceFromNotes(const juce::Array<MidiNoteSpec>& notes,
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

    // Replace the current sequence (thread-safe via swap)
    auto sharedSeq = std::make_shared<juce::MidiMessageSequence>(newSequence);
    {
        const juce::ScopedLock sl(midiLock);
        midiSequence_.store(sharedSeq, std::memory_order_release);

        // Update clip length based on the last MIDI event
        if (sharedSeq->getNumEvents() > 0)
        {
            const double lastEventTime = sharedSeq->getEndTime();
            const int64_t lengthInSamples = static_cast<int64_t>(lastEventTime * currentSampleRate);
            clipLength.store(lengthInSamples);
        }
    }

    DBG("Clip: Rebuilt MIDI sequence with " + juce::String(notes.size()) + " notes");
}

void Clip::getMidiEvents(juce::MidiBuffer& midiBuffer, int numSamples)
{
    if (clipType != Type::MIDI)
        return;

    auto sequence = midiSequence_.load(std::memory_order_acquire);
    if (!playing.load() || sequence == nullptr || sequence->getNumEvents() == 0)
        return;

    // NO ScopedLock on audio thread
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
    for (int i = 0; i < sequence->getNumEvents(); ++i)
    {
        auto* event = sequence->getEventPointer(i);
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
void Clip::setFadeIn(int64_t fadeInSamples)
{
    fadeInLength.store(juce::jmax(int64_t(0), fadeInSamples));
}

void Clip::setFadeOut(int64_t fadeOutSamples)
{
    fadeOutLength.store(juce::jmax(int64_t(0), fadeOutSamples));
}

//==============================================================================
void Clip::setGain(float newGain)
{
    gain.store(juce::jlimit(0.0f, 2.0f, newGain));
}

void Clip::setPlaybackRate(double newRate)
{
    // Clamp rate between 0.25x and 4.0x
    playbackRate_.store(juce::jlimit(0.25, 4.0, newRate));
}

double Clip::getPlaybackRate() const
{
    return playbackRate_.load();
}

void Clip::setPreservePitch(bool shouldPreserve)
{
    preservePitch_.store(shouldPreserve);
    
    // Initialize WSOLA buffers if needed (on message thread)
    // Note: We use static constants defined in header or locally if needed. 
    // Assuming kWsolaWindowSize=2048 is defined in class.
    if (shouldPreserve)
    {
        juce::ScopedLock sl(audioLock); // Protect buffer resizing
        
        // Pre-allocate window and buffer
        if (wsolaWindow_.empty())
        {
            wsolaWindow_.resize(kWsolaWindowSize);
            // Create Hann window
            for (int i = 0; i < kWsolaWindowSize; ++i)
            {
                wsolaWindow_[i] = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * i / (kWsolaWindowSize - 1)));
            }
            wsolaOutputBuffer_.resize(kWsolaWindowSize * 2, 0.0f); // Double size for safety
        }
    }
}

bool Clip::isPreservingPitch() const
{
    return preservePitch_.load();
}

//==============================================================================
void Clip::setLooping(bool shouldLoop)
{
    looping.store(shouldLoop);
}

//==============================================================================
void Clip::setColor(juce::Colour color)
{
    clipColor = color;
}

//==============================================================================
juce::ValueTree Clip::getState() const
{
    juce::ValueTree state("Clip");

    state.setProperty("name", clipName, nullptr);
    state.setProperty("type", static_cast<int>(clipType), nullptr);
    // Use juce::int64 to avoid overflow for large sample positions
    state.setProperty("startPosition", static_cast<juce::int64>(startPosition.load()), nullptr);
    state.setProperty("length", static_cast<juce::int64>(clipLength.load()), nullptr);
    state.setProperty("offset", static_cast<juce::int64>(clipOffset.load()), nullptr);
    state.setProperty("fadeIn", static_cast<juce::int64>(fadeInLength.load()), nullptr);
    state.setProperty("fadeOut", static_cast<juce::int64>(fadeOutLength.load()), nullptr);
    state.setProperty("gain", gain.load(), nullptr);
    state.setProperty("looping", looping.load(), nullptr);
    state.setProperty("color", clipColor.toString(), nullptr);

    if (clipType == Type::Audio && audioFile.existsAsFile())
    {
        state.setProperty("audioFile", audioFile.getFullPathName(), nullptr);
    }
    else if (clipType == Type::MIDI)
    {
        auto sequence = midiSequence_.load(std::memory_order_acquire);
        const juce::ScopedLock sl(midiLock);

        // Save MIDI sequence as base64
        juce::MidiFile midiFile;
        if (sequence)
            midiFile.addTrack(*sequence);
        juce::MemoryOutputStream stream;
        midiFile.writeTo(stream);
        state.setProperty("midiData", stream.getMemoryBlock().toBase64Encoding(), nullptr);
    }

    return state;
}

void Clip::loadState(const juce::ValueTree& state)
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
// Phase 1.3: Process audio clip with explicit playhead position and Time-Stretching
void Clip::processAudioClip(const juce::AudioSourceChannelInfo& bufferToFill, int64_t playheadSamples)
{
    // [DSP Optimization] Use RT-safe handle
    auto handle = std::static_pointer_cast<const AudioFilePool::AudioFileHandle>(
        std::atomic_load_explicit(&audioFileHandle_, std::memory_order_acquire));
    const juce::AudioBuffer<float>* sourceBuffer = nullptr;
    
    if (handle != nullptr && handle->isValid()) {
        sourceBuffer = &handle->buffer;
    } else {
        // Fallback to legacy audioBuffer (RT-unsafe if modified, but we remove the lock)
        sourceBuffer = &audioBuffer;
    }

    if (sourceBuffer == nullptr || sourceBuffer->getNumSamples() == 0)
        return;

    const double rate = playbackRate_.load();
    const int64_t clipStart = startPosition.load();
    const int64_t clipLen = clipLength.load();
    const int64_t clipOff = clipOffset.load();
    const float clipGain = gain.load();

    int64_t positionInClip = playheadSamples - clipStart;
    if (positionInClip < 0 || positionInClip >= clipLen)
        return;

    // Fast path: rate is 1.0 (Standard playback)
    if (std::abs(rate - 1.0) < 0.001)
    {
        int64_t sourcePos = positionInClip + clipOff;
        if (looping.load() && sourceBuffer->getNumSamples() > 0)
            sourcePos = sourcePos % sourceBuffer->getNumSamples();

        const int numToCopy = juce::jmin(bufferToFill.numSamples, (int)(sourceBuffer->getNumSamples() - sourcePos), (int)(clipLen - positionInClip));
        if (numToCopy <= 0) return;

        for (int ch = 0; ch < juce::jmin(bufferToFill.buffer->getNumChannels(), sourceBuffer->getNumChannels()); ++ch)
        {
            bufferToFill.buffer->copyFrom(ch, bufferToFill.startSample, *sourceBuffer, ch, (int)sourcePos, numToCopy);
            if (clipGain != 1.0f)
                juce::FloatVectorOperations::multiply(bufferToFill.buffer->getWritePointer(ch, bufferToFill.startSample), clipGain, numToCopy);
        }
        
        applyFadesSIMD(bufferToFill, positionInClip, numToCopy);
        return;
    }

    // Slow path: Linear Interpolation (SIMD candidate for alpha ramp)
    // For now, let's keep it simple but remove the fade calculations from the inner loop
    const int numSamples = juce::jmin(bufferToFill.numSamples, (int)(clipLen - positionInClip));
    const int sourceLen = sourceBuffer->getNumSamples();
    
    for (int ch = 0; ch < juce::jmin(bufferToFill.buffer->getNumChannels(), sourceBuffer->getNumChannels()); ++ch)
    {
        auto* outData = bufferToFill.buffer->getWritePointer(ch, bufferToFill.startSample);
        const auto* inData = sourceBuffer->getReadPointer(ch);
        double currentReadPos = (double)(positionInClip + clipOff) * rate;

        for (int i = 0; i < numSamples; ++i)
        {
            int idx0 = static_cast<int>(currentReadPos);
            int idx1 = idx0 + 1;
            float alpha = static_cast<float>(currentReadPos - idx0);

            // Bug 39 & 45: Guard modulo and boundary conditions
            if (looping.load() && sourceLen > 0) {
                idx0 = juce::positiveModulo(idx0, sourceLen);
                idx1 = juce::positiveModulo(idx1, sourceLen);
            } else {
                idx0 = juce::jlimit(0, sourceLen - 1, idx0);
                idx1 = juce::jlimit(0, sourceLen - 1, idx1);
            }

            outData[i] = (1.0f - alpha) * inData[idx0] + alpha * inData[idx1];
            currentReadPos += rate;
        }
        
        if (clipGain != 1.0f)
            juce::FloatVectorOperations::multiply(outData, clipGain, numSamples);
    }
    
    applyFadesSIMD(bufferToFill, positionInClip, numSamples);
}

// Legacy overload: uses internal transportPosition
void Clip::processAudioClip(const juce::AudioSourceChannelInfo& bufferToFill)
{
    processAudioClip(bufferToFill, transportPosition.load());
}

// Phase 2A: Process MIDI clip - schedule MIDI events into MidiBuffer
void Clip::processMidiClip(juce::MidiBuffer& midiBuffer, int64_t playheadSamples, int numSamples)
{
    // Get clip parameters (atomic loads)
    const int64_t clipStart = startPosition.load();
    const int64_t clipLen = clipLength.load();
    const int64_t clipOff = clipOffset.load();

    // Snapshot MIDI sequence
    auto sequence = midiSequence_.load(std::memory_order_acquire);
    if (sequence == nullptr)
        return;

    // Calculate position within clip
    const int64_t positionInClip = playheadSamples - clipStart;

    // Check if playhead is within clip bounds
    if (positionInClip < 0 || positionInClip >= clipLen)
        return;

    // Calculate the range of samples we're rendering: [blockStart, blockEnd)
    const int64_t blockStart = positionInClip;
    const int64_t blockEnd = positionInClip + numSamples;

    // NO ScopedLock on audio thread

    // Iterate through MIDI events and schedule those that fall within this block
    for (int i = 0; i < sequence->getNumEvents(); ++i)
    {
        auto* event = sequence->getEventPointer(i);
        if (event == nullptr)
            continue;

        // Get event time in clip-relative samples
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
void Clip::processMidiClip(const juce::AudioSourceChannelInfo& bufferToFill, int64_t playheadSamples)
{
    juce::ignoreUnused(bufferToFill, playheadSamples);
    // Legacy path - MIDI clips don't produce audio directly
    // They need to be processed by instrument plugins
    bufferToFill.clearActiveBufferRegion();
}

// Legacy overload: uses internal transportPosition
void Clip::processMidiClip(const juce::AudioSourceChannelInfo& bufferToFill)
{
    processMidiClip(bufferToFill, transportPosition.load());
}

float Clip::calculateFadeMultiplier(int64_t positionInClip) const
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

void Clip::applyFadesSIMD(const juce::AudioSourceChannelInfo& bufferToFill, 
                          int64_t startPositionInClip, 
                          int numSamples)
{
    const int64_t fadeIn = fadeInLength.load();
    const int64_t fadeOut = fadeOutLength.load();
    const int64_t clipLen = clipLength.load();
    
    if (fadeIn <= 0 && fadeOut <= 0)
        return;

    for (int i = 0; i < numSamples; ++i)
    {
        const int64_t pos = startPositionInClip + i;
        float multiplier = 1.0f;

        if (fadeIn > 0 && pos < fadeIn)
            multiplier *= static_cast<float>(pos) / static_cast<float>(fadeIn);

        if (fadeOut > 0 && pos > clipLen - fadeOut)
        {
            const int64_t posInFadeOut = clipLen - pos;
            multiplier *= static_cast<float>(posInFadeOut) / static_cast<float>(fadeOut);
        }

        if (multiplier != 1.0f)
        {
            for (int ch = 0; ch < bufferToFill.buffer->getNumChannels(); ++ch)
                bufferToFill.buffer->getWritePointer(ch, bufferToFill.startSample)[i] *= multiplier;
        }
    }
}

} // namespace zenith

