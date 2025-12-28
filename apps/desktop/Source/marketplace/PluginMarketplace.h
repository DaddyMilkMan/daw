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
struct PluginInfo {
    juce::String id;
    juce::String name;
    juce::String developer;
    juce::String version;
    juce::String description;
    juce::String category;  // "EQ", "Compressor", "Reverb", "Delay", etc.
    juce::String type;      // "VST3", "AU", "AAX", "Standalone"
    juce::String platform;  // "Windows", "macOS", "Linux"
    
    // Pricing
    float price = 0.0f;
    juce::String currency;
    bool isFree = false;
    bool hasTrial = false;
    int trialDays = 0;
    float discountPrice = 0.0f;
    juce::Time discountEnd;
    
    // Ratings and reviews
    float averageRating = 0.0f;
    int totalRatings = 0;
    int totalReviews = 0;
    std::vector<juce::String> tags;
    
    // Technical details
    juce::String architecture;  // "x64", "ARM64"
    juce::String minOSVersion;
    juce::String minDAWVersion;
    size_t downloadSize = 0;    // bytes
    juce::String licenseType;   // "GPL", "Commercial", "Freeware"
    
    // Media
    juce::String thumbnailUrl;
    juce::String previewUrl;
    juce::String manualUrl;
    std::vector<juce::String> screenshots;
    
    // Download and installation
    juce::String downloadUrl;
    juce::String installerPath;
    bool isInstalled = false;
    juce::String installedVersion;
    bool updateAvailable = false;
    juce::String latestVersion;
    
    // Usage statistics
    int downloadCount = 0;
    juce::Time lastUpdated;
    juce::Time addedDate;
};

// User account information
struct UserAccount {
    juce::String userId;
    juce::String username;
    juce::String email;
    juce::String displayName;
    juce::String avatarUrl;
    bool isPremium = false;
    juce::Time premiumExpiry;
    
    // Purchased plugins
    std::vector<juce::String> purchasedPluginIds;
    std::vector<juce::String> licensedPluginIds;
    
    // Preferences
    juce::String preferredCurrency = "USD";
    bool autoUpdate = true;
    bool betaProgram = false;
};

// Download progress
struct DownloadProgress {
    juce::String pluginId;
    float progress = 0.0f;      // 0.0 to 1.0
    size_t bytesDownloaded = 0;
    size_t totalBytes = 0;
    float downloadSpeed = 0.0f; // KB/s
    juce::Time estimatedCompletion;
    bool isPaused = false;
    bool isCompleted = false;
    bool hasError = false;
    juce::String errorMessage;
};

// Marketplace configuration
struct MarketplaceConfig {
    juce::String apiUrl = "https://api.zenithdaw.com/marketplace";
    juce::String cdnUrl = "https://cdn.zenithdaw.com/plugins";
    juce::String authUrl = "https://auth.zenithdaw.com";
    
    // Download settings
    juce::File downloadDirectory;
    int maxConcurrentDownloads = 3;
    bool enableResume = true;
    bool verifyChecksums = true;
    
    // Cache settings
    juce::File cacheDirectory;
    int cacheExpiryDays = 7;
    bool enableOfflineMode = false;
    
    // Update settings
    bool autoCheckUpdates = true;
    int updateCheckInterval = 24;  // hours
    bool autoInstallUpdates = false;
    
    // UI settings
    bool showFreePlugins = true;
    bool showPaidPlugins = true;
    bool showBetaVersions = false;
    juce::String defaultCurrency = "USD";
};

// Plugin marketplace manager
class PluginMarketplace : public juce::Timer,
                         public juce::Thread,
                         public juce::AsyncUpdater {
public:
    PluginMarketplace();
    ~PluginMarketplace() override;
    
    // Configuration
    void setConfig(const MarketplaceConfig& config);
    MarketplaceConfig getConfig() const;
    
    // Authentication
    bool login(const juce::String& email, const juce::String& password);
    bool logout();
    bool isLoggedIn() const;
    UserAccount getCurrentUser() const;
    
    // Plugin catalog
    void refreshCatalog();
    std::vector<PluginInfo> searchPlugins(const juce::String& query,
                                         const juce::String& category = "",
                                         bool freeOnly = false,
                                         float minRating = 0.0f);
    std::vector<PluginInfo> getFeaturedPlugins();
    std::vector<PluginInfo> getNewReleases();
    std::vector<PluginInfo> getTopRated();
    std::vector<PluginInfo> getOnSale();
    PluginInfo getPluginInfo(const juce::String& pluginId);
    
    // Categories and tags
    std::vector<juce::String> getCategories();
    std::vector<juce::String> getTags();
    std::vector<PluginInfo> getPluginsByCategory(const juce::String& category);
    std::vector<PluginInfo> getPluginsByTag(const juce::String& tag);
    
    // Plugin management
    bool downloadPlugin(const juce::String& pluginId);
    bool pauseDownload(const juce::String& pluginId);
    bool resumeDownload(const juce::String& pluginId);
    bool cancelDownload(const juce::String& pluginId);
    bool installPlugin(const juce::String& pluginId);
    bool uninstallPlugin(const juce::String& pluginId);
    bool updatePlugin(const juce::String& pluginId);
    
    // Download progress
    DownloadProgress getDownloadProgress(const juce::String& pluginId);
    std::vector<DownloadProgress> getAllDownloads();
    
    // Installed plugins
    std::vector<PluginInfo> getInstalledPlugins();
    std::vector<PluginInfo> getAvailableUpdates();
    void scanInstalledPlugins();
    
    // Purchases and licensing
    bool purchasePlugin(const juce::String& pluginId);
    bool activateLicense(const juce::String& pluginId, const juce::String& licenseKey);
    bool deactivateLicense(const juce::String& pluginId);
    std::vector<PluginInfo> getPurchasedPlugins();
    
    // Reviews and ratings
    bool submitReview(const juce::String& pluginId, int rating, const juce::String& review);
    std::vector<juce::String> getReviews(const juce::String& pluginId);
    bool ratePlugin(const juce::String& pluginId, int rating);
    
    // Wishlist
    void addToWishlist(const juce::String& pluginId);
    void removeFromWishlist(const juce::String& pluginId);
    std::vector<PluginInfo> getWishlist();
    
    // Updates and notifications
    void checkForUpdates();
    void enableNotifications(bool enabled);
    bool areNotificationsEnabled() const;
    
    // Thread and timer callbacks
    void run() override;
    void timerCallback() override;
    void handleAsyncUpdate() override;
    
    // Listeners
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
class PluginMarketplaceUI : public juce::Component,
                           public PluginMarketplace::Listener,
                           public juce::Button::Listener,
                           public juce::TextEditor::Listener,
                           public juce::ComboBox::Listener,
                           public juce::ListBoxModel {
public:
    PluginMarketplaceUI();
    ~PluginMarketplaceUI() override;
    
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Marketplace access
    void setMarketplace(PluginMarketplace* marketplace);
    PluginMarketplace* getMarketplace() const { return marketplace; }
    
    // UI control
    void showLoginDialog();
    void showPluginDetails(const juce::String& pluginId);
    void showDownloads();
    void showInstalled();
    void showWishlist();
    
    // PluginMarketplace::Listener
    void loginStatusChanged(bool isLoggedIn) override;
    void catalogUpdated() override;
    void pluginDownloadProgress(const juce::String& pluginId, float progress) override;
    void pluginDownloadCompleted(const juce::String& pluginId) override;
    void updateAvailable(const juce::String& pluginId) override;
    
    // Button::Listener
    void buttonClicked(juce::Button* button) override;
    
    // TextEditor::Listener
    void textEditorTextChanged(juce::TextEditor& editor) override;
    
    // ComboBox::Listener
    void comboBoxChanged(juce::ComboBox* comboBox) override;
    
    // ListBoxModel
    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height) override;
    juce::Component* refreshComponentForRow(int rowNumber, bool isRowSelected, juce::Component* existingComponentToUpdate) override;
    
private:
    PluginMarketplace* marketplace = nullptr;
    
    // UI components
    std::unique_ptr<juce::Viewport> mainViewport;
    std::unique_ptr<juce::Component> mainComponent;
    
    // Header
    std::unique_ptr<juce::TextButton> loginButton;
    std::unique_ptr<juce::TextButton> accountButton;
    std::unique_ptr<juce::Label> userLabel;
    
    // Search and filters
    std::unique_ptr<juce::TextEditor> searchEditor;
    std::unique_ptr<juce::ComboBox> categoryComboBox;
    std::unique_ptr<juce::ComboBox> sortByComboBox;
    std::unique_ptr<juce::ToggleButton> freeOnlyToggle;
    std::unique_ptr<juce::ToggleButton> installedToggle;
    
    // Plugin list
    std::unique_ptr<juce::ListBox> pluginListBox;
    std::vector<PluginInfo> displayedPlugins;
    
    // Featured section
    std::unique_ptr<juce::Component> featuredComponent;
    std::vector<std::unique_ptr<juce::Component>> featuredCards;
    
    // Status bar
    std::unique_ptr<juce::Label> statusLabel;
    std::unique_ptr<juce::ProgressBar> progressBar;
    std::unique_ptr<juce::TextButton> refreshButton;
    
    // Plugin card component
    class PluginCard : public juce::Component,
                      public juce::Button::Listener {
    public:
        PluginCard(const PluginInfo& pluginInfo);
        ~PluginCard() override;
        
        void paint(juce::Graphics& g) override;
        void resized() override;
        void buttonClicked(juce::Button* button) override;
        
        const PluginInfo& getPluginInfo() const { return pluginInfo; }
        
    private:
        PluginInfo pluginInfo;
        
        std::unique_ptr<juce::Label> nameLabel;
        std::unique_ptr<juce::Label> developerLabel;
        std::unique_ptr<juce::Label> priceLabel;
        std::unique_ptr<juce::Label> ratingLabel;
        std::unique_ptr<juce::TextButton> downloadButton;
        std::unique_ptr<juce::TextButton> trialButton;
        std::unique_ptr<juce::ProgressBar> progressBar;
        std::unique_ptr<juce::ToggleButton> wishlistButton;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginCard)
    };
    
    // UI creation
    void createHeader();
    void createSearchAndFilters();
    void createPluginList();
    void createFeaturedSection();
    void createStatusBar();
    
    // Updates
    void updatePluginList();
    void updateFeaturedSection();
    void updateStatusBar();
    void filterPlugins();
    
    // Dialogs
    void showLoginDialog();
    void showPluginDetailsDialog(const PluginInfo& plugin);
    void showDownloadManager();
    void showPreferences();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginMarketplaceUI)
};

// Plugin installer
class PluginInstaller {
public:
    struct InstallResult {
        bool success = false;
        juce::String message;
        juce::String installedPath;
    };
    
    PluginInstaller();
    ~PluginInstaller() = default;
    
    // Installation
    InstallResult installPlugin(const juce::File& installerFile, const juce::String& pluginId);
    bool uninstallPlugin(const juce::String& pluginId);
    
    // Validation
    bool validateInstaller(const juce::File& installerFile);
    bool verifySignature(const juce::File& file, const juce::String& signature);
    
    // Paths
    juce::File getVST3Path();
    juce::File getAUPath();
    juce::File getAAXPath();
    juce::File getStandalonePath();
    juce::File getCustomPath(const juce::String& pluginId);
    
private:
    bool runWindowsInstaller(const juce::File& installer);
    bool runMacInstaller(const juce::File& installer);
    bool runLinuxInstaller(const juce::File& installer);
    
    bool copyVST3Plugin(const juce::File& source, const juce::File& destination);
    bool copyAUPlugin(const juce::File& source, const juce::File& destination);
    
    void registerPlugin(const juce::String& pluginId, const juce::File& path);
    void unregisterPlugin(const juce::String& pluginId);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginInstaller)
};

} // namespace marketplace
} // namespace zenith
