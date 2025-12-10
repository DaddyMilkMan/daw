/*
  ==============================================================================

    SampleHunterAgent.h
    Created: 2025-12-08
    Author:  Zenith DAW AI Team

    "The Sample Hunter" - Asset Acquisition Agent

    Role: The Crate Digger. It autonomously builds your sample library while
    you sleep. It uses the Freesound API to find royalty-free samples that match
    your project's genre, downloads them, analyzes their Key/BPM, and tags
    them in your browser.

    Features:
    - Genre Detection from Project Context (tempo, existing tracks)
    - Smart Search Query Generation (via GrokAPIClient)
    - Freesound API Integration (v2)
    - Automated Download Queue Management
    - Audio Analysis for Key/BPM Detection
    - Smart File Renaming with Metadata Tags
    - AudioFilePool Integration for instant DAW access

    Time Factor: Searching, downloading, and FFT analysis of hundreds of
    files takes 15-30 minutes. This is a "walk away" agent.

  ==============================================================================
*/

#pragma once

#include "../../include/Engine.h"
#include "../../include/ProjectState.h"
#include "../network/AudioAnalysisService.h"
#include "../network/GrokAPIClient.h"
#include <atomic>
#include <functional>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <memory>
#include <queue>
#include <unordered_set>
#include <vector>

namespace zenith {
namespace ai {

//==============================================================================
/**
    Detected genre and style information for sample search targeting
*/
struct GenreContext {
  juce::String primaryGenre = "General"; // Main genre (e.g., "Lo-Fi Hip Hop")
  juce::String subGenre = "";            // Sub-genre (e.g., "Boom Bap")
  juce::StringArray
      styleKeywords;           // Style descriptors ["chill", "jazzy", "dusty"]
  double estimatedBpm = 120.0; // Target BPM
  juce::String keySignature = ""; // Target key (if detected)

  // Instruments commonly used in this genre
  juce::StringArray targetInstruments; // ["drums", "piano", "bass"]

  juce::String toString() const {
    juce::String result = primaryGenre;
    if (subGenre.isNotEmpty())
      result += " / " + subGenre;
    result += " @ " + juce::String(estimatedBpm, 1) + " BPM";
    return result;
  }
};

//==============================================================================
/**
    Freesound API Configuration
*/
struct FreesoundConfig {
  juce::String apiKey; // Client Secret/API Key
  juce::String baseUrl = "https://freesound.org/apiv2";

  // Safety limits
  int connectionTimeoutMs = 5000;
  int responseTimeoutMs = 15000;

  // Anti-trickle protection
  // Minimum speed: 1KB/s. If average speed drops below this for a period,
  // abort.
  double minDownloadSpeedBps = 1024.0;

  // Rate limiting
  int requestsPerMinute = 60;

  FreesoundConfig() {
    // In a real app, this would come from SecureKeyStore
    // For now, we use a placeholder or assume the user has set it up
    apiKey = "";
  }
};

//==============================================================================
/**
    Represents a sample found via API
*/
struct FoundSample {
  juce::String id;          // API ID
  juce::String title;       // Original title
  juce::String downloadUrl; // High-quality download URL
  juce::String previewUrl;  // Preview URL
  juce::String license;     // License URL or Name
  juce::String username;    // Creator
  double duration = 0.0;    // Duration in seconds
  int sampleRate = 0;       // Sample rate
  int bitDepth = 0;         // Bit depth
  juce::String type;        // wav, aiff, etc.
  int64_t fileSize = 0;     // File size in bytes (from API)

  // Tagging
  juce::StringArray tags;

  // After download and analysis
  juce::File localFile; // Local file path after download
  bool downloaded = false;
  bool analyzed = false;

  // Analysis results
  juce::String detectedKey = "";
  double detectedBpm = 0.0;
  juce::String instrumentType = "";
  juce::String brightness = ""; // "bright", "dark", "neutral"

  juce::String getSafeFilename() const {
    // Create a filesystem-safe name: "ID_Title_User.wav"
    juce::String safeTitle =
        juce::File::createLegalFileName(title).replace(" ", "_");
    juce::String safeUser =
        juce::File::createLegalFileName(username).replace(" ", "_");

    // Truncate if too long (max 50 chars for title)
    if (safeTitle.length() > 50)
      safeTitle = safeTitle.substring(0, 50);

    // Ensure extension exists
    juce::String ext = "." + type;
    if (!safeTitle.endsWithIgnoreCase(ext)) {
      // If title doesn't have extension, append it
    }

    return safeTitle + "_" + id + "_" + safeUser + "." + type;
  }
};

//==============================================================================
/**
    Search and download statistics
*/
struct HuntingStats {
  int apiRequestsExecuted = 0;
  int samplesFound = 0;
  int samplesFiltered = 0; // Removed by safety/quality filters
  int downloadsAttempted = 0;
  int downloadsSucceeded = 0;
  int downloadsFailed = 0;
  int downloadsTimedOut = 0; // Specific metric for timeouts
  int samplesAnalyzed = 0;
  int samplesImported = 0;
  int64_t totalBytesDownloaded = 0;

  juce::Time startTime;
  juce::Time endTime;

  double getElapsedSeconds() const { return (endTime - startTime).inSeconds(); }

  juce::String getSummary() const {
    return juce::String::formatted(
        "Sample Hunt Complete (Freesound API)!\n"
        "API Requests: %d\n"
        "Samples Found: %d (Filtered: %d)\n"
        "Downloads: %d/%d (Timeouts: %d)\n"
        "Imported: %d\n"
        "Data: %s\n"
        "Time: %.1f min",
        apiRequestsExecuted, samplesFound, samplesFiltered, downloadsSucceeded,
        downloadsAttempted, downloadsTimedOut, samplesImported,
        juce::File::descriptionOfSizeInBytes(totalBytesDownloaded).toRawUTF8(),
        getElapsedSeconds() / 60.0);
  }
};

//==============================================================================
/**
    Configuration for the sample hunting session
*/
struct HuntingConfig {
  // Search settings
  int maxSearchQueries = 10;   // Maximum search queries to execute
  int maxResultsPerQuery = 20; // Max results to fetch per query
  int maxTotalDownloads = 100; // Total download limit

  // Download settings
  juce::File downloadDirectory; // Where to save downloaded samples
  int maxFileSizeMb = 50;       // Maximum file size in MB

  // Quality filters
  double minDurationSeconds = 0.5;  // Minimum sample duration
  double maxDurationSeconds = 30.0; // Maximum sample duration
  bool skipDuplicates = true;       // Skip files with similar MD5

  // Rate limiting
  int delayBetweenSearchesMs = 1000;
  int delayBetweenDownloadsMs = 500;

  HuntingConfig() {
    // Default download directory
    downloadDirectory =
        juce::File::getSpecialLocation(juce::File::userMusicDirectory)
            .getChildFile("Zenith/Samples/Hunter");
  }
};

//==============================================================================
/**
    Main Sample Hunter Agent
*/
class SampleHunterAgent : public juce::Thread, public juce::ChangeBroadcaster {
public:
  //==========================================================================
  /**
   * @brief Construct the Sample Hunter Agent
   */
  explicit SampleHunterAgent(Engine &engine);
  ~SampleHunterAgent() override;

  //==========================================================================
  // Simple Command Interface (for WingmanPanel/AI Chat)
  //==========================================================================

  /**
   * Start a hunt with a natural language query
   * e.g., hunt("punchy 808 kick")
   */
  void hunt(const juce::String &query);

  /**
   * Download a specific result by index from the last search
   * @return true if download was queued
   */
  bool downloadResult(int index);

  //==========================================================================
  // Agent Control
  //==========================================================================

  void startHunting(const HuntingConfig &config = HuntingConfig());
  void stopHunting();
  bool isHunting() const { return isHunting_.load(); }
  float getProgress() const { return progress_.load(); }
  juce::String getStatusMessage() const;

  //==========================================================================
  // Results Access
  //==========================================================================

  const HuntingStats &getStats() const { return stats_; }
  const std::vector<FoundSample> &getFoundSamples() const {
    return foundSamples_;
  }
  std::vector<juce::File> getImportedFiles() const;
  const GenreContext &getDetectedGenre() const { return genreContext_; }

  //==========================================================================
  // Configuration
  //==========================================================================

  HuntingConfig &getConfig() { return config_; }
  const HuntingConfig &getConfig() const { return config_; }

  void setFreesoundConfig(const FreesoundConfig &freesoundConfig) {
    freesoundConfig_ = freesoundConfig;
  }
  const FreesoundConfig &getFreesoundConfig() const { return freesoundConfig_; }

  //==========================================================================
  // Listeners
  //==========================================================================

  class Listener {
  public:
    virtual ~Listener() = default;
    virtual void sampleDownloaded(const FoundSample &sample) = 0;
    virtual void sampleAnalyzed(const FoundSample &sample) = 0;
    virtual void sampleImported(const juce::File &file) = 0;
    virtual void huntingProgressChanged(float progress,
                                        const juce::String &status) = 0;
    virtual void huntingComplete(const HuntingStats &stats, bool success) = 0;
  };

  void addListener(Listener *listener);
  void removeListener(Listener *listener);

private:
  //==========================================================================
  // Thread callback
  void run() override;

  //==========================================================================
  // Internal Stages
  //==========================================================================

  void analyzeProjectContext();
  void refineGenreWithAI();
  juce::String detectGenreFromTempo(double tempo);

  void generateSearchQueries();
  void generateAiSearchQueries();

  std::vector<FoundSample> executeFreesoundSearch(const juce::String &query);

  bool downloadSample(FoundSample &sample);
  void analyzeSample(FoundSample &sample);
  juce::String generateSmartFilename(const FoundSample &sample);
  bool importToPool(FoundSample &sample);

  //==========================================================================
  // Helpers
  //==========================================================================

  void setStatus(const juce::String &status);
  void updateProgress(float progress);
  juce::String generateFileHash(const juce::File &file);
  bool isDuplicate(const juce::File &file);

  //==========================================================================
  // Member Variables
  //==========================================================================

  Engine &engine_;
  HuntingConfig config_;
  FreesoundConfig freesoundConfig_;
  HuntingStats stats_;
  GenreContext genreContext_;

  // Thread state
  std::atomic<bool> isHunting_{false};
  std::atomic<float> progress_{0.0f};
  juce::CriticalSection statusLock_;
  juce::String currentStatus_;

  // Queues
  juce::StringArray searchQueue_;
  std::vector<FoundSample> foundSamples_;
  std::queue<size_t> downloadQueue_;

  // Duplicate detection
  std::unordered_set<juce::String> downloadedHashes_;

  // Services
  std::unique_ptr<AudioAnalysisService> analysisService_;
  std::unique_ptr<GrokAPIClient> grokClient_;

  // Listeners
  juce::ListenerList<Listener> listeners_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleHunterAgent)
};

} // namespace ai
} // namespace zenith
