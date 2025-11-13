/**
 * @file ProjectModel.h
 * @brief Plain C++ data structures for project model
 *
 * Defines the minimal v0.1 project data model:
 * - ProjectModel: Top-level container
 * - TrackModel: Audio track with clips
 * - ClipModel: Audio clip referencing a file
 *
 * Provides conversion functions to/from JUCE ValueTree for serialization.
 *
 * Scope: v0.1 minimal features only
 * - No automation, tempo changes, or time signature
 * - No MIDI tracks or clips
 * - No plugin state
 * - No undo/redo (that lives in ProjectState/ProjectManager)
 */

#pragma once

#include <JuceHeader.h>
#include <vector>

namespace zenith
{
    //==========================================================================
    // Type aliases
    //==========================================================================

    using SamplePos = juce::int64;

    //==========================================================================
    /**
     * @struct ClipModel
     * @brief Represents an audio clip on the timeline
     *
     * Position and length are in samples for sample-accurate placement.
     * Source offset allows trimming the start of the audio file.
     */
    struct ClipModel
    {
        int id = 0;                     ///< Unique clip ID
        juce::String name;              ///< Clip name (defaults to file name)

        juce::File file;                ///< Audio file path
        SamplePos startSample = 0;      ///< Timeline position (in samples)
        SamplePos lengthSamples = 0;    ///< Duration on timeline (in samples)
        SamplePos srcOffset = 0;        ///< Offset into source file (in samples)

        float gain = 1.0f;              ///< Clip gain (0.0 = silent, 1.0 = unity)
        int fadeInSamples = 0;          ///< Fade-in length (in samples)
        int fadeOutSamples = 0;         ///< Fade-out length (in samples)
        bool muted = false;             ///< Clip mute state
    };

    //==========================================================================
    /**
     * @struct TrackModel
     * @brief Represents an audio track containing clips
     *
     * Each track has mixer controls (gain, pan, mute) and a list of clips.
     */
    struct TrackModel
    {
        int id = 0;                     ///< Unique track ID
        juce::String name;              ///< Track name
        float gain = 1.0f;              ///< Track gain (0.0 = silent, 1.0 = unity)
        float pan = 0.0f;               ///< Pan position (-1.0 = left, +1.0 = right)
        bool muted = false;             ///< Track mute state

        std::vector<ClipModel> clips;   ///< Clips on this track
    };

    //==========================================================================
    /**
     * @struct ProjectModel
     * @brief Top-level project data
     *
     * Contains project-wide settings and all tracks.
     */
    struct ProjectModel
    {
        juce::String name;              ///< Project name
        double sampleRate = 48000.0;    ///< Project sample rate

        std::vector<TrackModel> tracks; ///< All tracks in project
    };

    //==========================================================================
    // Conversion API
    //==========================================================================

    /**
     * @brief Convert ProjectModel to ValueTree for serialization
     * @param model Project model to convert
     * @return ValueTree representation
     *
     * Creates a hierarchical ValueTree that can be serialized to XML or binary.
     * All data is deep-copied into the ValueTree.
     */
    juce::ValueTree projectToValueTree(const ProjectModel& model);

    /**
     * @brief Convert ValueTree back to ProjectModel
     * @param rootVT ValueTree to parse (should be of type ids::project)
     * @return ProjectModel with data from ValueTree
     *
     * If rootVT is invalid or wrong type, returns a minimal empty project.
     * Missing properties use sane defaults (0, 1.0f, false, etc.).
     */
    ProjectModel projectFromValueTree(const juce::ValueTree& rootVT);

    //==========================================================================
    // Helper functions
    //==========================================================================

    /**
     * @brief Create an empty project with defaults
     * @param sampleRate Sample rate for the project
     * @return Empty ProjectModel ready for use
     *
     * Creates:
     * - name = "Untitled"
     * - sampleRate = specified value
     * - tracks = empty vector
     */
    ProjectModel makeEmptyProject(double sampleRate = 48000.0);
}
