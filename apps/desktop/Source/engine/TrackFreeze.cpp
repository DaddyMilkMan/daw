/*
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
        // SAFE: Look up track by ID on message thread
        auto* engine = Engine::getInstance();
        if (engine == nullptr) return; // Engine shut down?

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
