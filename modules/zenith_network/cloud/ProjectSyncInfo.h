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
struct ProjectSyncInfo {
    juce::String projectId;
    juce::String projectName;
    juce::String localPath;
    juce::String remotePath;
    juce::Time lastSync;
    juce::Time lastModified;
    bool autoSyncEnabled = true;
    bool hasConflicts = false;
    std::vector<FileSyncInfo> files;
    std::vector<juce::String> conflictedFiles;
};

// Backup information

} // namespace
