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
struct FileSyncInfo {
    juce::String localPath;
    juce::String remotePath;
    juce::String fileId;
    juce::String versionId;
    juce::Time lastModified;
    size_t fileSize = 0;
    juce::String checksum;
    bool isDirectory = false;
    bool isSynced = false;
    SyncStatus status = SyncStatus::Idle;
    juce::String errorMessage;
};

// Project sync information

} // namespace
