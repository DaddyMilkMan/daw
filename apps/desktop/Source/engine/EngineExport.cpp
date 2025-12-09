/**
 * @file EngineExport.cpp
 * @brief Audio export and offline rendering implementation
 */

#include "../include/Engine.h"
#include "../include/ProjectState.h"
#include "AudioFilePool.h"
#include "Track.h"
#include "Clip.h"
#include "MixerChannel.h"
#include "AuxBus.h"
#include "PluginHost.h"

namespace zenith {

//==============================================================================
// Offline Export Implementation
//==============================================================================

void Engine::prepareBuffersForOfflineRender(int blockSize, int numChannels)
{
    DBG("Engine: Preparing buffers for offline render - blockSize=" + juce::String(blockSize) +
        ", numChannels=" + juce::String(numChannels) +
        ", numTracks=" + juce::String(tracks_.size()));

    // Resize trackBuffers_ to match the number of tracks
    trackBuffers_.resize(tracks_.size());

    // Allocate each track buffer with the specified block size and channel count
    for (size_t i = 0; i < trackBuffers_.size(); ++i)
    {
        trackBuffers_[i].setSize(numChannels, blockSize, false, true, false);
        trackBuffers_[i].clear();
    }

    // Resize Aux Bus buffers
    auxBusBuffers_.resize(auxBuses_.size());
    for (size_t i = 0; i < auxBusBuffers_.size(); ++i) {
        auxBusBuffers_[i].setSize(numChannels, blockSize, false, true, false);
        auxBusBuffers_[i].clear();
    }

    DBG("Engine: Buffers prepared successfully");
}

void Engine::registerFormats()
{
    formatManager.registerBasicFormats(); 
    formatManager.registerFormat(new juce::FlacAudioFormat(), false);
    formatManager.registerFormat(new juce::OggVorbisAudioFormat(), false);
}

bool Engine::exportProjectToWav(const juce::File& outputFile,
                                double sampleRate,
                                int bitDepth,
                                double durationInSeconds)
{
    DBG("Engine: Starting WAV export to " + outputFile.getFullPathName());
    DBG("  Sample Rate: " + juce::String(sampleRate) + " Hz");
    DBG("  Bit Depth: " + juce::String(bitDepth));
    DBG("  Duration: " + juce::String(durationInSeconds) + " seconds");

    // Validate parameters
    if (sampleRate <= 0.0)
    {
        DBG("Engine: Error - Invalid sample rate");
        return false;
    }

    if (bitDepth != 16 && bitDepth != 24 && bitDepth != 32)
    {
        DBG("Engine: Error - Invalid bit depth (must be 16, 24, or 32)");
        return false;
    }

    // Auto-detect duration if not specified
    if (durationInSeconds <= 0.0)
    {
        durationInSeconds = 10.0;  // Default duration
        DBG("Engine: Auto-detected duration: " + juce::String(durationInSeconds) + " seconds");
    }

    // Calculate total samples
    const juce::int64 totalSamples = static_cast<juce::int64>(durationInSeconds * sampleRate);

    // Use 4096-sample blocks for efficient offline rendering
    constexpr int offlineBlockSize = 4096;
    const int numChannels = 2;  // Stereo output

    DBG("Engine: Using offline block size of " + juce::String(offlineBlockSize) + " samples");

    // CRITICAL: Prepare buffers for offline rendering BEFORE calling renderBlock
    prepareBuffersForOfflineRender(offlineBlockSize, numChannels);

    // Create WAV file writer
    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::AudioFormatWriter> writer;

    writer.reset(wavFormat.createWriterFor(
        new juce::FileOutputStream(outputFile),
        sampleRate,
        static_cast<unsigned int>(numChannels),
        bitDepth,
        {},
        0    // Default quality
    ));

    if (writer == nullptr)
    {
        DBG("Engine: Error - Failed to create WAV writer");
        return false;
    }

    // Create render buffer
    juce::AudioBuffer<float> renderBuffer(numChannels, offlineBlockSize);

    // Render loop
    juce::int64 samplesRendered = 0;

    while (samplesRendered < totalSamples)
    {
        // Calculate how many samples to render in this block
        const int samplesToRender = static_cast<int>(
            juce::jmin(static_cast<juce::int64>(offlineBlockSize),
                      totalSamples - samplesRendered));

        // Render this block
        renderAudioGraph(renderBuffer, samplesToRender, samplesRendered);

        // Write to file
        if (!writer->writeFromAudioSampleBuffer(renderBuffer, 0, samplesToRender))
        {
            DBG("Engine: Error - Failed to write audio data");
            return false;
        }

        samplesRendered += samplesToRender;

        // Log progress every second
        if (samplesRendered % static_cast<juce::int64>(sampleRate) == 0)
        {
            double progress = static_cast<double>(samplesRendered) / totalSamples * 100.0;
            DBG("Engine: Export progress: " + juce::String(progress, 1) + "%\n");
        }
    }

    // Flush and close writer
    writer.reset();

    DBG("Engine: Export complete - " + juce::String(samplesRendered) + " samples written");
    return true;
}

bool Engine::exportProject(const ExportOptions& options)
{
    DBG("Engine: Starting Advanced Export...");
    registerFormats(); 

    if (options.sampleRate <= 0) return false;
    
    if (options.bitDepth == 8 && options.format != ExportFormat::WAV)
    {
        DBG("Engine: 8-bit export is only supported for WAV format.");
        return false;
    }

    juce::AudioFormat* format = nullptr;
    switch (options.format) {
        case ExportFormat::WAV: format = formatManager.findFormatForFileExtension("wav"); break;
        case ExportFormat::FLAC: format = formatManager.findFormatForFileExtension("flac"); break;
        case ExportFormat::OGG: format = formatManager.findFormatForFileExtension("ogg"); break;
    }

    if (!format) return false;

    // Create file stream
    auto fileStream = std::make_unique<juce::FileOutputStream>(options.outputFile);
    if (fileStream->failedToOpen()) return false;

    std::unique_ptr<juce::AudioFormatWriter> writer(format->createWriterFor(
        fileStream.release(),
        options.sampleRate,
        2, // Stereo
        options.bitDepth,
        {},
        0   // Quality
    ));

    if (!writer) return false;

    const int blockSize = 4096;
    juce::AudioBuffer<float> renderBuffer(2, blockSize);
    prepareBuffersForOfflineRender(blockSize, 2);
    
    if (options.enableDither) dither.prepare(2);

    // Use auto-detect duration if not specified
    double duration = options.duration > 0 ? options.duration : autoDetectProjectDuration();
    DBG("Engine: Export duration: " + juce::String(duration, 2) + " seconds");
    juce::int64 totalSamples = static_cast<juce::int64>(options.sampleRate * duration);
    juce::int64 samplesWritten = 0;

    while (samplesWritten < totalSamples)
    {
        int numSamples = (int)juce::jmin((juce::int64)blockSize, totalSamples - samplesWritten);
        
        // Render Mix
        renderAudioGraph(renderBuffer, numSamples, samplesWritten);

        // Apply Dithering
        if (options.enableDither && options.bitDepth < 32)
        {
            dither.process(renderBuffer, options.bitDepth);
        }
        
        // Normalization
        if (options.normalize)
        {
             applyNormalization(renderBuffer, 1.0f, (float)options.normalizeDb);
        }

        if (!writer->writeFromAudioSampleBuffer(renderBuffer, 0, numSamples))
        {
            return false;
        }
        
        samplesWritten += numSamples;
    }

    return true;
}

void Engine::applyNormalization(juce::AudioBuffer<float>& buffer, float maxPeak, float targetDb)
{
    juce::ignoreUnused(maxPeak);
    float targetLinear = juce::Decibels::decibelsToGain(targetDb);
    float blockPeak = buffer.getMagnitude(0, buffer.getNumSamples());
    if (blockPeak > targetLinear)
    {
        float gain = targetLinear / blockPeak;
        buffer.applyGain(gain);
    }
}

double Engine::autoDetectProjectDuration() const
{
    double maxDuration = 0.0;
    const double sampleRate = currentSampleRate.load();
    
    if (sampleRate <= 0.0)
        return 10.0;  // Fallback
    
    // Scan all tracks for the latest clip end position
    for (const auto& track : tracks_)
    {
        if (track == nullptr)
            continue;
        
        for (int i = 0; i < track->getNumClips(); ++i)
        {
            const auto* clip = track->getClip(i);
            if (clip != nullptr)
            {
                // Get clip end position in samples and convert to seconds
                juce::int64 clipEnd = clip->getStartPosition() + clip->getLength();
                double endSeconds = static_cast<double>(clipEnd) / sampleRate;
                
                if (endSeconds > maxDuration)
                {
                    maxDuration = endSeconds;
                }
            }
        }
    }
    
    // Add a small tail (2 seconds) for reverb/delay tails
    if (maxDuration > 0.0)
    {
        maxDuration += 2.0;
    }
    else
    {
        maxDuration = 10.0;  // Default if no clips
    }
    
    return maxDuration;
}

} // namespace zenith
