/*
  ==============================================================================
    GrokJourney.cpp
    Implementation of disk-efficient learning system with thread safety
  ==============================================================================
*/

#include "GrokJourney.h"
#include <juce_cryptography/juce_cryptography.h>
#include <algorithm>
#include <cstring>

namespace zenith {
namespace ai {

GrokJourney::GrokJourney() = default;

GrokJourney::~GrokJourney() {
    closeDatabase();  // Proper cleanup
}

bool GrokJourney::initialize(const juce::File& storagePath) {
    std::lock_guard<std::mutex> lock(dataMutex);
    
    this->storagePath = storagePath;
    
    // Create parent directory if needed
    if (!storagePath.getParentDirectory().createDirectory()) {
        DBG("Failed to create database directory");
        return false;
    }

#if USE_SQLITE
    // Try SQLite first
    try {
        sqliteDb = std::make_unique<juce::SQLite::Database>(storagePath);
        
        // Enable foreign keys and WAL mode for better performance
        sqliteDb->execute("PRAGMA foreign_keys = ON");
        sqliteDb->execute("PRAGMA journal_mode = WAL");
        
        return createTables();
    } catch (const std::exception& e) {
        DBG("SQLite failed, falling back to JSON: " << e.what());
        // Fall through to JSON fallback
    }
#endif

    // JSON fallback
    jsonStorageFile = storagePath.withFileExtension("json");
    loadFromJSON();
    return true;
}

bool GrokJourney::createTables() {
#if USE_SQLITE
    if (!sqliteDb) return false;

    try {
        const char* createPrefsTable = R"(
            CREATE TABLE IF NOT EXISTS user_preferences (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                genre TEXT NOT NULL,
                instrument TEXT NOT NULL,
                parameter TEXT NOT NULL,
                value REAL NOT NULL,
                confidence REAL NOT NULL,
                timestamp INTEGER NOT NULL,
                UNIQUE(genre, instrument, parameter)
            ))";

        const char* createFingerprintsTable = R"(
            CREATE TABLE IF NOT EXISTS reference_fingerprints (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                track_name TEXT NOT NULL,
                spectral_data BLOB NOT NULL,
                rms_data BLOB NOT NULL,
                stereo_data BLOB NOT NULL,
                genre TEXT,
                mood TEXT,
                timestamp INTEGER NOT NULL
            ))";

        const char* createMasteringTable = R"(
            CREATE TABLE IF NOT EXISTS successful_mastering (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                genre TEXT NOT NULL,
                settings_json TEXT NOT NULL,
                satisfaction REAL NOT NULL,
                timestamp INTEGER NOT NULL
            ))";

        sqliteDb->execute(createPrefsTable);
        sqliteDb->execute(createFingerprintsTable);
        sqliteDb->execute(createMasteringTable);
        
        return true;
    } catch (const std::exception& e) {
        DBG("Failed to create tables: " << e.what());
        return false;
    }
#else
    // JSON fallback - create structure in memory
    if (!jsonData) {
        jsonData = new juce::DynamicObject();
        jsonData->setProperty("preferences", juce::Array<juce::var>());
        jsonData->setProperty("fingerprints", juce::Array<juce::var>());
        jsonData->setProperty("mastering", juce::Array<juce::var>());
    }
    return true;
#endif
}

void GrokJourney::recordUserAction(const UserPreference& preference) {
    std::lock_guard<std::mutex> lock(dataMutex);

#if USE_SQLITE
    if (sqliteDb) {
        insertPreference(preference);
    } else
#endif
    {
        // JSON fallback
        auto prefs = jsonData->getProperties().getWithDefault("preferences", juce::Array<juce::var>());
        if (!prefs.isArray()) {
            prefs = juce::Array<juce::var>();
        }
        
        auto prefObj = new juce::DynamicObject();
        prefObj->setProperty("genre", preference.genre);
        prefObj->setProperty("instrument", preference.instrument);
        prefObj->setProperty("parameter", preference.parameter);
        prefObj->setProperty("value", preference.value);
        prefObj->setProperty("confidence", preference.confidence);
        prefObj->setProperty("timestamp", preference.timestamp.toMilliseconds());
        
        prefs.getArray()->add(juce::var(prefObj));
        jsonData->setProperty("preferences", prefs);
        
        saveToJSON();
    }

    // Check storage size and compact if needed
    if (getDatabaseSize() > maxStorageSize) {
        compactDatabase();
    }
}

void GrokJourney::storeReferenceFingerprint(const ReferenceFingerprint& fingerprint) {
    std::lock_guard<std::mutex> lock(dataMutex);

    // Compress the float arrays using proper delta encoding
    auto spectralCompressed = compressFloats(fingerprint.spectralCentroid);
    auto rmsCompressed = compressFloats(fingerprint.rmsProfile);
    auto stereoCompressed = compressFloats(fingerprint.stereoWidth);

#if USE_SQLITE
    if (sqliteDb) {
        insertFingerprint(fingerprint);
    } else
#endif
    {
        // JSON fallback
        auto fps = jsonData->getProperties().getWithDefault("fingerprints", juce::Array<juce::var>());
        if (!fps.isArray()) {
            fps = juce::Array<juce::var>();
        }
        
        auto fpObj = new juce::DynamicObject();
        fpObj->setProperty("track_name", fingerprint.trackName);
        fpObj->setProperty("spectral_data", juce::var(spectralCompressed.data(), static_cast<int>(spectralCompressed.size())));
        fpObj->setProperty("rms_data", juce::var(rmsCompressed.data(), static_cast<int>(rmsCompressed.size())));
        fpObj->setProperty("stereo_data", juce::var(stereoCompressed.data(), static_cast<int>(stereoCompressed.size())));
        fpObj->setProperty("genre", fingerprint.genre);
        fpObj->setProperty("mood", fingerprint.mood);
        fpObj->setProperty("timestamp", juce::Time::getCurrentTime().toMilliseconds());
        
        fps.getArray()->add(juce::var(fpObj));
        jsonData->setProperty("fingerprints", fps);
        
        saveToJSON();
    }
}

std::vector<ReferenceFingerprint> GrokJourney::findSimilarReferences(const ReferenceFingerprint& query) {
    std::lock_guard<std::mutex> lock(dataMutex);
    
    std::vector<ReferenceFingerprint> results;

#if USE_SQLITE
    if (sqliteDb) {
        // Real database query implementation
        try {
            auto stmt = sqliteDb->prepare("SELECT * FROM reference_fingerprints ORDER BY timestamp DESC LIMIT 50");
            
            // Helper to convert MemoryBlock to vector
            auto toVec = [](const juce::MemoryBlock& mb) {
                 if (mb.getSize() == 0) return std::vector<uint8_t>{};
                 const uint8_t* data = static_cast<const uint8_t*>(mb.getData());
                 return std::vector<uint8_t>(data, data + mb.getSize());
            };

            while (stmt->step()) {
                ReferenceFingerprint fp;
                fp.trackName = stmt->getColumnText(1);
                
                // Decompress data
                auto spectralData = stmt->getColumnBlob(2);
                fp.spectralCentroid = decompressFloats(toVec(spectralData));
                
                auto rmsData = stmt->getColumnBlob(3);
                fp.rmsProfile = decompressFloats(toVec(rmsData));
                
                auto stereoData = stmt->getColumnBlob(4);
                fp.stereoWidth = decompressFloats(toVec(stereoData));
                
                fp.genre = stmt->getColumnText(5);
                fp.mood = stmt->getColumnText(6);
                
                results.push_back(fp);
            }
        } catch (const std::exception& e) {
            DBG("Database query failed: " << e.what());
        }
    } else
#endif
    {
        // JSON fallback
        auto fps = jsonData->getProperties().getWithDefault("fingerprints", juce::Array<juce::var>());
        if (fps.isArray()) {
            
            // Helper for JSON binary data
            auto toVec = [](const juce::MemoryBlock& mb) {
                 if (mb.getSize() == 0) return std::vector<uint8_t>{};
                 const uint8_t* data = static_cast<const uint8_t*>(mb.getData());
                 return std::vector<uint8_t>(data, data + mb.getSize());
            };

            for (const auto& fpVar : *fps.getArray()) {
                if (auto* fpObj = fpVar.getDynamicObject()) {
                    ReferenceFingerprint fp;
                    fp.trackName = fpObj->getProperties().getWithDefault("track_name", "").toString();
                    fp.genre = fpObj->getProperties().getWithDefault("genre", "").toString();
                    fp.mood = fpObj->getProperties().getWithDefault("mood", "").toString();
                    
                    // Decompress data
                    auto spectralData = fpObj->getProperties().getWithDefault("spectral_data", juce::var());
                    if (spectralData.isBinaryData()) {
                        if (auto* mb = spectralData.getBinaryData())
                            fp.spectralCentroid = decompressFloats(toVec(*mb));
                    }
                    
                    auto rmsData = fpObj->getProperties().getWithDefault("rms_data", juce::var());
                    if (rmsData.isBinaryData()) {
                        if (auto* mb = rmsData.getBinaryData())
                            fp.rmsProfile = decompressFloats(toVec(*mb));
                    }
                    
                    auto stereoData = fpObj->getProperties().getWithDefault("stereo_data", juce::var());
                    if (stereoData.isBinaryData()) {
                        if (auto* mb = stereoData.getBinaryData())
                            fp.stereoWidth = decompressFloats(toVec(*mb));
                    }
                    
                    results.push_back(fp);
                }
            }
        }
    }

    return results;
}

std::vector<UserPreference> GrokJourney::getPreferencesFor(const juce::String& genre, 
                                                           const juce::String& instrument) {
    std::lock_guard<std::mutex> lock(dataMutex);
    
    std::vector<UserPreference> preferences;

#if USE_SQLITE
    if (sqliteDb) {
        try {
            auto stmt = sqliteDb->prepare(
                "SELECT genre, instrument, parameter, value, confidence, timestamp "
                "FROM user_preferences "
                "WHERE genre = ? AND instrument = ? "
                "ORDER BY confidence DESC "
                "LIMIT 20"
            );
            
            stmt->bindText(1, genre);
            stmt->bindText(2, instrument);
            
            while (stmt->step()) {
                UserPreference pref;
                pref.genre = stmt->getColumnText(0);
                pref.instrument = stmt->getColumnText(1);
                pref.parameter = stmt->getColumnText(2);
                pref.value = stmt->getColumnDouble(3);
                pref.confidence = stmt->getColumnDouble(4);
                pref.timestamp = juce::Time(stmt->getColumnInt64(5));
                
                preferences.push_back(pref);
            }
        } catch (const std::exception& e) {
            DBG("Preference query failed: " << e.what());
        }
    } else
#endif
    {
        // JSON fallback
        auto prefs = jsonData->getProperties().getWithDefault("preferences", juce::Array<juce::var>());
        if (prefs.isArray()) {
            for (const auto& prefVar : *prefs.getArray()) {
                if (auto* prefObj = prefVar.getDynamicObject()) {
                    if (prefObj->getProperties().getWithDefault("genre", "").toString() == genre &&
                        prefObj->getProperties().getWithDefault("instrument", "").toString() == instrument) {
                        
                        UserPreference pref;
                        pref.genre = prefObj->getProperties().getWithDefault("genre", "").toString();
                        pref.instrument = prefObj->getProperties().getWithDefault("instrument", "").toString();
                        pref.parameter = prefObj->getProperties().getWithDefault("parameter", "").toString();
                        pref.value = prefObj->getProperties().getWithDefault("value", 0.0);
                        pref.confidence = prefObj->getProperties().getWithDefault("confidence", 0.0);
                        pref.timestamp = juce::Time(static_cast<juce::int64>(prefObj->getProperties().getWithDefault("timestamp", 0.0)));
                        
                        preferences.push_back(pref);
                    }
                }
            }
        }
    }

    return preferences;
}

// Proper compression using delta encoding + run-length encoding
std::vector<uint8_t> GrokJourney::compressFloats(const std::vector<float>& data) {
    if (data.empty()) return {};
    
    std::vector<uint8_t> result;
    
    // Delta encoding: store differences between consecutive values
    std::vector<float> deltas;
    deltas.reserve(data.size());
    deltas.push_back(data[0]);  // First value as-is
    
    for (size_t i = 1; i < data.size(); ++i) {
        deltas.push_back(data[i] - data[i-1]);
    }
    
    // Simple run-length encoding for zeros
    for (float delta : deltas) {
        if (std::abs(delta) < 1e-6f) {
            // Zero - use run-length
            uint8_t count = 1;
            while (count < 255 && &delta - &deltas[0] + count < deltas.size() &&
                   std::abs(deltas[&delta - &deltas[0] + count]) < 1e-6f) {
                count++;
            }
            result.push_back(0xFF);  // Zero marker
            result.push_back(count);
            // Skip the zeros we just encoded
            for (int i = 0; i < count - 1; ++i) {
                if (&delta - &deltas[0] + i + 1 < deltas.size()) {
                    delta = deltas[&delta - &deltas[0] + i + 1];
                }
            }
        } else {
            // Non-zero - quantize to 16-bit
            int16_t quantized = static_cast<int16_t>(std::round(delta * 10000.0f));
            result.push_back(static_cast<uint8_t>(quantized & 0xFF));
            result.push_back(static_cast<uint8_t>((quantized >> 8) & 0xFF));
        }
    }
    
    return result;
}

std::vector<float> GrokJourney::decompressFloats(const std::vector<uint8_t>& compressed) {
    std::vector<float> result;
    
    if (compressed.empty()) return result;
    
    float currentValue = 0.0f;
    
    for (size_t i = 0; i < compressed.size(); ) {
        if (compressed[i] == 0xFF && i + 1 < compressed.size()) {
            // Run of zeros
            uint8_t count = compressed[i + 1];
            for (int j = 0; j < count; ++j) {
                result.push_back(currentValue);
            }
            i += 2;
        } else if (i + 1 < compressed.size()) {
            // Regular value
            int16_t quantized = static_cast<int16_t>((compressed[i + 1] << 8) | compressed[i]);
            float delta = static_cast<float>(quantized) / 10000.0f;
            currentValue += delta;
            result.push_back(currentValue);
            i += 2;
        } else {
            // Invalid data
            break;
        }
    }
    
    return result;
}

void GrokJourney::loadFromJSON() {
    if (!jsonStorageFile.exists()) {
        jsonData = new juce::DynamicObject();
        jsonData->setProperty("preferences", juce::Array<juce::var>());
        jsonData->setProperty("fingerprints", juce::Array<juce::var>());
        jsonData->setProperty("mastering", juce::Array<juce::var>());
        return;
    }
    
    auto jsonContent = jsonStorageFile.loadFileAsString();
    auto parsed = juce::JSON::parse(jsonContent);
    
    if (parsed.isObject()) {
        jsonData = parsed.getDynamicObject();
    } else {
        jsonData = new juce::DynamicObject();
        jsonData->setProperty("preferences", juce::Array<juce::var>());
        jsonData->setProperty("fingerprints", juce::Array<juce::var>());
        jsonData->setProperty("mastering", juce::Array<juce::var>());
    }
}

void GrokJourney::saveToJSON() {
    if (jsonData) {
        auto jsonString = juce::JSON::toString(juce::var(jsonData));
        jsonStorageFile.replaceWithText(jsonString);
    }
}

size_t GrokJourney::getDatabaseSize() const {
    std::lock_guard<std::mutex> lock(dataMutex);
    
#if USE_SQLITE
    if (sqliteDb) {
        return storagePath.exists() ? static_cast<size_t>(storagePath.getSize()) : 0;
    } else
#endif
    {
        return jsonStorageFile.exists() ? static_cast<size_t>(jsonStorageFile.getSize()) : 0;
    }
}

void GrokJourney::compactDatabase() {
    std::lock_guard<std::mutex> lock(dataMutex);
    
#if USE_SQLITE
    if (sqliteDb) {
        try {
            // Delete old entries (older than 30 days)
            int64_t cutoffTime = juce::Time::getCurrentTime().toMilliseconds() - (30LL * 24 * 60 * 60 * 1000);
            
            sqliteDb->execute("DELETE FROM user_preferences WHERE timestamp < " + juce::String(cutoffTime));
            sqliteDb->execute("DELETE FROM reference_fingerprints WHERE timestamp < " + juce::String(cutoffTime));
            sqliteDb->execute("DELETE FROM successful_mastering WHERE timestamp < " + juce::String(cutoffTime));
            
            // Run VACUUM to reclaim space
            sqliteDb->execute("VACUUM");
        } catch (const std::exception& e) {
            DBG("Database compaction failed: " << e.what());
        }
    } else
#endif
    {
        // JSON fallback - remove old entries
        if (jsonData) {
            auto cutoffTime = juce::Time::getCurrentTime().toMilliseconds() - (30LL * 24 * 60 * 60 * 1000);
            
            // Clean preferences
            auto prefs = jsonData->getProperties().getWithDefault("preferences", juce::Array<juce::var>());
            if (prefs.isArray()) {
                juce::Array<juce::var> filteredPrefs;
                for (const auto& pref : *prefs.getArray()) {
                    if (auto* prefObj = pref.getDynamicObject()) {
                        if (static_cast<juce::int64>(prefObj->getProperties().getWithDefault("timestamp", 0.0)) >= cutoffTime) {
                            filteredPrefs.add(pref);
                        }
                    }
                }
                jsonData->setProperty("preferences", filteredPrefs);
            }
            
            saveToJSON();
        }
    }
}

void GrokJourney::closeDatabase() {
    std::lock_guard<std::mutex> lock(dataMutex);
    
#if USE_SQLITE
    sqliteDb.reset();
#else
    saveToJSON();
    jsonData.reset();
#endif
}

} // namespace ai
} // namespace zenith
