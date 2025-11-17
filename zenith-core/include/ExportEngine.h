/**
 * @file ExportEngine.h
 * @brief Offline WAV export engine for Zenith DAW
 *
 * Renders the entire project to a WAV file synchronously on the message thread.
 * This is a BLOCKING operation - the UI will be frozen during export.
 *
 * Thread Safety:
 * - All methods must be called from the MESSAGE THREAD only
 * - Uses offline rendering (not real-time audio thread)
 */

#pragma once

#include <JuceHeader.h>

// Forward declaration
class Engine;

//==============================================================================
/**
 * @class ExportEngine
 * @brief Handles offline rendering of projects to WAV files
 *
 * This engine performs synchronous (blocking) offline rendering:
 * 1. Processes audio in offline mode (faster than real-time)
 * 2. Writes directly to WAV file using JUCE's AudioFormatWriter
 * 3. Returns when export is complete
 *
 * Usage:
 * @code
 * ExportEngine exporter;
 * juce::String errorMsg;
 * bool success = exporter.exportToWav(
 *     engine,
 *     outputFile,
 *     0.0,      // start time
 *     10.0,     // end time
 *     errorMsg
 * );
 * @endcode
 */
class ExportEngine
{
public:
    //==========================================================================
    ExportEngine() = default;
    ~ExportEngine() = default;

    //==========================================================================
    // Export API
    //==========================================================================

    /**
     * @brief Export project to WAV file (blocking operation)
     *
     * This method:
     * 1. Creates a WAV file at the specified path
     * 2. Renders audio offline from startTime to endTime
     * 3. Writes audio data to the WAV file
     * 4. Returns when complete
     *
     * ⚠️ BLOCKING: This will freeze the UI during export!
     * ⚠️ MESSAGE THREAD ONLY: Must not be called from audio thread!
     *
     * @param engine Reference to the audio engine
     * @param outputFile Destination WAV file
     * @param startTime Start time in seconds
     * @param endTime End time in seconds
     * @param errorMessage Output parameter for error messages
     * @return true if export succeeded, false otherwise
     *
     * @note Currently renders silence for Phase 0
     *       TODO: Integrate with actual audio processing in Phase 1
     */
    bool exportToWav(
        Engine& engine,
        const juce::File& outputFile,
        double startTime,
        double endTime,
        juce::String& errorMessage);

private:
    //==========================================================================
    // Helper methods
    //==========================================================================

    /**
     * @brief Render audio offline (placeholder implementation)
     * @param buffer Buffer to fill with audio
     * @param sampleRate Sample rate for rendering
     * @param startTime Start time in seconds
     * @param numSamples Number of samples to render
     *
     * @note Currently generates silence
     *       TODO: Replace with actual rendering in Phase 1
     */
    void renderAudioOffline(
        juce::AudioBuffer<float>& buffer,
        double sampleRate,
        double startTime,
        int numSamples);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ExportEngine)
};
