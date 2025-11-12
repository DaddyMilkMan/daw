/**
 * @file Clip.cpp
 * @brief Clip loader implementation
 */

#include "Clip.h"

using namespace zenith;

//==============================================================================
ClipLoader::ClipLoader()
{
    // Register standard audio formats
    formatManager.registerBasicFormats();  // WAV, AIFF, FLAC, Ogg Vorbis
}

ClipLoader::~ClipLoader()
{
}

//==============================================================================
Clip ClipLoader::loadFromFile(const juce::File& file,
                               juce::int64 startSample,
                               double fadeInMs,
                               double fadeOutMs)
{
    // ⚠️ MESSAGE THREAD ONLY!
    //
    // This function:
    // - Opens file
    // - Decodes entire file into memory
    // - Allocates AudioBuffer
    //
    // NEVER call from AUDIO THREAD!

    Clip clip;

    // Validate file
    if (!file.existsAsFile())
    {
        DBG("ClipLoader: File not found: " + file.getFullPathName());
        return clip;  // Invalid clip
    }

    // Create reader
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));

    if (reader == nullptr)
    {
        DBG("ClipLoader: Failed to create reader for: " + file.getFullPathName());
        return clip;  // Invalid clip
    }

    // W13.1: Check sample rate mismatch (pitch shift prevention)
    const double fileSR = reader->sampleRate;
    if (engineSampleRate_ > 0.0 && std::abs(fileSR - engineSampleRate_) > 1e-3)
    {
        DBG("ClipLoader: Sample rate mismatch! File=" + juce::String(fileSR, 0)
            + " Hz, Engine=" + juce::String(engineSampleRate_, 0) + " Hz");
        DBG("  -> Rejecting file to prevent pitch shift: " + file.getFileName());
        return clip;  // Invalid clip
    }

    // Store sample rate for fade calculations
    currentSampleRate = reader->sampleRate;

    // Read entire file into buffer
    const int numChannels = static_cast<int>(reader->numChannels);
    const juce::int64 numSamples = reader->lengthInSamples;

    if (numSamples <= 0 || numChannels <= 0)
    {
        DBG("ClipLoader: Invalid audio file (0 samples or 0 channels)");
        return clip;  // Invalid clip
    }

    // Allocate PCM buffer
    auto pcmBuffer = std::make_shared<juce::AudioBuffer<float>>(numChannels, static_cast<int>(numSamples));

    // Read all samples
    if (!reader->read(pcmBuffer.get(), 0, static_cast<int>(numSamples), 0, true, true))
    {
        DBG("ClipLoader: Failed to read samples from file");
        return clip;  // Invalid clip
    }

    // Create clip
    clip.pcm = pcmBuffer;
    clip.startSample = startSample;
    clip.lengthSamples = numSamples;
    clip.srcOffset = 0;
    clip.gain = 1.0f;

    // Convert fade durations from milliseconds to samples
    if (fadeInMs > 0.0)
        clip.fadeInSamples = static_cast<int>(fadeInMs * currentSampleRate / 1000.0);

    if (fadeOutMs > 0.0)
        clip.fadeOutSamples = static_cast<int>(fadeOutMs * currentSampleRate / 1000.0);

    // W13.1: Clamp fades to prevent overlap on short clips
    const int maxFade = static_cast<int>(clip.lengthSamples / 2);
    if (clip.fadeInSamples > maxFade)
        clip.fadeInSamples = maxFade;
    if (clip.fadeOutSamples > maxFade)
        clip.fadeOutSamples = maxFade;

    clip.loop = false;

    DBG("ClipLoader: Loaded " + file.getFileName()
        + " (" + juce::String(numChannels) + " ch, "
        + juce::String(numSamples) + " samples @ "
        + juce::String(currentSampleRate, 0) + " Hz)");

    if (clip.fadeInSamples > 0)
        DBG("  Fade in: " + juce::String(clip.fadeInSamples) + " samples");

    if (clip.fadeOutSamples > 0)
        DBG("  Fade out: " + juce::String(clip.fadeOutSamples) + " samples");

    return clip;
}
