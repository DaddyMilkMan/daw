/**
 * @file ProjectModel.h
 * @brief Project data model (v0.1) - audio tracks with audio clips
 *
 * This is the in-memory representation of the arrangement.
 * No engine interaction - pure data layer on message thread.
 *
 * v0.1 scope:
 * - Audio tracks
 * - Audio clips with timing, fades, gain
 * - No FX, no tempo, no markers
 */

#pragma once

#include <JuceHeader.h>
#include <vector>

namespace zenith::model
{
    /**
     * @brief Single audio clip on the timeline
     */
    struct ClipModel
    {
        int64_t      id             { 0 };       // Unique within project
        juce::File   file;                       // Resolved path to audio file
        int64_t      startSample    { 0 };       // Timeline position (absolute)
        int64_t      lengthSamples  { 0 };       // How long on timeline
        int64_t      srcOffset      { 0 };       // Offset inside source file (trim)
        float        gain           { 1.0f };    // 0.0 .. 2.0
        int          fadeInSamples  { 0 };       // Fade-in length
        int          fadeOutSamples { 0 };       // Fade-out length
        bool         muted          { false };   // Muted state
    };

    /**
     * @brief Single audio track containing clips
     */
    struct TrackModel
    {
        int                     trackId { 0 };   // Track index/ID
        juce::String            name;            // Track name
        std::vector<ClipModel>  clips;           // All clips on this track
    };

    /**
     * @brief Complete project model (single source of truth)
     */
    struct ProjectModel
    {
        juce::String             name;                     // Project name
        double                   sampleRate { 48000.0 };   // Project sample rate
        std::vector<TrackModel>  tracks;                   // All tracks
    };

    //==========================================================================
    // API (message thread only)
    //==========================================================================

    /**
     * @brief Create an empty project with given sample rate and name
     * @param sampleRate Project sample rate (default: 48000.0)
     * @param name Project name (default: "Untitled")
     * @return Empty ProjectModel with 0 tracks
     */
    ProjectModel makeEmptyProject(double sampleRate = 48000.0,
                                  const juce::String& name = "Untitled");

    /**
     * @brief Convert ValueTree to ProjectModel
     * @param root ValueTree with ID_PROJECT root (or empty)
     * @param projectFileDirectory Directory containing .zenithproj file (for resolving relative paths)
     * @return Parsed ProjectModel (or default empty project if invalid)
     *
     * @note Robust parsing: missing properties use sensible defaults, never crashes
     */
    ProjectModel projectFromValueTree(const juce::ValueTree& root,
                                      const juce::File& projectFileDirectory);

    /**
     * @brief Convert ProjectModel to ValueTree
     * @param project Project to serialize
     * @param projectFileDirectory Directory where .zenithproj will be saved (for relative paths)
     * @return ValueTree with ID_PROJECT root
     *
     * @note Stores clip paths relative to projectFileDirectory when possible
     */
    juce::ValueTree projectToValueTree(const ProjectModel& project,
                                       const juce::File& projectFileDirectory);
}
