/*
  ==============================================================================
    CollaborativeSession.h
    Real-time collaborative editing system
    Phase 5: Advanced Features
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_network/juce_network.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>

namespace zenith {
namespace collaboration {

// User information
struct SessionConfig {
    juce::String sessionId;
    juce::String name;
    juce::String description;
    UserInfo host;
    int maxUsers = 10;
    bool autoSync = true;
    int syncInterval = 100;  // ms
    ConflictResolution conflictResolution = ConflictResolution::OperationalTransform;
    bool allowAudioStreaming = false;
    int audioQuality = 128;  // kbps
    bool enableChat = true;
    bool enableVideo = false;
    bool requirePassword = false;
    juce::String password;
};

// Audio stream data

} // namespace
