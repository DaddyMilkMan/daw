/**
 * @file Track.cpp
 * @brief Track implementation with proper transport position handling
 */

#include "../include/Track.h"

//==============================================================================
// Clip Implementation
//==============================================================================

Track::Clip::Clip(const juce::File& audioFile, double start, double len)
    : file(audioFile),
      startTime(start),
      length(len),
      transportPosition(0.0),
      currentReadPosition(0)
{
    // Register audio formats
    formatManager.registerBasicFormats();

    // Load audio file
    reader.reset(formatManager.createReaderFor(audioFile));

    if (reader == nullptr)
    {
        DBG("Track::Clip: Failed to load audio file: " + audioFile.getFullPathName());
    }
    else
    {
        DBG("Track::Clip: Loaded audio file: " + audioFile.getFullPathName() +
            " (" + juce::String(reader->lengthInSamples) + " samples)");
    }
}

void Track::Clip::setTransportPosition(double positionInSeconds)
{
    // **CRITICAL FIX:** Update transport position
    // This MUST be called before getNextAudioBlock()
    transportPosition.store(positionInSeconds);
}

double Track::Clip::getTransportPosition() const
{
    return transportPosition.load();
}

bool Track::Clip::isActive() const
{
    // **CRITICAL:** Use transport position to determine if clip is active
    double currentPos = transportPosition.load();
    double endTime = startTime + length;

    return (currentPos >= startTime && currentPos < endTime);
}

void Track::Clip::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    // ⚠️ REAL-TIME AUDIO THREAD
    // - No heap allocations
    // - No logging or String creation
    // - No locks or UI calls

    if (reader == nullptr)
        return;

    // **CRITICAL FIX:** Use transport position to determine activity
    double currentPos = transportPosition.load();
    double endTime = startTime + length;

    // Check if this clip should be playing at current transport position
    if (currentPos < startTime || currentPos >= endTime)
    {
        // Clip is not active at this position
        // **CRITICAL FIX:** Don't clear buffer - would erase other clips' audio!
        return;
    }

    // **CRITICAL FIX:** Calculate correct read offset based on transport position
    // How far into the clip are we? (in seconds)
    double offsetIntoClip = currentPos - startTime;

    // Convert to samples
    double sampleRate = reader->sampleRate;
    int64_t sampleOffsetIntoClip = static_cast<int64_t>(offsetIntoClip * sampleRate);

    // Calculate how many samples to read
    int numSamples = bufferToFill.numSamples;
    int64_t samplesAvailable = reader->lengthInSamples - sampleOffsetIntoClip;

    if (samplesAvailable <= 0)
    {
        // Reached end of clip
        return;
    }

    // Don't read more than available
    int samplesToRead = static_cast<int>(juce::jmin<int64_t>(numSamples, samplesAvailable));

    // Read from file at correct position
    reader->read(bufferToFill.buffer,
                 bufferToFill.startSample,
                 samplesToRead,
                 sampleOffsetIntoClip,  // **CRITICAL:** Read from correct position
                 true,  // Use left channel
                 true); // Use right channel

    // RT-SAFETY: No logging on audio thread!
    // For debugging, use jassert or set atomic flags and poll from message thread
}

//==============================================================================
// Track Implementation
//==============================================================================

Track::Track(const juce::String& trackName, bool isAudioTrack)
    : name(trackName),
      audioTrack(isAudioTrack)
{
    DBG("Track: Created track '" + name + "' (" +
        (audioTrack ? "Audio" : "MIDI") + ")");
}

Track::~Track()
{
    DBG("Track: Destroyed track '" + name + "'");
}

//==============================================================================
// Playback
//==============================================================================

void Track::processAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill,
                               double transportPosition)
{
    // ⚠️ REAL-TIME AUDIO THREAD
    // - No heap allocations
    // - No logging or String creation
    // - No locks or UI calls

    // Clear buffer
    bufferToFill.clearActiveBufferRegion();

    // Check mute
    if (muted.load())
        return;

    // **CRITICAL FIX:** Update transport position for ALL clips before processing
    // This ensures each clip knows where the playhead is
    for (auto& clip : clips)
    {
        // **STEP 1:** Set transport position (was missing before!)
        clip->setTransportPosition(transportPosition);

        // **STEP 2:** Get audio block (now uses correct position)
        clip->getNextAudioBlock(bufferToFill);
    }

    // Apply volume
    float volumeLevel = volume.load();
    if (volumeLevel != 1.0f)
    {
        bufferToFill.buffer->applyGain(
            bufferToFill.startSample,
            bufferToFill.numSamples,
            volumeLevel);
    }

    // Apply pan (simple stereo panning)
    float panValue = pan.load();  // -1.0 (left) to +1.0 (right)

    if (panValue != 0.0f && bufferToFill.buffer->getNumChannels() >= 2)
    {
        // Calculate left/right gains using constant-power panning
        float leftGain = std::cos((panValue + 1.0f) * juce::MathConstants<float>::pi / 4.0f);
        float rightGain = std::sin((panValue + 1.0f) * juce::MathConstants<float>::pi / 4.0f);

        bufferToFill.buffer->applyGain(0,
                                       bufferToFill.startSample,
                                       bufferToFill.numSamples,
                                       leftGain);

        bufferToFill.buffer->applyGain(1,
                                       bufferToFill.startSample,
                                       bufferToFill.numSamples,
                                       rightGain);
    }
}

//==============================================================================
// Clip Management
//==============================================================================

Track::Clip* Track::addClip(const juce::File& audioFile, double startTime, double length)
{
    auto clip = std::make_unique<Clip>(audioFile, startTime, length);
    auto* clipPtr = clip.get();

    clips.push_back(std::move(clip));

    DBG("Track: Added clip '" + audioFile.getFileName() +
        "' at " + juce::String(startTime) + "s");

    return clipPtr;
}

void Track::removeClip(Clip* clip)
{
    clips.erase(std::remove_if(clips.begin(), clips.end(),
                               [clip](const std::unique_ptr<Clip>& c) {
                                   return c.get() == clip;
                               }),
                clips.end());

    DBG("Track: Removed clip");
}
