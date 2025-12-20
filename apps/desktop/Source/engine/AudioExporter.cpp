#include "AudioExporter.h"
#include "Engine.h"
#include "ProjectState.h"
#include "AudioRenderer.h" // Required for audioRenderer_ access

namespace zenith {

juce::AudioFormat* AudioExporter::getFormat(Format format, juce::AudioFormatManager& manager)
{
    switch (format)
    {
        case Format::WAV:  return manager.findFormatForFileExtension("wav");
        case Format::AIFF: return manager.findFormatForFileExtension("aiff");
        case Format::FLAC: return manager.findFormatForFileExtension("flac");
        case Format::OGG:  return manager.findFormatForFileExtension("ogg");
        case Format::MP3:  return manager.findFormatForFileExtension("mp3");
        default:           return nullptr;
    }
}

void AudioExporter::exportProject(ProjectState& state, Engine& engine, 
                        const ExportSettings& settings,
                        std::function<void(float)> progressCallback)
{
    // 1. Setup Formats
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();
    formatManager.registerFormat(new juce::FlacAudioFormat(), false);
    formatManager.registerFormat(new juce::OggVorbisAudioFormat(), false);
    // Note: MP3 export requires LAME or OS codecs which might not be present

    juce::AudioFormat* format = getFormat(settings.format, formatManager);
    if (!format) {
        DBG("AudioExporter: Format not supported");
        return;
    }

    // 2. Determine Duration
    double duration = settings.endTime;
    if (duration < 0) {
        // Use Engine's auto-detect logic (friend access)
        duration = engine.autoDetectProjectDuration();
        if (settings.startTime > 0) duration -= settings.startTime;
    }
    
    // Safety minimum duration
    if (duration <= 0.001) duration = 4.0; 

    // 3. Prepare Engine
    double originalRate = engine.getSampleRate();
    int originalBlockSize = engine.getBufferSize();
    
    // Pause any playback? Engine assumes message thread safety for prepareTracks
    // Ideally we should ensure the audio device isn't calling back, but we follow 
    // the existing pattern of modifying engine state on message thread.
    
    // Prepare for offline rendering (usually larger block size)
    const int offlineBlockSize = 4096;
    engine.prepareTracks(offlineBlockSize, settings.sampleRate);
    if (engine.audioRenderer_) {
         engine.audioRenderer_->prepare(settings.sampleRate, offlineBlockSize, engine.tracks_.size(), engine.auxBuses_.size());
    }

    // 4. Setup Output
    juce::File outputFile = settings.outputFile;
    juce::File tempFile;
    
    // If normalizing, we write to a temp file first (always WAV for quality)
    // Then read it back, apply gain, and write to the final format
    if (settings.normalize) {
        tempFile = outputFile.getParentDirectory().getChildFile(outputFile.getFileNameWithoutExtension() + "_temp.wav");
        if (tempFile.exists()) tempFile.deleteFile();
        if (outputFile.exists()) outputFile.deleteFile();
        
        // Switch to WAV for the temp file
        format = formatManager.findFormatForFileExtension("wav");
    } else {
        if (outputFile.exists()) outputFile.deleteFile();
    }
    
    juce::File& writeToFile = settings.normalize ? tempFile : outputFile;

    std::unique_ptr<juce::AudioFormatWriter> writer(format->createWriterFor(
        new juce::FileOutputStream(writeToFile), 
        settings.sampleRate, 
        2, // Stereo
        settings.bitDepth, 
        {}, 0));

    if (writer) {
        juce::AudioBuffer<float> buffer(2, offlineBlockSize);
        int64_t totalSamples = (int64_t)(duration * settings.sampleRate);
        int64_t samplesWritten = 0;
        int64_t startSample = (int64_t)(settings.startTime * settings.sampleRate);

        float maxPeak = 0.0f;

        // --- PASS 1: RENDER ---
        while (samplesWritten < totalSamples) {
            int numSamples = (int)std::min((int64_t)offlineBlockSize, totalSamples - samplesWritten);
            
            // Render block via Engine
            engine.renderOfflineBlock(buffer, numSamples, startSample + samplesWritten);
            
            // Track peak for normalization
            if (settings.normalize) {
                float peak = buffer.getMagnitude(0, numSamples);
                if (peak > maxPeak) maxPeak = peak;
            }
            
            writer->writeFromAudioSampleBuffer(buffer, 0, numSamples);
            samplesWritten += numSamples;
            
            if (progressCallback) {
                float progress = (float)samplesWritten / totalSamples;
                if (settings.normalize) progress *= 0.5f; // First pass is 0-50%
                progressCallback(progress);
            }
        }
        
        writer.reset(); // Flush and close file

        // --- PASS 2: NORMALIZE (If enabled) ---
        if (settings.normalize && tempFile.existsAsFile()) {
            float targetLevel = 1.0f; // 0dB
            // Avoid extreme gain if silent
            float gain = (maxPeak > 0.0001f) ? (targetLevel / maxPeak) : 1.0f;
            
            // If gain is effectively 1.0 and format is WAV, we could just rename, 
            // but we might be converting to MP3/FLAC, so we must re-encode.
            
            std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(tempFile));
            
            // Get final format writer
            juce::AudioFormat* finalFormat = getFormat(settings.format, formatManager);
            // Fallback to WAV if final format not found
            if (!finalFormat) finalFormat = formatManager.findFormatForFileExtension("wav");

            std::unique_ptr<juce::AudioFormatWriter> finalWriter(finalFormat->createWriterFor(
                new juce::FileOutputStream(settings.outputFile), 
                settings.sampleRate, 
                2, 
                settings.bitDepth, 
                {}, 0));
                
            if (reader && finalWriter) {
                int64_t samplesProcessed = 0;
                int64_t total = reader->lengthInSamples;
                
                while (samplesProcessed < total) {
                    int numSamples = (int)std::min((int64_t)offlineBlockSize, total - samplesProcessed);
                    buffer.clear();
                    
                    // Read from temp
                    reader->read(&buffer, 0, numSamples, samplesProcessed, true, true);
                    
                    // Apply normalization gain
                    buffer.applyGain(gain);
                    
                    // Write to final
                    finalWriter->writeFromAudioSampleBuffer(buffer, 0, numSamples);
                    
                    samplesProcessed += numSamples;
                    
                     if (progressCallback) {
                        float progress = 0.5f + ((float)samplesProcessed / total) * 0.5f; // 50-100%
                        progressCallback(progress);
                    }
                }
            }
            
            // Clean up
            reader.reset();
            finalWriter.reset();
            tempFile.deleteFile();
        }
    }
    
    // 5. Restore Engine State
    engine.prepareTracks(originalBlockSize, originalRate);
     if (engine.audioRenderer_) {
         engine.audioRenderer_->prepare(originalRate, originalBlockSize, engine.tracks_.size(), engine.auxBuses_.size());
    }
}

} // namespace zenith
