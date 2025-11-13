/**
 * @file Track.cpp
 * @brief Track implementation with proper transport position handling
 */

#include "../include/Track.h"

//==============================================================================
// Clip Implementation
//==============================================================================

Track::Clip::Clip(const juce::File& audioFile,
                   double start,
                   double len,
                   int64_t srcOffset,
                   float clipGain,
                   int fadeIn,
                   int fadeOut)
    : file(audioFile),
      startTime(start),
      length(len),
      srcOffsetSamples(srcOffset),
      gain(clipGain),
      fadeInSamples(fadeIn),
      fadeOutSamples(fadeOut),
      transportPosition(0.0),
      currentReadPosition(0)
{
    // Validate and clamp parameters
    if (srcOffsetSamples < 0)
        srcOffsetSamples = 0;

    if (gain < 0.0f)
        gain = 0.0f;

    if (fadeInSamples < 0)
        fadeInSamples = 0;

    if (fadeOutSamples < 0)
        fadeOutSamples = 0;

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
            " (" + juce::String(reader->lengthInSamples) + " samples)" +
            " srcOffset=" + juce::String(srcOffsetSamples) +
            " gain=" + juce::String(gain, 2) +
            " fadeIn=" + juce::String(fadeInSamples) +
            " fadeOut=" + juce::String(fadeOutSamples));

        // Validate srcOffset doesn't exceed file length
        if (srcOffsetSamples >= reader->lengthInSamples)
        {
            DBG("  WARNING: srcOffset >= file length, clamping to 0");
            srcOffsetSamples = 0;
        }

        // Calculate effective clip length in samples
        const double sampleRate = reader->sampleRate;
        int64_t lengthInSamples = (length > 0.0)
            ? static_cast<int64_t>(length * sampleRate)
            : (reader->lengthInSamples - srcOffsetSamples);

        // Clamp length to not exceed available samples
        int64_t availableSamples = reader->lengthInSamples - srcOffsetSamples;
        if (lengthInSamples > availableSamples)
        {
            DBG("  WARNING: lengthSamples exceeds available samples, clamping");
            lengthInSamples = availableSamples;
        }

        // Validate fades don't exceed clip length
        if (fadeInSamples + fadeOutSamples > lengthInSamples)
        {
            DBG("  WARNING: fadeIn + fadeOut exceeds clip length, clamping");
            // Proportionally reduce both fades
            double scale = static_cast<double>(lengthInSamples) /
                           static_cast<double>(fadeInSamples + fadeOutSamples);
            fadeInSamples = static_cast<int>(fadeInSamples * scale);
            fadeOutSamples = static_cast<int>(fadeOutSamples * scale);
        }
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

    // Apply srcOffset: read from srcOffset + sampleOffsetIntoClip in the source file
    int64_t fileReadPosition = srcOffsetSamples + sampleOffsetIntoClip;

    // Calculate how many samples to read
    int numSamples = bufferToFill.numSamples;
    int64_t samplesAvailable = reader->lengthInSamples - fileReadPosition;

    if (samplesAvailable <= 0)
    {
        // Reached end of file
        return;
    }

    // Don't read more than available
    int samplesToRead = static_cast<int>(juce::jmin<int64_t>(numSamples, samplesAvailable));

    // Also don't read past the clip's length
    int64_t clipLengthInSamples = (length > 0.0)
        ? static_cast<int64_t>(length * sampleRate)
        : (reader->lengthInSamples - srcOffsetSamples);

    int64_t samplesRemainingInClip = clipLengthInSamples - sampleOffsetIntoClip;
    if (samplesRemainingInClip <= 0)
        return;

    samplesToRead = static_cast<int>(juce::jmin<int64_t>(samplesToRead, samplesRemainingInClip));

    // Read from file at correct position (including srcOffset)
    reader->read(bufferToFill.buffer,
                 bufferToFill.startSample,
                 samplesToRead,
                 fileReadPosition,  // Read from srcOffset + offsetIntoClip
                 true,  // Use left channel
                 true); // Use right channel

    // Apply clip gain
    if (gain != 1.0f)
    {
        bufferToFill.buffer->applyGain(bufferToFill.startSample,
                                       samplesToRead,
                                       gain);
    }

    // Apply fades (linear for v0.1, could be improved to equal-power later)
    if (fadeInSamples > 0 || fadeOutSamples > 0)
    {
        for (int channel = 0; channel < bufferToFill.buffer->getNumChannels(); ++channel)
        {
            float* channelData = bufferToFill.buffer->getWritePointer(channel, bufferToFill.startSample);

            for (int i = 0; i < samplesToRead; ++i)
            {
                int64_t samplePosInClip = sampleOffsetIntoClip + i;
                float fadeMult = 1.0f;

                // Fade in
                if (samplePosInClip < fadeInSamples)
                {
                    fadeMult *= static_cast<float>(samplePosInClip) / static_cast<float>(fadeInSamples);
                }

                // Fade out
                int64_t samplesFromEnd = clipLengthInSamples - samplePosInClip;
                if (samplesFromEnd < fadeOutSamples)
                {
                    fadeMult *= static_cast<float>(samplesFromEnd) / static_cast<float>(fadeOutSamples);
                }

                channelData[i] *= fadeMult;
            }
        }
    }

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

void Track::clearClips()
{
    // MESSAGE THREAD ONLY
    clips.clear();
    DBG("Track: Cleared all clips from '" + name + "'");
}
