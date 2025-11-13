/**
 * @file Track.h
 * @brief Audio track with clip voice management
 *
 * Phase 1: Sample-accurate clip scheduling with fade-in/fade-out
 * Minimal voice allocation, no plugins yet (Phase 2)
 */

#pragma once

#include <JuceHeader.h>
#include "Clip.h"
#include <vector>
#include <array>
#include <cstdint>

// Forward declarations
class Engine;

namespace zenith {

//==============================================================================
/**
 * @brief Clip definition (message thread only)
 *
 * Static arrangement object holding clip ID and PCM data.
 * Created/modified only when playback is stopped.
 */
struct ClipDef
{
    int64_t id;      ///< Unique clip identifier (matches TransportEvent.clipId)
    Clip clip;       ///< PCM data and playback parameters

    ClipDef() : id(0) {}
    ClipDef(int64_t clipId, const Clip& c) : id(clipId), clip(c) {}
};

//==============================================================================
/**
 * @brief Active voice (audio thread only)
 *
 * Runtime state for a playing clip. No allocations, just indices and counters.
 */
struct ActiveVoice
{
    /// Pointer into Track::clipDefs_ (never mutated on RT thread)
    const ClipDef* def = nullptr;

    /// Current read position within clip (samples)
    int64_t clipPos = 0;

    /// Fade-in progress (samples faded in so far)
    int fadeInDone = 0;

    /// Fade-out progress (samples faded out so far)
    int fadeOutDone = 0;

    /// True once stopClipRT() called, triggers fade-out
    bool fadingOut = false;

    /// True = stop at clip end, false = loop (Phase 1: always true)
    bool oneShot = true;

    /**
     * @brief Check if voice is active (has a clip assigned)
     */
    bool isActive() const noexcept { return def != nullptr; }

    /**
     * @brief Check if voice has finished playing
     */
    bool isFinished(int64_t clipLengthSamples) const noexcept
    {
        return fadeOutDone >= def->clip.fadeOutSamples
            || (oneShot && clipPos >= clipLengthSamples);
    }
};

//==============================================================================
/**
 * @brief Audio track with sample-accurate clip scheduling
 *
 * Thread model:
 * - clipDefs_: MESSAGE THREAD only (when !isPlaying)
 * - voices_: AUDIO THREAD only
 * - No shared mutable state between threads
 */
class Track
{
public:
    Track();
    ~Track();

    //==========================================================================
    // Clip Definition Management (MESSAGE THREAD ONLY)
    //==========================================================================

    /**
     * @brief Add clip definition to track
     * @param def Clip definition with unique ID
     *
     * MUST be called when playback is stopped (asserted)
     * MESSAGE THREAD only
     */
    void addClipDefinition(const ClipDef& def);

    /**
     * @brief Remove all clip definitions
     *
     * MUST be called when playback is stopped
     * MESSAGE THREAD only
     */
    void clearClipDefinitions();

    /**
     * @brief Find clip definition by ID
     * @param clipId Clip ID to find
     * @return Pointer to ClipDef, or nullptr if not found
     *
     * Safe to call from audio thread (read-only access)
     */
    const ClipDef* findClipDef(int64_t clipId) const noexcept;

    //==========================================================================
    // Transport Event Handling (AUDIO THREAD)
    //==========================================================================

    /**
     * @brief Handle transport event (start/stop clip)
     * @param ev Transport event
     * @param offsetInBlock Offset within current block (unused for now)
     * @param timelineSample Absolute timeline position (unused for now)
     *
     * AUDIO THREAD only - RT-safe
     */
    void handleEventRT(const struct TransportEvent& ev,
                       int offsetInBlock,
                       int64_t timelineSample);

    //==========================================================================
    // Audio Processing (AUDIO THREAD)
    //==========================================================================

    /**
     * @brief Process audio segment for this track
     * @param mixBuffer Destination buffer (additive mix)
     * @param segmentOffset Offset into mixBuffer
     * @param segmentLength Number of samples to process
     * @param timelineSample Absolute timeline position
     *
     * AUDIO THREAD only - RT-safe
     * Renders all active voices with fades into mixBuffer
     */
    void processSegment(juce::AudioBuffer<float>& mixBuffer,
                        int segmentOffset,
                        int segmentLength,
                        int64_t timelineSample);

private:
    //==========================================================================
    // Voice Management (AUDIO THREAD)
    //==========================================================================

    /**
     * @brief Start clip playback (allocate voice)
     * @param clipId Clip ID to start
     *
     * AUDIO THREAD only
     */
    void startClipRT(int64_t clipId);

    /**
     * @brief Stop clip playback (trigger fade-out)
     * @param clipId Clip ID to stop
     *
     * AUDIO THREAD only
     */
    void stopClipRT(int64_t clipId);

    /**
     * @brief Allocate voice for clip
     * @param def Clip definition
     * @return Pointer to allocated voice, or nullptr if all busy
     *
     * AUDIO THREAD only
     * Voice stealing: reuse first inactive voice, or steal oldest
     */
    ActiveVoice* allocVoiceRT(const ClipDef* def);

    /**
     * @brief Initialize voice state
     * @param v Voice to initialize
     * @param def Clip definition
     * @return Pointer to initialized voice
     *
     * AUDIO THREAD only
     */
    ActiveVoice* initVoice(ActiveVoice& v, const ClipDef* def);

    //==========================================================================
    // Member Variables
    //==========================================================================

    /// Clip definitions (MESSAGE THREAD only)
    std::vector<ClipDef> clipDefs_;

#if ZENITH_ENABLE_PHASE1_AUDIO
    /// Active voices (AUDIO THREAD only)
    static constexpr int kMaxVoicesPerTrack = 32;
    std::array<ActiveVoice, kMaxVoicesPerTrack> voices_;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Track)
};

} // namespace zenith
