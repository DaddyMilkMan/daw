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
struct UserInfo {
    juce::String id;
    juce::String name;
    juce::String email;
    juce::Colour avatarColor;
    bool isOnline = false;
    bool isHost = false;
    juce::Time lastSeen;
    juce::String currentTrack;
    juce::String currentAction;
};

// Operation types for collaborative editing
enum class OperationType {
    AddTrack,
    RemoveTrack,
    MoveTrack,
    AddEffect,
    RemoveEffect,
    MoveEffect,
    ChangeParameter,
    ChangeVolume,
    ChangePan,
    MuteTrack,
    SoloTrack,
    ArmTrack,
    EditAudio,
    EditMidi,
    AddMarker,
    RemoveMarker,
    TempoChange,
    TimeSignatureChange,
    Custom
};

// Collaborative operation

} // namespace
