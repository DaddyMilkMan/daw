/**
 * @file OfflineRender.h
 * @brief Offline (headless) rendering of ProjectModel to WAV
 *
 * Provides offline export functionality using the same audio path as
 * real-time playback, but without an audio device.
 *
 * Thread Safety:
 * - All functions are MESSAGE THREAD ONLY
 * - No RT constraints (offline rendering can allocate)
 */

#pragma once

#include <JuceHeader.h>
#include "../model/ProjectModel.h"

namespace zenith
{
    /**
     * @brief Render a ProjectModel to a 32-bit float stereo WAV file
     *
     * This function creates a temporary headless engine instance, loads the
     * project into it (decoding all audio files), and renders the mix to a
     * WAV file using the exact same audio processing path as real-time playback.
     *
     * @param project      The project to render (audio-only in v0.1)
     * @param outputFile   Target WAV file (.wav extension recommended)
     * @param sampleRate   Target sample rate (must match project.sampleRate in v0.1)
     * @param blockSize    Offline processing block size (e.g. 512 or 1024)
     * @param tailSeconds  Extra seconds to render after project end (for reverb tails, etc.)
     *
     * @return juce::Result::ok() on success, or Result::fail() with error message
     *
     * @note MESSAGE THREAD ONLY
     *
     * Limitations (v0.1):
     * - No sample rate conversion: sampleRate must equal project.sampleRate
     * - Output is always 32-bit float stereo WAV
     * - No normalization or dithering
     * - No progress callbacks
     *
     * The function will:
     * 1. Validate parameters (sample rate, project data)
     * 2. Compute total render length from clip positions
     * 3. Create output directory if needed
     * 4. Create temporary Engine and load project
     * 5. Render in blocks, writing to WAV file
     * 6. Return success or detailed error message
     */
    juce::Result renderProjectToWav(const ProjectModel& project,
                                    const juce::File& outputFile,
                                    double sampleRate,
                                    int blockSize = 1024,
                                    double tailSeconds = 0.5);
}
