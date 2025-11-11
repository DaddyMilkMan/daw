/*
  ==============================================================================

    AudioEngine.cpp
    Implementation of core audio engine

  ==============================================================================
*/

#include "AudioEngine.h"

//==============================================================================
AudioEngine::AudioEngine()
{
    // Create master channel
    masterChannel = std::make_unique<MixerChannel>("Master", true);
}

AudioEngine::~AudioEngine()
{
    // Ensure we're stopped before destruction
    playing = false;
    recording = false;

    // Clear all tracks
    const juce::ScopedLock sl (tracksLock);
    tracks.clear();
}

//==============================================================================
// AudioSource implementation
void AudioEngine::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    currentSampleRate = sampleRate;
    blockSize = samplesPerBlockExpected;

    DBG ("AudioEngine::prepareToPlay - SR: " + juce::String (sampleRate)
         + " Hz, Block: " + juce::String (samplesPerBlockExpected));

    // Prepare all tracks
    const juce::ScopedLock sl (tracksLock);
    for (auto* track : tracks)
        track->prepareToPlay (samplesPerBlockExpected, sampleRate);

    // Prepare master channel
    masterChannel->prepareToPlay (samplesPerBlockExpected, sampleRate);
}

void AudioEngine::releaseResources()
{
    const juce::ScopedLock sl (tracksLock);
    for (auto* track : tracks)
        track->releaseResources();

    masterChannel->releaseResources();
}

void AudioEngine::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    auto startTime = juce::Time::getMillisecondCounterHiRes();

    // Clear output buffer
    bufferToFill.clearActiveBufferRegion();

    // If not playing, return silence
    if (!playing.load())
        return;

    // Create a temporary buffer for mixing
    juce::AudioBuffer<float> mixBuffer (bufferToFill.buffer->getNumChannels(),
                                        bufferToFill.numSamples);
    mixBuffer.clear();

    // Process all tracks and mix them together
    {
        const juce::ScopedLock sl (tracksLock);

        for (auto* track : tracks)
        {
            if (track->isEnabled() && !track->isMuted())
            {
                // Create a temp buffer for this track
                juce::AudioBuffer<float> trackBuffer (mixBuffer.getNumChannels(),
                                                      bufferToFill.numSamples);
                trackBuffer.clear();

                juce::AudioSourceChannelInfo trackInfo (&trackBuffer, 0, bufferToFill.numSamples);
                track->getNextAudioBlock (trackInfo);

                // Add track output to mix
                for (int ch = 0; ch < mixBuffer.getNumChannels(); ++ch)
                    mixBuffer.addFrom (ch, 0, trackBuffer, ch, 0, bufferToFill.numSamples);
            }
        }
    }

    // Apply master processing
    applyMasterProcessing (mixBuffer);

    // Copy mixed buffer to output
    for (int ch = 0; ch < bufferToFill.buffer->getNumChannels(); ++ch)
    {
        bufferToFill.buffer->copyFrom (ch,
                                       bufferToFill.startSample,
                                       mixBuffer,
                                       ch,
                                       0,
                                       bufferToFill.numSamples);
    }

    // Advance playhead
    advancePlayhead (bufferToFill.numSamples);

    // Update CPU usage
    auto endTime = juce::Time::getMillisecondCounterHiRes();
    updateCpuUsage (endTime - startTime);

    totalSamplesProcessed += bufferToFill.numSamples;
}

//==============================================================================
// Transport control
void AudioEngine::play()
{
    playing = true;
    DBG ("Transport: PLAY");
}

void AudioEngine::pause()
{
    playing = false;
    DBG ("Transport: PAUSE");
}

void AudioEngine::stop()
{
    playing = false;
    playheadPosition = 0.0;
    DBG ("Transport: STOP - Playhead reset to 0");
}

void AudioEngine::setLooping (bool shouldLoop)
{
    looping = shouldLoop;
}

//==============================================================================
// Tempo and timing
void AudioEngine::setTempo (double newTempo)
{
    if (newTempo >= 20.0 && newTempo <= 999.0)
    {
        tempo = newTempo;
        DBG ("Tempo set to: " + juce::String (newTempo) + " BPM");
    }
}

void AudioEngine::setTimeSignature (int numerator, int denominator)
{
    timeSignatureNumerator = numerator;
    timeSignatureDenominator = denominator;
    DBG ("Time signature: " + juce::String (numerator) + "/" + juce::String (denominator));
}

//==============================================================================
// Track management
Track* AudioEngine::addTrack (const juce::String& name, Track::Type type)
{
    const juce::ScopedLock sl (tracksLock);

    auto* track = tracks.add (new Track (name, type));
    track->prepareToPlay (blockSize, currentSampleRate);

    DBG ("Track added: " + name + " (" + Track::getTypeString (type) + ")");

    return track;
}

void AudioEngine::removeTrack (int trackIndex)
{
    const juce::ScopedLock sl (tracksLock);

    if (juce::isPositiveAndBelow (trackIndex, tracks.size()))
    {
        tracks.remove (trackIndex);
        DBG ("Track removed at index: " + juce::String (trackIndex));
    }
}

void AudioEngine::clearAllTracks()
{
    const juce::ScopedLock sl (tracksLock);
    tracks.clear();
    DBG ("All tracks cleared");
}

//==============================================================================
// Master controls
void AudioEngine::setMasterVolume (float volume)
{
    masterVolume = juce::jlimit (0.0f, 1.0f, volume);
}

void AudioEngine::setMasterPan (float pan)
{
    masterPan = juce::jlimit (-1.0f, 1.0f, pan);
}

//==============================================================================
// Playback position
void AudioEngine::setPlayheadPosition (double newPosition)
{
    playheadPosition = juce::jmax (0.0, newPosition);
}

//==============================================================================
// Private helper methods
void AudioEngine::advancePlayhead (int numSamples)
{
    if (!playing.load())
        return;

    // Calculate time advancement
    double secondsPerSample = 1.0 / currentSampleRate;
    double timeAdvancement = numSamples * secondsPerSample;

    // Calculate beats per second
    double beatsPerSecond = tempo.load() / 60.0;
    double beatsAdvancement = timeAdvancement * beatsPerSecond;

    // Update playhead position (in beats)
    double currentPos = playheadPosition.load();
    playheadPosition = currentPos + beatsAdvancement;

    // Handle looping if enabled
    if (looping.load())
    {
        // TODO: Implement loop points
        // For now, just keep playing forward
    }
}

void AudioEngine::applyMasterProcessing (juce::AudioBuffer<float>& buffer)
{
    // Apply master volume
    buffer.applyGain (masterVolume.load());

    // Apply master panning
    applyMasterPanning (buffer);

    // Apply any master effects/processing here
    // TODO: Add master compressor, limiter, etc.
}

void AudioEngine::applyMasterPanning (juce::AudioBuffer<float>& buffer)
{
    if (buffer.getNumChannels() < 2)
        return;  // Only works for stereo

    float panValue = masterPan.load();

    if (panValue == 0.0f)
        return;  // No panning needed

    // Constant power panning
    float leftGain = std::cos ((panValue + 1.0f) * juce::MathConstants<float>::pi / 4.0f);
    float rightGain = std::sin ((panValue + 1.0f) * juce::MathConstants<float>::pi / 4.0f);

    buffer.applyGain (0, 0, buffer.getNumSamples(), leftGain);
    buffer.applyGain (1, 0, buffer.getNumSamples(), rightGain);
}

void AudioEngine::updateCpuUsage (double processingTimeMs)
{
    // Calculate expected time for this block
    double expectedTimeMs = (blockSize / currentSampleRate) * 1000.0;

    // CPU usage as percentage
    double usage = (processingTimeMs / expectedTimeMs) * 100.0;

    // Smooth the reading with a simple moving average
    double currentUsage = cpuUsage.load();
    cpuUsage = currentUsage * 0.95 + usage * 0.05;  // 95% old, 5% new
}

//==============================================================================
// ChangeListener implementation
void AudioEngine::changeListenerCallback (juce::ChangeBroadcaster* source)
{
    // Handle state changes from tracks or other sources
}
