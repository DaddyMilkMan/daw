/**
 * @file ClipSynchronizer.h
 * @brief Synchronizes clips between Engine and ProjectState
 *
 * Integration stub: Shows how ClipSynchronizer would work when recording
 * features are merged from U3 branch.
 *
 * Data Flow:
 * 1. RecordingManager creates clips in Engine tracks (zenith::Clip)
 * 2. ClipSynchronizer monitors Engine track changes
 * 3. When new Engine clip appears, creates corresponding ProjectState CLIP node
 * 4. ArrangerView listens to ProjectState changes and displays clips
 *
 * Thread Safety:
 * - Runs on MESSAGE THREAD (timer-based polling)
 * - Reads Engine clips from message thread (safe via dirty flags)
 * - Writes to ProjectState (message thread only)
 */

#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_events/juce_events.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_data_structures/juce_data_structures.h>
#include "ProjectState.h"
#include "Engine.h"
#include <map>

//==============================================================================
/**
 * @class ClipSynchronizer
 * @brief Syncs Engine clips to ProjectState clips
 *
 * This class bridges the audio engine's clip data with the project state:
 * - When recording creates new clips in Engine, they appear in ProjectState
 * - When user adds clips via UI, they're created in both Engine and ProjectState
 * - Keeps clip positions, lengths, and properties synchronized
 */
class ClipSynchronizer : public juce::Timer
{
public:
    //==========================================================================
    /**
     * @brief Constructor
     * @param projectState Reference to project state (must outlive this object)
     * @param engine Reference to audio engine (must outlive this object)
     */
    ClipSynchronizer(zenith::ProjectState& projectState, zenith::Engine& engine);

    /**
     * @brief Destructor
     */
    ~ClipSynchronizer() override;

    //==========================================================================
    /**
     * @brief Start clip synchronization
     * @param updateRateHz Update rate in Hz (default 30 for clip changes)
     */
    void start(int updateRateHz = 30);

    /**
     * @brief Stop clip synchronization
     */
    void stop();

    /**
     * @brief Check if synchronizer is running
     */
    bool isRunning() const { return isTimerRunning(); }

    //==========================================================================
    /**
     * @brief Create clip in both Engine and ProjectState
     * @param trackId ProjectState track ID
     * @param startBeats Start position in beats
     * @param lengthBeats Length in beats
     * @param clipType "audio" or "midi"
     * @return Clip ID
     * @note Message thread only
     */
    juce::String createClip(const juce::String& trackId, double startBeats,
                            double lengthBeats, const juce::String& clipType);

private:
    //==========================================================================
    // Timer callback (MESSAGE THREAD)
    //==========================================================================

    /**
     * @brief Timer callback - checks for new clips from recording engine
     * @note Called on MESSAGE THREAD at regular intervals
     */
    void timerCallback() override;

    //==========================================================================
    // Helper Methods
    //==========================================================================

    /**
     * @brief Sync clips from Engine to ProjectState
     * @note Checks for new clips created by RecordingEngine
     */
    void syncEngineToProjectState();

    /**
     * @brief Convert beats to samples
     */
    int64_t beatsToSamples(double beats, double tempo, double sampleRate) const;

    /**
     * @brief Convert samples to beats
     */
    double samplesToBeats(int64_t samples, double tempo, double sampleRate) const;

    //==========================================================================
    // Member Variables
    //==========================================================================

    zenith::ProjectState& projectState;
    zenith::Engine& engine;

    // Track clip counts to detect new clips
    std::map<int, int> engineClipCounts;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClipSynchronizer)
};

