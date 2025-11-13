/**
 * @file ArrangementPlaybackController.h
 * @brief Bridge between ProjectModel and Engine playback
 *
 * Converts ProjectModel data structures into Engine tracks and clips,
 * enabling playback of arrangements.
 *
 * Scope: v0.1 minimal features
 * - Load ProjectModel into Engine tracks/clips
 * - Basic transport controls
 * - Sample-accurate positioning
 *
 * NOT included in v0.1:
 * - No looping, time-stretch, automation
 * - No plugin state management
 * - No MIDI playback
 * - No real-time editing during playback
 */

#pragma once

#include <JuceHeader.h>
#include "../model/ProjectModel.h"

// Forward declarations
class Engine;

namespace zenith
{
    /**
     * @class ArrangementPlaybackController
     * @brief Bridges ProjectModel to Engine for playback
     *
     * **Thread Safety:**
     * - All methods are MESSAGE THREAD ONLY
     * - Do NOT call while engine is playing
     * - Always stop() before calling setProject()
     */
    class ArrangementPlaybackController
    {
    public:
        using SamplePos = juce::int64;

        /**
         * @brief Constructor
         * @param engine Engine instance to control
         */
        explicit ArrangementPlaybackController(Engine& engine);

        /**
         * @brief Destructor
         */
        ~ArrangementPlaybackController();

        //======================================================================
        // Project Management
        //======================================================================

        /**
         * @brief Replace current project and rebuild engine state
         * @param project Project model to load
         *
         * @note MESSAGE THREAD ONLY
         * @note Engine must be stopped before calling this
         *
         * This will:
         * 1. Stop the engine
         * 2. Clear all existing tracks
         * 3. Create new tracks from project model
         * 4. Load all clips into tracks
         * 5. Reset transport to position 0
         */
        void setProject(const ProjectModel& project);

        /**
         * @brief Get current project model
         * @return Reference to project model
         */
        const ProjectModel& getProject() const noexcept { return project_; }

        //======================================================================
        // Transport Control
        //======================================================================

        /**
         * @brief Play from start (seek to 0, then play)
         * @note MESSAGE THREAD ONLY
         */
        void playFromStart();

        /**
         * @brief Play from specific sample position
         * @param pos Sample position to start from
         * @note MESSAGE THREAD ONLY
         */
        void playFromSamples(SamplePos pos);

        /**
         * @brief Pause playback (stop advancing transport, keep position)
         * @note MESSAGE THREAD ONLY
         */
        void pause();

        /**
         * @brief Stop playback and seek to 0
         * @note MESSAGE THREAD ONLY
         */
        void stop();

        /**
         * @brief Get current transport position in samples
         * @return Current playback position
         */
        SamplePos getTransportSamples() const;

    private:
        //======================================================================
        // Internal Methods
        //======================================================================

        /**
         * @brief Apply project model to engine
         *
         * Converts ProjectModel tracks/clips into Engine Track/Clip objects.
         * This is where the magic happens - bridging data model to audio engine.
         */
        void applyProjectToEngine_();

        //======================================================================
        // Member Variables
        //======================================================================

        Engine&      engine_;      ///< Reference to audio engine
        ProjectModel project_;     ///< Local copy of project (source of truth)

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArrangementPlaybackController)
    };
}
