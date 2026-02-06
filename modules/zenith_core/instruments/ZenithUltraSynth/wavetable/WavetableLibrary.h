/*
  ==============================================================================

    WavetableLibrary.h
    Created: [Date] Author: Claude AI
    Extensive wavetable library with built-in and custom collections

  ==============================================================================
*/

#pragma once

#include "../../../JuceLibraryCode/JuceHeader.h"
#include "AdvancedWavetableEngine.h"

namespace Zenith
{

class WavetableLibrary
{
public:
    // Wavetable categories
    enum class Category
    {
        Analog,      // Classic analog waveforms
        Digital,     // Harsh digital waves
        Pads,        // Evolving pad textures
        Bass,        // Deep bass waves
        Lead,        // Cutting lead waves
        FX,          // Sound effects
        Vocals,      // Formant waves
        Acoustic,    // Acoustic instrument waves
        Experimental // Avant-garde waves
    };

    // Search filters
    struct SearchFilter
    {
        Category category = Category::Analog;
        juce::StringArray tags;
        juce::StringArray excludedTags;
        int minFrames = 1;
        int maxFrames = 256;
        juce::Range<float> frequencyRange;
        juce::Range<float> complexityRange;
        juce::Range<float> brightnessRange;
        juce::Range<float> warmthRange;
        bool includeCustom = true;
        bool includeBuiltIn = true;
        bool sortByName = true;
        bool sortByPopularity = false;
        bool sortByRating = false;
        bool sortByDate = false;
    };

    // Wavetable metadata
    struct WavetableInfo
    {
        juce::String id;
        juce::String name;
        Category category;
        juce::StringArray tags;
        juce::String description;
        juce::String author;
        juce::String source;
        juce::Date createdDate;
        juce::Date modifiedDate;
        int frames;
        int samplesPerFrame;
        float duration;
        float frequency;
        float complexity;
        float brightness;
        float warmth;
        float popularity;
        float rating;
        int downloadCount;
        int useCount;
        juce::StringArray similarTo;
        juce::Array<float> previewData;
        juce::File filePath;
        bool isBuiltIn;
        bool isCustom;
        bool isFavorite;
        juce::Colour color;
        juce::Image thumbnail;
    };

    // Library statistics
    struct LibraryStats
    {
        int totalWavetables;
        int builtInCount;
        int customCount;
        int categoryCounts[10]; // Count per category
        float averageComplexity;
        float averageBrightness;
        float averageWarmth;
        float totalSizeMB;
        juce::Date oldestEntry;
        juce::Date newestEntry;
        juce::StringArray topTags;
        juce::StringArray topAuthors;
    };

    WavetableLibrary();
    ~WavetableLibrary();

    // Initialization
    void initialize(const juce::File& libraryDirectory);
    void shutdown();

    // Library management
    void loadLibrary();
    void saveLibrary();
    void refreshLibrary();
    void rebuildIndex();

    // Built-in wavetables
    void loadBuiltInWavetables();
    void addBuiltInWavetable(const WavetableInfo& info, const AdvancedWavetableEngine::WavetableData& data);
    WavetableInfo getBuiltInInfo(const juce::String& id) const;
    bool isBuiltIn(const juce::String& id) const;

    // Custom wavetables
    void addCustomWavetable(const WavetableInfo& info, const AdvancedWavetableEngine::WavetableData& data);
    void updateCustomWavetable(const juce::String& id, const WavetableInfo& info, const AdvancedWavetableEngine::WavetableData& data);
    void removeCustomWavetable(const juce::String& id);
    bool isCustom(const juce::String& id) const;

    // Wavetable access
    AdvancedWavetableEngine::WavetableData getWavetableData(const juce::String& id) const;
    WavetableInfo getWavetableInfo(const juce::String& id) const;
    bool hasWavetable(const juce::String& id) const;
    juce::StringArray getAllWavetableIds() const;

    // Search and browsing
    juce::StringArray findWavetables(const SearchFilter& filter) const;
    juce::StringArray getWavetablesInCategory(Category category) const;
    juce::StringArray getWavetablesWithTag(const juce::String& tag) const;
    juce::StringArray searchByName(const juce::String& name) const;
    juce::StringArray searchByDescription(const juce::String& description) const;

    // Collection management
    void addToCollection(const juce::String& wavetableId, const juce::String& collectionName);
    void removeFromCollection(const juce::String& wavetableId, const juce::String& collectionName);
    juce::StringArray getCollections() const;
    juce::StringArray getCollection(const juce::String& collectionName) const;
    bool isInCollection(const juce::String& wavetableId, const juce::String& collectionName) const;

    // Favorites
    void addToFavorites(const juce::String& wavetableId);
    void removeFromFavorites(const juce::String& wavetableId);
    bool isFavorite(const juce::String& wavetableId) const;
    juce::StringArray getFavorites() const;

    // Rating and popularity
    void rateWavetable(const juce::String& id, float rating);
    float getRating(const juce::String& id) const;
    void incrementUsage(const juce::String& id);
    int getUsageCount(const juce::String& id) const;
    void incrementDownloads(const juce::String& id);
    int getDownloadCount(const juce::String& id) const;

    // Import/Export
    bool importWavetable(const juce::File& file);
    bool exportWavetable(const juce::String& id, const juce::File& file);
    bool exportCollection(const juce::String& collectionName, const juce::File& file);
    bool importCollection(const juce::File& file);

    // Statistics and analytics
    LibraryStats getLibraryStats() const;
    std::vector<std::pair<juce::String, int>> getTagFrequency() const;
    std::vector<std::pair<juce::String, int>> getAuthorFrequency() const;
    std::vector<std::pair<juce::String, float>> getMostPopular() const;
    std::vector<std::pair<juce::String, float>> getHighestRated() const;

    // Recommendations
    juce::StringArray getRecommendations(const juce::String& id, int count = 5) const;
    juce::StringArray getRecommendationsByCategory(Category category, int count = 5) const;
    juce::StringArray getRecommendationsByTags(const juce::StringArray& tags, int count = 5) const;
    juce::StringArray getSimilarWavetables(const juce::String& id, int count = 5) const;

    // Validation
    bool validateWavetable(const juce::String& id) const;
    std::vector<juce::String> getValidationErrors(const juce::String& id) const;
    void repairWavetable(const juce::String& id);

    // Performance optimization
    void setCacheSize(int size);
    int getCacheSize() const;
    void clearCache();
    void optimizeLibrary();
    void cleanupUnusedWavetables();

    // Callbacks
    void setLibraryChangeListener(std::function<void()> callback);
    void setWavetableChangeListener(std::function<void(const juce::String&)> callback);
    void setStatisticsChangeListener(std::function<void()> callback);

    // Backup and recovery
    void backupLibrary(const juce::File& backupDirectory);
    void restoreLibrary(const juce::File& backupDirectory);
    juce::StringArray getBackupList() const;
    bool createBackup(const juce::String& name);
    bool restoreBackup(const juce::String& name);

private:
    // Library state
    juce::File libraryDirectory_;
    bool initialized_;
    bool libraryLoaded_;

    // Built-in wavetables
    juce::HashMap<juce::String, WavetableInfo> builtInWavetables_;
    juce::HashMap<juce::String, AdvancedWavetableEngine::WavetableData> builtInData_;

    // Custom wavetables
    juce::HashMap<juce::String, WavetableInfo> customWavetables_;
    juce::HashMap<juce::String, AdvancedWavetableEngine::WavetableData> customData_;

    // Collections
    std::map<juce::String, juce::StringArray> collections_;
    std::map<juce::String, juce::StringArray> reverseCollections_; // wavetableId -> collectionNames

    // Search index
    struct SearchIndex
    {
        std::map<Category, std::set<juce::String>> byCategory;
        std::map<juce::String, std::set<juce::String>> byTag;
        std::map<juce::String, std::set<juce::String>> byAuthor;
        std::map<juce::String, std::set<juce::String>> byName;
        std::map<juce::String, std::set<juce::String>> byDescription;
        std::map<float, std::set<juce::String>> byRating;
        std::map<int, std::set<juce::String>> byUsage;
        std::map<int, std::set<juce::String>> byDownloads;
        std::map<float, std::set<juce::String>> byComplexity;
        std::map<float, std::set<juce::String>> byBrightness;
        std::map<float, std::set<juce::String>> byWarmth;
    } searchIndex_;

    // Cache
    struct CacheEntry
    {
        WavetableInfo info;
        AdvancedWavetableEngine::WavetableData data;
        juce::uint64 timestamp;
    };
    std::map<juce::String, CacheEntry> cache_;
    int cacheSize_;
    juce::CriticalSection cacheLock_;

    // Statistics
    LibraryStats stats_;
    juce::uint64 lastStatsUpdate_;

    // Callbacks
    std::function<void()> libraryChangeListener_;
    std::function<void(const juce::String&)> wavetableChangeListener_;
    std::function<void()> statisticsChangeListener_;

    // Backup system
    struct BackupEntry
    {
        juce::String name;
        juce::Date date;
        juce::File file;
        size_t size;
    };
    std::vector<BackupEntry> backups_;

    // Private helper methods
    void initializeSearchIndex();
    void updateSearchIndex(const WavetableInfo& info);
    void removeFromSearchIndex(const juce::String& id);
    void updateStatistics();
    void updateCache(const juce::String& id, const WavetableInfo& info, const AdvancedWavetableEngine::WavetableData& data);
    void clearCacheInternal();

    // File operations
    juce::File getWavetableFile(const juce::String& id, bool isBuiltIn = false) const;
    bool loadWavetableFromFile(const juce::File& file, WavetableInfo& info, AdvancedWavetableEngine::WavetableData& data);
    bool saveWavetableToFile(const juce::String& id, const WavetableInfo& info, const AdvancedWavetableEngine::WavetableData& data);
    bool deleteWavetableFile(const juce::String& id);

    // Built-in wavetable generation
    void generateAnalogWavetables();
    void generateDigitalWavetables();
    void generatePadWavetables();
    void generateBassWavetables();
    void generateLeadWavetables();
    void generateFXWavetables();
    void generateVocalWavetables();
    void generateAcousticWavetables();
    void generateExperimentalWavetables();

    // Wavetable analysis
    void analyzeWavetable(WavetableInfo& info, const AdvancedWavetableEngine::WavetableData& data);
    float calculateComplexity(const AdvancedWavetableEngine::WavetableData& data) const;
    float calculateBrightness(const AdvancedWavetableEngine::WavetableData& data) const;
    float calculateWarmth(const AdvancedWavetableEngine::WavetableData& data) const;
    juce::StringArray extractTags(const WavetableInfo& info) const;
    juce::Array<float> generatePreview(const AdvancedWavetableEngine::WavetableData& data) const;

    // Recommendation algorithms
    float calculateSimilarity(const WavetableInfo& info1, const WavetableInfo& info2) const;
    juce::StringArray findSimilarByContent(const juce::String& id, int count) const;
    juce::StringArray findSimilarByMetadata(const juce::String& id, int count) const;
    juce::StringArray findSimilarByUsage(const juce::String& id, int count) const;

    // Validation
    bool validateWavetableData(const AdvancedWavetableEngine::WavetableData& data) const;
    bool validateWavetableInfo(const WavetableInfo& info) const;
    std::vector<juce::String> validateData(const AdvancedWavetableEngine::WavetableData& data) const;
    std::vector<juce::String> validateInfo(const WavetableInfo& info) const;
    void repairData(AdvancedWavetableEngine::WavetableData& data) const;
    void repairInfo(WavetableInfo& info) const;

    // Backup operations
    juce::File getBackupDirectory() const;
    juce::File createBackupFile(const juce::String& name) const;
    void backupToFile(const juce::File& backupFile);
    void restoreFromFile(const juce::File& backupFile);

    // Utility methods
    juce::String generateWavetableId() const;
    juce::String generateCollectionId() const;
    juce::String generateBackupId() const;
    juce::String normalizeString(const juce::String& str) const;
    juce::StringArray tokenizeString(const juce::String& str) const;

    // Thread safety
    juce::CriticalSection libraryLock_;
    juce::ScopedLock scopedLock_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WavetableLibrary)
};

} // namespace Zenith