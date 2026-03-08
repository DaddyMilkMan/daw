/*
  ==============================================================================

    UpdateService.h
    Created: 2026-01-04
    Author:  Zenith DAW Team

    Handles checking for application updates from a remote server.
    
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <functional>

namespace zenith {
namespace network {

class UpdateService : public juce::Thread {
public:
    UpdateService();
    ~UpdateService() override;

    struct UpdateInfo {
        bool available = false;
        juce::String version; // e.g., "1.2.0"
        juce::String url;     // Download URL
        juce::String releaseNotes;
    };

    /**
     * @brief Trigger an asynchronous update check.
     * @param onCompletion Callback receiving the UpdateInfo result.
     */
    void checkForUpdates(std::function<void(const UpdateInfo&)> onCompletion);

    /**
     * @brief Get the cached update state.
     */
    const UpdateInfo& getLastUpdateInfo() const { return updateInfo_; }

    void run() override;

private:
    std::function<void(const UpdateInfo&)> callback_;
    UpdateInfo updateInfo_;
    
    // Simulate current version
    const juce::String currentVersion_ = "1.0.0";
    
    // Remote endpoint (Replace with real URL in production)
    const juce::String updateUrl_ = "https://api.zenithdaw.com/version/latest"; 

    void checkForUpdatesSync();
};

} // namespace network
} // namespace zenith
