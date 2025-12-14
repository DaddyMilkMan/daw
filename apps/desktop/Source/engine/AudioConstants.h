/*
  ==============================================================================

    AudioConstants.h
    Created: 2025-12-14
    Author:  Zenith DAW

    Centralized audio engine constants for use across the audio processing
    subsystem. This header includes EngineConstants.h for the full set of
    engine constants and provides additional aliases for convenience.

    Thread Safety:
    - All values are constexpr and compile-time constant
    - Safe to use from any thread without synchronization

  ==============================================================================
*/

#pragma once

// Include the main engine constants header - this provides the canonical
// definitions for all audio engine constants
#include "EngineConstants.h"

namespace zenith {
namespace constants {

//==============================================================================
// Convenience Aliases
// These provide alternative names for commonly used constants to improve
// readability in different contexts.
//==============================================================================

/// Alias for kDefaultCompAttackMs - shorter form for compressor attack time
constexpr float kCompAttackMs = kDefaultCompAttackMs;

/// Alias for kDefaultCompReleaseMs - shorter form for compressor release time
constexpr float kCompReleaseMs = kDefaultCompReleaseMs;

} // namespace constants
} // namespace zenith
