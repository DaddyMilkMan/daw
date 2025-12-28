/*
  ==============================================================================
    PluginMarketplace.cpp
    Plugin marketplace integration implementation
  ==============================================================================
*/

#include "PluginMarketplace.h"
#include <random>

namespace zenith {
namespace marketplace {

// PluginMarketplace Implementation
PluginMarketplace::PluginMarketplace() {
    // Initialize config
    config = MarketplaceConfig();
    
    // Initialize download manager
    downloadManager = std::make_unique<DownloadManager>();
    downloadManager->addListener(this);
    
    // Initialize license manager
    licenseManager = std::make_unique<LicenseManager>();
    
    // Start timer for updates
    startTimerHz(1);
    
    // Load local plugins
    loadLocalPlugins();
}

PluginMarketplace::~PluginMarketplace() {
    stopTimer();
    
    // Cancel all downloads
    downloadManager->cancelAllDownloads();
}

void PluginMarketplace::setConfig(const MarketplaceConfig& newConfig) {
    config = newConfig;
}

PluginMarketplace::MarketplaceConfig PluginMarketplace::getConfig() const {
    return config;
}

void PluginMarketplace::searchPlugins(const juce::String& query, 
                                    const std::vector<juce::String>& categories,
                                    const std::vector<juce::String>& tags,
                                    float minRating,
                                    float maxPrice,
                                    bool freeOnly) {
    currentSearchQuery = query;
    currentSearchResults.clear();
    
    // Build search request
    juce::DynamicObject::Ptr searchRequest = new juce::DynamicObject();
    searchRequest->setProperty("query", query);
    searchRequest->setProperty("categories", juce::var(categories.data(), static_cast<int>(categories.size())));
    searchRequest->setProperty("tags", juce::var(tags.data(), static_cast<int>(tags.size())));
    searchRequest->setProperty("minRating", minRating);
    searchRequest->setProperty("maxPrice", maxPrice);
    searchRequest->setProperty("freeOnly", freeOnly);
    searchRequest->setProperty("limit", config.maxSearchResults);
    searchRequest->setProperty("offset", 0);
    
    // Send search request
    juce::URL searchUrl(config.apiEndpoint + "/search");
    
    juce::StringPairArray headers;
    headers.set("Authorization", "Bearer " + config.apiKey);
    headers.set("Content-Type", "application/json");
    
    auto options = std::make_unique<juce::URL::InputStreamOptions>(juce::URL::InputStreamOptions{
        .withPostData = juce::JSON::toString(searchRequest).toUTF8(),
        .withRequestHeaders = headers,
        .withConnectionTimeoutMs = 30000,
        .withResponseTimeoutMs = 60000
    });
    
    // Perform search in background
    juce::Thread::launch([this, searchUrl, options = std::move(options)]() {
        try {
            auto stream = std::make_unique<juce::WebInputStream>(searchUrl, *options);
            
            if (stream->connect(nullptr)) {
                int responseCode = stream->getStatusCode();
                
                if (responseCode == 200) {
                    juce::String response = stream->readEntireStreamAsString();
                    auto json = juce::JSON::parse(response);
                    
                    if (json.isObject()) {
                        auto plugins = json.getProperty("plugins", juce::var());
                        if (plugins.isArray()) {
                            for (int i = 0; i < plugins.size(); ++i) {
                                PluginInfo info = parsePluginInfo(plugins[i]);
                                currentSearchResults.push_back(info);
                            }
                        }
                        
                        // Notify on message thread
                        juce::MessageManager::callAsync([this]() {
                            notifySearchCompleted(currentSearchResults);
                        });
                    }
                }
            }
        } catch (const std::exception& e) {
            juce::MessageManager::callAsync([this, error = juce::String(e.what())]() {
                notifySearchError(error);
            });
        }
    });
}

void PluginMarketplace::getFeaturedPlugins() {
    juce::URL url(config.apiEndpoint + "/featured");
    
    juce::StringPairArray headers;
    headers.set("Authorization", "Bearer " + config.apiKey);
    
    auto options = std::make_unique<juce::URL::InputStreamOptions>(juce::URL::InputStreamOptions{
        .withRequestHeaders = headers,
        .withConnectionTimeoutMs = 30000,
        .withResponseTimeoutMs = 60000
    });
    
    juce::Thread::launch([this, url, options = std::move(options)]() {
        try {
            auto stream = std::make_unique<juce::WebInputStream>(url, *options);
            
            if (stream->connect(nullptr) && stream->getStatusCode() == 200) {
                juce::String response = stream->readEntireStreamAsString();
                auto json = juce::JSON::parse(response);
                
                std::vector<PluginInfo> featured;
                auto plugins = json.getProperty("plugins", juce::var());
                if (plugins.isArray()) {
                    for (int i = 0; i < plugins.size(); ++i) {
                        featured.push_back(parsePluginInfo(plugins[i]));
                    }
                }
                
                juce::MessageManager::callAsync([this, featured]() {
                    notifyFeaturedPluginsLoaded(featured);
                });
            }
        } catch (const std::exception& e) {
            juce::Logger::writeToLog("Featured plugins fetch error: " + juce::String(e.what()));
        }
    });
}

void PluginMarketplace::getPluginDetails(const juce::String& pluginId) {
    juce::URL url(config.apiEndpoint + "/plugins/" + pluginId);
    
    juce::StringPairArray headers;
    headers.set("Authorization", "Bearer " + config.apiKey);
    
    auto options = std::make_unique<juce::URL::InputStreamOptions>(juce::URL::InputStreamOptions{
        .withRequestHeaders = headers,
        .withConnectionTimeoutMs = 30000,
        .withResponseTimeoutMs = 60000
    });
    
    juce::Thread::launch([this, url, options = std::move(options)]() {
        try {
            auto stream = std::make_unique<juce::WebInputStream>(url, *options);
            
            if (stream->connect(nullptr) && stream->getStatusCode() == 200) {
                juce::String response = stream->readEntireStreamAsString();
                auto json = juce::JSON::parse(response);
                
                PluginInfo info = parsePluginInfo(json);
                
                juce::MessageManager::callAsync([this, info]() {
                    notifyPluginDetailsLoaded(info);
                });
            }
        } catch (const std::exception& e) {
            juce::Logger::writeToLog("Plugin details fetch error: " + juce::String(e.what()));
        }
    });
}

bool PluginMarketplace::downloadPlugin(const juce::String& pluginId, 
                                     const juce::String& version,
                                     const juce::File& destination) {
    // Get download URL
    juce::URL url(config.apiEndpoint + "/plugins/" + pluginId + "/download");
    url = url.withParameter("version", version);
    
    juce::StringPairArray headers;
    headers.set("Authorization", "Bearer " + config.apiKey);
    
    auto options = std::make_unique<juce::URL::InputStreamOptions>(juce::URL::InputStreamOptions{
        .withRequestHeaders = headers,
        .withConnectionTimeoutMs = 30000,
        .withResponseTimeoutMs = 60000
    });
    
    // Start download
    DownloadProgress progress;
    progress.downloadId = juce::Uuid().toString();
    progress.pluginId = pluginId;
    progress.version = version;
    progress.destination = destination;
    progress.status = DownloadStatus::Downloading;
    progress.startTime = juce::Time::getCurrentTime();
    
    activeDownloads[progress.downloadId] = progress;
    
    return downloadManager->startDownload(url, destination, progress.downloadId);
}

bool PluginMarketplace::installPlugin(const juce::File& pluginFile, 
                                    const juce::String& pluginId) {
    // Verify plugin
    if (!verifyPlugin(pluginFile)) {
        return false;
    }
    
    // Extract to plugins directory
    juce::File pluginsDir = getPluginsDirectory();
    juce::File installDir = pluginsDir.getChildFile(pluginId);
    
    if (!installDir.createDirectory()) {
        return false;
    }
    
    // Extract archive
    if (!extractPlugin(pluginFile, installDir)) {
        return false;
    }
    
    // Register plugin
    registerInstalledPlugin(pluginId, installDir);
    
    // Scan for new plugins
    scanForPlugins();
    
    notifyPluginInstalled(pluginId);
    return true;
}

void PluginMarketplace::uninstallPlugin(const juce::String& pluginId) {
    // Find plugin
    auto it = installedPlugins.find(pluginId);
    if (it == installedPlugins.end()) return;
    
    // Remove files
    if (it->second.installDirectory.exists()) {
        it->second.installDirectory.deleteRecursively();
    }
    
    // Unregister
    installedPlugins.erase(it);
    
    // Save plugin list
    saveInstalledPlugins();
    
    notifyPluginUninstalled(pluginId);
}

void PluginMarketplace::updatePlugin(const juce::String& pluginId) {
    // Check for updates
    getPluginDetails(pluginId);
}

bool PluginMarketplace::purchasePlugin(const juce::String& pluginId, 
                                     const juce::String& paymentMethod) {
    // Process payment
    juce::URL url(config.apiEndpoint + "/purchase");
    
    juce::DynamicObject::Ptr purchaseData = new juce::DynamicObject();
    purchaseData->setProperty("pluginId", pluginId);
    purchaseData->setProperty("paymentMethod", paymentMethod);
    purchaseData->setProperty("userId", config.userId);
    
    juce::StringPairArray headers;
    headers.set("Authorization", "Bearer " + config.apiKey);
    headers.set("Content-Type", "application/json");
    
    auto options = std::make_unique<juce::URL::InputStreamOptions>(juce::URL::InputStreamOptions{
        .withPostData = juce::JSON::toString(purchaseData).toUTF8(),
        .withRequestHeaders = headers,
        .withConnectionTimeoutMs = 30000,
        .withResponseTimeoutMs = 60000
    });
    
    juce::Thread::launch([this, url, options = std::move(options), pluginId]() {
        try {
            auto stream = std::make_unique<juce::WebInputStream>(url, *options);
            
            if (stream->connect(nullptr) && stream->getStatusCode() == 200) {
                juce::String response = stream->readEntireStreamAsString();
                auto json = juce::JSON::parse(response);
                
                if (json.getProperty("success", false)) {
                    juce::String licenseKey = json.getProperty("licenseKey", "");
                    
                    // Store license
                    licenseManager->storeLicense(pluginId, licenseKey);
                    
                    juce::MessageManager::callAsync([this, pluginId]() {
                        notifyPluginPurchased(pluginId);
                    });
                }
            }
        } catch (const std::exception& e) {
            juce::Logger::writeToLog("Purchase error: " + juce::String(e.what()));
        }
    });
    
    return true;
}

bool PluginMarketplace::activateLicense(const juce::String& pluginId, 
                                      const juce::String& licenseKey) {
    return licenseManager->activateLicense(pluginId, licenseKey);
}

void PluginMarketplace::deactivateLicense(const juce::String& pluginId) {
    licenseManager->deactivateLicense(pluginId);
}

bool PluginMarketplace::isLicenseValid(const juce::String& pluginId) const {
    return licenseManager->isLicenseValid(pluginId);
}

void PluginMarketplace::ratePlugin(const juce::String& pluginId, 
                                 int rating,
                                 const juce::String& review) {
    juce::URL url(config.apiEndpoint + "/plugins/" + pluginId + "/rate");
    
    juce::DynamicObject::Ptr ratingData = new juce::DynamicObject();
    ratingData->setProperty("pluginId", pluginId);
    ratingData->setProperty("rating", rating);
    ratingData->setProperty("review", review);
    ratingData->setProperty("userId", config.userId);
    
    juce::StringPairArray headers;
    headers.set("Authorization", "Bearer " + config.apiKey);
    headers.set("Content-Type", "application/json");
    
    auto options = std::make_unique<juce::URL::InputStreamOptions>(juce::URL::InputStreamOptions{
        .withPostData = juce::JSON::toString(ratingData).toUTF8(),
        .withRequestHeaders = headers,
        .withConnectionTimeoutMs = 30000,
        .withResponseTimeoutMs = 60000
    });
    
    juce::Thread::launch([this, url, options = std::move(options)]() {
        try {
            auto stream = std::make_unique<juce::WebInputStream>(url, *options);
            stream->connect(nullptr);
        } catch (const std::exception& e) {
            juce::Logger::writeToLog("Rating error: " + juce::String(e.what()));
        }
    });
}

void PluginMarketplace::addToWishlist(const juce::String& pluginId) {
    wishlist.push_back(pluginId);
    saveWishlist();
    notifyWishlistUpdated();
}

void PluginMarketplace::removeFromWishlist(const juce::String& pluginId) {
    wishlist.erase(std::remove(wishlist.begin(), wishlist.end(), pluginId), wishlist.end());
    saveWishlist();
    notifyWishlistUpdated();
}

bool PluginMarketplace::isInWishlist(const juce::String& pluginId) const {
    return std::find(wishlist.begin(), wishlist.end(), pluginId) != wishlist.end();
}

std::vector<PluginInfo> PluginMarketplace::getSearchResults() const {
    return currentSearchResults;
}

std::vector<PluginInfo> PluginMarketplace::getInstalledPlugins() const {
    std::vector<PluginInfo> plugins;
    
    for (const auto& pair : installedPlugins) {
        plugins.push_back(pair.second);
    }
    
    return plugins;
}

PluginInfo PluginMarketplace::getPluginInfo(const juce::String& pluginId) const {
    auto it = installedPlugins.find(pluginId);
    if (it != installedPlugins.end()) {
        return it->second;
    }
    
    // Check search results
    for (const auto& plugin : currentSearchResults) {
        if (plugin.pluginId == pluginId) {
            return plugin;
        }
    }
    
    return {};
}

std::vector<DownloadProgress> PluginMarketplace::getActiveDownloads() const {
    std::vector<DownloadProgress> downloads;
    
    for (const auto& pair : activeDownloads) {
        downloads.push_back(pair.second);
    }
    
    return downloads;
}

std::vector<juce::String> PluginMarketplace::getWishlist() const {
    return wishlist;
}

void PluginMarketplace::scanForPlugins() {
    juce::File pluginsDir = getPluginsDirectory();
    
    if (!pluginsDir.exists()) {
        pluginsDir.createDirectory();
        return;
    }
    
    // Scan for plugin directories
    juce::Array<juce::File> pluginDirs;
    pluginsDir.findChildFiles(pluginDirs, juce::File::findDirectories, false);
    
    for (const auto& dir : pluginDirs) {
        // Look for plugin manifest
        juce::File manifestFile = dir.getChildFile("plugin.json");
        
        if (manifestFile.exists()) {
            try {
                auto content = manifestFile.loadFileAsString();
                auto json = juce::JSON::parse(content);
                
                if (json.isObject()) {
                    PluginInfo info = parsePluginInfo(json);
                    info.installDirectory = dir;
                    info.isInstalled = true;
                    
                    installedPlugins[info.pluginId] = info;
                }
            } catch (const std::exception& e) {
                juce::Logger::writeToLog("Invalid plugin manifest: " + juce::String(e.what()));
            }
        }
    }
    
    saveInstalledPlugins();
    notifyPluginsUpdated();
}

void PluginMarketplace::checkForUpdates() {
    for (const auto& pair : installedPlugins) {
        const auto& plugin = pair.second;
        getPluginDetails(plugin.pluginId);
    }
}

void PluginMarketplace::cancelDownload(const juce::String& downloadId) {
    downloadManager->cancelDownload(downloadId);
    activeDownloads.erase(downloadId);
}

void PluginMarketplace::pauseDownload(const juce::String& downloadId) {
    downloadManager->pauseDownload(downloadId);
}

void PluginMarketplace::resumeDownload(const juce::String& downloadId) {
    downloadManager->resumeDownload(downloadId);
}

void PluginMarketplace::timerCallback() {
    // Update download progress
    for (auto& pair : activeDownloads) {
        auto& progress = pair.second;
        
        if (progress.status == DownloadStatus::Downloading) {
            progress.bytesDownloaded = downloadManager->getDownloadedBytes(pair.first);
            progress.totalBytes = downloadManager->getTotalBytes(pair.first);
            
            if (progress.bytesDownloaded >= progress.totalBytes) {
                progress.status = DownloadStatus::Completed;
                progress.endTime = juce::Time::getCurrentTime();
                
                // Install plugin
                installPlugin(progress.destination, progress.pluginId);
            }
        }
    }
    
    notifyDownloadsUpdated();
}

void PluginMarketplace::addListener(Listener* listener) {
    listeners.push_back(listener);
}

void PluginMarketplace::removeListener(Listener* listener) {
    listeners.erase(std::remove(listeners.begin(), listeners.end(), listener), listeners.end());
}

// Helper methods
PluginInfo PluginMarketplace::parsePluginInfo(const juce::var& json) const {
    PluginInfo info;
    
    info.pluginId = json.getProperty("id", "");
    info.name = json.getProperty("name", "");
    info.version = json.getProperty("version", "");
    info.description = json.getProperty("description", "");
    info.author = json.getProperty("author", "");
    info.category = json.getProperty("category", "");
    info.price = json.getProperty("price", 0.0f);
    info.rating = json.getProperty("rating", 0.0f);
    info.downloadCount = json.getProperty("downloadCount", 0);
    info.isFree = json.getProperty("isFree", false);
    info.hasTrial = json.getProperty("hasTrial", false);
    info.licenseType = json.getProperty("licenseType", "");
    
    // Parse tags
    auto tagsArray = json.getProperty("tags", juce::var());
    if (tagsArray.isArray()) {
        for (int i = 0; i < tagsArray.size(); ++i) {
            info.tags.push_back(tagsArray[i]);
        }
    }
    
    // Parse requirements
    auto requirements = json.getProperty("requirements", juce::var());
    if (requirements.isObject()) {
        info.minOSVersion = requirements.getProperty("minOSVersion", "");
        info.requiredRAM = requirements.getProperty("requiredRAM", "");
        info.requiredDiskSpace = requirements.getProperty("requiredDiskSpace", "");
    }
    
    // Parse URLs
    auto urls = json.getProperty("urls", juce::var());
    if (urls.isObject()) {
        info.downloadUrl = urls.getProperty("download", "");
        info.trialUrl = urls.getProperty("trial", "");
        info.documentationUrl = urls.getProperty("documentation", "");
        info.supportUrl = urls.getProperty("support", "");
    }
    
    return info;
}

juce::File PluginMarketplace::getPluginsDirectory() const {
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
           .getChildFile("ZenithDAW")
           .getChildFile("Plugins");
}

bool PluginMarketplace::verifyPlugin(const juce::File& pluginFile) {
    // Verify file signature
    // This would implement actual verification
    return pluginFile.exists() && pluginFile.getSize() > 0;
}

bool PluginMarketplace::extractPlugin(const juce::File& archiveFile, 
                                    const juce::File& destination) {
    // Extract archive
    // This would use actual extraction library
    return true;
}

void PluginMarketplace::registerInstalledPlugin(const juce::String& pluginId, 
                                              const juce::File& installDir) {
    PluginInfo info;
    info.pluginId = pluginId;
    info.installDirectory = installDir;
    info.isInstalled = true;
    info.installDate = juce::Time::getCurrentTime();
    
    installedPlugins[pluginId] = info;
    saveInstalledPlugins();
}

void PluginMarketplace::loadLocalPlugins() {
    juce::File pluginsFile = getPluginsDirectory().getChildFile("installed_plugins.json");
    
    if (pluginsFile.exists()) {
        try {
            auto content = pluginsFile.loadFileAsString();
            auto json = juce::JSON::parse(content);
            
            if (json.isObject()) {
                auto plugins = json.getProperty("plugins", juce::var());
                if (plugins.isArray()) {
                    for (int i = 0; i < plugins.size(); ++i) {
                        PluginInfo info = parsePluginInfo(plugins[i]);
                        installedPlugins[info.pluginId] = info;
                    }
                }
            }
        } catch (const std::exception& e) {
            juce::Logger::writeToLog("Failed to load local plugins: " + juce::String(e.what()));
        }
    }
}

void PluginMarketplace::saveInstalledPlugins() {
    juce::DynamicObject::Ptr data = new juce::DynamicObject();
    juce::Array<juce::var> pluginArray;
    
    for (const auto& pair : installedPlugins) {
        pluginArray.add(pluginInfoToJSON(pair.second));
    }
    
    data->setProperty("plugins", pluginArray);
    
    juce::File pluginsFile = getPluginsDirectory().getChildFile("installed_plugins.json");
    pluginsFile.replaceWithText(juce::JSON::toString(data));
}

void PluginMarketplace::saveWishlist() {
    juce::DynamicObject::Ptr data = new juce::DynamicObject();
    data->setProperty("wishlist", juce::var(wishlist.data(), static_cast<int>(wishlist.size())));
    
    juce::File wishlistFile = getPluginsDirectory().getChildFile("wishlist.json");
    wishlistFile.replaceWithText(juce::JSON::toString(data));
}

juce::var PluginMarketplace::pluginInfoToJSON(const PluginInfo& info) const {
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    obj->setProperty("pluginId", info.pluginId);
    obj->setProperty("name", info.name);
    obj->setProperty("version", info.version);
    obj->setProperty("installDate", info.installDate.toMilliseconds());
    return obj;
}

// Notification methods
void PluginMarketplace::notifySearchCompleted(const std::vector<PluginInfo>& results) {
    for (auto* listener : listeners) {
        listener->searchCompleted(results);
    }
}

void PluginMarketplace::notifySearchError(const juce::String& error) {
    for (auto* listener : listeners) {
        listener->searchError(error);
    }
}

void PluginMarketplace::notifyPluginDetailsLoaded(const PluginInfo& info) {
    for (auto* listener : listeners) {
        listener->pluginDetailsLoaded(info);
    }
}

void PluginMarketplace::notifyPluginDownloaded(const juce::String& pluginId) {
    for (auto* listener : listeners) {
        listener->pluginDownloaded(pluginId);
    }
}

void PluginMarketplace::notifyPluginInstalled(const juce::String& pluginId) {
    for (auto* listener : listeners) {
        listener->pluginInstalled(pluginId);
    }
}

void PluginMarketplace::notifyPluginUninstalled(const juce::String& pluginId) {
    for (auto* listener : listeners) {
        listener->pluginUninstalled(pluginId);
    }
}

void PluginMarketplace::notifyPluginPurchased(const juce::String& pluginId) {
    for (auto* listener : listeners) {
        listener->pluginPurchased(pluginId);
    }
}

void PluginMarketplace::notifyPluginsUpdated() {
    for (auto* listener : listeners) {
        listener->pluginsUpdated();
    }
}

void PluginMarketplace::notifyDownloadsUpdated() {
    for (auto* listener : listeners) {
        listener->downloadsUpdated();
    }
}

void PluginMarketplace::notifyWishlistUpdated() {
    for (auto* listener : listeners) {
        listener->wishlistUpdated();
    }
}

void PluginMarketplace::notifyFeaturedPluginsLoaded(const std::vector<PluginInfo>& plugins) {
    for (auto* listener : listeners) {
        listener->featuredPluginsLoaded(plugins);
    }
}

// DownloadManager Implementation
PluginMarketplace::DownloadManager::DownloadManager() {
    startThread();
}

PluginMarketplace::DownloadManager::~DownloadManager() {
    stopThread(1000);
}

bool PluginMarketplace::DownloadManager::startDownload(const juce::URL& url, 
                                                     const juce::File& destination,
                                                     const juce::String& downloadId) {
    DownloadInfo info;
    info.url = url;
    info.destination = destination;
    info.downloadId = downloadId;
    info.status = DownloadStatus::Queued;
    
    {
        std::lock_guard<std::mutex> lock(downloadsMutex);
        downloads[downloadId] = info;
    }
    
    notify();
    return true;
}

void PluginMarketplace::DownloadManager::cancelDownload(const juce::String& downloadId) {
    std::lock_guard<std::mutex> lock(downloadsMutex);
    
    auto it = downloads.find(downloadId);
    if (it != downloads.end()) {
        it->second.status = DownloadStatus::Cancelled;
        if (it->second.stream) {
            it->second.stream->cancel();
        }
    }
}

void PluginMarketplace::DownloadManager::pauseDownload(const juce::String& downloadId) {
    std::lock_guard<std::mutex> lock(downloadsMutex);
    
    auto it = downloads.find(downloadId);
    if (it != downloads.end()) {
        it->second.status = DownloadStatus::Paused;
    }
}

void PluginMarketplace::DownloadManager::resumeDownload(const juce::String& downloadId) {
    std::lock_guard<std::mutex> lock(downloadsMutex);
    
    auto it = downloads.find(downloadId);
    if (it != downloads.end()) {
        it->second.status = DownloadStatus::Queued;
        notify();
    }
}

int64 PluginMarketplace::DownloadManager::getDownloadedBytes(const juce::String& downloadId) const {
    std::lock_guard<std::mutex> lock(downloadsMutex);
    
    auto it = downloads.find(downloadId);
    if (it != downloads.end()) {
        return it->second.bytesDownloaded;
    }
    
    return 0;
}

int64 PluginMarketplace::DownloadManager::getTotalBytes(const juce::String& downloadId) const {
    std::lock_guard<std::mutex> lock(downloadsMutex);
    
    auto it = downloads.find(downloadId);
    if (it != downloads.end()) {
        return it->second.totalBytes;
    }
    
    return 0;
}

void PluginMarketplace::DownloadManager::run() {
    while (!threadShouldExit()) {
        std::unique_lock<std::mutex> lock(downloadsMutex);
        
        // Find next download
        DownloadInfo* nextDownload = nullptr;
        for (auto& pair : downloads) {
            if (pair.second.status == DownloadStatus::Queued) {
                nextDownload = &pair.second;
                break;
            }
        }
        
        if (!nextDownload) {
            wait(100);
            continue;
        }
        
        // Start download
        nextDownload->status = DownloadStatus::Downloading;
        lock.unlock();
        
        try {
            auto stream = std::make_unique<juce::WebInputStream>(nextDownload->url, false);
            
            if (stream->connect(nullptr)) {
                nextDownload->totalBytes = stream->getTotalLength();
                nextDownload->stream = stream.get();
                
                // Create output file
                juce::FileOutputStream output(nextDownload->destination);
                output.setPosition(0);
                output.truncate();
                
                // Download data
                uint8_t buffer[8192];
                int bytesRead;
                
                while (!threadShouldExit() && 
                       (bytesRead = stream->read(buffer, sizeof(buffer))) > 0) {
                    output.write(buffer, bytesRead);
                    nextDownload->bytesDownloaded += bytesRead;
                    
                    // Check if paused
                    if (nextDownload->status == DownloadStatus::Paused) {
                        break;
                    }
                }
                
                if (nextDownload->status != DownloadStatus::Paused && 
                    nextDownload->status != DownloadStatus::Cancelled) {
                    nextDownload->status = DownloadStatus::Completed;
                }
            } else {
                nextDownload->status = DownloadStatus::Failed;
            }
        } catch (const std::exception& e) {
            nextDownload->status = DownloadStatus::Failed;
            juce::Logger::writeToLog("Download error: " + juce::String(e.what()));
        }
        
        lock.lock();
        nextDownload->stream = nullptr;
    }
}

void PluginMarketplace::DownloadManager::addListener(Listener* listener) {
    listeners.push_back(listener);
}

void PluginMarketplace::DownloadManager::removeListener(Listener* listener) {
    listeners.erase(std::remove(listeners.begin(), listeners.end(), listener), listeners.end());
}

// LicenseManager Implementation
PluginMarketplace::LicenseManager::LicenseManager() {
    loadLicenses();
}

bool PluginMarketplace::LicenseManager::activateLicense(const juce::String& pluginId, 
                                                      const juce::String& licenseKey) {
    // Validate license with server
    // For now, just store locally
    licenses[pluginId] = licenseKey;
    saveLicenses();
    return true;
}

void PluginMarketplace::LicenseManager::deactivateLicense(const juce::String& pluginId) {
    licenses.erase(pluginId);
    saveLicenses();
}

bool PluginMarketplace::LicenseManager::isLicenseValid(const juce::String& pluginId) const {
    auto it = licenses.find(pluginId);
    return it != licenses.end() && !it->second.isEmpty();
}

void PluginMarketplace::LicenseManager::storeLicense(const juce::String& pluginId, 
                                                   const juce::String& licenseKey) {
    licenses[pluginId] = licenseKey;
    saveLicenses();
}

void PluginMarketplace::LicenseManager::loadLicenses() {
    juce::File licensesFile = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                              .getChildFile("ZenithDAW")
                              .getChildFile("licenses.json");
    
    if (licensesFile.exists()) {
        try {
            auto content = licensesFile.loadFileAsString();
            auto json = juce::JSON::parse(content);
            
            if (json.isObject()) {
                auto props = json.getDynamicObject()->getProperties();
                for (const auto& prop : props) {
                    licenses[prop.name.toString()] = prop.value.toString();
                }
            }
        } catch (const std::exception& e) {
            juce::Logger::writeToLog("Failed to load licenses: " + juce::String(e.what()));
        }
    }
}

void PluginMarketplace::LicenseManager::saveLicenses() {
    juce::DynamicObject::Ptr data = new juce::DynamicObject();
    
    for (const auto& pair : licenses) {
        data->setProperty(pair.first, pair.second);
    }
    
    juce::File licensesFile = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                              .getChildFile("ZenithDAW")
                              .getChildFile("licenses.json");
    
    licensesFile.replaceWithText(juce::JSON::toString(data));
}

} // namespace marketplace
} // namespace zenith
