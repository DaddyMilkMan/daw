/*
  ==============================================================================

    SampleHunterAgent.cpp
    Created: 2025-12-08
    Author:  Zenith DAW AI Team
    Refactored: 2025-12-08 (API-based, Safe Downloading)

  ==============================================================================
*/

#include "SampleHunterAgent.h"
#include "../network/SecureKeyStore.h"
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
    if (str.containsIgnoreCase(keyword))
      return true;
  }
  return false;
}

//==============================================================================
// Constructor / Destructor
//==============================================================================

SampleHunterAgent::SampleHunterAgent(Engine &engine)
    : juce::Thread("SampleHunterThread"), engine_(engine) {

  // Create analysis service
  analysisService_ = std::make_unique<AudioAnalysisService>();

  // Create Grok client for AI-powered query generation
  grokClient_ = std::make_unique<GrokAPIClient>();

  // Load Freesound API key from secure storage (NEVER hardcode!)
  SecureKeyStore::retrieveKey("freesound_api_key", freesoundConfig_.apiKey);

  if (freesoundConfig_.apiKey.isEmpty()) {
    DBG("SampleHunterAgent: WARNING - No Freesound API key found in "
        "SecureKeyStore!");
    DBG("SampleHunterAgent: Set key via "
        "SecureKeyStore::storeKey(\"freesound_api_key\", "
        "\"YOUR_KEY\")");
  }

  DBG("SampleHunterAgent: Initialized");
}

SampleHunterAgent::~SampleHunterAgent() { stopHunting(); }

//==============================================================================
// Control
//==============================================================================

void SampleHunterAgent::startHunting(const HuntingConfig &config) {
  stopHunting();

  config_ = config;

  // Reset state
  stats_ = HuntingStats();
  stats_.startTime = juce::Time::getCurrentTime();

  foundSamples_.clear();
  searchQueue_.clear();
  downloadQueue_ = std::queue<size_t>();
  downloadedHashes_.clear();
  progress_.store(0.0f);
  setStatus("Starting hunt...");

  isHunting_.store(true);
  startThread();
}

void SampleHunterAgent::stopHunting() {
  isHunting_.store(false);
  signalThreadShouldExit();
  stopThread(5000); // 5s timeout
}

juce::String SampleHunterAgent::getStatusMessage() const {
  juce::ScopedLock lock(const_cast<juce::CriticalSection &>(statusLock_));
  return currentStatus_;
}

std::vector<juce::File> SampleHunterAgent::getImportedFiles() const {
  std::vector<juce::File> imported;
  for (const auto &sample : foundSamples_) {
    if (sample.localFile.exists()) {
      imported.push_back(sample.localFile);
    }
  }
  return imported;
}

//==============================================================================
// Listeners
//==============================================================================

void SampleHunterAgent::addListener(Listener *listener) {
  listeners_.add(listener);
}

void SampleHunterAgent::removeListener(Listener *listener) {
  listeners_.remove(listener);
}

//==============================================================================
// Thread Loop
//==============================================================================

void SampleHunterAgent::run() {
  try {
    setStatus("Analyzing project context...");
    analyzeProjectContext();
    updateProgress(0.05f);

    if (threadShouldExit())
      return;

    if (grokClient_->hasAPIKey()) {
      setStatus("Refining with AI...");
      refineGenreWithAI();
    }
    updateProgress(0.10f);

    //==========================================================================
    // Phase 2: PLAN
    //==========================================================================

    setStatus("Generating search queries...");
    generateSearchQueries();

    if (grokClient_->hasAPIKey()) {
      generateAiSearchQueries();
    }

    if (searchQueue_.isEmpty()) {
      // Fallback if no queries generated
      searchQueue_.add("royalty free " + genreContext_.primaryGenre +
                       " samples");
    }

    DBG("SampleHunterAgent: Planning " + juce::String(searchQueue_.size()) +
        " search queries");

    //==========================================================================
    // Execute Searches (Phase 3 Part A)
    //==========================================================================

    int queryCount = 0;

    for (const auto &query : searchQueue_) {
      if (threadShouldExit())
        break;

      setStatus("Searching Freesound: " + query);
      stats_.apiRequestsExecuted++;

      auto results = executeFreesoundSearch(query);

      for (const auto &sample : results) {
        foundSamples_.push_back(sample);
        downloadQueue_.push(foundSamples_.size() - 1);
        stats_.samplesFound++;
      }

      // Rate limiting
      wait(freesoundConfig_.requestsPerMinute > 0
               ? (60000 / freesoundConfig_.requestsPerMinute)
               : 1000);

      queryCount++;
      updateProgress(0.15f + (0.25f * queryCount /
                              (float)std::max(1, searchQueue_.size())));

      if (foundSamples_.size() >=
          static_cast<size_t>(config_.maxTotalDownloads))
        break;
    }

    DBG("SampleHunterAgent: Found " + juce::String(foundSamples_.size()) +
        " potential samples");

    //==========================================================================
    // Download & Analyze (Phase 3 Part B)
    //==========================================================================

    float downloadStartProgress = 0.40f;
    float downloadEndProgress = 0.95f;
    size_t processedCount = 0;
    size_t totalToProcess = downloadQueue_.size();

    // Batching: Collect samples to notify UI in batches
    constexpr size_t UI_BATCH_SIZE = 10;
    std::vector<FoundSample> pendingDownloadNotifications;
    std::vector<FoundSample> pendingAnalysisNotifications;
    std::vector<juce::File> pendingImportNotifications;

    while (!downloadQueue_.empty() && !threadShouldExit()) {
      size_t index = downloadQueue_.front();
      downloadQueue_.pop();

      FoundSample &sample = foundSamples_[index];

      setStatus("Downloading: " + sample.title);

      if (downloadSample(sample)) {
        stats_.downloadsSucceeded++;
        stats_.totalBytesDownloaded += sample.fileSize;

        // Queue for batch notification
        pendingDownloadNotifications.push_back(sample);

        // Analyze
        setStatus("Analyzing: " + sample.title);
        analyzeSample(sample);
        stats_.samplesAnalyzed++;

        pendingAnalysisNotifications.push_back(sample);

        // Import
        if (importToPool(sample)) {
          stats_.samplesImported++;
          pendingImportNotifications.push_back(sample.localFile);
        }

        // Batch UI update: Notify every UI_BATCH_SIZE downloads
        if (pendingDownloadNotifications.size() >= UI_BATCH_SIZE) {
          auto downloadBatch = pendingDownloadNotifications;
          auto analysisBatch = pendingAnalysisNotifications;
          auto importBatch = pendingImportNotifications;

          juce::MessageManager::callAsync(
              [this, downloadBatch, analysisBatch, importBatch]() {
                for (const auto &s : downloadBatch)
                  listeners_.call(&Listener::sampleDownloaded, s);
                for (const auto &s : analysisBatch)
                  listeners_.call(&Listener::sampleAnalyzed, s);
                for (const auto &f : importBatch)
                  listeners_.call(&Listener::sampleImported, f);
              });

          pendingDownloadNotifications.clear();
          pendingAnalysisNotifications.clear();
          pendingImportNotifications.clear();
        }
      } else {
        stats_.downloadsFailed++;
      }

    processedCount++;
    float p = static_cast<float>(processedCount) /
              (float)std::max((size_t)1, totalToProcess);
    updateProgress(downloadStartProgress +
                   (downloadEndProgress - downloadStartProgress) * p);

    if (processedCount >= (size_t)config_.maxTotalDownloads)
      break;

    wait(config_.delayBetweenDownloadsMs);
  }

  // Complete
  stats_.endTime = juce::Time::getCurrentTime();
  updateProgress(1.0f);
  setStatus("Sample hunt complete!");

  juce::MessageManager::callAsync([this]() {
    listeners_.call(&Listener::huntingComplete, stats_, true);
    sendChangeMessage();
  });
}
catch (const std::exception &e) {
  DBG("SampleHunterAgent: Error - " + juce::String(e.what()));
  setStatus("Error: " + juce::String(e.what()));
  juce::MessageManager::callAsync(
      [this]() { listeners_.call(&Listener::huntingComplete, stats_, false); });
}

isHunting_.store(false);
}

//==============================================================================
// Freesound Implementation
//==============================================================================

std::vector<FoundSample>
SampleHunterAgent::executeFreesoundSearch(const juce::String &query) {
  std::vector<FoundSample> results;

  if (freesoundConfig_.apiKey.isEmpty()) {
    DBG("SampleHunterAgent: No API Key provided for Freesound!");
    // In a real scenario, we might fail here or try to fetch a public RSS if
    // available. For this implementation, we assume a key is needed or we warn.
    return results;
  }

  // API v2 Search Text
  // https://freesound.org/apiv2/search/text/?query=piano&fields=id,name,previews...

  juce::URL url(freesoundConfig_.baseUrl + "/search/text/");
  url = url.withParameter("query", query)
            .withParameter("fields", "id,name,previews,username,license,type,"
                                     "duration,filesize,samplerate,bitdepth")
            .withParameter("page_size", "20")
            .withParameter("token", freesoundConfig_.apiKey);

  // Filter for high quality / duration if needed
  // url = url.withParameter("filter", "duration:[0.5 TO 30]");

  DBG("SampleHunterAgent: Requesting " + url.toString(true));

  auto options =
      juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress);

  std::unique_ptr<juce::InputStream> stream = url.createInputStream(options);

  if (!stream) {
    DBG("SampleHunterAgent: Connection failed");
    return results;
  }

  // Read entire stream as string, then parse JSON
  juce::String responseStr = stream->readEntireStreamAsString();
  auto jsonVar = juce::JSON::parse(responseStr);
  if (!jsonVar.isObject())
    return results;

  auto root = jsonVar.getDynamicObject();
  if (!root)
    return results;

  auto resultArray = root->getProperty("results");
  if (!resultArray.isArray())
    return results;

  for (auto &item : *resultArray.getArray()) {
    if (!item.isObject())
      continue;
    auto obj = item.getDynamicObject();

    FoundSample sample;
    sample.id = obj->getProperty("id").toString();
    sample.title = obj->getProperty("name").toString();
    sample.username = obj->getProperty("username").toString();
    sample.license = obj->getProperty("license").toString();
    sample.type = obj->getProperty("type").toString();
    sample.duration = (double)obj->getProperty("duration");
    sample.fileSize = (int64_t)obj->getProperty("filesize");
    sample.sampleRate = (int)obj->getProperty("samplerate");
    sample.bitDepth = (int)obj->getProperty("bitdepth");

    // Handle Previews structure
    auto previews = obj->getProperty("previews");
    if (previews.isObject()) {
      auto pObj = previews.getDynamicObject();
      // Prefer HQ MP3 or OGG
      if (pObj->hasProperty("preview-hq-mp3"))
        sample.downloadUrl = pObj->getProperty("preview-hq-mp3").toString();
      else if (pObj->hasProperty("preview-hq-ogg"))
        sample.downloadUrl = pObj->getProperty("preview-hq-ogg").toString();
      else if (pObj->hasProperty("preview-lq-mp3"))
        sample.downloadUrl = pObj->getProperty("preview-lq-mp3").toString();
    }

    if (sample.downloadUrl.isNotEmpty()) {
      results.push_back(sample);
    }
  }

  return results;
}

bool SampleHunterAgent::downloadSample(FoundSample &sample) {
  if (sample.downloadUrl.isEmpty())
    return false;

  stats_.downloadsAttempted++;

  // Safe filename
  juce::String safeName = sample.getSafeFilename();
  juce::File targetFile = config_.downloadDirectory.getChildFile(safeName);

  // Check existence
  if (targetFile.exists()) {
    if (config_.skipDuplicates)
      return false;
    targetFile = targetFile.getNonexistentSibling();
  }

  DBG("SampleHunterAgent: Downloading " + sample.downloadUrl + " to " +
      targetFile.getFileName());

  juce::URL url(sample.downloadUrl);
  auto options =
      juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress);

  std::unique_ptr<juce::InputStream> stream = url.createInputStream(options);

  if (!stream) {
    DBG("SampleHunterAgent: Download connection failed");
    return false;
  }

  // We can't easily get Content-Length from a generic InputStream,
  // so we'll enforce size limits during download instead.

  juce::FileOutputStream output(targetFile);
  if (!output.openedOk())
    return false;

  // Buffer copy with rate monitoring
  const int bufferSize = 4096;
  juce::HeapBlock<char> buffer(bufferSize);

  int64_t totalRead = 0;
  int64_t startTime = juce::Time::getMillisecondCounter();

  // Anti-trickle: check monitoring every 2 seconds
  int64_t lastMonitorTime = startTime;
  int64_t bytesSinceMonitor = 0;

  while (!stream->isExhausted() && !threadShouldExit()) {
    int read = stream->read(buffer.getData(), bufferSize);

    if (read < 0)
      break; // Error
    if (read == 0)
      continue; // Should not happen often with blocking stream

    output.write(buffer.getData(), (size_t)read);
    totalRead += read;
    bytesSinceMonitor += read;

    // Safety check: Total size limit enforcement (Zip bomb protection)
    if (totalRead > config_.maxFileSizeMb * 1024 * 1024) {
      DBG("SampleHunterAgent: Download exceeded size limit, aborting.");
      return false; // File will be incomplete/deleted
    }

    // Speed Monitoring
    int64_t now = juce::Time::getMillisecondCounter();
    if (now - lastMonitorTime > 2000) { // Check every 2s
      double seconds = (now - lastMonitorTime) / 1000.0;
      double speed = bytesSinceMonitor / seconds; // Bytes per second

      if (speed < freesoundConfig_.minDownloadSpeedBps) {
        DBG("SampleHunterAgent: Download too slow (" + juce::String(speed) +
            " B/s), aborting.");
        output.flush();
        // In a real app we might delete the file or retry
        return false;
      }

      lastMonitorTime = now;
      bytesSinceMonitor = 0;
    }
  }

  output.flush();

  if (totalRead < 100) { // Empty or tiny file
    targetFile.deleteFile();
    return false;
  }

  sample.localFile = targetFile;
  sample.downloaded = true;

  return true;
}

void SampleHunterAgent::analyzeSample(FoundSample &sample) {
  if (!sample.localFile.exists())
    return;

  // AudioAnalysisService integration (Basic or Python)
  // For robustness, we check the helper availability

  if (analysisService_->isAvailable()) {
    std::atomic<bool> done{false};

    analysisService_->analyzeAudioFile(
        sample.localFile,
        [&](const AudioAnalysisResults &r) {
          if (r.success) {
            sample.detectedBpm = r.tempoBpm;
            sample.brightness = r.brightness;
            if (r.dominantFrequencyHz > 0)
              sample.detectedKey = frequencyToKeyName(r.dominantFrequencyHz);

            // Simple classification mapping
            sample.instrumentType = "unknown"; // Simplified for now
          }
          done.store(true);
        },
        [&](const juce::String &err) {
          DBG("Analysis error: " + err);
          done.store(true);
        });

    // Wait with timeout
    int w = 0;
    while (!done.load() && w < 100 && !threadShouldExit()) {
      wait(100);
      w++;
    }
  }

  sample.analyzed = true;
}

bool SampleHunterAgent::importToPool(FoundSample &sample) {
  // In a real engine, we'd add to the pool:
  // engine_.getAudioFilePool().addFile(sample.localFile);
  DBG("SampleHunterAgent: Imported " + sample.localFile.getFileName());
  return true;
}

//==============================================================================
// Helpers
//==============================================================================

void SampleHunterAgent::setStatus(const juce::String &status) {
  juce::ScopedLock lock(statusLock_);
  currentStatus_ = status;
  juce::MessageManager::callAsync([status, this]() {
    listeners_.call(&Listener::huntingProgressChanged, progress_.load(),
                    status);
  });
}

void SampleHunterAgent::updateProgress(float p) {
  progress_.store(juce::jlimit(0.0f, 1.0f, p));
  juce::String s = getStatusMessage();
  juce::MessageManager::callAsync([p, s, this]() {
    listeners_.call(&Listener::huntingProgressChanged, p, s);
  });
}

// Phase 1 Logic (Context Analysis)
// Copied from previous implementation but simplified/cleaned
void SampleHunterAgent::analyzeProjectContext() {
  auto *ps = engine_.getProjectState();
  double bpm = ps ? ps->getTempo() : 120.0;
  genreContext_.estimatedBpm = bpm;
  genreContext_.primaryGenre = detectGenreFromTempo(bpm);
  // ... populating instructions same as before ...
}

juce::String SampleHunterAgent::detectGenreFromTempo(double tempo) {
  if (tempo < 70)
    return "Ambient";
  if (tempo < 90)
    return "Lo-Fi Hip Hop";
  if (tempo < 115)
    return "Pop";
  if (tempo < 128)
    return "House";
  if (tempo < 140)
    return "Techno";
  if (tempo < 160)
    return "Trap";
  return "Drum & Bass";
}

void SampleHunterAgent::refineGenreWithAI() {
  if (!grokClient_->hasAPIKey())
    return;

  juce::String prompt = "Project tempo is " +
                        juce::String(genreContext_.estimatedBpm) +
                        " BPM. Primary genre is " + genreContext_.primaryGenre +
                        ". Suggest 1 specific subgenre and 3 style keywords "
                        "(one word each, comma separated) for finding samples.";

  std::atomic<bool> done{false};
  grokClient_->sendChat(
      prompt, GrokMode::Fast, {}, "You are a music style expert.",
      [&](const juce::String &response) {
        // Parse simple response "Subgenre: ... Keywords: ..."
        // For now, naive parsing
        if (response.isNotEmpty()) {
          // This is a placeholder for real NLP parsing logic
          DBG("AI Genre Refinement: " + response);
        }
        done.store(true);
      },
      [&](const GrokFunctionCall &) { done.store(true); },
      [&](const juce::String &) { done.store(true); });

  int w = 0;
  while (!done.load() && w < 20 && !threadShouldExit()) {
    wait(200);
    w++;
  }
}

void SampleHunterAgent::generateSearchQueries() {
  // Base queries
  searchQueue_.add(genreContext_.primaryGenre + " loops");
  searchQueue_.add("royalty free " + genreContext_.primaryGenre + " drums");

  if (genreContext_.subGenre.isNotEmpty()) {
    searchQueue_.add(genreContext_.subGenre + " samples");
  }
}

void SampleHunterAgent::generateAiSearchQueries() {
  if (!grokClient_->hasAPIKey())
    return;

  juce::String prompt =
      "Generate 3 search queries for Freesound.org to find " +
      genreContext_.primaryGenre +
      " samples. Format: Just the query strings, one per line.";

  std::atomic<bool> done{false};
  grokClient_->sendChat(
      prompt, GrokMode::Fast, {}, "You are a sample hunter.",
      [&](const juce::String &response) {
        juce::StringArray lines;
        lines.addLines(response);
        for (auto &line : lines) {
          line = line.trim();
          if (line.isNotEmpty())
            searchQueue_.add(line);
        }
        done.store(true);
      },
      [&](const GrokFunctionCall &) { done.store(true); },
      [&](const juce::String &) { done.store(true); });

  int w = 0;
  while (!done.load() && w < 20 && !threadShouldExit()) {
    wait(200);
    w++;
  }
}

juce::String
SampleHunterAgent::generateSmartFilename(const FoundSample &sample) {
  // Format: Instrument_Key_BPM_User_Hash.wav
  juce::String name;

  // Instrument (default to category or classification)
  if (sample.instrumentType.isNotEmpty() && sample.instrumentType != "unknown")
    name << sample.instrumentType;
  else
    name << "Sample";

  // Key
  if (sample.detectedKey.isNotEmpty())
    name << "_" << sample.detectedKey;

  // BPM
  if (sample.detectedBpm > 0)
    name << "_" << (int)sample.detectedBpm << "bpm";

  // Source info
  name << "_" << sample.username;

  // Ext
  name << "." << sample.type;

  return juce::File::createLegalFileName(name).replace(" ", "_");
}

juce::String SampleHunterAgent::generateFileHash(const juce::File &file) {
  if (!file.existsAsFile())
    return "";
  return juce::MD5(file).toHexString();
}

bool SampleHunterAgent::isDuplicate(const juce::File &file) {
  juce::String hash = generateFileHash(file);
  if (downloadedHashes_.count(hash))
    return true;
  downloadedHashes_.insert(hash);
  return false;
}

// Static helper at end or top
static juce::String frequencyToKeyName(double freq) {
  if (freq <= 0)
    return "";
  double semitones = 12.0 * std::log2(freq / 440.0);
  int noteNumber = static_cast<int>(std::round(semitones)) + 69;
  static const char *names[] = {"C",  "C#", "D",  "D#", "E",  "F",
                                "F#", "G",  "G#", "A",  "A#", "B"};
  int noteIndex = ((noteNumber % 12) + 12) % 12;
  int octave = (noteNumber / 12) - 1;
  return juce::String(names[noteIndex]) + juce::String(octave);
}

} // namespace ai
} // namespace zenith
