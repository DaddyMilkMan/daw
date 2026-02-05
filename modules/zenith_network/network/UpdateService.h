/*
  ==============================================================================

    UpdateService.h
    Created: 2026-02-04
    Author:  Zenith DAW Team

    Production-ready update checking with HTTP networking, semantic versioning,
    caching, and multi-channel support (stable/beta/nightly).
    
    Features:
    - Real HTTP requests to update server
    - Semantic version comparison (MAJOR.MINOR.PATCH)
    - Response caching for offline use
    - Automatic retry with exponential backoff
    - Multiple release channels
    - Platform-specific update URLs

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <functional>
#include <optional>

namespace zenith {
namespace network {

//==============================================================================
// Version Structure
//==============================================================================

/**
 * @brief Semantic version (MAJOR.MINOR.PATCH-BUILD+METADATA)
 */
struct SemanticVersion {
    int major = 0;
    int minor = 0;
    int patch = 0;
    juce::String prerelease;  // e.g., "beta.2"
    juce::String buildMetadata;
    
    SemanticVersion() = default;
    explicit SemanticVersion(const juce::String& versionString);
    
    juce::String toString() const;
    bool isValid() const { return major >= 0 && minor >= 0 && patch >= 0; }
    
    // Comparison operators
    bool operator<(const SemanticVersion& other) const;
    bool operator>(const SemanticVersion& other) const;
    bool operator==(const SemanticVersion& other) const;
    bool operator!=(const SemanticVersion& other) const { return !(*this == other); }
    bool operator<=(const SemanticVersion& other) const { return *this < other || *this == other; }
    bool operator>=(const SemanticVersion& other) const { return *this > other || *this == other; }
    
    /**
     * @brief Compare pre-release versions according to semver spec
     */
    int comparePrerelease(const juce::String& a, const juce::String& b) const;
};

//==============================================================================
// Release Channel
//==============================================================================

enum class ReleaseChannel {
    Stable,   // Production releases only
    Beta,     // Beta releases
    Nightly,  // Development builds
    Internal  // Internal testing
};

juce::String channelToString(ReleaseChannel channel);
ReleaseChannel channelFromString(const juce::String& str);

//==============================================================================
// Update Info
//==============================================================================

struct UpdateInfo {
    bool available = false;
    SemanticVersion currentVersion;
    SemanticVersion latestVersion;
    juce::String downloadUrl;
    juce::String releaseNotes;
    juce::String releaseDate;  // ISO 8601 format
    juce::String checksum;     // SHA256 of installer
    int64_t fileSize = 0;      // Size in bytes
    bool isMandatory = false;  // Force update
    ReleaseChannel channel = ReleaseChannel::Stable;
    
    // Platform-specific URLs
    juce::String windowsUrl;
    juce::String macUrl;
    juce::String linuxUrl;
    
    juce::String getDownloadUrlForCurrentPlatform() const;
    bool isNewerThanCurrent() const;
};

//==============================================================================
// Update Service
//==============================================================================

class UpdateService : public juce::Thread {
public:
    //==========================================================================
    // Construction
    //==========================================================================
    
    UpdateService();
    ~UpdateService() override;
    
    //==========================================================================
    // Configuration
    //==========================================================================
    
    /**
     * @brief Set the update server URL
     */
    void setUpdateUrl(const juce::String& url);
    
    /**
     * @brief Set the release channel to check
     */
    void setReleaseChannel(ReleaseChannel channel);
    
    /**
     * @brief Set current application version
     */
    void setCurrentVersion(const juce::String& version);
    
    /**
     * @brief Set API key for authenticated updates (enterprise)
     */
    void setApiKey(const juce::String& apiKey);
    
    /**
     * @brief Enable/disable automatic caching
     */
    void setCacheEnabled(bool enabled);
    
    /**
     * @brief Set cache expiration time in minutes
     */
    void setCacheExpiration(int minutes);
    
    //==========================================================================
    // Update Checking
    //==========================================================================
    
    /**
     * @brief Check for updates asynchronously
     * @param onCompletion Callback with result
     * @param onError Callback for errors
     */
    void checkForUpdates(std::function<void(const UpdateInfo&)> onCompletion,
                        std::function<void(const juce::String&)> onError = nullptr);
    
    /**
     * @brief Check for updates synchronously (blocking)
     * @return Update info, or nullopt on error
     */
    std::optional<UpdateInfo> checkForUpdatesSync();
    
    /**
     * @brief Get cached update info
     */
    std::optional<UpdateInfo> getCachedUpdateInfo() const;
    
    /**
     * @brief Clear cached update info
     */
    void clearCache();
    
    /**
     * @brief Get the last check result
     */
    const std::optional<UpdateInfo>& getLastResult() const { return lastResult_; }
    
    /**
     * @brief Check if an update check is in progress
     */
    bool isChecking() const { return isChecking_.load(); }
    
    //==========================================================================
    // Download Management
    //==========================================================================
    
    /**
     * @brief Download update to file
     * @param info Update info
     * @param targetFile Destination file
     * @param onProgress Progress callback (0.0 - 1.0)
     * @param onComplete Completion callback (success, error message)
     */
    void downloadUpdate(const UpdateInfo& info,
                       const juce::File& targetFile,
                       std::function<void(float)> onProgress,
                       std::function<void(bool, const juce::String&)> onComplete);
    
    /**
     * @brief Verify downloaded file checksum
     */
    bool verifyChecksum(const juce::File& file, const juce::String& expectedChecksum);
    
    //==========================================================================
    // Thread Implementation
    //==========================================================================
    
    void run() override;

private:
    //==========================================================================
    // Network Implementation
    //==========================================================================
    
    std::optional<UpdateInfo> fetchUpdateInfoFromServer();
    juce::String buildRequestUrl() const;
    UpdateInfo parseUpdateResponse(const juce::var& json);
    
    //==========================================================================
    // Cache Management
    //==========================================================================
    
    void saveToCache(const UpdateInfo& info);
    std::optional<UpdateInfo> loadFromCache();
    juce::File getCacheFile() const;
    bool isCacheValid() const;
    
    //==========================================================================
    // HTTP Headers
    //==========================================================================
    
    juce::StringArray buildRequestHeaders() const;
    
    //==========================================================================
    // State
    //==========================================================================
    
    std::atomic<bool> isChecking_{false};
    std::optional<UpdateInfo> lastResult_;
    
    // Configuration
    juce::String updateUrl_ = "https://api.zenithdaw.com/v1/updates";
    SemanticVersion currentVersion_;
    ReleaseChannel channel_ = ReleaseChannel::Stable;
    juce::String apiKey_;
    
    // Cache settings
    bool cacheEnabled_ = true;
    int cacheExpirationMinutes_ = 60;
    
    // Callbacks
    std::function<void(const UpdateInfo&)> completionCallback_;
    std::function<void(const juce::String&)> errorCallback_;
    
    // Retry settings
    static constexpr int MAX_RETRIES = 3;
    static constexpr int INITIAL_RETRY_DELAY_MS = 1000;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UpdateService)
};

} // namespace network
} // namespace zenith
