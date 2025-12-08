/*
  ==============================================================================

    SampleHunterAgent.h
    Created: 2025-12-08
    Author:  Zenith DAW AI Team

    "The Sample Hunter" - Asset Acquisition Agent

    Role: The Crate Digger. It autonomously builds your sample library while
    you sleep. It searches the web for free, royalty-free samples that match
    your project's genre, downloads them, analyzes their Key/BPM, and tags
    them in your browser.

    Features:
    - Genre Detection from Project Context (tempo, existing tracks)
    - Smart Search Query Generation (via GrokAPIClient)
    - Web Scraping for Free Sample Sites
    - Automated Download Queue Management
    - Audio Analysis for Key/BPM Detection
    - Smart File Renaming with Metadata Tags
    - AudioFilePool Integration for instant DAW access

    Time Factor: Web scraping, downloading, and FFT analysis of hundreds of
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
    A single sample source/site configuration
*/
struct SampleSource {
  juce::String name;            // Human-readable name
  juce::String baseUrl;         // Base URL for the source
  juce::String searchTemplate;  // URL template with {query} placeholder
  juce::String licenseType;     // "CC0", "Royalty-Free", "Creative Commons"
  bool requiresCredits = false; // Whether attribution is needed
  int priority = 5;             // 1-10, higher = preferred

  SampleSource() = default;

  SampleSource(const juce::String &n, const juce::String &url,
               const juce::String &search, const juce::String &license,
               bool credits = false, int prio = 5)
      : name(n), baseUrl(url), searchTemplate(search), licenseType(license),
        requiresCredits(credits), priority(prio) {}
};

//==============================================================================
/**
    Represents a sample found during search
*/
struct FoundSample {
  juce::String downloadUrl;       // Direct download URL
  juce::String originalName;      // Original filename
  juce::String sourceName;        // Which source it came from
  juce::String licenseType;       // License of this sample
  juce::String category;          // "drums", "bass", "oneshot", "loop"
  int64_t estimatedSizeBytes = 0; // Estimated file size

  // After download and analysis
  juce::File localFile; // Local file path after download
  bool downloaded = false;
  bool analyzed = false;

  // Analysis results
  juce::String detectedKey = "";
  double detectedBpm = 0.0;
  juce::String instrumentType = "";
  juce::String brightness = ""; // "bright", "dark", "neutral"
};

//==============================================================================
/**
    Search and download statistics
*/
struct HuntingStats {
  int searchQueriesExecuted = 0;
  int urlsFound = 0;
  int urlsFiltered = 0; // Removed by safety/quality filters
  int downloadsAttempted = 0;
  int downloadsSucceeded = 0;
  int downloadsFailed = 0;
  int samplesAnalyzed = 0;
  int samplesImported = 0; // Successfully added to AudioFilePool
  int64_t totalBytesDownloaded = 0;

  juce::Time startTime;
  juce::Time endTime;

  double getElapsedSeconds() const { return (endTime - startTime).inSeconds(); }

  juce::String getSummary() const {
    return juce::String::formatted(
        "Sample Hunt Complete!\n"
        "Searches: %d\n"
        "URLs Found: %d (Filtered: %d)\n"
        "Downloads: %d/%d succeeded\n"
        "Samples Imported: %d\n"
        "Total Size: %s\n"
        "Duration: %.1f min",
        searchQueriesExecuted, urlsFound, urlsFiltered, downloadsSucceeded,
        downloadsAttempted, samplesImported,
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
  int maxResultsPerQuery = 20; // Max URLs to process per query
  int maxTotalDownloads = 100; // Total download limit

  // Download settings
  juce::File downloadDirectory; // Where to save downloaded samples
  int maxFileSizeMb = 50;       // Maximum file size in MB
  bool onlyWavFiles = true;     // Only download WAV files (ignore MP3, etc.)

  // Quality filters
  double minDurationSeconds = 0.1;  // Minimum sample duration
  double maxDurationSeconds = 60.0; // Maximum sample duration (loops)
  bool skipDuplicates = true;       // Skip files with similar MD5

  // Copyright safety
  bool verifyCopyrightClaims = true; // Extra verification step
  juce::StringArray blockedSites;    // Sites to never download from

  // Rate limiting (be nice to servers)
  int delayBetweenSearchesMs = 2000;  // Delay between searches
  int delayBetweenDownloadsMs = 1000; // Delay between downloads

  // Analysis
  bool analyzeKey = true;         // Detect musical key
  bool analyzeBpm = true;         // Detect BPM
  bool classifyInstrument = true; // Classify instrument type

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

    Autonomously searches for, downloads, analyzes, and imports royalty-free
    samples based on the current project's genre and requirements.

    This is a "long-running" agent designed to run for 15-30 minutes while
    the user is away from the computer.
*/
class SampleHunterAgent : public juce::Thread, public juce::ChangeBroadcaster {
public:
  //==========================================================================
  /**
   * @brief Construct the Sample Hunter Agent
   * @param engine Reference to the Engine (for project state access)
   */
  explicit SampleHunterAgent(Engine &engine);
  ~SampleHunterAgent() override;

  //==========================================================================
  // Agent Control
  //==========================================================================

  /**
   * @brief Start the sample hunting process
   * @param config Configuration for this hunting session
   */
  void startHunting(const HuntingConfig &config = HuntingConfig());

  /**
   * @brief Stop the hunting process gracefully
   */
  void stopHunting();

  /**
   * @brief Check if hunting is active
   */
  bool isHunting() const { return isHunting_.load(); }

  /**
   * @brief Get current progress (0.0 - 1.0)
   */
  float getProgress() const { return progress_.load(); }

  /**
   * @brief Get current status message
   */
  juce::String getStatusMessage() const;

  //==========================================================================
  // Results Access
  //==========================================================================

  /**
   * @brief Get statistics for the current/last hunting session
   */
  const HuntingStats &getStats() const { return stats_; }

  /**
   * @brief Get all found samples (even non-downloaded)
   */
  const std::vector<FoundSample> &getFoundSamples() const {
    return foundSamples_;
  }

  /**
   * @brief Get successfully imported samples
   */
  std::vector<juce::File> getImportedFiles() const;

  /**
   * @brief Get detected genre context
   */
  const GenreContext &getDetectedGenre() const { return genreContext_; }

  //==========================================================================
  // Configuration
  //==========================================================================

  /**
   * @brief Get current hunting configuration
   */
  HuntingConfig &getConfig() { return config_; }
  const HuntingConfig &getConfig() const { return config_; }

  /**
   * @brief Add a sample source
   */
  void addSource(const SampleSource &source);

  /**
   * @brief Get registered sample sources
   */
  const std::vector<SampleSource> &getSources() const { return sources_; }

  /**
   * @brief Clear all sources and use defaults
   */
  void resetToDefaultSources();

  //==========================================================================
  // Listeners
  //==========================================================================

  class Listener {
  public:
    virtual ~Listener() = default;

    /** Called when a sample is successfully downloaded */
    virtual void sampleDownloaded(const FoundSample &sample) = 0;

    /** Called when a sample is analyzed (key/BPM detected) */
    virtual void sampleAnalyzed(const FoundSample &sample) = 0;

    /** Called when a sample is imported into the AudioFilePool */
    virtual void sampleImported(const juce::File &file) = 0;

    /** Called when hunting progress updates */
    virtual void huntingProgressChanged(float progress,
                                        const juce::String &status) = 0;

    /** Called when hunting completes or errors */
    virtual void huntingComplete(const HuntingStats &stats, bool success) = 0;
  };

  void addListener(Listener *listener);
  void removeListener(Listener *listener);

private:
  //==========================================================================
  // Thread callback
  void run() override;

  //==========================================================================
  // Phase 1: ANALYZE - Determine Genre Context
  //==========================================================================

  /**
   * @brief Analyze the current project to determine genre context
   *
   * Examines tempo, existing track names, and any existing samples
   * to build a GenreContext for targeted searching.
   */
  void analyzeProjectContext();

  /**
   * @brief Use AI (Grok) to refine genre detection
   */
  void refineGenreWithAI();

  /**
   * @brief Detect genre from tempo range
   */
  juce::String detectGenreFromTempo(double tempo);

  //==========================================================================
  // Phase 2: PLAN - Generate Search Queries
  //==========================================================================

  /**
   * @brief Generate search queries based on genre context
   */
  void generateSearchQueries();

  /**
   * @brief Use AI to generate creative search queries
   */
  void generateAiSearchQueries();

  /**
   * @brief Build the download queue from search results
   */
  void buildDownloadQueue();

  /**
   * @brief Filter URLs by safety and quality rules
   */
  bool filterUrl(const juce::String &url, const juce::String &sourceName);

  //==========================================================================
  // Phase 3: EXECUTE - Download and Process
  //==========================================================================

  /**
   * @brief Execute a web search query
   * @return URLs found
   */
  juce::StringArray executeSearch(const juce::String &query,
                                  const SampleSource &source);

  /**
   * @brief Download a single sample file
   * @return true if download succeeded
   */
  bool downloadSample(FoundSample &sample);

  /**
   * @brief Analyze a downloaded sample (Key/BPM/Instrument)
   */
  void analyzeSample(FoundSample &sample);

  /**
   * @brief Generate smart filename based on analysis
   */
  juce::String generateSmartFilename(const FoundSample &sample);

  /**
   * @brief Import sample into the AudioFilePool
   */
  bool importToPool(FoundSample &sample);

  //==========================================================================
  // Helpers
  //==========================================================================

  void setStatus(const juce::String &status);
  void updateProgress(float progress);
  void notifyListeners();

  /**
   * @brief Check if a URL is from a blocked site
   */
  bool isBlockedSite(const juce::String &url);

  /**
   * @brief Generate unique hash for duplicate detection
   */
  juce::String generateFileHash(const juce::File &file);

  /**
   * @brief Check if we already have this sample
   */
  bool isDuplicate(const juce::File &file);

  //==========================================================================
  // Member Variables
  //==========================================================================

  Engine &engine_;
  HuntingConfig config_;
  HuntingStats stats_;
  GenreContext genreContext_;

  // Thread state
  std::atomic<bool> isHunting_{false};
  std::atomic<float> progress_{0.0f};
  juce::CriticalSection statusLock_;
  juce::String currentStatus_;

  // Sample sources (trusted sites)
  std::vector<SampleSource> sources_;

  // Search and download queues
  juce::StringArray searchQueue_;
  std::vector<FoundSample> foundSamples_;
  std::queue<size_t> downloadQueue_; // Indices into foundSamples_

  // Duplicate detection
  std::unordered_set<juce::String> downloadedHashes_;

  // Audio analysis service
  std::unique_ptr<AudioAnalysisService> analysisService_;

  // AI client for smart query generation
  std::unique_ptr<GrokAPIClient> grokClient_;

  // Listeners
  juce::ListenerList<Listener> listeners_;

  //==========================================================================
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleHunterAgent)
};

//==============================================================================
/**
    Default sample sources with royalty-free content
*/
inline std::vector<SampleSource> getDefaultSampleSources() {
  return {
      // Top-tier free sample sites
      SampleSource("Freesound", "https://freesound.org",
                   "https://freesound.org/search/?q={query}&filter=license:cc0",
                   "CC0", false, 10),

      SampleSource("SampleFocus", "https://samplefocus.com",
                   "https://samplefocus.com/search?q={query}", "Royalty-Free",
                   false, 9),

      SampleSource("Looperman", "https://www.looperman.com",
                   "https://www.looperman.com/loops?q={query}", "Royalty-Free",
                   true, 8), // Requires credit

      SampleSource("99Sounds", "https://99sounds.org",
                   "https://99sounds.org/?s={query}", "Royalty-Free", false, 7),

      SampleSource("SampleSwap", "https://sampleswap.org",
                   "https://sampleswap.org/search.php?q={query}", "CC-BY", true,
                   6),

      // GitHub sample repositories
      SampleSource(
          "GitHub Samples", "https://github.com",
          "https://github.com/search?q={query}+extension:wav&type=code",
          "Various", false, 5),
  };
}

} // namespace ai
} // namespace zenith
