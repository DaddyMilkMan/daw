/**
 * @file InstrumentCommandHelpers.h
 * @brief Helper utilities for instrument CommandAPI operations
 *
 * Provides helper functions for:
 * - Finding tracks and their instruments
 * - Validating parameter and macro IDs
 * - Creating standardized error responses
 */

#pragma once

#include <JuceHeader.h>
#include "../Source/instruments/Instrument.h"
#include "../Source/engine/Track.h"
#include "ProjectState.h"
#include "Engine.h"

namespace InstrumentCommandHelpers {

//==============================================================================
/**
 * @brief Error codes for instrument operations
 */
enum class ErrorCode
{
    UnknownTrack,
    TrackHasNoInstrument,
    UnknownInstrument,
    UnknownParameter,
    UnknownMacro,
    UnknownPreset,
    InvalidValue,
    InternalError
};

/**
 * @brief Convert error code to string
 */
inline juce::String errorCodeToString(ErrorCode code)
{
    switch (code)
    {
        case ErrorCode::UnknownTrack:        return "UNKNOWN_TRACK";
        case ErrorCode::TrackHasNoInstrument: return "TRACK_HAS_NO_INSTRUMENT";
        case ErrorCode::UnknownInstrument:   return "UNKNOWN_INSTRUMENT";
        case ErrorCode::UnknownParameter:    return "UNKNOWN_PARAMETER";
        case ErrorCode::UnknownMacro:        return "UNKNOWN_MACRO";
        case ErrorCode::UnknownPreset:       return "UNKNOWN_PRESET";
        case ErrorCode::InvalidValue:        return "INVALID_VALUE";
        case ErrorCode::InternalError:       return "INTERNAL_ERROR";
        default:                             return "UNKNOWN_ERROR";
    \n    default: break;\n\n    default: break;\n}
}

/**
 * @brief Create standardized error response
 */
inline juce::var createError(ErrorCode code, const juce::String& message)
{
    auto* errorObj = new juce::DynamicObject();
    errorObj->setProperty("code", errorCodeToString(code));
    errorObj->setProperty("message", message);

    auto* result = new juce::DynamicObject();
    result->setProperty("success", false);
    result->setProperty("error", juce::var(errorObj));

    return juce::var(result);
}

/**
 * @brief Create success response
 */
inline juce::var createSuccess(const juce::var& data = juce::var())
{
    auto* result = new juce::DynamicObject();
    result->setProperty("success", true);
    if (!data.isVoid())
        result->setProperty("data", data);

    return juce::var(result);
}

//==============================================================================
/**
 * @brief Find track by ID in the Engine
 * @param engine Engine instance
 * @param trackId Track ID to find
 * @return Pointer to track, or nullptr if not found
 */
inline zenith::Track* findTrack(Engine& engine, const juce::String& trackId)
{
    // For now, we'll need to enhance Engine to support track lookup by ID
    // This is a simplified implementation
    // TODO: Add proper track ID mapping to Engine or ProjectState

    // Parse track index from ID (assuming format like "track_0", "track_1")
    int trackIndex = trackId.getTrailingIntValue();
    if (trackIndex >= 0 && trackIndex < engine.getNumTracks())
    {
        return const_cast<zenith::Track*>(engine.tracks()[trackIndex].get());
    }

    return nullptr;
}

/**
 * @brief Get instrument from track with validation
 * @param track Track to get instrument from
 * @param errorOut Output parameter for error response (if any)
 * @return Pointer to instrument, or nullptr on error (errorOut will be set)
 */
inline zenith::Instrument* getTrackInstrument(zenith::Track* track, juce::var& errorOut)
{
    if (track == nullptr)
    {
        errorOut = createError(ErrorCode::UnknownTrack, "Track not found");
        return nullptr;
    }

    auto* instrument = track->getInstrument();
    if (instrument == nullptr)
    {
        errorOut = createError(ErrorCode::TrackHasNoInstrument,
                              "Track '" + track->getName() + "' has no instrument attached");
        return nullptr;
    }

    return instrument;
}

/**
 * @brief Validate and clamp normalized value
 * @param value Value to validate
 * @return Clamped value in range [0, 1]
 */
inline float clampNormalizedValue(float value)
{
    return juce::jlimit(0.0f, 1.0f, value);
}

} // namespace InstrumentCommandHelpers


