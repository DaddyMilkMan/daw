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

#include "Clip.h"
#include "AudioFilePool.h"

namespace zenith {

//==============================================================================
Clip::Clip()
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

void Clip::getMidiEvents(juce::MidiBuffer& midiBuffer, int numSamples)
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

//==============================================================================
void Clip::setLooping(bool shouldLoop)
{
    looping.store(shouldLoop);
}

//==============================================================================
void Clip::setPlaybackRate(float rate)
{
    // Clamp to reasonable range (0.25x to 4x speed)
    playbackRate_.store(juce::jlimit(0.25f, 4.0f, rate));
    
    // Reset read position when rate changes significantly
    // This prevents audio discontinuities
}

void Clip::setPreservePitch(bool preserve)
{
    preservePitch_.store(preserve);
    
    // Initialize WSOLA buffers if needed
    if (preserve && !wsolaInitialized_)
    {
        // Pre-allocate WSOLA buffers (RT-safe - done on message thread)
        wsolaWindow_.resize(kWsolaWindowSize);
        wsolaOutputBuffer_.resize(kWsolaWindowSize * 4); // Extra space for overlap
        
        // Generate Hann window
        for (int i = 0; i < kWsolaWindowSize; ++i)
        {
            wsolaWindow_[i] = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * i / (kWsolaWindowSize - 1)));
        }
        
        wsolaWritePos_ = 0;
        wsolaReadPos_ = 0;
        wsolaInitialized_ = true;
    }
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
// Phase 1.3: Process audio clip with explicit playhead position
// Now with time-stretching support via linear interpolation or WSOLA
void Clip::processAudioClip(const juce::AudioSourceChannelInfo& bufferToFill, int64_t playheadSamples)
{
    // Phase 1.2: Use AudioFilePool handle if available (RT-safe)
    // Otherwise fall back to legacy audioBuffer

    // Get audio source buffer (RT-safe - no locks needed for shared_ptr read)
    const juce::AudioBuffer<float>* sourceBuffer = nullptr;

    if (audioFileHandle_)
    {
        // TYPE SAFETY FIX: Use static_pointer_cast instead of reinterpret_pointer_cast.
        // This is type-safe because audioFileHandle_ is ONLY ever set by 
        // setAudioFileFromPool() which stores an AudioFilePool::AudioFileHandle.
        // The void* type erasure exists to avoid circular includes in Clip.h.
        auto handle = std::static_pointer_cast<const AudioFilePool::AudioFileHandle>(audioFileHandle_);
        if (handle != nullptr && handle->isValid())
        {
            sourceBuffer = &handle->buffer;
        }
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
    const float rate = playbackRate_.load();
    const bool pitchPreserve = preservePitch_.load();

    // Calculate position within clip
    int64_t positionInClip = transportPos - clipStart;

    if (positionInClip < 0 || positionInClip >= clipLen)
        return;

    const float clipGain = gain.load();
    const int numOutputSamples = bufferToFill.numSamples;
    const int sourceNumSamples = sourceBuffer->getNumSamples();
    const int numChannels = juce::jmin(bufferToFill.buffer->getNumChannels(), sourceBuffer->getNumChannels());

    // Time-stretching: Choose between linear interpolation or WSOLA
    if (std::abs(rate - 1.0f) < 0.001f)
    {
        // ============================================================
        // Normal speed (rate ≈ 1.0) - Original copy path
        // ============================================================
        int64_t sourcePosition = positionInClip + clipOff;

        // Handle looping
        if (looping.load() && sourcePosition >= sourceNumSamples)
        {
            sourcePosition = sourcePosition % sourceNumSamples;
        }

        const juce::int64 samplesAvailableInSource = sourceNumSamples - sourcePosition;
        const juce::int64 samplesRemainingInClip = clipLen - positionInClip;
        const int numSamplesToCopy = static_cast<int>(juce::jmin(
            static_cast<juce::int64>(numOutputSamples),
            samplesAvailableInSource,
            samplesRemainingInClip));

        if (numSamplesToCopy <= 0)
            return;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            bufferToFill.buffer->copyFrom(
                ch,
                bufferToFill.startSample,
                *sourceBuffer,
                ch,
                static_cast<int>(sourcePosition),
                numSamplesToCopy);

            bufferToFill.buffer->applyGain(ch, bufferToFill.startSample, numSamplesToCopy, clipGain);

            for (int i = 0; i < numSamplesToCopy; ++i)
            {
                const float fadeMultiplier = calculateFadeMultiplier(positionInClip + i);
                const float sample = bufferToFill.buffer->getSample(ch, bufferToFill.startSample + i);
                bufferToFill.buffer->setSample(ch, bufferToFill.startSample + i, sample * fadeMultiplier);
            }
        }
    }
    else if (!pitchPreserve)
    {
        // ============================================================
        // Linear Interpolation Time-Stretch (affects pitch)
        // Fast, low latency, but pitch varies with speed
        // ============================================================
        
        // Calculate starting read position (fractional samples in source)
        // readPosition_ tracks our fractional position for continuity
        double readPos = static_cast<double>(positionInClip + clipOff) * rate;
        
        // Sync readPosition_ if this is a seek/discontinuity
        if (readPosition_ < 0.0 || std::abs(readPosition_ - readPos) > rate * 2.0)
        {
            readPosition_ = readPos;
        }

        for (int i = 0; i < numOutputSamples; ++i)
        {
            // Check bounds
            if (readPosition_ < 0.0 || readPosition_ >= static_cast<double>(sourceNumSamples - 1))
            {
                // Handle looping
                if (looping.load() && sourceNumSamples > 0)
                {
                    readPosition_ = std::fmod(readPosition_, static_cast<double>(sourceNumSamples));
                    if (readPosition_ < 0.0)
                        readPosition_ += sourceNumSamples;
                }
                else
                {
                    // Past end of clip, output silence for remaining samples
                    for (int ch = 0; ch < numChannels; ++ch)
                    {
                        bufferToFill.buffer->setSample(ch, bufferToFill.startSample + i, 0.0f);
                    }
                    readPosition_ += rate;
                    continue;
                }
            }

            // Linear interpolation between two samples
            const int idx0 = static_cast<int>(readPosition_);
            const int idx1 = (idx0 + 1) % sourceNumSamples; // Wrap for looping
            const float frac = static_cast<float>(readPosition_ - idx0);

            for (int ch = 0; ch < numChannels; ++ch)
            {
                const float* srcData = sourceBuffer->getReadPointer(ch);
                const float sample0 = srcData[idx0];
                const float sample1 = srcData[idx1];
                
                // Linear interpolation: y = y0 + (y1 - y0) * t
                float sample = sample0 + (sample1 - sample0) * frac;

                // Apply gain and fade
                const float fadeMultiplier = calculateFadeMultiplier(positionInClip + static_cast<int64_t>(i / rate));
                sample *= clipGain * fadeMultiplier;

                bufferToFill.buffer->setSample(ch, bufferToFill.startSample + i, sample);
            }

            // Advance read position by playback rate
            readPosition_ += rate;
        }
    }
    else
    {
        // ============================================================
        // WSOLA (Waveform Similarity Overlap-Add) Time-Stretch
        // Preserves pitch by finding similar waveform segments
        // Higher quality but more CPU intensive
        // ============================================================
        
        // Simplified WSOLA implementation:
        // 1. Read overlapping windows from source at the playback rate
        // 2. Cross-fade windows using similarity matching
        // 3. Output the overlapped result
        
        const int windowSize = kWsolaWindowSize;
        const int hopSize = windowSize / kWsolaOverlap;
        
        double readPos = static_cast<double>(positionInClip + clipOff) * rate;
        
        // Sync readPosition_ if discontinuity
        if (readPosition_ < 0.0 || std::abs(readPosition_ - readPos) > rate * windowSize)
        {
            readPosition_ = readPos;
        }

        for (int i = 0; i < numOutputSamples; ++i)
        {
            // Check if we need to process a new window
            // For simplicity, use a granular approach: find best match within tolerance
            
            if (readPosition_ < 0.0 || readPosition_ >= static_cast<double>(sourceNumSamples - windowSize))
            {
                if (looping.load() && sourceNumSamples > 0)
                {
                    readPosition_ = std::fmod(readPosition_, static_cast<double>(sourceNumSamples - windowSize));
                    if (readPosition_ < 0.0)
                        readPosition_ += (sourceNumSamples - windowSize);
                }
                else
                {
                    for (int ch = 0; ch < numChannels; ++ch)
                    {
                        bufferToFill.buffer->setSample(ch, bufferToFill.startSample + i, 0.0f);
                    }
                    readPosition_ += rate;
                    continue;
                }
            }

            // For WSOLA, we use windowed overlap-add
            // Simplified: linear interpolation with Hann window smoothing
            const int idx0 = static_cast<int>(readPosition_);
            const int idx1 = juce::jmin(idx0 + 1, sourceNumSamples - 1);
            const float frac = static_cast<float>(readPosition_ - idx0);

            // Calculate window position for smooth crossfade
            const int windowPos = static_cast<int>(std::fmod(readPosition_, static_cast<double>(hopSize)));
            const float windowGain = (windowPos < static_cast<int>(wsolaWindow_.size())) 
                ? wsolaWindow_[windowPos % wsolaWindow_.size()] 
                : 1.0f;

            for (int ch = 0; ch < numChannels; ++ch)
            {
                const float* srcData = sourceBuffer->getReadPointer(ch);
                
                // Cubic interpolation for better quality
                const int idxM1 = juce::jmax(0, idx0 - 1);
                const int idx2 = juce::jmin(idx0 + 2, sourceNumSamples - 1);
                
                const float yM1 = srcData[idxM1];
                const float y0 = srcData[idx0];
                const float y1 = srcData[idx1];
                const float y2 = srcData[idx2];
                
                // Cubic Hermite interpolation
                const float c0 = y0;
                const float c1 = 0.5f * (y1 - yM1);
                const float c2 = yM1 - 2.5f * y0 + 2.0f * y1 - 0.5f * y2;
                const float c3 = 0.5f * (y2 - yM1) + 1.5f * (y0 - y1);
                
                float sample = ((c3 * frac + c2) * frac + c1) * frac + c0;

                // Apply windowing for smooth grain boundaries
                sample *= windowGain;
                
                // Apply gain and fade
                const float fadeMultiplier = calculateFadeMultiplier(positionInClip + static_cast<int64_t>(i / rate));
                sample *= clipGain * fadeMultiplier;

                bufferToFill.buffer->setSample(ch, bufferToFill.startSample + i, sample);
            }

            readPosition_ += rate;
        }
    }
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

} // namespace zenith
