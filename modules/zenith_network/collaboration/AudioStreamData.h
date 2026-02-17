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
struct AudioStreamData {
    juce::String userId;
    juce::AudioBuffer<float> audioBuffer;
    double sampleRate;
    int bitDepth;
    bool isMuted;
    float volume;
    juce::Time timestamp;
};

// Chat message

} // namespace
