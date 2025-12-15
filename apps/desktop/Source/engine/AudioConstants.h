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
// Audio Constants
//==============================================================================

// Currently effectively a forward to EngineConstants.h
// Future audio-specific constants can be added here.

} // namespace constants
} // namespace zenith
