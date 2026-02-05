/*
  ==============================================================================

    UpdateService.cpp
    Created: 2026-02-04
    Author:  Zenith DAW Team

    Production-ready update checking implementation with HTTP networking,
    semantic versioning, and caching.

  ==============================================================================
*/

#include "UpdateService.h"
#include <juce_cryptography/juce_cryptography.h>

namespace zenith {
namespace network {

//==============================================================================
// Semantic Version Implementation
//==============================================================================

SemanticVersion::SemanticVersion(const juce::String& versionString) {
    juce::String v = versionString.trim();
    
    // Remove 'v' prefix if present
    if (v.startsWithChar('v') || v.startsWithChar('V'))
        v = v.substring(1);
    
    // Split into version + metadata
    int plusPos = v.indexOfChar('+');
    if (plusPos >= 0) {
        buildMetadata = v.substring(plusPos + 1);
        v = v.substring(0, plusPos);
    }
    
    // Split into version + prerelease
    int hyphenPos = v.indexOfChar('-');
    if (hyphenPos >= 0) {
        prerelease = v.substring(hyphenPos + 1);
        v = v.substring(0, hyphenPos);
    }
    
    // Parse major.minor.patch
    auto parts = juce::StringArray::fromTokens(v, ".", "");
    if (parts.size() >= 1) major = parts[0].getIntValue();
    if (parts.size() >= 2) minor = parts[1].getIntValue();
    if (parts.size() >= 3) patch = parts[2].getIntValue();
}

juce::String SemanticVersion::toString() const {
    juce::String result = juce::String(major) + "." + juce::String(minor) + "." + juce::String(patch);
    if (prerelease.isNotEmpty())
        result += "-" + prerelease;
    if (buildMetadata.isNotEmpty())
        result += "+" + buildMetadata;
    return result;
}

bool SemanticVersion::operator<(const SemanticVersion& other) const {
    if (major != other.major) return major < other.major;
    if (minor != other.minor) return minor < other.minor;
    if (patch != other.patch) return patch < other.patch;
    
    // A version without prerelease is greater than one with
    if (prerelease.isEmpty() && !other.prerelease.isEmpty()) return false;
    if (!prerelease.isEmpty() && other.prerelease.isEmpty()) return true;
    
    // Compare prerelease versions
    int prereleaseCmp = comparePrerelease(prerelease, other.prerelease);
    return prereleaseCmp < 0;
}

bool SemanticVersion::operator>(const SemanticVersion& other) const {
    return other < *this;
}

bool SemanticVersion::operator==(const SemanticVersion& other) const {
    return major == other.major && 
           minor == other.minor && 
           patch == other.patch &&
           prerelease == other.prerelease;
    // Note: build metadata is ignored in comparison per semver spec
}

int SemanticVersion::comparePrerelease(const juce::String& a, const juce::String& b) const {
    if (a == b) return 0;
    if (a.isEmpty()) return 1;  // No prerelease is greater
    if (b.isEmpty()) return -1;
    
    auto partsA = juce::StringArray::fromTokens(a, ".", "");
    auto partsB = juce::StringArray::fromTokens(b, ".", "");
    
    int maxParts = std::max(partsA.size(), partsB.size());
    
    for (int i = 0; i < maxParts; ++i) {
        if (i >= partsA.size()) return -1;
        if (i >= partsB.size()) return 1;
        
        const juce::String& partA = partsA[i];
        const juce::String& partB = partsB[i];
        
        // Numeric identifiers have lower precedence than non-numeric
        bool isNumA = partA.containsOnly("0123456789");
        bool isNumB = partB.containsOnly("0123456789");
        
        if (isNumA && isNumB) {
            int numA = partA.getIntValue();
            int numB = partB.getIntValue();
            if (numA != numB) return numA - numB;
        } else if (isNumA) {
            return -1;
        } else if (isNumB) {
            return 1;
        } else {
            int cmp = partA.compare(partB);
            if (cmp != 0) return cmp;
        }
    }
    
    return 0;
}

//==============================================================================
// Release Channel
//==============================================================================

juce::String channelToString(ReleaseChannel channel) {
    switch (channel) {
        case ReleaseChannel::Stable: return "stable";
        case ReleaseChannel::Beta: return "beta";
        case ReleaseChannel::Nightly: return "nightly";
        case ReleaseChannel::Internal: return "internal";
    }
    return "stable";
}

ReleaseChannel channelFromString(const juce::String& str) {
    juce::String s = str.toLowerCase();
    if (s == "stable") return ReleaseChannel::Stable;
    if (s == "beta") return ReleaseChannel::Beta;
    if (s == "nightly") return ReleaseChannel::Nightly;
    if (s == "internal") return ReleaseChannel::Internal;
    return ReleaseChannel::Stable;
}

//==============================================================================
// Update Info
//==============================================================================

juce::String UpdateInfo::getDownloadUrlForCurrentPlatform() const {
#if JUCE_WINDOWS
    return windowsUrl.isEmpty() ? downloadUrl : windowsUrl;
#elif JUCE_MAC
    return macUrl.isEmpty() ? downloadUrl : macUrl;
#elif JUCE_LINUX
    return linuxUrl.isEmpty() ? downloadUrl : linuxUrl;
#else
    return downloadUrl;
#endif
}

bool UpdateInfo::isNewerThanCurrent() const {
    return latestVersion > currentVersion;
}

//==============================================================================
// Update Service Implementation
//==============================================================================

UpdateService::UpdateService() 
    : juce::Thread("UpdateServiceThread") 
{
    // Set default current version from JUCE_APPLICATION_VERSION_STRING if available
    #ifdef JUCE_APPLICATION_VERSION_STRING
    currentVersion_ = SemanticVersion(JUCE_APPLICATION_VERSION_STRING);
    #endif
}

UpdateService::~UpdateService() {
    stopThread(5000);
}

void UpdateService::setUpdateUrl(const juce::String& url) {
    updateUrl_ = url;
}

void UpdateService::setReleaseChannel(ReleaseChannel channel) {
    channel_ = channel;
}

void UpdateService::setCurrentVersion(const juce::String& version) {
    currentVersion_ = SemanticVersion(version);
}

void UpdateService::setApiKey(const juce::String& apiKey) {
    apiKey_ = apiKey;
}

void UpdateService::setCacheEnabled(bool enabled) {
    cacheEnabled_ = enabled;
}

void UpdateService::setCacheExpiration(int minutes) {
    cacheExpirationMinutes_ = minutes;
}

void UpdateService::checkForUpdates(std::function<void(const UpdateInfo&)> onCompletion,
                                   std::function<void(const juce::String&)> onError) {
    if (isChecking_.exchange(true)) {
        if (onError)
            onError("Update check already in progress");
        return;
    }
    
    completionCallback_ = onCompletion;
    errorCallback_ = onError;
    
    startThread();
}

std::optional<UpdateInfo> UpdateService::checkForUpdatesSync() {
    isChecking_ = true;
    auto result = fetchUpdateInfoFromServer();
    isChecking_ = false;
    return result;
}

void UpdateService::run() {
    auto result = fetchUpdateInfoFromServer();
    isChecking_ = false;
    
    juce::MessageManager::callAsync([this, result]() {
        if (result.has_value()) {
            lastResult_ = result;
            if (completionCallback_)
                completionCallback_(*result);
        } else {
            if (errorCallback_)
                errorCallback_("Failed to fetch update information");
        }
    });
}

std::optional<UpdateInfo> UpdateService::fetchUpdateInfoFromServer() {
    // Try cache first if enabled and recent
    if (cacheEnabled_ && isCacheValid()) {
        auto cached = loadFromCache();
        if (cached.has_value()) {
            return cached;
        }
    }
    
    // Build request URL
    juce::String url = buildRequestUrl();
    
    // Attempt with retries
    int retryDelay = INITIAL_RETRY_DELAY_MS;
    
    for (int attempt = 0; attempt < MAX_RETRIES; ++attempt) {
        juce::URL updateUrl(url);
        
        // Add headers
        juce::StringArray headers = buildRequestHeaders();
        
        // Create input stream with timeout
        std::unique_ptr<juce::InputStream> stream(updateUrl.createInputStream(
            juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
            .withConnectionTimeoutMs(10000)
            .withExtraHeaders(headers.joinIntoString("\n"))
        ));
        
        if (stream != nullptr) {
            auto jsonString = stream->readEntireStreamAsString();
            
            // Check HTTP status
            if (jsonString.isEmpty()) {
                // Wait before retry
                juce::Thread::sleep(retryDelay);
                retryDelay *= 2; // Exponential backoff
                continue;
            }
            
            // Parse JSON response
            auto json = juce::JSON::parse(jsonString);
            
            if (json.isObject()) {
                UpdateInfo info = parseUpdateResponse(json);
                info.currentVersion = currentVersion_;
                
                // Determine if update is available
                info.available = info.isNewerThanCurrent();
                
                // Cache the result
                if (cacheEnabled_) {
                    saveToCache(info);
                }
                
                return info;
            } else {
                if (errorCallback_) {
                    juce::MessageManager::callAsync([this]() {
                        errorCallback_("Invalid response format from update server");
                    });
                }
                return std::nullopt;
            }
        } else {
            // Connection failed - retry
            juce::Thread::sleep(retryDelay);
            retryDelay *= 2;
        }
    }
    
    // All retries exhausted - try to use cached data even if expired
    if (cacheEnabled_) {
        auto cached = loadFromCache();
        if (cached.has_value()) {
            return cached;
        }
    }
    
    if (errorCallback_) {
        juce::MessageManager::callAsync([this]() {
            errorCallback_("Failed to connect to update server after " + 
                          juce::String(MAX_RETRIES) + " attempts");
        });
    }
    
    return std::nullopt;
}

juce::String UpdateService::buildRequestUrl() const {
    juce::String url = updateUrl_;
    
    // Add query parameters
    url += "?platform=";
#if JUCE_WINDOWS
    url += "windows";
#elif JUCE_MAC
    url += "macos";
#elif JUCE_LINUX
    url += "linux";
#else
    url += "unknown";
#endif
    
    url += "&arch=";
#if JUCE_64BIT
    url += "x64";
#else
    url += "x86";
#endif
    
    url += "&channel=" + channelToString(channel_);
    url += "&current_version=" + currentVersion_.toString();
    
    return url;
}

UpdateInfo UpdateService::parseUpdateResponse(const juce::var& json) {
    UpdateInfo info;
    
    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return info;
    
    // Parse version
    juce::String versionStr = obj->getProperty("version").toString();
    info.latestVersion = SemanticVersion(versionStr);
    
    // Parse other fields
    info.downloadUrl = obj->getProperty("download_url").toString();
    info.releaseNotes = obj->getProperty("release_notes").toString();
    info.releaseDate = obj->getProperty("release_date").toString();
    info.checksum = obj->getProperty("checksum").toString();
    info.fileSize = static_cast<juce::int64>(obj->getProperty("file_size"));
    info.isMandatory = static_cast<bool>(obj->getProperty("mandatory"));
    info.channel = channelFromString(obj->getProperty("channel").toString());
    
    // Platform-specific URLs
    info.windowsUrl = obj->getProperty("windows_url").toString();
    info.macUrl = obj->getProperty("mac_url").toString();
    info.linuxUrl = obj->getProperty("linux_url").toString();
    
    return info;
}

juce::StringArray UpdateService::buildRequestHeaders() const {
    juce::StringArray headers;
    
    headers.add("Accept: application/json");
    headers.add("User-Agent: ZenithDAW/" + currentVersion_.toString());
    headers.add("X-Client-Version: " + currentVersion_.toString());
    headers.add("X-Platform: " + juce::SystemStats::getOperatingSystemName());
    
    if (apiKey_.isNotEmpty()) {
        headers.add("Authorization: Bearer " + apiKey_);
    }
    
    return headers;
}

//==============================================================================
// Cache Management
//==============================================================================

void UpdateService::saveToCache(const UpdateInfo& info) {
    auto cacheFile = getCacheFile();
    cacheFile.getParentDirectory().createDirectory();
    
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    obj->setProperty("version", info.latestVersion.toString());
    obj->setProperty("download_url", info.downloadUrl);
    obj->setProperty("release_notes", info.releaseNotes);
    obj->setProperty("release_date", info.releaseDate);
    obj->setProperty("checksum", info.checksum);
    obj->setProperty("file_size", static_cast<juce::int64>(info.fileSize));
    obj->setProperty("mandatory", info.isMandatory);
    obj->setProperty("channel", channelToString(info.channel));
    obj->setProperty("timestamp", juce::Time::getCurrentTime().toISO8601(true));
    
    juce::var json(obj.get());
    cacheFile.replaceWithText(juce::JSON::toString(json));
}

std::optional<UpdateInfo> UpdateService::loadFromCache() {
    auto cacheFile = getCacheFile();
    if (!cacheFile.existsAsFile())
        return std::nullopt;
    
    auto json = juce::JSON::parse(cacheFile.loadFileAsString());
    if (!json.isObject())
        return std::nullopt;
    
    UpdateInfo info;
    auto* obj = json.getDynamicObject();
    
    info.latestVersion = SemanticVersion(obj->getProperty("version").toString());
    info.downloadUrl = obj->getProperty("download_url").toString();
    info.releaseNotes = obj->getProperty("release_notes").toString();
    info.releaseDate = obj->getProperty("release_date").toString();
    info.checksum = obj->getProperty("checksum").toString();
    info.fileSize = static_cast<juce::int64>(obj->getProperty("file_size"));
    info.isMandatory = static_cast<bool>(obj->getProperty("mandatory"));
    info.channel = channelFromString(obj->getProperty("channel").toString());
    info.currentVersion = currentVersion_;
    info.available = info.isNewerThanCurrent();
    
    return info;
}

juce::File UpdateService::getCacheFile() const {
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("ZenithDAW")
        .getChildFile("update_cache.json");
}

bool UpdateService::isCacheValid() const {
    auto cacheFile = getCacheFile();
    if (!cacheFile.existsAsFile())
        return false;
    
    auto json = juce::JSON::parse(cacheFile.loadFileAsString());
    if (!json.isObject())
        return false;
    
    auto* obj = json.getDynamicObject();
    juce::String timestamp = obj->getProperty("timestamp").toString();
    
    if (timestamp.isEmpty())
        return false;
    
    auto cacheTime = juce::Time::fromISO8601(timestamp);
    auto expiryTime = cacheTime + juce::RelativeTime::minutes(cacheExpirationMinutes_);
    
    return juce::Time::getCurrentTime() < expiryTime;
}

std::optional<UpdateInfo> UpdateService::getCachedUpdateInfo() const {
    if (isCacheValid()) {
        return const_cast<UpdateService*>(this)->loadFromCache();
    }
    return std::nullopt;
}

void UpdateService::clearCache() {
    getCacheFile().deleteFile();
}

//==============================================================================
// Download Management
//==============================================================================

void UpdateService::downloadUpdate(const UpdateInfo& info,
                                  const juce::File& targetFile,
                                  std::function<void(float)> onProgress,
                                  std::function<void(bool, const juce::String&)> onComplete) {
    // Start download on background thread
    juce::Thread::launch([this, info, targetFile, onProgress, onComplete]() mutable {
        juce::String url = info.getDownloadUrlForCurrentPlatform();
        juce::URL downloadUrl(url);
        
        // Ensure parent directory exists
        targetFile.getParentDirectory().createDirectory();
        
        // Delete existing file
        if (targetFile.existsAsFile())
            targetFile.deleteFile();
        
        // Create output stream
        std::unique_ptr<juce::FileOutputStream> outputStream(targetFile.createOutputStream());
        if (outputStream == nullptr) {
            juce::MessageManager::callAsync([onComplete]() {
                onComplete(false, "Failed to create output file");
            });
            return;
        }
        
        // Create input stream with progress callback
        class DownloadProgressListener : public juce::URL::DownloadTaskListener {
        public:
            DownloadProgressListener(std::function<void(float)> progressCallback, std::function<void(bool, const juce::String&)> completeCallback)
                : progressCallback_(progressCallback), completeCallback_(completeCallback), success_(false) {}

            void progress(juce::URL::DownloadTask* task, juce::int64 bytesDownloaded, juce::int64 totalLength) override {
                if (totalLength > 0 && progressCallback_) {
                    float progress = static_cast<float>(bytesDownloaded) / static_cast<float>(totalLength);
                    juce::MessageManager::callAsync([progressCallback = progressCallback_, progress]() {
                        progressCallback(progress);
                    });
                }
            }

            void finished(juce::URL::DownloadTask* task, bool success) override {
                success_ = success;
                juce::MessageManager::callAsync([completeCallback = completeCallback_, success]() {
                    completeCallback(success, success ? "" : "Download failed");
                });
            }

            bool getSuccess() const { return success_; }

        private:
            std::function<void(float)> progressCallback_;
            std::function<void(bool, const juce::String&)> completeCallback_;
            bool success_;
        };

        DownloadProgressListener listener(onProgress, onComplete);

        juce::URL::DownloadTaskOptions options = juce::URL::DownloadTaskOptions{}.withListener(&listener);
        
        auto task = downloadUrl.downloadToFile(targetFile, options);

        if (task == nullptr) {
            juce::MessageManager::callAsync([onComplete]() {
                onComplete(false, "Failed to start download");
            });
            return;
        }

        // Store the task pointer for cleanup (the listener will handle completion)
        // Note: We need to keep the task alive until completion
        while (!task->isFinished()) {
            juce::Thread::sleep(100);
        }
    });
}

bool UpdateService::verifyChecksum(const juce::File& file, const juce::String& expectedChecksum) {
    if (!file.existsAsFile())
        return false;

    try {
        // Create SHA256 hash from file
        juce::SHA256 sha256(file);
        juce::String calculatedHash = sha256.toHexString().toLowerCase();
        return calculatedHash == expectedChecksum.toLowerCase();
    } catch (...) {
        return false;
    }
}

} // namespace network
} // namespace zenith
