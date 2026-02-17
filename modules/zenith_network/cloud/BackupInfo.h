/*
  ==============================================================================
    CloudSyncSystem.h
    Cloud synchronization and backup system with version control
    Phase 5: Advanced Features
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_network/juce_network.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <mutex>

namespace zenith {
namespace cloud {

// Sync status
enum class SyncStatus {
    Idle,
    Syncing,
    Uploading,
    Downloading,
    Conflict,
    Error,
    Offline
};

// File sync information
struct BackupInfo {
    juce::String backupId;
    juce::String projectId;
    juce::String name;
    juce::String description;
    juce::Time created;
    juce::Time expires;
    size_t totalSize = 0;
    int fileCount = 0;
    bool isAutomatic = false;
    bool isEncrypted = true;
    juce::String compressionLevel;  // "none", "fast", "normal", "maximum"
    std::vector<juce::String> tags;
};

// Cloud provider configuration

} // namespace
