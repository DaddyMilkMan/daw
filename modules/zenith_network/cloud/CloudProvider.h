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
struct CloudProvider {
    enum class Type {
        GoogleDrive,
        Dropbox,
        OneDrive,
        AWS,
        Custom,
        Local
    };

    Type type = Type::GoogleDrive;
    juce::String name;
    juce::String apiEndpoint;
    juce::String authEndpoint;
    juce::String clientId;
    juce::String clientSecret;
    juce::String accessToken;
    juce::String refreshToken;
    juce::Time tokenExpiry;
    size_t maxFileSize = 5ULL * 1024 * 1024 * 1024;  // 5GB
    size_t totalStorage = 15ULL * 1024 * 1024 * 1024;  // 15GB
    size_t usedStorage = 0;
    bool isConnected = false;
};

// Sync configuration

} // namespace
