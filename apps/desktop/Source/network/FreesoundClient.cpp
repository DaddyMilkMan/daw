/*
  ==============================================================================

    FreesoundClient.cpp
    Created: 2025-12-09
    Author:  Zenith DAW AI Team

    HTTP client for Freesound.org API v2

    Safety Features:
    - Rate limiting (1 request/second)
    - Connection timeouts (10 seconds)
    - Response size limits
    - Filename sanitization

  ==============================================================================
*/

#include "FreesoundClient.h"
#include "SecureKeyStore.h"
#include <juce_cryptography/juce_cryptography.h>

namespace zenith {
namespace network {

//==============================================================================
// Constants
//==============================================================================

namespace {
constexpr int kConnectionTimeoutMs = 10000;
constexpr int kMinTimeBetweenRequestsMs = 1000; // Rate limit: 1 req/sec
constexpr int64_t kMaxResponseSizeBytes = 10 * 1024 * 1024; // 10MB max response
constexpr int64_t kMaxDownloadSizeBytes = 50 * 1024 * 1024; // 50MB max download
constexpr int kMinDownloadSpeedBps = 1024;                  // 1KB/s minimum
constexpr int kSpeedCheckIntervalMs = 2000; // Check speed every 2s
} // namespace

//==============================================================================
// Constructor / Destructor
//==============================================================================

FreesoundClient::FreesoundClient() {
  // Load API key from secure storage
  apiKey_ = SecureKeyStore::getInstance().getKey("freesound_api_key");

  if (apiKey_.isEmpty()) {
    DBG("FreesoundClient: WARNING - No API key found!");
    DBG("FreesoundClient: Set via "
        "SecureKeyStore::getInstance().setKey(\"freesound_api_key\", "
        "\"YOUR_KEY\")");
  }
}

FreesoundClient::~FreesoundClient() = default;

//==============================================================================
// Public API
//==============================================================================

bool FreesoundClient::hasApiKey() const { return apiKey_.isNotEmpty(); }

void FreesoundClient::setApiKey(const juce::String &key) {
  apiKey_ = key;
  SecureKeyStore::getInstance().setKey("freesound_api_key", key);
}

std::vector<SampleResult>
FreesoundClient::searchSounds(const juce::String &query, int page,
                              int pageSize) {

  std::vector<SampleResult> results;

  if (!hasApiKey()) {
    lastError_ = "No API key configured";
    return results;
  }

  // Rate limiting
  enforceRateLimit();

  // Build URL
  juce::URL url(kBaseUrl + "/search/text/");
  url =
      url.withParameter("query", query)
          .withParameter("fields", "id,name,previews,username,license,type,"
                                   "duration,filesize,samplerate,bitdepth,tags")
          .withParameter("page_size", juce::String(pageSize))
          .withParameter("page", juce::String(page))
          .withParameter("token", apiKey_);

  DBG("FreesoundClient: Searching: " + query);

  // Execute request with timeout
  auto response = executeRequest(url);
  if (!response.success) {
    lastError_ = response.error;
    return results;
  }

  // Parse JSON
  auto jsonVar = juce::JSON::parse(response.body);
  if (!jsonVar.isObject()) {
    lastError_ = "Invalid JSON response";
    return results;
  }

  auto *root = jsonVar.getDynamicObject();
  if (!root)
    return results;

  auto resultArray = root->getProperty("results");
  if (!resultArray.isArray())
    return results;

  for (const auto &item : *resultArray.getArray()) {
    if (!item.isObject())
      continue;
    auto *obj = item.getDynamicObject();

    SampleResult sample;
    sample.id = obj->getProperty("id").toString();
    sample.name = obj->getProperty("name").toString();
    sample.username = obj->getProperty("username").toString();
    sample.license = obj->getProperty("license").toString();
    sample.duration = static_cast<double>(obj->getProperty("duration"));
    sample.fileSize = static_cast<int64_t>(obj->getProperty("filesize"));
    sample.sampleRate = static_cast<int>(obj->getProperty("samplerate"));
    sample.bitDepth = static_cast<int>(obj->getProperty("bitdepth"));
    sample.type = obj->getProperty("type").toString();

    // Parse tags
    auto tagsVar = obj->getProperty("tags");
    if (tagsVar.isArray()) {
      for (const auto &tag : *tagsVar.getArray()) {
        sample.tags.add(tag.toString());
      }
    }

    // Parse previews
    auto previews = obj->getProperty("previews");
    if (previews.isObject()) {
      auto *pObj = previews.getDynamicObject();
      // Prefer HQ MP3 for preview, HQ OGG for download
      if (pObj->hasProperty("preview-hq-mp3"))
        sample.previewUrl = pObj->getProperty("preview-hq-mp3").toString();
      else if (pObj->hasProperty("preview-hq-ogg"))
        sample.previewUrl = pObj->getProperty("preview-hq-ogg").toString();
      else if (pObj->hasProperty("preview-lq-mp3"))
        sample.previewUrl = pObj->getProperty("preview-lq-mp3").toString();

      // Download URL (same as preview for free tier)
      sample.downloadUrl = sample.previewUrl;
    }

    if (sample.downloadUrl.isNotEmpty()) {
      results.push_back(sample);
    }
  }

  DBG("FreesoundClient: Found " + juce::String(results.size()) + " results");
  return results;
}

FreesoundClient::DownloadResult
FreesoundClient::downloadSample(const juce::String &sampleId,
                                const juce::String &downloadUrl,
                                const juce::File &destinationFile,
                                std::function<void(float)> progressCallback) {

  DownloadResult result;
  result.success = false;

  if (downloadUrl.isEmpty()) {
    result.error = "No download URL provided";
    return result;
  }

  // Rate limiting
  enforceRateLimit();

  // Sanitize destination filename
  juce::File safeDestination = sanitizeDestinationFile(destinationFile);

  // Create parent directory if needed
  safeDestination.getParentDirectory().createDirectory();

  DBG("FreesoundClient: Downloading to " + safeDestination.getFullPathName());

  juce::URL url(downloadUrl);

  auto options =
      juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
          .withConnectionTimeoutMs(kConnectionTimeoutMs);

  std::unique_ptr<juce::InputStream> stream = url.createInputStream(options);

  if (!stream) {
    result.error = "Connection failed";
    return result;
  }

  // Create output file
  juce::FileOutputStream output(safeDestination);
  if (!output.openedOk()) {
    result.error = "Failed to create output file";
    return result;
  }

  // Download with monitoring
  const int bufferSize = 8192;
  juce::HeapBlock<char> buffer(bufferSize);

  int64_t totalRead = 0;
  int64_t lastMonitorTime = juce::Time::getMillisecondCounter();
  int64_t bytesSinceMonitor = 0;

  while (!stream->isExhausted()) {
    int bytesRead = stream->read(buffer.getData(), bufferSize);

    if (bytesRead < 0) {
      result.error = "Read error";
      safeDestination.deleteFile();
      return result;
    }

    if (bytesRead == 0)
      continue;

    output.write(buffer.getData(), static_cast<size_t>(bytesRead));
    totalRead += bytesRead;
    bytesSinceMonitor += bytesRead;

    // Size limit check
    if (totalRead > kMaxDownloadSizeBytes) {
      result.error = "File exceeds maximum size limit";
      output.flush();
      safeDestination.deleteFile();
      return result;
    }

    // Speed monitoring
    int64_t now = juce::Time::getMillisecondCounter();
    if (now - lastMonitorTime > kSpeedCheckIntervalMs) {
      double seconds = (now - lastMonitorTime) / 1000.0;
      double speed = bytesSinceMonitor / seconds;

      if (speed < kMinDownloadSpeedBps) {
        result.error = "Download speed too slow (anti-trickle protection)";
        output.flush();
        safeDestination.deleteFile();
        return result;
      }

      lastMonitorTime = now;
      bytesSinceMonitor = 0;
    }

    // Progress callback
    if (progressCallback) {
      // We don't know total size, so use indeterminate progress
      progressCallback(-1.0f);
    }
  }

  output.flush();

  // Verify file
  if (totalRead < 100) {
    result.error = "Downloaded file too small";
    safeDestination.deleteFile();
    return result;
  }

  result.success = true;
  result.localFile = safeDestination;
  result.bytesDownloaded = totalRead;

  DBG("FreesoundClient: Downloaded " + juce::String(totalRead) + " bytes");
  return result;
}

juce::String FreesoundClient::getLastError() const { return lastError_; }

//==============================================================================
// Private Implementation
//==============================================================================

FreesoundClient::HttpResponse
FreesoundClient::executeRequest(const juce::URL &url) {
  HttpResponse response;
  response.success = false;

  auto options =
      juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
          .withConnectionTimeoutMs(kConnectionTimeoutMs);

  std::unique_ptr<juce::InputStream> stream = url.createInputStream(options);

  if (!stream) {
    response.error = "Connection failed (timeout or network error)";
    return response;
  }

  // Read with size limit
  juce::MemoryOutputStream memStream;
  int64_t totalRead = 0;
  const int bufferSize = 8192;
  juce::HeapBlock<char> buffer(bufferSize);

  while (!stream->isExhausted()) {
    int bytesRead = stream->read(buffer.getData(), bufferSize);
    if (bytesRead <= 0)
      break;

    totalRead += bytesRead;
    if (totalRead > kMaxResponseSizeBytes) {
      response.error = "Response exceeds size limit";
      return response;
    }

    memStream.write(buffer.getData(), static_cast<size_t>(bytesRead));
  }

  response.body = memStream.toString();
  response.success = true;
  return response;
}

void FreesoundClient::enforceRateLimit() {
  int64_t now = juce::Time::getMillisecondCounter();
  int64_t elapsed = now - lastRequestTime_;

  if (elapsed < kMinTimeBetweenRequestsMs) {
    int64_t waitTime = kMinTimeBetweenRequestsMs - elapsed;
    juce::Thread::sleep(static_cast<int>(waitTime));
  }

  lastRequestTime_ = juce::Time::getMillisecondCounter();
}

juce::File FreesoundClient::sanitizeDestinationFile(const juce::File &file) {
  // Get the filename and sanitize it
  juce::String filename = file.getFileName();

  // Remove or replace dangerous characters for path traversal protection
  filename = filename.replace("..", "_")
                 .replace("/", "_")
                 .replace("\\", "_")
                 .replace(":", "_")
                 .replace("*", "_")
                 .replace("?", "_")
                 .replace("\"", "_")
                 .replace("<", "_")
                 .replace(">", "_")
                 .replace("|", "_");

  // Use JUCE's built-in sanitization as well
  filename = juce::File::createLegalFileName(filename);

  // Truncate if too long (max 200 chars)
  if (filename.length() > 200) {
    juce::String ext = file.getFileExtension();
    filename = filename.substring(0, 200 - ext.length()) + ext;
  }

  return file.getParentDirectory().getChildFile(filename);
}

juce::String FreesoundClient::generateFileHash(const juce::File &file) {
  if (!file.existsAsFile())
    return "";
  return juce::MD5(file).toHexString();
}

} // namespace network
} // namespace zenith
