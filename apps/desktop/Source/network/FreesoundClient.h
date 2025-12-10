/*
  ==============================================================================

    FreesoundClient.h
    Created: 2025-12-09
    Author:  Zenith DAW AI Team

    HTTP client for Freesound.org API v2

    Thread-safe, rate-limited client for searching and downloading
    royalty-free samples from Freesound.org.

  ==============================================================================
*/

#pragma once

#include <functional>
#include <juce_core/juce_core.h>
#include <vector>


namespace zenith {
namespace network {

//==============================================================================
/**
    Represents a sample search result from Freesound API
*/
struct SampleResult {
  juce::String id;          // Freesound ID
  juce::String name;        // Sample name
  juce::String username;    // Uploader username
  juce::String license;     // License type
  juce::String type;        // File type (wav, mp3, etc.)
  juce::String previewUrl;  // Preview audio URL
  juce::String downloadUrl; // Full download URL

  double duration = 0.0; // Duration in seconds
  int64_t fileSize = 0;  // File size in bytes
  int sampleRate = 0;    // Sample rate
  int bitDepth = 0;      // Bit depth

  juce::StringArray tags; // User tags

  /**
   * Generate a filesystem-safe filename
   */
  juce::String getSafeFilename() const {
    juce::String safeName =
        juce::File::createLegalFileName(name).replace(" ", "_").substring(
            0, 50); // Truncate to 50 chars

    juce::String safeUser =
        juce::File::createLegalFileName(username).replace(" ", "_").substring(
            0, 20);

    juce::String ext = type.isNotEmpty() ? type : "wav";

    return safeName + "_" + id + "_" + safeUser + "." + ext;
  }
};

//==============================================================================
/**
    HTTP client for Freesound.org API v2

    Features:
    - Rate limiting (1 request per second)
    - Connection timeout (10 seconds)
    - File size limits (50MB max)
    - Anti-trickle protection (minimum download speed)
    - Filename sanitization (path traversal protection)
*/
class FreesoundClient {
public:
  //==========================================================================
  FreesoundClient();
  ~FreesoundClient();

  //==========================================================================
  // API Key Management
  //==========================================================================

  bool hasApiKey() const;
  void setApiKey(const juce::String &key);

  //==========================================================================
  // Search API
  //==========================================================================

  /**
   * Search for sounds on Freesound.org
   *
   * @param query     Search query (e.g., "808 kick")
   * @param page      Page number (1-based)
   * @param pageSize  Results per page (max 150)
   * @return          Vector of SampleResult
   */
  std::vector<SampleResult> searchSounds(const juce::String &query,
                                         int page = 1, int pageSize = 20);

  //==========================================================================
  // Download API
  //==========================================================================

  struct DownloadResult {
    bool success = false;
    juce::File localFile;
    int64_t bytesDownloaded = 0;
    juce::String error;
  };

  /**
   * Download a sample to a local file
   *
   * @param sampleId          Freesound sample ID
   * @param downloadUrl       URL to download from
   * @param destinationFile   Target file path
   * @param progressCallback  Optional progress callback (0.0-1.0, or -1 for
   * indeterminate)
   * @return                  DownloadResult with success status
   */
  DownloadResult
  downloadSample(const juce::String &sampleId, const juce::String &downloadUrl,
                 const juce::File &destinationFile,
                 std::function<void(float)> progressCallback = nullptr);

  //==========================================================================
  // Error Handling
  //==========================================================================

  juce::String getLastError() const;

private:
  //==========================================================================
  static constexpr const char *kBaseUrl = "https://freesound.org/apiv2";

  juce::String apiKey_;
  juce::String lastError_;
  int64_t lastRequestTime_ = 0;

  //==========================================================================
  // Internal request handling

  struct HttpResponse {
    bool success = false;
    juce::String body;
    juce::String error;
  };

  HttpResponse executeRequest(const juce::URL &url);
  void enforceRateLimit();
  juce::File sanitizeDestinationFile(const juce::File &file);
  juce::String generateFileHash(const juce::File &file);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FreesoundClient)
};

} // namespace network
} // namespace zenith
