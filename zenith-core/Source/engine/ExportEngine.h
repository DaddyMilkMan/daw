/**
 * @file ExportEngine.h
 * @brief Offline audio export engine for Zenith DAW
 *
 * Handles rendering the project to WAV files using offline processing.
 * Runs on the message thread, separate from real-time audio callback.
 *
 * Features:
 * - Blocking offline export (no real-time constraints)
 * - 24-bit PCM WAV format at 44.1 kHz
 * - Stereo output
 * - Progress tracking
 * - Error handling
 */

#pragma once

#include <JuceHeader.h>
#include <memory>

// Forward declarations
class Engine;
class ProjectState;

//==============================================================================
/**
 * @class ExportEngine
 * @brief Handles offline export of audio projects to WAV files
 *
 * This class orchestrates the offline rendering process:
 * 1. Creates an AudioFormatWriter for the output file
 * 2. Iterates through the timeline in small blocks
 * 3. Calls the engine's mixing code for each block
 * 4. Writes the rendered audio to the WAV file
 *
 * The export runs on the message thread and blocks until complete.
 * This is intentional for simplicity - the user starts export and waits.
 */
class ExportEngine
{
public:
    //==========================================================================
    ExportEngine();
    ~ExportEngine();

    //==========================================================================
    // Export Configuration
    //==========================================================================

    /**
     * @brief Export the project to a WAV file
     *
     * This is a BLOCKING operation that runs on the message thread.
     * It will:
     * 1. Stop playback if active
     * 2. Create the WAV file
     * 3. Render the entire project offline
     * 4. Close the file
     *
     * @param engine Engine instance to render from
     * @param outputFile Destination WAV file
     * @param startSeconds Start time in seconds
     * @param endSeconds End time in seconds (0 = auto-detect from project)
     * @param projectState Optional project state for auto-detecting length
     * @param errorMessage Output error message if export fails
     * @return true if export succeeded, false otherwise
     *
     * @note MESSAGE THREAD ONLY - will assert in debug builds
     * @note Blocks until export completes or fails
     */
    bool exportToWav(
        Engine& engine,
        const juce::File& outputFile,
        double startSeconds,
        double endSeconds,
        ProjectState* projectState,
        juce::String& errorMessage);

private:
    //==========================================================================
    // Helper Methods
    //==========================================================================

    /**
     * @brief Calculate project length by scanning clips
     * @param projectState Project state to scan
     * @return Project length in seconds (or 0 if no clips found)
     */
    double calculateProjectLength(ProjectState* projectState);

    /**
     * @brief Render a block of audio offline
     * @param engine Engine to render from
     * @param outputBuffer Buffer to fill with rendered audio
     * @param numSamples Number of samples to render
     */
    void renderBlock(
        Engine& engine,
        juce::AudioBuffer<float>& outputBuffer,
        int numSamples);

    //==========================================================================
    // Member Variables
    //==========================================================================

    // Export settings (stored for reuse)
    double sampleRate_ = 44100.0;
    int bufferSize_ = 512;

    // Rendering state (reset per export)
    double renderPhase_ = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ExportEngine)
};

