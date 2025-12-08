/*
  ==============================================================================

    SampleHunterAgent.cpp
    Created: 2025-12-08
    Author:  Zenith DAW AI Team

    Implementation of the Sample Hunter Agent - "The Crate Digger"

    This agent performs labor-intensive tasks:
    1. Web searching for royalty-free samples
    2. Downloading audio files
    3. FFT analysis for Key/BPM detection
    4. Smart renaming and tagging
    5. AudioFilePool integration

    Expected runtime: 15-30 minutes for a full hunting session

  ==============================================================================
*/

#include "SampleHunterAgent.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_cryptography/juce_cryptography.h>

namespace zenith {
namespace ai {

//==============================================================================
// Forward declarations and helper functions
//==============================================================================

static juce::String frequencyToKeyName(double freq);

// Helper: Check if string contains any of the space-separated keywords
static bool containsAnyKeyword(const juce::String &str,
                               const juce::String &keywords) {
  juce::StringArray keywordList;
  keywordList.addTokens(keywords, " ", "");
  for (const auto &keyword : keywordList) {
    if (str.contains(keyword))
      return true;
  }
  return false;
}

// Helper: Check if string ends with any of the given extensions
static bool
endsWithAnyExtension(const juce::String &str,
                     const std::initializer_list<const char *> &extensions) {
  for (const auto &ext : extensions) {
    if (str.endsWith(ext))
      return true;
  }
  return false;
}

//==============================================================================
// Constructor / Destructor
//==============================================================================

SampleHunterAgent::SampleHunterAgent(Engine &engine)
    : juce::Thread("SampleHunterThread"), engine_(engine) {
  // Initialize with default sources
  resetToDefaultSources();

  // Create analysis service
  analysisService_ = std::make_unique<AudioAnalysisService>();

  // Create Grok client for AI-powered query generation
  grokClient_ = std::make_unique<GrokAPIClient>();

  DBG("SampleHunterAgent: Initialized with " + juce::String(sources_.size()) +
      " sample sources");
}

SampleHunterAgent::~SampleHunterAgent() {
  // Ensure thread is stopped
  stopHunting();
}

//==============================================================================
// Agent Control
//==============================================================================

void SampleHunterAgent::startHunting(const HuntingConfig &config) {
  if (isHunting_.load()) {
    DBG("SampleHunterAgent: Already hunting, ignoring start request");
    return;
  }

  // Store configuration
  config_ = config;

  // Create download directory if it doesn't exist
  if (!config_.downloadDirectory.exists()) {
    config_.downloadDirectory.createDirectory();
  }

  // Reset stats
  stats_ = HuntingStats();
  stats_.startTime = juce::Time::getCurrentTime();

  // Clear previous results
  foundSamples_.clear();
  searchQueue_.clear();
  while (!downloadQueue_.empty())
    downloadQueue_.pop();
  downloadedHashes_.clear();

  // Start the thread
  isHunting_.store(true);
  progress_.store(0.0f);
  setStatus("Starting sample hunt...");

  startThread();

  DBG("SampleHunterAgent: Sample hunting started");
}

void SampleHunterAgent::stopHunting() {
  if (!isHunting_.load())
    return;

  setStatus("Stopping sample hunt...");
  signalThreadShouldExit();
  stopThread(5000);

  isHunting_.store(false);
  stats_.endTime = juce::Time::getCurrentTime();

  DBG("SampleHunterAgent: Sample hunting stopped");
}

juce::String SampleHunterAgent::getStatusMessage() const {
  juce::ScopedLock lock(statusLock_);
  return currentStatus_;
}

std::vector<juce::File> SampleHunterAgent::getImportedFiles() const {
  std::vector<juce::File> result;
  for (const auto &sample : foundSamples_) {
    if (sample.downloaded && sample.analyzed && sample.localFile.exists()) {
      result.push_back(sample.localFile);
    }
  }
  return result;
}

//==============================================================================
// Source Management
//==============================================================================

void SampleHunterAgent::addSource(const SampleSource &source) {
  sources_.push_back(source);
}

void SampleHunterAgent::resetToDefaultSources() {
  sources_ = getDefaultSampleSources();
}

//==============================================================================
// Listener Management
//==============================================================================

void SampleHunterAgent::addListener(Listener *listener) {
  listeners_.add(listener);
}

void SampleHunterAgent::removeListener(Listener *listener) {
  listeners_.remove(listener);
}

//==============================================================================
// Main Thread Entry Point
//==============================================================================

void SampleHunterAgent::run() {
  DBG("SampleHunterAgent: Thread started");

  try {
    //==========================================================================
    // PHASE 1: ANALYZE - Determine Genre Context
    //==========================================================================
    setStatus("Phase 1: Analyzing project context...");
    updateProgress(0.05f);

    if (threadShouldExit())
      return;

    analyzeProjectContext();

    if (threadShouldExit())
      return;

    // Optionally refine with AI
    if (grokClient_->hasAPIKey()) {
      setStatus("Phase 1: Refining genre detection with AI...");
      refineGenreWithAI();
    }

    updateProgress(0.10f);

    //==========================================================================
    // PHASE 2: PLAN - Generate Search Queries
    //==========================================================================
    setStatus("Phase 2: Generating search queries...");

    if (threadShouldExit())
      return;

    generateSearchQueries();

    if (grokClient_->hasAPIKey()) {
      setStatus("Phase 2: Generating AI-powered queries...");
      generateAiSearchQueries();
    }

    updateProgress(0.15f);

    DBG("SampleHunterAgent: Generated " + juce::String(searchQueue_.size()) +
        " search queries");

    //==========================================================================
    // PHASE 3: EXECUTE - Search, Download, Analyze, Import
    //==========================================================================

    // Step 3a: Execute searches
    setStatus("Phase 3: Executing web searches...");

    int queryCount = 0;
    for (const auto &query : searchQueue_) {
      if (threadShouldExit())
        return;

      for (const auto &source : sources_) {
        if (threadShouldExit())
          return;

        setStatus("Searching: " + source.name + " for \"" + query + "\"...");

        juce::StringArray urls = executeSearch(query, source);
        stats_.searchQueriesExecuted++;

        // Add found URLs to our sample list
        for (const auto &url : urls) {
          if (threadShouldExit())
            return;

          stats_.urlsFound++;

          // Apply filters
          if (!filterUrl(url, source.name)) {
            stats_.urlsFiltered++;
            continue;
          }

          // Create a found sample entry
          FoundSample sample;
          sample.downloadUrl = url;
          sample.originalName = juce::URL(url).getFileName();
          sample.sourceName = source.name;
          sample.licenseType = source.licenseType;

          // Guess category from URL/filename
          juce::String lowerUrl = url.toLowerCase();
          if (containsAnyKeyword(lowerUrl, "kick drum snare hat perc"))
            sample.category = "drums";
          else if (containsAnyKeyword(lowerUrl, "bass sub 808"))
            sample.category = "bass";
          else if (lowerUrl.contains("loop"))
            sample.category = "loop";
          else
            sample.category = "oneshot";

          foundSamples_.push_back(sample);
          downloadQueue_.push(foundSamples_.size() - 1);

          // Limit total samples
          if (foundSamples_.size() >=
              static_cast<size_t>(config_.maxTotalDownloads)) {
            break;
          }
        }

        // Rate limiting between searches
        if (!threadShouldExit()) {
          wait(config_.delayBetweenSearchesMs);
        }

        if (foundSamples_.size() >=
            static_cast<size_t>(config_.maxTotalDownloads)) {
          break;
        }
      }

      queryCount++;
      updateProgress(0.15f + (0.25f * queryCount / searchQueue_.size()));

      if (queryCount >= config_.maxSearchQueries)
        break;
    }

    DBG("SampleHunterAgent: Found " + juce::String(foundSamples_.size()) +
        " potential samples");

    // Step 3b: Download and analyze samples
    float downloadStartProgress = 0.40f;
    float downloadEndProgress = 0.95f;
    size_t downloadCount = 0;
    size_t totalToDownload = downloadQueue_.size();

    while (!downloadQueue_.empty() && !threadShouldExit()) {
      size_t sampleIndex = downloadQueue_.front();
      downloadQueue_.pop();

      if (sampleIndex >= foundSamples_.size())
        continue;

      FoundSample &sample = foundSamples_[sampleIndex];

      setStatus("Downloading (" + juce::String(downloadCount + 1) + "/" +
                juce::String(totalToDownload) + "): " + sample.originalName);

      // Download
      stats_.downloadsAttempted++;
      bool downloaded = downloadSample(sample);

      if (downloaded) {
        stats_.downloadsSucceeded++;
        stats_.totalBytesDownloaded += sample.localFile.getSize();

        // Notify listeners
        juce::MessageManager::callAsync([this, sample]() {
          listeners_.call(&Listener::sampleDownloaded, sample);
        });

        // Analyze
        setStatus("Analyzing: " + sample.originalName);
        analyzeSample(sample);
        stats_.samplesAnalyzed++;

        // Notify listeners
        juce::MessageManager::callAsync([this, sample]() {
          listeners_.call(&Listener::sampleAnalyzed, sample);
        });

        // Import to AudioFilePool
        if (importToPool(sample)) {
          stats_.samplesImported++;

          juce::File importedFile = sample.localFile;
          juce::MessageManager::callAsync([this, importedFile]() {
            listeners_.call(&Listener::sampleImported, importedFile);
          });
        }
      } else {
        stats_.downloadsFailed++;
      }

      downloadCount++;
      float downloadProgress = static_cast<float>(downloadCount) /
                               static_cast<float>(totalToDownload);
      updateProgress(downloadStartProgress +
                     (downloadEndProgress - downloadStartProgress) *
                         downloadProgress);

      // Rate limiting between downloads
      if (!threadShouldExit()) {
        wait(config_.delayBetweenDownloadsMs);
      }
    }

    //==========================================================================
    // COMPLETE
    //==========================================================================
    stats_.endTime = juce::Time::getCurrentTime();
    updateProgress(1.0f);
    setStatus("Sample hunt complete!");

    DBG("SampleHunterAgent: Hunt complete - " + stats_.getSummary());

    // Notify completion
    juce::MessageManager::callAsync([this]() {
      listeners_.call(&Listener::huntingComplete, stats_, true);
      sendChangeMessage();
    });

  } catch (const std::exception &e) {
    DBG("SampleHunterAgent: Exception - " + juce::String(e.what()));
    stats_.endTime = juce::Time::getCurrentTime();
    setStatus("Error: " + juce::String(e.what()));

    juce::MessageManager::callAsync([this]() {
      listeners_.call(&Listener::huntingComplete, stats_, false);
    });
  }

  isHunting_.store(false);
  DBG("SampleHunterAgent: Thread finished");
}

//==============================================================================
// Phase 1: ANALYZE
//==============================================================================

void SampleHunterAgent::analyzeProjectContext() {
  ProjectState *projectState = engine_.getProjectState();
  if (!projectState) {
    genreContext_.primaryGenre = "General";
    genreContext_.estimatedBpm = 120.0;
    return;
  }

  // Get tempo
  double tempo = projectState->getTempo();
  genreContext_.estimatedBpm = tempo;

  // Detect genre from tempo
  genreContext_.primaryGenre = detectGenreFromTempo(tempo);

  // Add target instruments based on genre
  juce::String genre = genreContext_.primaryGenre.toLowerCase();

  if (genre.contains("hip") || genre.contains("hop") ||
      genre.contains("lofi")) {
    genreContext_.targetInstruments = {"drums", "piano", "bass", "vinyl",
                                       "keys"};
    genreContext_.styleKeywords = {"chill", "jazzy", "dusty", "mellow"};
    genreContext_.subGenre =
        (tempo < 85) ? "Lo-Fi" : (tempo > 95 ? "Boom Bap" : "Chill Hop");
  } else if (genre.contains("techno") || genre.contains("house")) {
    genreContext_.targetInstruments = {"kick", "synth", "bass", "percussion",
                                       "fx"};
    genreContext_.styleKeywords = {"punchy", "driving", "dark", "acid"};
    genreContext_.subGenre = (tempo > 135) ? "Hard Techno" : "Deep House";
  } else if (genre.contains("dnb") || genre.contains("drum")) {
    genreContext_.targetInstruments = {"breaks", "bass", "reese", "amen",
                                       "pads"};
    genreContext_.styleKeywords = {"jungle", "liquid", "neurofunk"};
  } else if (genre.contains("ambient")) {
    genreContext_.targetInstruments = {"pads", "texture", "atmosphere",
                                       "drones"};
    genreContext_.styleKeywords = {"ethereal", "atmospheric", "cinematic"};
  } else {
    // General/Pop
    genreContext_.targetInstruments = {"drums", "bass", "synth", "guitar",
                                       "vocals"};
    genreContext_.styleKeywords = {"modern", "crisp", "clean"};
  }

  DBG("SampleHunterAgent: Detected genre context - " +
      genreContext_.toString());
}

void SampleHunterAgent::refineGenreWithAI() {
  // Build a prompt for Grok to analyze and suggest search terms
  if (!grokClient_->hasAPIKey()) {
    return;
  }

  juce::String prompt =
      "You are a music producer AI assistant. Based on the following project "
      "context, suggest 3 niche sub-genres or production styles that would fit "
      "well, and 5 specific search keywords for finding royalty-free "
      "samples.\n\n"
      "Project Context:\n"
      "- Tempo: " +
      juce::String(genreContext_.estimatedBpm, 1) +
      " BPM\n"
      "- Primary Genre: " +
      genreContext_.primaryGenre +
      "\n\n"
      "Respond in this exact format:\n"
      "SUBGENRES: subgenre1, subgenre2, subgenre3\n"
      "KEYWORDS: keyword1, keyword2, keyword3, keyword4, keyword5";

  std::atomic<bool> responseReceived{false};
  juce::String aiResponse;

  grokClient_->sendChat(
      prompt, GrokMode::Fast, {},           // No functions
      "You are a music production expert.", // System prompt
      [&](const juce::String &response) {
        aiResponse = response;
        responseReceived.store(true);
      },
      [&](const GrokFunctionCall & /*call*/) {
        // Not expecting function calls
        responseReceived.store(true);
      },
      [&](const juce::String &error) {
        DBG("SampleHunterAgent: AI refinement error - " + error);
        responseReceived.store(true);
      });

  // Wait for response with timeout
  int waitCount = 0;
  while (!responseReceived.load() && waitCount < 30 && !threadShouldExit()) {
    wait(500);
    waitCount++;
  }

  // Parse AI response
  if (aiResponse.isNotEmpty()) {
    // Extract subgenres
    if (aiResponse.contains("SUBGENRES:")) {
      juce::String subgenreLine =
          aiResponse.fromFirstOccurrenceOf("SUBGENRES:", false, true)
              .upToFirstOccurrenceOf("\n", false, true);
      juce::StringArray subgenres;
      subgenres.addTokens(subgenreLine, ",", "");
      if (subgenres.size() > 0) {
        genreContext_.subGenre = subgenres[0].trim();
      }
    }

    // Extract keywords
    if (aiResponse.contains("KEYWORDS:")) {
      juce::String keywordLine =
          aiResponse.fromFirstOccurrenceOf("KEYWORDS:", false, true)
              .upToFirstOccurrenceOf("\n", false, true);
      juce::StringArray keywords;
      keywords.addTokens(keywordLine, ",", "");
      for (const auto &kw : keywords) {
        if (kw.trim().isNotEmpty()) {
          genreContext_.styleKeywords.add(kw.trim());
        }
      }
    }

    DBG("SampleHunterAgent: AI refined genre - " + genreContext_.toString());
  }
}

juce::String SampleHunterAgent::detectGenreFromTempo(double tempo) {
  // Standard tempo ranges for genres
  if (tempo < 70)
    return "Ambient/Downtempo";
  else if (tempo >= 70 && tempo < 90)
    return "Lo-Fi Hip Hop";
  else if (tempo >= 90 && tempo < 100)
    return "Hip Hop";
  else if (tempo >= 100 && tempo < 115)
    return "Pop/R&B";
  else if (tempo >= 115 && tempo < 125)
    return "House";
  else if (tempo >= 125 && tempo < 140)
    return "Techno";
  else if (tempo >= 140 && tempo < 160)
    return "Trance";
  else if (tempo >= 160 && tempo < 180)
    return "Drum & Bass";
  else
    return "Hardcore/Speedcore";
}

//==============================================================================
// Phase 2: PLAN
//==============================================================================

void SampleHunterAgent::generateSearchQueries() {
  searchQueue_.clear();

  // Generate queries for each target instrument
  for (const auto &instrument : genreContext_.targetInstruments) {
    // Basic genre + instrument query
    searchQueue_.add("royalty free " + genreContext_.primaryGenre + " " +
                     instrument + " samples");

    // Add style variant
    if (!genreContext_.styleKeywords.isEmpty()) {
      juce::String style = genreContext_.styleKeywords[0];
      searchQueue_.add("free " + style + " " + instrument + " wav");
    }

    // CC0 specific query
    searchQueue_.add(instrument + " samples cc0 wav");
  }

  // Genre-specific queries
  searchQueue_.add("free " + genreContext_.primaryGenre + " sample pack");
  searchQueue_.add(genreContext_.primaryGenre + " loops royalty free wav");

  // BPM-specific for loops
  if (genreContext_.estimatedBpm > 0) {
    searchQueue_.add(
        "free loop " +
        juce::String(static_cast<int>(genreContext_.estimatedBpm)) + " bpm");
  }

  DBG("SampleHunterAgent: Generated " + juce::String(searchQueue_.size()) +
      " base search queries");
}

void SampleHunterAgent::generateAiSearchQueries() {
  if (!grokClient_->hasAPIKey())
    return;

  juce::String prompt =
      "Generate 5 creative web search queries to find free, royalty-free audio "
      "samples for a " +
      genreContext_.primaryGenre + " production at " +
      juce::String(genreContext_.estimatedBpm, 0) +
      " BPM.\n\n"
      "Requirements:\n"
      "- Focus on CC0, Creative Commons, or royalty-free samples\n"
      "- Target .wav files when possible\n"
      "- Include niche/underground sample sources\n"
      "- One query should focus on drum sounds\n"
      "- One query should focus on melodic elements\n\n"
      "Respond with exactly 5 search queries, one per line, no numbering:";

  std::atomic<bool> responseReceived{false};
  juce::String aiResponse;

  grokClient_->sendChat(
      prompt, GrokMode::Fast, {}, "You are a sample library curation expert.",
      [&](const juce::String &response) {
        aiResponse = response;
        responseReceived.store(true);
      },
      [&](const GrokFunctionCall & /*call*/) { responseReceived.store(true); },
      [&](const juce::String & /*error*/) { responseReceived.store(true); });

  int waitCount = 0;
  while (!responseReceived.load() && waitCount < 20 && !threadShouldExit()) {
    wait(500);
    waitCount++;
  }

  if (aiResponse.isNotEmpty()) {
    juce::StringArray lines;
    lines.addLines(aiResponse);
    for (const auto &line : lines) {
      juce::String query = line.trim();
      if (query.isNotEmpty() && query.length() > 10) {
        searchQueue_.add(query);
      }
    }
    DBG("SampleHunterAgent: AI added " + juce::String(lines.size()) +
        " search queries");
  }
}

bool SampleHunterAgent::filterUrl(const juce::String &url,
                                  const juce::String & /*sourceName*/) {
  // Check blocked sites
  if (isBlockedSite(url))
    return false;

  // Check file extension
  juce::String lowerUrl = url.toLowerCase();
  if (config_.onlyWavFiles) {
    if (!lowerUrl.endsWith(".wav"))
      return false;
  } else {
    // Allow common audio formats
    if (!endsWithAnyExtension(lowerUrl,
                              {".wav", ".mp3", ".aiff", ".flac", ".ogg"}))
      return false;
  }

  // Filter out obvious non-sample URLs
  if (lowerUrl.contains("youtube.com") || lowerUrl.contains("spotify.com") ||
      lowerUrl.contains("soundcloud.com") ||
      lowerUrl.contains("bandcamp.com")) {
    return false;
  }

  // Filter out very short filenames (likely garbage)
  juce::String filename = juce::URL(url).getFileName();
  if (filename.length() < 5)
    return false;

  return true;
}

//==============================================================================
// Phase 3: EXECUTE
//==============================================================================

juce::StringArray SampleHunterAgent::executeSearch(const juce::String &query,
                                                   const SampleSource &source) {
  juce::StringArray results;

  // Build search URL from template
  juce::String searchUrl = source.searchTemplate;
  searchUrl =
      searchUrl.replace("{query}", juce::URL::addEscapeChars(query, true));

  DBG("SampleHunterAgent: Searching " + searchUrl);

  // Create URL connection
  juce::URL url(searchUrl);

  // Use JUCE's URL input stream to fetch the page
  auto options =
      juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
          .withConnectionTimeoutMs(10000)
          .withResponseHeaders(nullptr);

  std::unique_ptr<juce::InputStream> stream = url.createInputStream(options);

  if (!stream) {
    DBG("SampleHunterAgent: Failed to connect to " + source.name);
    return results;
  }

  // Read response
  juce::String response = stream->readEntireStreamAsString();

  if (response.isEmpty()) {
    DBG("SampleHunterAgent: Empty response from " + source.name);
    return results;
  }

  // Parse response for audio file URLs
  // This is a simplified parser - real implementation would use proper HTML
  // parsing

  // Find all href and src attributes that might contain audio files
  int searchPos = 0;
  while (searchPos < response.length() &&
         results.size() < config_.maxResultsPerQuery) {
    // Look for potential audio file URLs
    int wavPos = response.indexOfIgnoreCase(searchPos, ".wav");
    int mp3Pos = response.indexOfIgnoreCase(searchPos, ".mp3");

    int foundPos = -1;
    juce::String extension;

    if (wavPos >= 0 && (mp3Pos < 0 || wavPos < mp3Pos)) {
      foundPos = wavPos;
      extension = ".wav";
    } else if (mp3Pos >= 0 && !config_.onlyWavFiles) {
      foundPos = mp3Pos;
      extension = ".mp3";
    }

    if (foundPos < 0)
      break;

    // Try to extract the full URL
    // Look backwards for start of URL
    int urlStart = foundPos;
    while (urlStart > 0 &&
           !juce::CharacterFunctions::isWhitespace(response[urlStart - 1]) &&
           response[urlStart - 1] != '"' && response[urlStart - 1] != '\'') {
      urlStart--;
    }

    // Check if this looks like a URL
    juce::String potentialUrl = response.substring(urlStart, foundPos + 4);

    if (potentialUrl.startsWith("http") || potentialUrl.startsWith("//")) {
      // Fix protocol-relative URLs
      if (potentialUrl.startsWith("//")) {
        potentialUrl = "https:" + potentialUrl;
      }

      // Clean up the URL
      if (potentialUrl.contains("\""))
        potentialUrl = potentialUrl.upToFirstOccurrenceOf("\"", false, true);
      if (potentialUrl.contains("'"))
        potentialUrl = potentialUrl.upToFirstOccurrenceOf("'", false, true);

      if (juce::URL::isProbablyAWebsiteURL(potentialUrl)) {
        results.addIfNotAlreadyThere(potentialUrl);
      }
    }

    searchPos = foundPos + 4;
  }

  DBG("SampleHunterAgent: Found " + juce::String(results.size()) +
      " URLs from " + source.name);

  return results;
}

bool SampleHunterAgent::downloadSample(FoundSample &sample) {
  if (sample.downloadUrl.isEmpty())
    return false;

  // Generate target path
  juce::String safeFilename = sample.originalName;
  safeFilename = safeFilename.replaceCharacters(":/\\?*\"<>|", "_________")
                     .substring(0, 100);

  juce::File targetFile = config_.downloadDirectory.getChildFile(safeFilename);

  // Make sure filename is unique
  if (targetFile.exists()) {
    targetFile = targetFile.getNonexistentSibling(true);
  }

  DBG("SampleHunterAgent: Downloading to " + targetFile.getFullPathName());

  // Download the file
  juce::URL url(sample.downloadUrl);

  auto options =
      juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
          .withConnectionTimeoutMs(30000)
          .withResponseHeaders(nullptr);

  std::unique_ptr<juce::InputStream> stream = url.createInputStream(options);

  if (!stream) {
    DBG("SampleHunterAgent: Failed to connect for download");
    return false;
  }

  // Create output file
  juce::FileOutputStream output(targetFile);
  if (!output.openedOk()) {
    DBG("SampleHunterAgent: Failed to create output file");
    return false;
  }

  // Copy data
  constexpr int bufferSize = 65536;
  juce::HeapBlock<char> buffer(bufferSize);
  int64_t totalBytes = 0;

  while (!stream->isExhausted() && !threadShouldExit()) {
    int bytesRead = stream->read(buffer.getData(), bufferSize);
    if (bytesRead <= 0)
      break;

    output.write(buffer.getData(), static_cast<size_t>(bytesRead));
    totalBytes += bytesRead;

    // File size limit check
    if (totalBytes > config_.maxFileSizeMb * 1024 * 1024) {
      DBG("SampleHunterAgent: File too large, aborting download");
      output.flush();
      targetFile.deleteFile();
      return false;
    }
  }

  output.flush();

  if (totalBytes == 0) {
    targetFile.deleteFile();
    return false;
  }

  // Check for duplicate
  if (config_.skipDuplicates && isDuplicate(targetFile)) {
    DBG("SampleHunterAgent: Duplicate detected, skipping");
    targetFile.deleteFile();
    return false;
  }

  sample.localFile = targetFile;
  sample.downloaded = true;
  sample.estimatedSizeBytes = totalBytes;

  DBG("SampleHunterAgent: Downloaded " +
      juce::File::descriptionOfSizeInBytes(totalBytes));

  return true;
}

void SampleHunterAgent::analyzeSample(FoundSample &sample) {
  if (!sample.localFile.exists())
    return;

  // Use Python-based AudioAnalysisService if available
  if (analysisService_->isAvailable()) {
    std::atomic<bool> analysisComplete{false};
    AudioAnalysisResults results;

    analysisService_->analyzeAudioFile(
        sample.localFile,
        [&](const AudioAnalysisResults &r) {
          results = r;
          analysisComplete.store(true);
        },
        [&](const juce::String &error) {
          DBG("SampleHunterAgent: Analysis error - " + error);
          analysisComplete.store(true);
        });

    // Wait for analysis (with timeout)
    int waitCount = 0;
    while (!analysisComplete.load() && waitCount < 60 && !threadShouldExit()) {
      wait(500);
      waitCount++;
    }

    if (results.success) {
      sample.detectedBpm = results.tempoBpm;
      sample.brightness = results.brightness;

      // Convert dominant frequency to key (simplified)
      if (results.dominantFrequencyHz > 0) {
        sample.detectedKey = frequencyToKeyName(results.dominantFrequencyHz);
      }

      // Classify instrument based on spectral content
      if (results.subBassDb + results.bassDb >
          results.midsDb + results.highMidsDb + results.brillianceDb) {
        sample.instrumentType = "bass";
      } else if (results.subBassDb > -40 && results.zeroCrossingRate < 0.2) {
        sample.instrumentType = "kick";
      } else if (results.brillianceDb > -30 && results.zeroCrossingRate > 0.3) {
        sample.instrumentType = "hihat";
      } else if (results.midsDb > results.subBassDb + results.bassDb) {
        sample.instrumentType = "melodic";
      } else {
        sample.instrumentType = "percussion";
      }
    }
  } else {
    // Fallback: Basic analysis using JUCE audio reading
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader(
        formatManager.createReaderFor(sample.localFile));

    if (reader) {
      // Basic duration check
      double duration =
          static_cast<double>(reader->lengthInSamples) / reader->sampleRate;

      // Skip if too short or too long
      if (duration < config_.minDurationSeconds ||
          duration > config_.maxDurationSeconds) {
        sample.localFile.deleteFile();
        sample.downloaded = false;
        return;
      }

      // Very basic classification from filename
      juce::String lowerName = sample.originalName.toLowerCase();
      if (lowerName.contains("kick"))
        sample.instrumentType = "kick";
      else if (containsAnyKeyword(lowerName, "snare clap"))
        sample.instrumentType = "snare";
      else if (containsAnyKeyword(lowerName, "hat hh hihat"))
        sample.instrumentType = "hihat";
      else if (containsAnyKeyword(lowerName, "bass sub"))
        sample.instrumentType = "bass";
      else if (containsAnyKeyword(lowerName, "pad ambient"))
        sample.instrumentType = "pad";
      else if (lowerName.contains("loop"))
        sample.instrumentType = "loop";
      else
        sample.instrumentType = "oneshot";
    }
  }

  sample.analyzed = true;

  // Rename file with smart naming
  juce::String newName = generateSmartFilename(sample);
  juce::File newFile = sample.localFile.getSiblingFile(newName);

  if (newFile != sample.localFile && !newFile.exists()) {
    sample.localFile.moveFileTo(newFile);
    sample.localFile = newFile;
  }
}

juce::String
SampleHunterAgent::generateSmartFilename(const FoundSample &sample) {
  juce::String filename;

  // Type prefix
  if (sample.instrumentType.isNotEmpty()) {
    filename += sample.instrumentType.substring(0, 1).toUpperCase() +
                sample.instrumentType.substring(1);
  } else {
    filename += "Sample";
  }

  // Key (if detected)
  if (sample.detectedKey.isNotEmpty()) {
    filename += "_" + sample.detectedKey;
  }

  // BPM (if detected and > 0)
  if (sample.detectedBpm > 0) {
    filename +=
        "_" + juce::String(static_cast<int>(sample.detectedBpm)) + "bpm";
  }

  // Brightness
  if (sample.brightness.isNotEmpty() && sample.brightness != "neutral") {
    filename += "_" + sample.brightness;
  }

  // Source tag
  filename += "_" + sample.sourceName.replace(" ", "").substring(0, 6);

  // Unique identifier
  filename +=
      "_" + juce::String::toHexString(juce::Random::getSystemRandom().nextInt())
                .substring(0, 4);

  // Extension
  juce::String extension = sample.localFile.getFileExtension();
  if (extension.isEmpty())
    extension = ".wav";

  filename += extension;

  return filename;
}

bool SampleHunterAgent::importToPool(FoundSample &sample) {
  if (!sample.localFile.exists())
    return false;

  // Get AudioFilePool from Engine (if available)
  // The actual implementation depends on how AudioFilePool is exposed
  // For now, we'll just ensure the file is in the right location

  ProjectState *projectState = engine_.getProjectState();
  if (projectState) {
    // The file is already in the download directory
    // We could add it to a project-specific sample browser here
    DBG("SampleHunterAgent: Imported " + sample.localFile.getFileName());
    return true;
  }

  return true;
}

//==============================================================================
// Helpers
//==============================================================================

void SampleHunterAgent::setStatus(const juce::String &status) {
  {
    juce::ScopedLock lock(statusLock_);
    currentStatus_ = status;
  }
  DBG("SampleHunterAgent: " + status);
}

void SampleHunterAgent::updateProgress(float progress) {
  progress_.store(juce::jlimit(0.0f, 1.0f, progress));

  juce::MessageManager::callAsync([this, progress]() {
    listeners_.call(&Listener::huntingProgressChanged, progress,
                    currentStatus_);
  });
}

bool SampleHunterAgent::isBlockedSite(const juce::String &url) {
  juce::String lowerUrl = url.toLowerCase();

  // Built-in blocked sites (known copyright issues)
  juce::StringArray defaultBlocked = {"splice.com", "loopmasters.com",
                                      "native-instruments.com", "output.com",
                                      "beatport.com"};

  for (const auto &blocked : defaultBlocked) {
    if (lowerUrl.contains(blocked))
      return true;
  }

  // User-configured blocked sites
  for (const auto &blocked : config_.blockedSites) {
    if (lowerUrl.contains(blocked.toLowerCase()))
      return true;
  }

  return false;
}

juce::String SampleHunterAgent::generateFileHash(const juce::File &file) {
  // Read first 64KB and hash it (fast duplicate detection)
  constexpr int hashSize = 65536;

  juce::FileInputStream stream(file);
  if (!stream.openedOk())
    return "";

  juce::MemoryBlock buffer(hashSize);
  int bytesRead = stream.read(buffer.getData(), hashSize);

  if (bytesRead <= 0)
    return "";

  // Use MD5 for hashing
  juce::MD5 md5(buffer.getData(), static_cast<size_t>(bytesRead));
  return md5.toHexString();
}

bool SampleHunterAgent::isDuplicate(const juce::File &file) {
  juce::String hash = generateFileHash(file);
  if (hash.isEmpty())
    return false;

  if (downloadedHashes_.count(hash) > 0)
    return true;

  downloadedHashes_.insert(hash);
  return false;
}

// Helper function to convert frequency to key name
static juce::String frequencyToKeyName(double freq) {
  // A4 = 440 Hz
  if (freq <= 0)
    return "";

  // Calculate semitones from A4
  double semitones = 12.0 * std::log2(freq / 440.0);
  int noteNumber =
      static_cast<int>(std::round(semitones)) + 69; // MIDI note for A4

  // Note names
  static const char *noteNames[] = {"C",  "C#", "D",  "D#", "E",  "F",
                                    "F#", "G",  "G#", "A",  "A#", "B"};

  int noteIndex = ((noteNumber % 12) + 12) % 12;
  int octave = (noteNumber / 12) - 1;

  return juce::String(noteNames[noteIndex]) + juce::String(octave);
}

} // namespace ai
} // namespace zenith
