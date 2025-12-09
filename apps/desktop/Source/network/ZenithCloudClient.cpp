/*
  ==============================================================================

    ZenithCloudClient.cpp
    Created: 2025-12-08
    Author:  Zenith DAW - Cloud Collaboration Features

    Implementation of async cloud API client.

  ==============================================================================
*/

#include "ZenithCloudClient.h"

namespace zenith {

//==============================================================================
ZenithCloudClient::ZenithCloudClient()
{
    // Create thread pool with 2 threads for network operations
    threadPool_ = std::make_unique<juce::ThreadPool>(2);
    DBG("ZenithCloudClient: Initialized with base URL " + baseUrl_);
}

ZenithCloudClient::~ZenithCloudClient()
{
    DBG("ZenithCloudClient: Shutting down");
    threadPool_->removeAllJobs(true, 5000);
}

//==============================================================================
void ZenithCloudClient::setBaseUrl(const juce::String& url)
{
    baseUrl_ = url.trimCharactersAtEnd("/");
    DBG("ZenithCloudClient: Base URL set to " + baseUrl_);
}

void ZenithCloudClient::setAuthToken(const juce::String& token)
{
    authToken_ = token;
    DBG("ZenithCloudClient: Auth token " + (token.isEmpty() ? "cleared" : "set"));
}

juce::String ZenithCloudClient::getLastError() const
{
    const juce::ScopedLock lock(errorLock_);
    return lastError_;
}

//==============================================================================
// URL Builder
//==============================================================================

juce::URL ZenithCloudClient::buildUrl(const juce::String& endpoint, const juce::StringPairArray& params) const
{
    juce::String fullUrl = baseUrl_ + endpoint;
    juce::URL url(fullUrl);
    
    for (int i = 0; i < params.size(); ++i)
    {
        url = url.withParameter(params.getAllKeys()[i], params.getAllValues()[i]);
    }
    
    return url;
}

//==============================================================================
// Preset Parsing
//==============================================================================

CloudPreset ZenithCloudClient::parsePreset(const juce::var& presetVar) const
{
    CloudPreset preset;
    
    if (!presetVar.isObject())
        return preset;
    
    preset.id = presetVar.getProperty("_id", "").toString();
    preset.name = presetVar.getProperty("name", "").toString();
    preset.description = presetVar.getProperty("description", "").toString();
    preset.pluginId = presetVar.getProperty("pluginId", "").toString();
    preset.pluginName = presetVar.getProperty("pluginName", "").toString();
    preset.category = presetVar.getProperty("category", "other").toString();
    preset.downloads = static_cast<int>(presetVar.getProperty("downloads", 0));
    preset.likes = static_cast<int>(presetVar.getProperty("likes", 0));
    preset.size = static_cast<int64_t>(presetVar.getProperty("size", 0));
    preset.previewUrl = presetVar.getProperty("previewUrl", "").toString();
    
    // Parse owner
    auto ownerVar = presetVar.getProperty("owner", juce::var());
    if (ownerVar.isObject())
    {
        preset.ownerName = ownerVar.getProperty("name", "Unknown").toString();
    }
    
    // Parse tags
    auto tagsVar = presetVar.getProperty("tags", juce::var());
    if (tagsVar.isArray())
    {
        for (const auto& tag : *tagsVar.getArray())
        {
            preset.tags.add(tag.toString());
        }
    }
    
    // Parse date
    auto dateStr = presetVar.getProperty("createdAt", "").toString();
    if (dateStr.isNotEmpty())
    {
        preset.createdAt = juce::Time::fromISO8601(dateStr);
    }
    
    return preset;
}

CloudProjectVersion ZenithCloudClient::parseVersion(const juce::var& versionVar) const
{
    CloudProjectVersion version;
    
    if (!versionVar.isObject())
        return version;
    
    version.versionNumber = static_cast<int>(versionVar.getProperty("versionNumber", 0));
    version.label = versionVar.getProperty("label", "").toString();
    version.changelog = versionVar.getProperty("changelog", "").toString();
    version.filename = versionVar.getProperty("filename", "").toString();
    version.size = static_cast<int64_t>(versionVar.getProperty("size", 0));
    
    auto dateStr = versionVar.getProperty("createdAt", "").toString();
    if (dateStr.isNotEmpty())
    {
        version.createdAt = juce::Time::fromISO8601(dateStr);
    }
    
    return version;
}

//==============================================================================
// Request Execution
//==============================================================================

void ZenithCloudClient::performRequest(const RequestContext& ctx)
{
    activeRequests_++;
    
    // Execute on thread pool
    threadPool_->addJob([this, ctx]()
    {
        juce::var response;
        bool success = false;
        juce::String errorMsg;
        
        try
        {
            juce::URL requestUrl = ctx.url;
            
            // Create input stream options
            juce::URL::InputStreamOptions options(juce::URL::ParameterHandling::inAddress);
            options = options.withConnectionTimeoutMs(30000);
            options = options.withExtraHeaders("Content-Type: application/json\r\n");
            
            if (ctx.authToken.isNotEmpty())
            {
                options = options.withExtraHeaders("Content-Type: application/json\r\nAuthorization: Bearer " + ctx.authToken + "\r\n");
            }
            
            // For POST/PUT with data
            if ((ctx.method == "POST" || ctx.method == "PUT") && !ctx.postData.isVoid())
            {
                juce::String jsonBody = juce::JSON::toString(ctx.postData);
                requestUrl = requestUrl.withPOSTData(jsonBody);
            }
            
            auto stream = requestUrl.createInputStream(options);
            
            if (stream != nullptr)
            {
                juce::String responseText = stream->readEntireStreamAsString();
                
                auto parseResult = juce::JSON::parse(responseText, response);
                
                if (parseResult.wasOk())
                {
                    success = true;
                }
                else
                {
                    errorMsg = "Failed to parse response: " + parseResult.getErrorMessage();
                }
            }
            else
            {
                errorMsg = "Failed to connect to server";
            }
        }
        catch (const std::exception& e)
        {
            errorMsg = juce::String("Request failed: ") + e.what();
        }
        
        // Store error
        if (!success)
        {
            const juce::ScopedLock lock(errorLock_);
            lastError_ = errorMsg;
        }
        
        activeRequests_--;
        
        // Callback on message thread
        if (ctx.onComplete)
        {
            juce::MessageManager::callAsync([ctx, response, success, errorMsg]()
            {
                ctx.onComplete(response, success, errorMsg);
            });
        }
    });
}

void ZenithCloudClient::performDownload(const juce::URL& url, const juce::File& destination,
                                        std::function<void(const juce::File&, bool)> onComplete,
                                        std::function<void(float)> onProgress)
{
    activeRequests_++;
    
    threadPool_->addJob([this, url, destination, onComplete, onProgress]()
    {
        bool success = false;
        juce::File savedFile;
        
        try
        {
            juce::URL::InputStreamOptions options(juce::URL::ParameterHandling::inAddress);
            options = options.withConnectionTimeoutMs(60000);
            
            if (authToken_.isNotEmpty())
            {
                options = options.withExtraHeaders("Authorization: Bearer " + authToken_ + "\r\n");
            }
            
            auto stream = url.createInputStream(options);
            
            if (stream != nullptr)
            {
                // Get content length for progress
                // Note: This is a simplified approach - in production, use proper HTTP headers
                
                // Create temp file
                juce::File tempFile = destination.getNonexistentChildFile("download", ".tmp");
                
                juce::FileOutputStream output(tempFile);
                if (output.openedOk())
                {
                    // Read in chunks for progress reporting
                    const int bufferSize = 8192;
                    juce::HeapBlock<char> buffer(bufferSize);
                    int64_t totalRead = 0;
                    
                    while (!stream->isExhausted())
                    {
                        int bytesRead = stream->read(buffer.getData(), bufferSize);
                        if (bytesRead > 0)
                        {
                            output.write(buffer.getData(), bytesRead);
                            totalRead += bytesRead;
                            
                            // Report progress (estimated - we don't always know total size)
                            if (onProgress)
                            {
                                juce::MessageManager::callAsync([onProgress, totalRead]()
                                {
                                    // Approximate progress - cap at 0.95 until complete
                                    float progress = juce::jmin(0.95f, static_cast<float>(totalRead) / (1024.0f * 1024.0f));
                                    onProgress(progress);
                                });
                            }
                        }
                    }
                    
                    output.flush();
                    
                    // Rename to final file (extract name from Content-Disposition or use generic)
                    savedFile = destination.getNonexistentChildFile("preset", ".zpreset");
                    tempFile.moveFileTo(savedFile);
                    success = true;
                    
                    if (onProgress)
                    {
                        juce::MessageManager::callAsync([onProgress]()
                        {
                            onProgress(1.0f);
                        });
                    }
                }
            }
        }
        catch (const std::exception& e)
        {
            DBG("ZenithCloudClient: Download failed - " + juce::String(e.what()));
        }
        
        activeRequests_--;
        
        if (onComplete)
        {
            juce::MessageManager::callAsync([onComplete, savedFile, success]()
            {
                onComplete(savedFile, success);
            });
        }
    });
}

void ZenithCloudClient::performUpload(const juce::URL& url, const juce::File& file, const juce::var& metadata,
                                      std::function<void(const juce::var&, bool, const juce::String&)> onComplete)
{
    activeRequests_++;
    
    threadPool_->addJob([this, url, file, metadata, onComplete]()
    {
        juce::var response;
        bool success = false;
        juce::String errorMsg;
        
        try
        {
            // Read file content
            juce::MemoryBlock fileData;
            file.loadFileAsData(fileData);
            
            // Create multipart form data
            juce::URL uploadUrl = url;
            
            // Add file as POST data
            uploadUrl = uploadUrl.withFileToUpload("presetFile", file, "application/octet-stream");
            
            // Add metadata fields
            if (metadata.isObject())
            {
                auto* obj = metadata.getDynamicObject();
                if (obj)
                {
                    for (const auto& prop : obj->getProperties())
                    {
                        uploadUrl = uploadUrl.withParameter(prop.name.toString(), prop.value.toString());
                    }
                }
            }
            
            juce::URL::InputStreamOptions options(juce::URL::ParameterHandling::inPostData);
            options = options.withConnectionTimeoutMs(120000);
            
            if (authToken_.isNotEmpty())
            {
                options = options.withExtraHeaders("Authorization: Bearer " + authToken_ + "\r\n");
            }
            
            auto stream = uploadUrl.createInputStream(options);
            
            if (stream != nullptr)
            {
                juce::String responseText = stream->readEntireStreamAsString();
                auto parseResult = juce::JSON::parse(responseText, response);
                
                if (parseResult.wasOk())
                {
                    success = true;
                }
                else
                {
                    errorMsg = "Failed to parse upload response";
                }
            }
            else
            {
                errorMsg = "Failed to connect for upload";
            }
        }
        catch (const std::exception& e)
        {
            errorMsg = juce::String("Upload failed: ") + e.what();
        }
        
        activeRequests_--;
        
        if (onComplete)
        {
            juce::MessageManager::callAsync([onComplete, response, success, errorMsg]()
            {
                onComplete(response, success, errorMsg);
            });
        }
    });
}

//==============================================================================
// Public Preset API
//==============================================================================

void ZenithCloudClient::fetchPublicPresets(
    std::function<void(const std::vector<CloudPreset>&, bool success)> onComplete,
    const juce::String& pluginId,
    const juce::String& category,
    int page,
    const juce::String& sortBy)
{
    juce::StringPairArray params;
    params.set("page", juce::String(page));
    params.set("limit", "50");
    
    if (pluginId.isNotEmpty())
        params.set("pluginId", pluginId);
    if (category.isNotEmpty())
        params.set("category", category);
    if (sortBy.isNotEmpty())
        params.set("sort", sortBy);
    
    juce::URL url = buildUrl("/api/presets/public", params);
    
    RequestContext ctx;
    ctx.url = url;
    ctx.method = "GET";
    ctx.authToken = authToken_;
    ctx.onComplete = [this, onComplete](const juce::var& response, bool success, const juce::String& error)
    {
        std::vector<CloudPreset> presets;
        
        if (success && response.isObject())
        {
            auto presetsVar = response.getProperty("presets", juce::var());
            if (presetsVar.isArray())
            {
                for (const auto& presetVar : *presetsVar.getArray())
                {
                    presets.push_back(parsePreset(presetVar));
                }
            }
        }
        else
        {
            DBG("ZenithCloudClient: fetchPublicPresets failed - " + error);
        }
        
        if (onComplete)
            onComplete(presets, success);
    };
    
    performRequest(ctx);
}

void ZenithCloudClient::searchPresets(
    const juce::String& query,
    std::function<void(const std::vector<CloudPreset>&, bool success)> onComplete)
{
    juce::StringPairArray params;
    params.set("q", query);
    
    juce::URL url = buildUrl("/api/presets/search", params);
    
    RequestContext ctx;
    ctx.url = url;
    ctx.method = "GET";
    ctx.authToken = authToken_;
    ctx.onComplete = [this, onComplete](const juce::var& response, bool success, const juce::String& error)
    {
        std::vector<CloudPreset> presets;
        
        if (success && response.isObject())
        {
            auto presetsVar = response.getProperty("presets", juce::var());
            if (presetsVar.isArray())
            {
                for (const auto& presetVar : *presetsVar.getArray())
                {
                    presets.push_back(parsePreset(presetVar));
                }
            }
        }
        else
        {
            DBG("ZenithCloudClient: searchPresets failed - " + error);
        }
        
        if (onComplete)
            onComplete(presets, success);
    };
    
    performRequest(ctx);
}

void ZenithCloudClient::downloadPreset(
    const juce::String& presetId,
    const juce::File& destinationFolder,
    std::function<void(const juce::File& savedFile, bool success)> onComplete,
    std::function<void(float progress)> onProgress)
{
    juce::URL url = buildUrl("/api/presets/" + presetId + "/download");
    performDownload(url, destinationFolder, onComplete, onProgress);
}

void ZenithCloudClient::uploadPreset(
    const juce::File& presetFile,
    const juce::var& metadata,
    std::function<void(bool success, const juce::String& presetId)> onComplete)
{
    if (!isAuthenticated())
    {
        if (onComplete)
        {
            juce::MessageManager::callAsync([onComplete]()
            {
                onComplete(false, "");
            });
        }
        return;
    }
    
    juce::URL url = buildUrl("/api/presets");
    
    performUpload(url, presetFile, metadata, 
        [onComplete](const juce::var& response, bool success, const juce::String& error)
        {
            juce::String presetId;
            if (success && response.isObject())
            {
                presetId = response.getProperty("_id", "").toString();
            }
            
            if (onComplete)
                onComplete(success, presetId);
        });
}

//==============================================================================
// Project Version Control API
//==============================================================================

void ZenithCloudClient::saveProjectVersion(
    const juce::String& projectId,
    const juce::File& projectFile,
    const juce::String& label,
    const juce::String& changelog,
    std::function<void(bool success, int versionNumber)> onComplete)
{
    if (!isAuthenticated())
    {
        if (onComplete)
        {
            juce::MessageManager::callAsync([onComplete]()
            {
                onComplete(false, -1);
            });
        }
        return;
    }
    
    juce::URL url = buildUrl("/api/projects/" + projectId + "/versions");
    
    juce::DynamicObject::Ptr metadata = new juce::DynamicObject();
    metadata->setProperty("label", label);
    metadata->setProperty("changelog", changelog);
    
    performUpload(url, projectFile, juce::var(metadata.get()),
        [onComplete](const juce::var& response, bool success, const juce::String& error)
        {
            int versionNumber = -1;
            if (success && response.isObject())
            {
                auto versionVar = response.getProperty("version", juce::var());
                if (versionVar.isObject())
                {
                    versionNumber = static_cast<int>(versionVar.getProperty("versionNumber", -1));
                }
            }
            
            if (onComplete)
                onComplete(success, versionNumber);
        });
}

void ZenithCloudClient::getProjectVersions(
    const juce::String& projectId,
    std::function<void(const std::vector<CloudProjectVersion>&, bool success)> onComplete)
{
    if (!isAuthenticated())
    {
        if (onComplete)
        {
            juce::MessageManager::callAsync([onComplete]()
            {
                onComplete({}, false);
            });
        }
        return;
    }
    
    juce::URL url = buildUrl("/api/projects/" + projectId + "/versions");
    
    RequestContext ctx;
    ctx.url = url;
    ctx.method = "GET";
    ctx.authToken = authToken_;
    ctx.onComplete = [this, onComplete](const juce::var& response, bool success, const juce::String& error)
    {
        std::vector<CloudProjectVersion> versions;
        
        if (success && response.isObject())
        {
            auto versionsVar = response.getProperty("versions", juce::var());
            if (versionsVar.isArray())
            {
                for (const auto& versionVar : *versionsVar.getArray())
                {
                    versions.push_back(parseVersion(versionVar));
                }
            }
        }
        else
        {
            DBG("ZenithCloudClient: getProjectVersions failed - " + error);
        }
        
        if (onComplete)
            onComplete(versions, success);
    };
    
    performRequest(ctx);
}

void ZenithCloudClient::downloadProjectVersion(
    const juce::String& projectId,
    int versionNumber,
    const juce::File& destinationFolder,
    std::function<void(const juce::File& savedFile, bool success)> onComplete)
{
    juce::URL url = buildUrl("/api/projects/" + projectId + "/versions/" + juce::String(versionNumber) + "/download");
    performDownload(url, destinationFolder, onComplete, nullptr);
}

} // namespace zenith
