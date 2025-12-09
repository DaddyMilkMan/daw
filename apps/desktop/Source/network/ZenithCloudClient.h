/*
  ==============================================================================

    ZenithCloudClient.h
    Created: 2025-12-08
    Author:  Zenith DAW - Cloud Collaboration Features

    Generic HTTP/REST client for Zenith Cloud services.
    
    Features:
    - Public preset browsing and download
    - Project version control
    - Async network operations using ThreadPool

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <queue>
#include <atomic>
#include <functional>

namespace zenith {

//==============================================================================
/**
    Cloud preset metadata structure
*/
struct CloudPreset
{
    juce::String id;
    juce::String name;
    juce::String description;
    juce::String pluginId;
    juce::String pluginName;
    juce::String category;
    juce::String ownerName;
    juce::String previewUrl;
    juce::StringArray tags;
    int downloads = 0;
    int likes = 0;
    int64_t size = 0;
    juce::Time createdAt;
};

/**
    Cloud project version metadata
*/
struct CloudProjectVersion
{
    int versionNumber = 0;
    juce::String label;
    juce::String changelog;
    juce::String filename;
    int64_t size = 0;
    juce::Time createdAt;
};

//==============================================================================
/**
    Async HTTP client for Zenith Cloud API
    
    All public methods are thread-safe and non-blocking.
    Results are delivered via callbacks on the message thread.
*/
class ZenithCloudClient : public juce::ChangeBroadcaster
{
public:
    //==============================================================================
    ZenithCloudClient();
    ~ZenithCloudClient();

    //==============================================================================
    // Configuration
    //==============================================================================
    
    /**
        Set the cloud API base URL
        Default: https://api.zenith.audio (or localhost for dev)
    */
    void setBaseUrl(const juce::String& url);
    juce::String getBaseUrl() const { return baseUrl_; }
    
    /**
        Set authentication token (received after OAuth login)
    */
    void setAuthToken(const juce::String& token);
    bool isAuthenticated() const { return authToken_.isNotEmpty(); }
    
    //==============================================================================
    // Cloud Presets API
    //==============================================================================
    
    /**
        Fetch public presets with optional filters
        
        @param onComplete Callback with preset list (called on message thread)
        @param pluginId Optional filter by plugin ID
        @param category Optional filter by category
        @param page Page number (1-indexed)
        @param sortBy Sort option: "newest", "popular", or "likes"
    */
    void fetchPublicPresets(
        std::function<void(const std::vector<CloudPreset>&, bool success)> onComplete,
        const juce::String& pluginId = "",
        const juce::String& category = "",
        int page = 1,
        const juce::String& sortBy = "newest");
    
    /**
        Search public presets
        
        @param query Search query (min 2 characters)
        @param onComplete Callback with search results
    */
    void searchPresets(
        const juce::String& query,
        std::function<void(const std::vector<CloudPreset>&, bool success)> onComplete);
    
    /**
        Download a preset file by ID
        
        @param presetId The preset's server ID
        @param destinationFolder Where to save the downloaded file
        @param onComplete Callback with file path on success
        @param onProgress Optional progress callback (0.0 - 1.0)
    */
    void downloadPreset(
        const juce::String& presetId,
        const juce::File& destinationFolder,
        std::function<void(const juce::File& savedFile, bool success)> onComplete,
        std::function<void(float progress)> onProgress = nullptr);
    
    /**
        Upload a preset to the cloud
        
        @param presetFile The preset file to upload
        @param metadata Preset metadata (name, pluginId, pluginName, category, tags, isPublic)
        @param onComplete Callback on completion
    */
    void uploadPreset(
        const juce::File& presetFile,
        const juce::var& metadata,
        std::function<void(bool success, const juce::String& presetId)> onComplete);
    
    //==============================================================================
    // Project Version Control API
    //==============================================================================
    
    /**
        Save a new version of a project
        
        @param projectId Server project ID
        @param projectFile Current project file
        @param label Version label
        @param changelog Description of changes
        @param onComplete Callback on completion
    */
    void saveProjectVersion(
        const juce::String& projectId,
        const juce::File& projectFile,
        const juce::String& label,
        const juce::String& changelog,
        std::function<void(bool success, int versionNumber)> onComplete);
    
    /**
        Get all versions of a project
        
        @param projectId Server project ID
        @param onComplete Callback with version list
    */
    void getProjectVersions(
        const juce::String& projectId,
        std::function<void(const std::vector<CloudProjectVersion>&, bool success)> onComplete);
    
    /**
        Download a specific project version
    */
    void downloadProjectVersion(
        const juce::String& projectId,
        int versionNumber,
        const juce::File& destinationFolder,
        std::function<void(const juce::File& savedFile, bool success)> onComplete);
    
    //==============================================================================
    // Status
    //==============================================================================
    
    bool isBusy() const { return activeRequests_.load() > 0; }
    juce::String getLastError() const;

private:
    //==============================================================================
    // Internal request handling
    //==============================================================================
    
    struct RequestContext
    {
        juce::URL url;
        juce::String method; // GET, POST, PUT, DELETE
        juce::var postData;
        juce::String authToken;
        std::function<void(const juce::var& response, bool success, const juce::String& error)> onComplete;
    };
    
    void performRequest(const RequestContext& ctx);
    void performDownload(const juce::URL& url, const juce::File& destination,
                        std::function<void(const juce::File&, bool)> onComplete,
                        std::function<void(float)> onProgress);
    void performUpload(const juce::URL& url, const juce::File& file, const juce::var& metadata,
                      std::function<void(const juce::var&, bool, const juce::String&)> onComplete);
    
    juce::URL buildUrl(const juce::String& endpoint, const juce::StringPairArray& params = {}) const;
    CloudPreset parsePreset(const juce::var& presetVar) const;
    CloudProjectVersion parseVersion(const juce::var& versionVar) const;
    
    //==============================================================================
    // Member variables
    //==============================================================================
    
    juce::String baseUrl_{"http://localhost:5000"};  // Default to local dev server
    juce::String authToken_;
    
    std::unique_ptr<juce::ThreadPool> threadPool_;
    std::atomic<int> activeRequests_{0};
    
    juce::CriticalSection errorLock_;
    juce::String lastError_;
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithCloudClient)
};

} // namespace zenith
