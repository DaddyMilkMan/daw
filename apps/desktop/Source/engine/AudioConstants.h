/*
  ==============================================================================

    AudioConstants.h
    Created: 2025-12-14
    Author:  Zenith DAW

    Centralized audio constants for the engine.
    Restores missing constants and improves organization.

  ==============================================================================
*/

#pragma once

namespace zenith {
namespace audio {

//==============================================================================
// Compressor/Dynamics Constants
//==============================================================================

constexpr float kDefaultCompThresholdDb = -10.0f;
constexpr float kDefaultCompRatio = 4.0f;
constexpr float kDefaultCompAttackMs = 10.0f;
constexpr float kDefaultCompReleaseMs = 100.0f;

constexpr float kMinCompAttackMs = 0.1f;
constexpr float kMaxCompAttackMs = 100.0f;

constexpr float kMinCompReleaseMs = 10.0f;
constexpr float kMaxCompReleaseMs = 1000.0f;

constexpr float kCompLookaheadMs = 5.0f;
constexpr float kCompRmsWindowMs = 10.0f;

} // namespace audio
} // namespace zenith
