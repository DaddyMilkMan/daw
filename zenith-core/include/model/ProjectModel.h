/**
 * @file ProjectModel.h
 * @brief v0.1 project data model (pure C++ types, no JUCE dependencies)
 *
 * Core data structures for Zenith v0.1:
 * - Audio tracks with clips
 * - Simple track-level gain/pan
 * - Clip-level mute
 * - Track-level mute
 *
 * No MIDI, no automation, no tempo, no looping yet.
 */

#pragma once

#include <vector>
#include <cstdint>
#include <string>

namespace zenith
{
    using SamplePos  = int64_t;
    using SampleLen  = int64_t;
    using ClipId     = int64_t;
    using TrackId    = int32_t;

    //==========================================================================
    /**
     * @brief Clip placement on timeline
     *
     * Represents a single audio clip instance on the timeline.
     * Links to an audio file via filePath (project-relative).
     */
    struct ClipModel
    {
        ClipId      id = 0;              ///< Stable ID for linking to engine ClipDef
        std::string filePath;            ///< Project-relative path, e.g. "Audio/kick.wav"

        SamplePos   startSample = 0;     ///< Timeline start (absolute)
        SampleLen   lengthSamples = 0;   ///< Duration on timeline
        SamplePos   srcOffset = 0;       ///< Offset in source file (for trims)

        float       gain = 1.0f;         ///< Linear gain [0..2]
        int         fadeInSamples = 0;   ///< Fade-in duration in samples
        int         fadeOutSamples = 0;  ///< Fade-out duration in samples

        bool        muted = false;       ///< Clip-level mute

        // v0.2+ (not used yet, but leave fields for future)
        bool        loopEnabled = false;
        SampleLen   loopLength = 0;
    };

    //==========================================================================
    /**
     * @brief FX slot configuration
     *
     * v0.1: Only track-level gain/pan stored in slot 0.
     * Other slots not persisted yet.
     */
    struct FxSlotModel
    {
        int         slotIndex = 0;       ///< 0..4
        std::string nodeType;            ///< "GainPan", "VST3", etc.
        std::string pluginId;            ///< VST3 UID or name
        bool        bypassed = false;
        std::string stateXml;            ///< Plugin state, serialized as XML (empty for GainPan)
    };

    //==========================================================================
    /**
     * @brief Audio track model
     *
     * Contains clips + track-level settings.
     * FX chain not fully persisted in v0.1 (only gain/pan).
     */
    struct TrackModel
    {
        TrackId     id = 0;
        std::string name;
        float       gain = 1.0f;         ///< Track-level gain [0..2]
        float       pan = 0.0f;          ///< Track-level pan [-1..+1]

        bool        muted = false;       ///< Track-level mute
        bool        solo = false;        ///< Solo (not implemented in v0.1)

        std::vector<ClipModel>   clips;
        std::vector<FxSlotModel> fxSlots;   ///< size <= 5, slotIndex unique (v0.1: not persisted)
    };

    //==========================================================================
    /**
     * @brief Project root model
     *
     * Top-level container for all project data.
     */
    struct ProjectModel
    {
        std::string name;
        double      sampleRate = 48000.0;
        int         blockSize = 512;
        SampleLen   lengthHint = 0;      ///< Optional; can be recomputed from clips

        int64_t     nextClipId = 1;      ///< Monotonic clip ID counter (starts at 1)

        std::vector<TrackModel> tracks;
    };

} // namespace zenith
