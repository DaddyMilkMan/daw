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
      transportPosition(0.0)
{
    // MESSAGE THREAD ONLY - decode entire file into PCM buffer

    // Validate and clamp parameters
    if (srcOffsetSamples < 0)
        srcOffsetSamples = 0;

    if (gain < 0.0f)
        gain = 0.0f;

    if (fadeInSamples < 0)
        fadeInSamples = 0;

    if (fadeOutSamples < 0)
        fadeOutSamples = 0;

    // Register audio formats and create reader
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(audioFile));

    if (reader == nullptr)
    {
        DBG("Track::Clip: Failed to load audio file: " + audioFile.getFullPathName());
        return;
    }

    DBG("Track::Clip: Decoding audio file: " + audioFile.getFullPathName() +
        " (" + juce::String(reader->lengthInSamples) + " samples)" +
        " srcOffset=" + juce::String(srcOffsetSamples) +
        " gain=" + juce::String(gain, 2) +
        " fadeIn=" + juce::String(fadeInSamples) +
        " fadeOut=" + juce::String(fadeOutSamples));

    // Store sample rate
    pcmSampleRate = reader->sampleRate;

    // Validate srcOffset doesn't exceed file length
    if (srcOffsetSamples >= reader->lengthInSamples)
    {
        DBG("  WARNING: srcOffset >= file length, clamping to 0");
        srcOffsetSamples = 0;
    }

    // Calculate effective clip length in samples
    int64_t lengthInSamples = (length > 0.0)
        ? static_cast<int64_t>(length * pcmSampleRate)
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

    // **CRITICAL: Pre-decode entire file into PCM buffer**
    // This happens on MESSAGE THREAD, NOT on audio thread
    const int numChannels = static_cast<int>(reader->numChannels);
    const int numSamples = static_cast<int>(reader->lengthInSamples);

    pcm = std::make_shared<juce::AudioBuffer<float>>(numChannels, numSamples);

    // Decode entire file in one go
    if (!reader->read(pcm.get(), 0, numSamples, 0, true, true))
    {
        DBG("  ERROR: Failed to decode audio file into PCM buffer");
        pcm.reset();
        return;
    }

    DBG("  SUCCESS: Decoded " + juce::String(numSamples) + " samples into PCM buffer");
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
    // - No file I/O (use pre-decoded PCM only!)

    // Check if PCM buffer is valid
    if (pcm == nullptr || pcmSampleRate <= 0.0)
        return;

    // Use transport position to determine activity
    double currentPos = transportPosition.load();
    double endTime = startTime + length;

    // Check if this clip should be playing at current transport position
    if (currentPos < startTime || currentPos >= endTime)
    {
        // Clip is not active at this position
        // Don't clear buffer - would erase other clips' audio!
        return;
    }

    // Calculate position within clip (samples from clip start)
    double offsetIntoClip = currentPos - startTime;
    int64_t clipPlayPos = static_cast<int64_t>(offsetIntoClip * pcmSampleRate);

    // Calculate effective clip length
    int64_t clipLengthInSamples = (length > 0.0)
        ? static_cast<int64_t>(length * pcmSampleRate)
        : (pcm->getNumSamples() - srcOffsetSamples);

    // Check if we're past the end of the clip
    if (clipPlayPos >= clipLengthInSamples)
        return;

    // Apply srcOffset: index into PCM buffer
    int64_t pcmStart = srcOffsetSamples + clipPlayPos;

    // Clamp to PCM buffer bounds
    if (pcmStart >= pcm->getNumSamples())
        return;

    // Calculate how many samples to process
    int samplesToProcess = bufferToFill.numSamples;

    // Don't read past end of clip
    int64_t samplesRemaining = clipLengthInSamples - clipPlayPos;
    samplesToProcess = static_cast<int>(juce::jmin<int64_t>(samplesToProcess, samplesRemaining));

    // Don't read past end of PCM buffer
    int64_t pcmSamplesAvailable = pcm->getNumSamples() - pcmStart;
    samplesToProcess = static_cast<int>(juce::jmin<int64_t>(samplesToProcess, pcmSamplesAvailable));

    if (samplesToProcess <= 0)
        return;

    // Process each channel (mixing into output buffer)
    const int numChannels = juce::jmin(pcm->getNumChannels(), bufferToFill.buffer->getNumChannels());

    for (int ch = 0; ch < numChannels; ++ch)
    {
        const float* src = pcm->getReadPointer(ch, static_cast<int>(pcmStart));
        float* dst = bufferToFill.buffer->getWritePointer(ch, bufferToFill.startSample);

        // Process each sample with gain and fades
        for (int i = 0; i < samplesToProcess; ++i)
        {
            int64_t playPos = clipPlayPos + i;
            int64_t fromEnd = clipLengthInSamples - playPos;

            // Calculate fade multiplier
            float fadeMult = 1.0f;

            // Fade in from start
            if (fadeInSamples > 0 && playPos < fadeInSamples)
                fadeMult *= static_cast<float>(playPos) / static_cast<float>(fadeInSamples);

            // Fade out to end
            if (fadeOutSamples > 0 && fromEnd < fadeOutSamples)
                fadeMult *= static_cast<float>(fromEnd) / static_cast<float>(fadeOutSamples);

            // Apply gain and fade, then mix into output
            dst[i] += src[i] * gain * fadeMult;
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
