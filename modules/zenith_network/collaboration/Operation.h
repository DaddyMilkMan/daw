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
struct Operation {
    juce::String id;
    OperationType type;
    juce::String userId;
    juce::Time timestamp;
    juce::var data;
    juce::String targetId;  // track/effect ID
    int version;
    bool isUndoable = true;
    juce::String description;
};

// Conflict resolution strategies
enum class ConflictResolution {
    LastWriterWins,
    FirstWriterWins,
    Merge,
    Manual,
    OperationalTransform
};

// Session configuration

} // namespace
