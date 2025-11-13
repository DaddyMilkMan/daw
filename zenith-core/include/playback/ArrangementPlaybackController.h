/**
 * @file ArrangementPlaybackController.h
 * @brief Bridge between ProjectModel and Engine for arrangement playback
 *
 * Responsibilities:
 * - Load ProjectModel and convert to engine tracks/clips
 * - Manage transport controls (play, pause, stop)
 * - Handle audio file decoding and clip setup
 * - MESSAGE THREAD ONLY - never called from audio thread
 *
 * Architecture:
 * ```
 * [ProjectModel] → [ArrangementPlaybackController] → [Engine]
 *                                                        ↓
 *                                                     [Mixer]
 *                                                        ↓
 *                                                   [AudioTracks]
 * ```
 */

#pragma once

#include <model/ProjectModel.h>
#include <JuceHeader.h>

namespace zenith
{
    // Forward declarations
    class Engine;

    /**
     * @class ArrangementPlaybackController
     * @brief Converts ProjectModel to engine state and manages playback
     *
     * This controller:
     * - Owns a copy of the ProjectModel
     * - Translates model tracks/clips → engine tracks/clips
     * - Provides high-level transport controls
     * - Handles audio file loading and decoding
     *
     * Thread Safety:
     * - ALL methods run on MESSAGE THREAD only
     * - Never called from audio thread
     * - Delegates to Engine which handles RT-safety
     */
    class ArrangementPlaybackController
    {
    public:
        using SamplePos = juce::int64;

        /**
         * @brief Constructor
         * @param engine Reference to audio engine
         * @note ClipLoader removed for v0.1 - we'll load audio files directly
         */
        ArrangementPlaybackController(Engine& engine);

        /**
         * @brief Replace the current project and rebuild engine state
         * @param project New project to load
         * @note MESSAGE THREAD ONLY
         * - Stops engine
         * - Decodes all audio files
         * - Rebuilds engine tracks and clips
         * - Resets transport to 0
         */
        void setProject(const ProjectModel& project);

        /**
         * @brief Get current project
         * @return Const reference to internal project
         */
        const ProjectModel& getProject() const noexcept { return project_; }

        //==========================================================================
        // Transport Controls (MESSAGE THREAD ONLY)
        //==========================================================================

        /**
         * @brief Start playback from sample 0
         */
        void playFromStart();

        /**
         * @brief Start playback from specific sample position
         * @param startSample Sample position to start from
         * @note v0.1: basic implementation, no mid-clip compensation yet
         */
        void playFromSamples(SamplePos startSample);

        /**
         * @brief Stop playback and reset transport to 0
         */
        void stop();

        /**
         * @brief Pause playback (stop transport but keep position)
         */
        void pause();

        /**
         * @brief Get current transport position
         * @return Transport position in samples
         */
        SamplePos getTransportSamples() const;

    private:
        //==========================================================================
        // Internal Methods
        //==========================================================================

        /**
         * @brief Apply project to engine (core logic)
         * @note MESSAGE THREAD ONLY
         * - Stops transport
         * - Decodes all audio files from project
         * - Creates engine tracks
         * - Adds clips to tracks
         */
        void applyProjectToEngine_();

        /**
         * @brief Load audio file and convert to JUCE buffer
         * @param file Audio file to load
         * @param outBuffer Output buffer to fill
         * @return true if successful, false if failed
         */
        bool loadAudioFile_(const juce::File& file, juce::AudioBuffer<float>& outBuffer);

        //==========================================================================
        // Member Variables
        //==========================================================================

        Engine& engine_;
        ProjectModel project_;

        // Audio format manager for loading WAV/AIFF/etc files
        juce::AudioFormatManager formatManager_;

        // Storage for decoded clip audio data
        // Maps clip ID → audio buffer (owned by controller)
        // These buffers are referenced by AudioTrack clips (via pointer)
        std::unordered_map<juce::int64, std::unique_ptr<juce::AudioBuffer<float>>> clipAudioData_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArrangementPlaybackController)
    };

} // namespace zenith
