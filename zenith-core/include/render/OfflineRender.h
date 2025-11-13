/**
 * @file OfflineRender.h
 * @brief Offline audio rendering (bounce to audio file)
 *
 * Renders ProjectModel to audio file offline with multiple format options.
 * Uses same engine path as real-time playback (W10/W10.3/W11).
 *
 * Thread safety: MESSAGE THREAD only (blocks until complete)
 */

#pragma once

#include <JuceHeader.h>
#include "../model/ProjectModel.h"

namespace zenith {

/**
 * @brief Audio export format
 */
enum class ExportFormat
{
    WAV,   ///< Waveform Audio File Format (PCM)
    AIFF,  ///< Audio Interchange File Format (PCM)
    // MP3 - v0.2+ (requires external encoder like LAME)
};

/**
 * @brief Export options for offline rendering
 */
struct ExportOptions
{
    ExportFormat format = ExportFormat::WAV;  ///< Output format
    int bitsPerSample = 32;                   ///< Bit depth (8, 16, 24, or 32)
    double tailSeconds = 0.5;                 ///< Extra silence at end (for reverb tails)
    int blockSize = 1024;                     ///< Processing block size

    /**
     * @brief Validate export options
     * @return Error message if invalid, empty string if valid
     */
    juce::String validate() const
    {
        if (bitsPerSample != 8 && bitsPerSample != 16 &&
            bitsPerSample != 24 && bitsPerSample != 32)
        {
            return "Invalid bit depth. Must be 8, 16, 24, or 32.";
        }

        if (blockSize <= 0)
            return "Invalid block size.";

        if (tailSeconds < 0.0)
            return "Invalid tail seconds (must be >= 0).";

        return {};
    }

    /**
     * @brief Get file extension for this format
     * @return File extension (e.g. ".wav", ".aiff")
     */
    juce::String getFileExtension() const
    {
        switch (format)
        {
            case ExportFormat::WAV:  return ".wav";
            case ExportFormat::AIFF: return ".aiff";
            default: return ".wav";
        }
    }

    /**
     * @brief Get format name for display
     * @return Human-readable format name
     */
    juce::String getFormatName() const
    {
        switch (format)
        {
            case ExportFormat::WAV:  return "WAV";
            case ExportFormat::AIFF: return "AIFF";
            default: return "Unknown";
        }
    }

    /**
     * @brief Check if format uses float samples for this bit depth
     * @return true if 32-bit float, false otherwise
     */
    bool usesFloatSamples() const
    {
        return bitsPerSample == 32;
    }
};

/**
 * @brief Render project to audio file (offline mixdown)
 * @param project Project data to render
 * @param outputFile Target audio file path
 * @param sampleRate Sample rate for export (must match project.sampleRate)
 * @param options Export options (format, bit depth, tail, etc.)
 * @return Result::ok() on success, or failure with error message
 *
 * Renders from t=0 to last clip end + tailSeconds.
 * Output: Stereo PCM audio, no normalization/dithering.
 *
 * Supported formats:
 * - WAV: 8, 16, 24, 32-bit (32-bit uses float)
 * - AIFF: 8, 16, 24, 32-bit (32-bit uses float)
 *
 * v0.1 limitations:
 * - No resampling (project.sampleRate must == sampleRate)
 * - Always stereo (2 channels)
 * - No progress callback (blocks until done)
 * - Separate engine instance (doesn't touch live audio device)
 *
 * MESSAGE THREAD only
 */
juce::Result renderProjectToFile(const ProjectModel& project,
                                 const juce::File& outputFile,
                                 double sampleRate,
                                 const ExportOptions& options);

/**
 * @brief Legacy function for backward compatibility
 * @deprecated Use renderProjectToFile() with ExportOptions instead
 */
inline juce::Result renderProjectToWav(const ProjectModel& project,
                                       const juce::File& outputFile,
                                       double sampleRate,
                                       int blockSize,
                                       double tailSeconds = 0.0)
{
    ExportOptions opts;
    opts.format = ExportFormat::WAV;
    opts.bitsPerSample = 32;
    opts.blockSize = blockSize;
    opts.tailSeconds = tailSeconds;
    return renderProjectToFile(project, outputFile, sampleRate, opts);
}

} // namespace zenith
