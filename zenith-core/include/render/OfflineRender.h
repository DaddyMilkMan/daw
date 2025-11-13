/**
 * @file OfflineRender.h
 * @brief Offline audio rendering (bounce to WAV) - v0.1
 *
 * Renders ProjectModel to stereo 32-bit float WAV file offline.
 * Uses same engine path as real-time playback (W10/W10.3/W11).
 *
 * Thread safety: MESSAGE THREAD only (blocks until complete)
 */

#pragma once

#include <JuceHeader.h>
#include "../model/ProjectModel.h"

namespace zenith {

/**
 * @brief Render project to WAV file (offline mixdown)
 * @param project Project data to render
 * @param outputFile Target WAV file path
 * @param sampleRate Sample rate for export (must match project.sampleRate)
 * @param blockSize Processing block size (1024 recommended)
 * @param tailSeconds Extra silence at end (for reverb tails, etc.)
 * @return Result::ok() on success, or failure with error message
 *
 * Renders from t=0 to last clip end + tailSeconds.
 * Output: Stereo 32-bit float WAV, no normalization/dithering.
 *
 * v0.1 limitations:
 * - No resampling (project.sampleRate must == sampleRate)
 * - Always stereo (2 channels)
 * - No progress callback (blocks until done)
 * - Separate engine instance (doesn't touch live audio device)
 *
 * MESSAGE THREAD only
 */
juce::Result renderProjectToWav(const ProjectModel& project,
                                const juce::File& outputFile,
                                double sampleRate,
                                int blockSize,
                                double tailSeconds = 0.0);

} // namespace zenith
