/**
 * @file OfflineRender.h
 * @brief Offline rendering (bounce to WAV file)
 *
 * Provides functionality to render a ProjectModel to a WAV file offline,
 * using the same engine path as real-time playback but driven manually.
 *
 * Thread Safety:
 * - MESSAGE THREAD ONLY
 * - All file I/O is blocking
 * - Uses a throwaway Engine instance (doesn't affect live playback)
 */

#pragma once

#include <model/ProjectModel.h>
#include <JuceHeader.h>

namespace zenith
{
    /**
     * @brief Render a project to a stereo 32-bit float WAV file offline
     * @param project Project data (tracks + clips)
     * @param outputFile Target .wav file (will be overwritten)
     * @param sampleRate Engine sample rate (must match project.sampleRate for v0.1)
     * @param blockSize Internal processing block size (e.g. 512/1024)
     * @param tailSeconds Extra time after last clip end to render (for FX tails)
     * @return juce::Result::ok() on success, or error message
     *
     * @note MESSAGE THREAD ONLY
     *
     * Behavior:
     * - Creates a throwaway Engine instance for rendering
     * - Uses ArrangementPlaybackController to load project
     * - Walks timeline in blocks, processing offline
     * - Writes 32-bit float stereo WAV to disk
     * - Same audio path as real-time playback (W10/W10.3 segment loop)
     *
     * v0.1 Constraints:
     * - No resampling: export SR must match project SR
     * - Always 32-bit float, stereo, WAV format
     * - No normalization or dithering
     * - No progress callback (blocking operation)
     */
    juce::Result renderProjectToWav(const ProjectModel& project,
                                    const juce::File& outputFile,
                                    double sampleRate,
                                    int blockSize,
                                    double tailSeconds = 0.0);

} // namespace zenith
