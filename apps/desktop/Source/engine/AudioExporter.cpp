/*
  ==============================================================================

    AudioExporter.cpp
    Created: 2025-12-23
    Author:  Zenith DAW

  ==============================================================================
*/

#include "AudioExporter.h"
#include "Engine.h"
#include "AudioRenderer.h"
#include "Track.h"
#include "AuxBus.h"
#include "ProjectState.h"
#include "../dsp/Dither.h"

namespace zenith {

AudioExporter::AudioExporter(Engine& engine) : engine_(engine) {}

AudioExporter::~AudioExporter() { cancel(); }

void AudioExporter::cancel() { shouldCancel_.store(true); }

bool AudioExporter::renderProject(const ExportSettings& settings,
                                 std::function<void(float, const juce::String&)> progressCallback) {
    if (isExporting_.load()) return false;
    
    isExporting_.store(true);
    shouldCancel_.store(false);
    
    bool success = false;
    
    if (settings.exportStems)
        success = renderStems(settings, progressCallback);
    else
        success = renderMixdown(settings, progressCallback);
        
    isExporting_.store(false);
    return success;
}

bool AudioExporter::renderMixdown(const ExportSettings& settings,
                                 std::function<void(float, const juce::String&)> progressCallback) {
    // 1. Setup format
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();
    
    std::unique_ptr<juce::AudioFormat> format;
    switch (settings.format) {
        case Format::WAV: format.reset(new juce::WavAudioFormat()); break;
        case Format::FLAC: format.reset(new juce::FlacAudioFormat()); break;
        case Format::AIFF: format.reset(new juce::AiffAudioFormat()); break;
        default: format.reset(new juce::WavAudioFormat()); break;
    }
    
    // 2. Determine duration
    double duration = settings.endTimeSeconds;
    if (duration < 0) {
        // Auto-detect duration from clips
        double maxEnd = 0;
        for (const auto& track : engine_.tracks()) {
            if (track) {
                for (int i = 0; i < track->getNumClips(); ++i) {
                    if (auto* clip = track->getClip(i)) {
                        double clipEnd = (double)(clip->getStartPosition() + clip->getLength()) / settings.sampleRate;
                        maxEnd = std::max(maxEnd, clipEnd);
                    }
                }
            }
        }
        duration = maxEnd + 2.0; // Add tail
    }
    
    juce::int64 totalSamples = (juce::int64)(duration * settings.sampleRate);
    
    // 3. Prepare Renderer
    // We create a temporary renderer to avoid messing with engine's state
    AudioRenderer renderer;
    renderer.prepare(settings.sampleRate, 4096, engine_.getNumTracks(), engine_.getNumAuxBuses());
    
    // 4. Create output stream
    auto outStream = std::make_unique<juce::FileOutputStream>(settings.outputFile);
    if (outStream->failedToOpen()) return false;
    
    juce::AudioFormatWriterOptions writerOptions;
    writerOptions = writerOptions.withSampleRate(settings.sampleRate)
                                 .withNumChannels(2)
                                 .withBitsPerSample(settings.bitDepth);
                                 
    std::unique_ptr<juce::AudioFormatWriter> writer(format->createWriterFor(outStream.release(), writerOptions));
    if (!writer) return false;
    
    // 5. Render loop
    const int blockSize = 4096;
    juce::AudioBuffer<float> blockBuffer(2, blockSize);
    juce::int64 samplesRendered = 0;
    
    zenith::dsp::Dither dither;
    if (settings.useDither) dither.prepare(2);
    
    // Get raw pointers for renderer
    std::vector<Track*> trackPtrs;
    for (const auto& t : engine_.tracks()) if (t) trackPtrs.push_back(t.get());
    
    std::vector<AuxBus*> auxPtrs;
    // We'd need access to engine's aux buses, assuming they exist
    // For now we'll pass empty as a safe fallback if Engine doesn't expose them directly easily
    
    while (samplesRendered < totalSamples && !shouldCancel_.load()) {
        int numToRender = (int)juce::jmin((juce::int64)blockSize, totalSamples - samplesRendered);
        
        // Render block
        renderer.renderAudioGraph(blockBuffer, numToRender, samplesRendered,
                                 trackPtrs, {}, engine_.getRoutingGraph(),
                                 const_cast<MasterLimiter&>(engine_.getMasterLimiter()), // Need master limiter
                                 const_cast<PluginChain&>(engine_.getMasterPluginChain()), // Need master chain
                                 nullptr); // No tempo map for now
                                 
        if (settings.useDither && settings.bitDepth < 32)
            dither.process(blockBuffer, settings.bitDepth);
            
        writer->writeFromAudioSampleBuffer(blockBuffer, 0, numToRender);
        
        samplesRendered += numToRender;
        if (progressCallback)
            progressCallback((float)samplesRendered / (float)totalSamples, "Rendering Mixdown...");
    }
    
    return !shouldCancel_.load();
}

bool AudioExporter::renderStems(const ExportSettings& settings,
                               std::function<void(float, const juce::String&)> progressCallback) {
    // Implementation for stem export (rendering each track separately)
    // For MVP, we'll keep this as a stub that returns false or implement basic version
    juce::ignoreUnused(settings, progressCallback);
    return false; 
}

} // namespace zenith
