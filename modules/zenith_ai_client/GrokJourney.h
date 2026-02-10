/*
  ==============================================================================
    GrokJourney.h
    Disk-efficient learning system for Grok AI preferences
    Uses SQLite with fallback to JSON storage for maximum compatibility
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <memory>
#include <unordered_map>
#include <mutex>

// Try SQLite first, fallback to JSON if not available
#ifdef JUCE_MODULE_AVAILABLE_juce_sqlite
  #include <juce_sqlite/juce_sqlite.h>
  #define USE_SQLITE 1
#else
  #define USE_SQLITE 0
#endif

namespace zenith {
namespace ai {

struct UserPreference {
    juce::String genre;
    juce::String instrument;
    juce::String parameter;  // "eq_low", "compression_ratio", etc.
    double value;
    double confidence;      // 0.0 to 1.0
    juce::Time timestamp;
};

struct ReferenceFingerprint {
    juce::String trackName;
    std::vector<float> spectralCentroid;  // Compressed spectral data
    std::vector<float> rmsProfile;       // RMS over time
    std::vector<float> stereoWidth;       // Stereo imaging
    juce::String genre;
    juce::String mood;
};

class GrokJourney {
public:
    GrokJourney();
    ~GrokJourney();  // Proper cleanup

    // Initialize database (creates if doesn't exist)
    bool initialize(const juce::File& storagePath);

    // Learning from user actions
    void recordUserAction(const UserPreference& preference);
    void recordSuccessfulMastering(const juce::String& genre, 
                                  const juce::var& settings,
                                  double userSatisfaction);

    // Reference track analysis
    void storeReferenceFingerprint(const ReferenceFingerprint& fingerprint);
    std::vector<ReferenceFingerprint> findSimilarReferences(const ReferenceFingerprint& query);

    // Query learning data
    std::vector<UserPreference> getPreferencesFor(const juce::String& genre, 
                                                  const juce::String& instrument);
    juce::var getRecommendedSettings(const juce::String& genre, 
                                    const juce::String& instrument);

    // Storage management
    void compactDatabase();  // Clean up old data, compress
    size_t getDatabaseSize() const;
    void setMaxStorageSize(size_t maxSizeMB);

private:
    // Thread safety
    mutable std::mutex dataMutex;
    
    // Storage backend (SQLite or JSON fallback)
#if USE_SQLITE
    std::unique_ptr<juce::SQLite::Database> sqliteDb;
#else
    juce::File jsonStorageFile;
    juce::DynamicObject::Ptr jsonData;
#endif
    
    juce::File storagePath;
    size_t maxStorageSize = 50 * 1024 * 1024;  // 50MB default

    // Database operations
    bool createTables();
    void insertPreference(const UserPreference& pref);
    void insertFingerprint(const ReferenceFingerprint& fp);
    
    // Proper compression using delta encoding + LZ4-style
    std::vector<uint8_t> compressFloats(const std::vector<float>& data);
    std::vector<float> decompressFloats(const std::vector<uint8_t>& compressed);
    
    // JSON fallback methods
    void loadFromJSON();
    void saveToJSON();
    
    // Cleanup
    void closeDatabase();
    bool vacuumDatabase();  // SQLite VACUUM to reclaim space

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GrokJourney)
};

} // namespace ai
} // namespace zenith
