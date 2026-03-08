/*
  ==============================================================================
    StringConstants.h
    Centralized string constants to avoid hardcoded literals.
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>

namespace zenith {
namespace constants {
namespace strings {

// Track Types
static constexpr const char* kAudioTrackType = "audio";
static constexpr const char* kMidiTrackType = "midi";
static constexpr const char* kInstrumentTrackType = "instrument";
static constexpr const char* kBusTrackType = "bus";
static constexpr const char* kReturnTrackType = "return";
static constexpr const char* kMasterTrackType = "master";

// File Extensions
static constexpr const char* kProjectFileExtension = ".zenith";
static constexpr const char* kPresetFileExtension = ".zpreset";
static constexpr const char* kAudioWavExtension = ".wav";

// Properties (ProjectState)
static constexpr const char* kPropId = "id";
static constexpr const char* kPropName = "name";
static constexpr const char* kPropType = "type";
static constexpr const char* kPropMuted = "muted";
static constexpr const char* kPropSolo = "solo";
static constexpr const char* kPropArmed = "armed";
static constexpr const char* kPropVolume = "volume";
static constexpr const char* kPropPan = "pan";

// UI Strings
static constexpr const char* kUntitledProject = "Untitled Project";
static constexpr const char* kDefaultTrackName = "Track";

} // namespace strings
} // namespace constants
} // namespace zenith
