/*
  ==============================================================================

    TrackFreeze.cpp
    Created: 2025-12-09
    Author:  Zenith DAW

    Track freeze implementation for CPU optimization.

  ==============================================================================
*/

#include "TrackFreeze.h"
#include "../instruments/Instrument.h"
#include "Track.h"
#include "Clip.h"
#include "Engine.h"
#include "EngineConstants.h"

namespace zenith {

//==============================================================================
TrackFreezeManager::~TrackFreezeManager() {
    cancelFreeze();
    
    if (freezeThread_ && freezeThread_->isThreadRunning()) {
        freezeThread_->stopThread(constants::kThreadStopTimeoutMs);
    }
}

//==============================================================================
bool TrackFreezeManager::freezeTrack(Track& track, 
                                     Engine& engine,
                                     const juce::File& outputDir,
                                     ProgressCallback progress) {
    if (!juce::MessageManager::getInstance()->isThisTheMessageThread()) {
        jassertfalse;
        return false;
    }
    
    // Check if already frozen
    if (isFrozen(track)) {
        DBG("TrackFreeze: Track already frozen: " + track.getName());
        return false;
    }
    
    // Check if already freezing
    if (isFreezing_.load()) {
        DBG("TrackFreeze: Already freezing another track");
        return false;
    }
    
    // Create output directory if needed
    if (!outputDir.exists()) {
        outputDir.createDirectory();
    }
    
    // Generate freeze file path
    juce::String filename = track.getName().replaceCharacter(' ', '_') 
                          + "_freeze_" 
                          + juce::String(juce::Time::currentTimeMillis())
                          + ".wav";
    juce::File freezeFile = outputDir.getChildFile(filename);
    
    // Create freeze state
    FreezeState& state = freezeStates_[track.getTrackId()];
    state.trackId = track.getTrackId();
    state.freezeFile = freezeFile;
    state.wasArmed = track.isArmed();
    state.hadInstrument = track.hasInstrument();
    
    // Save plugin states before disabling
    state.pluginStates = savePluginStates(track);
    
    // Save instrument state if present
    if (track.hasInstrument()) {
        auto* instrument = track.getInstrument();
        if (instrument != nullptr && instrument->getAudioProcessor() != nullptr) {
            // Save instrument state using JUCE's AudioProcessor state mechanism
            instrument->getAudioProcessor()->getStateInformation(state.instrumentState);
            DBG("TrackFreeze: Saved instrument state (" + 
                juce::String(state.instrumentState.getSize()) + " bytes)");
        }
    }
    
    // Mark as freezing
    isFreezing_.store(true);
    shouldCancel_.store(false);

    // Set track internal flag for audio thread safety
    track.setBeingFrozen(true);
    
    // Start background render thread
    freezeThread_ = std::make_unique<FreezeRenderThread>(
        track, engine, freezeFile, progress, shouldCancel_);
    freezeThread_->startThread();
    
    DBG("TrackFreeze: Started freezing track: " + track.getName());
    return true;
}

//==============================================================================
bool TrackFreezeManager::unfreezeTrack(Track& track) {
    if (!juce::MessageManager::getInstance()->isThisTheMessageThread()) {
        jassertfalse;
        return false;
    }
    
    juce::String trackId = track.getTrackId();
    auto it = freezeStates_.find(trackId);
    
    if (it == freezeStates_.end()) {
        DBG("TrackFreeze: Track not frozen: " + track.getName());
        return false;
    }
    
    FreezeState& state = it->second;
    
    // Restore plugin states
    restorePluginStates(track, state.pluginStates);
    
    // Restore armed state
    track.setArmed(state.wasArmed);
    
    // Remove freeze file
    if (state.freezeFile.existsAsFile()) {
        state.freezeFile.deleteFile();
    }
    
    // Remove freeze state
    freezeStates_.erase(it);
    
    // Unset frozen flag on track
    track.setFrozen(false);
    
    DBG("TrackFreeze: Unfroze track: " + track.getName());
    return true;
}

//==============================================================================
bool TrackFreezeManager::isFrozen(const Track& track) const {
    juce::String trackId = track.getTrackId();
    return freezeStates_.find(trackId) != freezeStates_.end();
}

//==============================================================================
void TrackFreezeManager::cancelFreeze() {
    shouldCancel_.store(true);
}

//==============================================================================
juce::ValueTree TrackFreezeManager::savePluginStates(Track& track) {
    juce::ValueTree states("PluginStates");
    
    for (int i = 0; i < track.getNumPlugins(); ++i) {
        auto* plugin = track.getPlugin(i);
        if (plugin != nullptr) {
            juce::ValueTree pluginState("Plugin");
            pluginState.setProperty("index", i, nullptr);
            pluginState.setProperty("name", plugin->getName(), nullptr);
            
            // Save plugin state as memory block
            juce::MemoryBlock stateData;
            plugin->getStateInformation(stateData);
            pluginState.setProperty("state", stateData.toBase64Encoding(), nullptr);
            
            states.appendChild(pluginState, nullptr);
        }
    }
    
    return states;
}

//==============================================================================
void TrackFreezeManager::restorePluginStates(Track& track, const juce::ValueTree& states) {
    for (auto pluginState : states) {
        if (!pluginState.hasType("Plugin")) continue;
        
        int index = pluginState.getProperty("index", -1);
        if (index < 0 || index >= track.getNumPlugins()) continue;
        
        auto* plugin = track.getPlugin(index);
        if (plugin == nullptr) continue;
        
        // Restore plugin state
        juce::String base64State = pluginState.getProperty("state", "").toString();
        if (base64State.isNotEmpty()) {
            juce::MemoryBlock stateData;
            if (stateData.fromBase64Encoding(base64State)) {
                plugin->setStateInformation(stateData.getData(), 
                                           static_cast<int>(stateData.getSize()));
            }
        }
        
        // Re-enable plugin
        plugin->suspendProcessing(false);
    }
}

//==============================================================================
TrackFreezeManager::FreezeState* TrackFreezeManager::getFreezeState(
    const juce::String& trackId) {
    auto it = freezeStates_.find(trackId);
    return (it != freezeStates_.end()) ? &it->second : nullptr;
}

//==============================================================================
// FreezeRenderThread Implementation
//==============================================================================

void FreezeRenderThread::run() {
    DBG("FreezeRenderThread: Starting render");
    
    // Get track parameters
    const double sampleRate = engine_.getSampleRate();
    const int blockSize = constants::kFreezeBlockSize;
    
    // Calculate track duration by finding the last clip end position
    juce::int64 totalSamples = 0;
    for (int i = 0; i < track_.getNumClips(); ++i) {
        auto* clip = track_.getClip(i);
        if (clip != nullptr) {
            juce::int64 clipEnd = clip->getStartPosition() + clip->getLength();
            totalSamples = juce::jmax(totalSamples, clipEnd);
        }
    }
    
    // Add tail for plugin tails (reverb, delay)
    totalSamples += static_cast<juce::int64>(
        constants::kExportTailSeconds * sampleRate);
    
    if (totalSamples == 0) {
        DBG("FreezeRenderThread: No content to freeze");
        return;
    }
    
    // Create output file
    juce::WavAudioFormat wavFormat;
    auto fileStream = std::make_unique<juce::FileOutputStream>(outputFile_);
    
    if (!fileStream->openedOk()) {
        DBG("FreezeRenderThread: Failed to create output file");
        return;
    }

    std::unique_ptr<juce::AudioFormatWriter> writer(
        wavFormat.createWriterFor(fileStream.release(), sampleRate, 2,
                                  constants::kFreezeBitDepth, {}, 0));
    
    if (writer == nullptr) {
        DBG("FreezeRenderThread: Failed to create audio writer");
        return;
    }
    
    // Prepare track for rendering
    track_.prepareToPlay(blockSize, sampleRate);
    
    // Render loop
    juce::AudioBuffer<float> buffer(2, blockSize);
    juce::int64 samplesRendered = 0;
    
    while (samplesRendered < totalSamples && !shouldCancel_.load()) {
        // Check for thread termination
        if (threadShouldExit()) {
            break;
        }
        
        // Clear buffer
        buffer.clear();
        
        // Calculate samples to render this block
        int samplesToRender = static_cast<int>(
            juce::jmin(static_cast<juce::int64>(blockSize), 
                      totalSamples - samplesRendered));
        
        // Create audio source channel info
        juce::AudioSourceChannelInfo info(&buffer, 0, samplesToRender);
        
        // Render track
        track_.getNextAudioBlock(info, samplesRendered, {});
        
        // Write to file
        writer->writeFromAudioSampleBuffer(buffer, 0, samplesToRender);
        
        samplesRendered += samplesToRender;
        
        // Report progress
        if (progress_) {
            float progressValue = static_cast<float>(samplesRendered) 
                                / static_cast<float>(totalSamples);
            juce::String status = "Freezing: " 
                                + juce::String(progressValue * 100.0f, 1) 
                                + "%";
            
            juce::MessageManager::callAsync([=, progress = progress_]() {
                progress(progressValue, status);
            });
        }
    }
    
    // Finalize
    writer.reset();
    
    if (shouldCancel_.load()) {
        DBG("FreezeRenderThread: Cancelled");
        outputFile_.deleteFile();
        
        // THREAD SAFETY FIX: Use Engine::getInstance() for safe async access
        const juce::String cancelledTrackId = track_.getTrackId();
        juce::MessageManager::callAsync([cancelledTrackId]() {
            if (auto* engine = Engine::getInstance()) {
                if (auto* track = engine->getTrackById(cancelledTrackId)) {
                    track->setBeingFrozen(false);
                }
            }
        });
        return;
    }
    
    // Success - finalize freeze on message thread
    // THREAD SAFETY FIX: Use Engine::getInstance() for safe async access
    const juce::String trackId = track_.getTrackId();
    const juce::String trackName = track_.getName();
    auto progressCopy = progress_; // Copy the callback
    
    juce::MessageManager::callAsync([trackId, trackName, progressCopy]() {
        // SAFE: Look up engine via singleton, check for null
        auto* engine = Engine::getInstance();
        if (engine == nullptr || engine->isShuttingDown()) {
            DBG("FreezeRenderThread: Engine shutting down, skipping finalize");
            return;
        }
        auto* track = engine->getTrackById(trackId);
        if (track == nullptr) {
            DBG("FreezeRenderThread: Track was deleted during freeze: " + trackName);
            return; // Track was deleted - nothing to do
        }
        
        // Disable all plugins on the track
        for (int i = 0; i < track->getNumPlugins(); ++i) {
            auto* plugin = track->getPlugin(i);
            if (plugin != nullptr) {
                plugin->suspendProcessing(true);
            }
        }
        
        // Mark track as frozen
        track->setFrozen(true);
        track->setBeingFrozen(false); // Enable access again
        
        // Disable arming
        track->setArmed(false);
        
        DBG("FreezeRenderThread: Freeze complete for " + trackName);
        
        if (progressCopy) {
            progressCopy(1.0f, "Freeze complete");
        }
    });
}

} // namespace zenith
