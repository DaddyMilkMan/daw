/*
  ==============================================================================
    PluginMarketplace.h
    Plugin marketplace integration with download and management
    Phase 5: Advanced Features
  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_network/juce_network.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include <atomic>

namespace zenith {
namespace marketplace {

// Plugin information
    struct Listener {
        virtual ~Listener() = default;
        virtual void loginStatusChanged(bool isLoggedIn) {}
        virtual void catalogUpdated() {}
        virtual void pluginDownloadStarted(const juce::String& pluginId) {}
        virtual void pluginDownloadProgress(const juce::String& pluginId, float progress) {}
        virtual void pluginDownloadCompleted(const juce::String& pluginId) {}
        virtual void pluginDownloadFailed(const juce::String& pluginId, const juce::String& error) {}
        virtual void pluginInstalled(const juce::String& pluginId) {}
        virtual void pluginUninstalled(const juce::String& pluginId) {}
        virtual void updateAvailable(const juce::String& pluginId) {}
        virtual void purchaseCompleted(const juce::String& pluginId) {}
        virtual void reviewSubmitted(const juce::String& pluginId) {}
    };

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

private:
    // Configuration
    MarketplaceConfig config;

    // Authentication
    std::unique_ptr<UserAccount> currentUser;
    juce::String authToken;
    bool isLoggedIn = false;

    // Plugin catalog
    std::unordered_map<juce::String, PluginInfo> pluginCatalog;
    std::vector<juce::String> categories;
    std::vector<juce::String> tags;
    std::mutex catalogMutex;

    // Downloads
    std::unordered_map<juce::String, DownloadProgress> downloads;
    std::unordered_map<juce::String, std::unique_ptr<juce::URL::DownloadTask>> downloadTasks;
    std::mutex downloadsMutex;

    // Installed plugins
    std::unordered_map<juce::String, PluginInfo> installedPlugins;
    std::mutex installedMutex;

    // Network
    std::unique_ptr<juce::URL> apiClient;
    std::unique_ptr<juce::WebInputStream> webStream;

    // State
    std::atomic<bool> isRefreshingCatalog{false};
    std::atomic<bool> needsUpdateCheck{false};
    std::atomic<bool> notificationsEnabled{true};
    juce::Time lastCatalogUpdate;
    juce::Time lastUpdateCheck;

    // Listeners
    std::vector<Listener*> listeners;

    // Network methods
    juce::String makeAPIRequest(const juce::String& endpoint, const juce::String& method = "GET", const juce::String& data = "");
    bool authenticateUser(const juce::String& email, const juce::String& password);
    void fetchCatalog();
    void fetchPluginDetails(const juce::String& pluginId);

    // Download methods
    bool startDownload(const juce::String& pluginId);
    void updateDownloadProgress(const juce::String& pluginId, int64 bytesDownloaded, int64 totalBytes);
    void completeDownload(const juce::String& pluginId);
    void failDownload(const juce::String& pluginId, const juce::String& error);

    // Installation methods
    bool verifyDownload(const juce::String& pluginId);
    bool runInstaller(const juce::File& installerPath);
    bool validateInstallation(const juce::String& pluginId);

    // Plugin scanning
    void scanVST3Directory();
    void scanAUDirectory();
    void scanAAXDirectory();
    void scanStandaloneDirectory();

    // Cache management
    void loadCachedCatalog();
    void saveCachedCatalog();
    void clearCache();
    bool isCacheValid() const;

    // Utility methods
    juce::String generateChecksum(const juce::File& file);
    juce::File getPluginInstallPath(const PluginInfo& plugin);
    juce::String formatFileSize(size_t bytes) const;
    juce::String formatPrice(float price, const juce::String& currency) const;

    // Notification
    void notifyLoginStatusChanged(bool loggedIn);
    void notifyCatalogUpdated();
    void notifyPluginDownloadStarted(const juce::String& pluginId);
    void notifyPluginDownloadProgress(const juce::String& pluginId, float progress);
    void notifyPluginDownloadCompleted(const juce::String& pluginId);
    void notifyPluginDownloadFailed(const juce::String& pluginId, const juce::String& error);
    void notifyPluginInstalled(const juce::String& pluginId);
    void notifyPluginUninstalled(const juce::String& pluginId);
    void notifyUpdateAvailable(const juce::String& pluginId);
    void notifyPurchaseCompleted(const juce::String& pluginId);
    void notifyReviewSubmitted(const juce::String& pluginId);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginMarketplace)
};

// Plugin marketplace UI

} // namespace
