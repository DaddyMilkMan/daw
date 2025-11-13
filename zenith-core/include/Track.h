/**
 * @file Track.h
 * @brief Audio/MIDI track with clips and playback
 *
 * Manages a single track with:
 * - Multiple audio clips
 * - Real-time playback with transport position tracking
 * - Mixing (volume, pan, mute, solo)
 * - Plugin chain (future)
 */

#pragma once

#include <JuceHeader.h>
#include <vector>
#include <memory>

//==============================================================================
/**
 * @class Track
 * @brief Represents an audio or MIDI track with clips
 *
 * Handles audio processing for a track, including:
 * - Clip playback with correct transport position handling
 * - Volume and pan control
 * - Mute and solo states
 *
 * **CRITICAL: Transport Position Handling**
 *
 * During playback, each clip must be informed of the current transport
 * position so it can:
 * 1. Determine if it's active (within its start/end time)
 * 2. Calculate correct read offset within the audio file
 * 3. Progress through the audio file on each callback
 *
 * Without updating transport position, clips will either:
 * - Never activate (always at position 0)
 * - Repeat the first buffer indefinitely
 */
class Track
{
public:
    //==========================================================================
    /**
     * @class Clip
     * @brief Represents an audio clip on the track
     */
    class Clip
    {
    public:
        /**
         * @brief Constructor
         * @param audioFile File to play
         * @param startTime Start time in seconds
         * @param length Length in seconds
         * @param srcOffsetSamples Offset into source file (in samples)
         * @param gain Clip gain (0.0 = silent, 1.0 = unity)
         * @param fadeInSamples Fade-in length (in samples, >= 0)
         * @param fadeOutSamples Fade-out length (in samples, >= 0)
         */
        Clip(const juce::File& audioFile,
             double startTime,
             double length,
             int64_t srcOffsetSamples = 0,
             float gain = 1.0f,
             int fadeInSamples = 0,
             int fadeOutSamples = 0);

        /**
         * @brief Set current transport position
         * @param positionInSeconds Current playhead position
         *
         * **CRITICAL:** Must be called before getNextAudioBlock()
         * This allows the clip to determine:
         * - If it should be playing at this position
         * - What sample offset to read from
         */
        void setTransportPosition(double positionInSeconds);

        /**
         * @brief Get current transport position
         * @return Current playhead position in seconds
         */
        double getTransportPosition() const;

        /**
         * @brief Get next audio block
         * @param bufferToFill Buffer to fill
         *
         * ⚠️ REAL-TIME AUDIO THREAD - Must be RT-safe!
         *
         * Uses the transport position set by setTransportPosition()
         * to determine:
         * 1. Is this clip active right now?
         * 2. What sample offset should we read from?
         * 3. How many samples to read?
         */
        void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill);

        /**
         * @brief Check if clip is active at current transport position
         * @return true if clip should be playing
         */
        bool isActive() const;

        /**
         * @brief Get clip start time
         * @return Start time in seconds
         */
        double getStartTime() const { return startTime; }

        /**
         * @brief Get clip length
         * @return Length in seconds
         */
        double getLength() const { return length; }

    private:
        juce::File file;
        double startTime{0.0};
        double length{0.0};

        // Clip properties
        int64_t srcOffsetSamples{0};    ///< Offset into source file (samples)
        float gain{1.0f};               ///< Clip gain (0.0 = silent, 1.0 = unity)
        int fadeInSamples{0};           ///< Fade-in length (samples, >= 0)
        int fadeOutSamples{0};          ///< Fade-out length (samples, >= 0)

        // **CRITICAL:** Transport position must be updated on each audio callback
        std::atomic<double> transportPosition{0.0};

        // Audio file reader
        std::unique_ptr<juce::AudioFormatReader> reader;
        juce::AudioFormatManager formatManager;

        // Current read position within the file
        int64_t currentReadPosition{0};

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Clip)
    };

    //==========================================================================
    /**
     * @brief Constructor
     * @param trackName Track name
     * @param isAudioTrack true for audio, false for MIDI
     */
    Track(const juce::String& trackName, bool isAudioTrack);

    /**
     * @brief Destructor
     */
    ~Track();

    //==========================================================================
    // Playback
    //==========================================================================

    /**
     * @brief Process audio block for this track
     * @param bufferToFill Buffer to fill
     * @param transportPosition Current playhead position in seconds
     *
     * ⚠️ REAL-TIME AUDIO THREAD - Must be RT-safe!
     *
     * **CRITICAL FIX:** Must pass transportPosition to each clip
     * before calling getNextAudioBlock()
     *
     * Before fix:
     * @code
     * // BUG: Transport position never updated!
     * for (auto& clip : clips) {
     *     clip->getNextAudioBlock(bufferToFill);
     * }
     * @endcode
     *
     * After fix:
     * @code
     * // CORRECT: Update transport position first
     * for (auto& clip : clips) {
     *     clip->setTransportPosition(transportPosition);
     *     clip->getNextAudioBlock(bufferToFill);
     * }
     * @endcode
     */
    void processAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill,
                           double transportPosition);

    //==========================================================================
    // Clip Management
    //==========================================================================

    /**
     * @brief Add audio clip to track
     * @param audioFile Audio file to play
     * @param startTime Start time in seconds
     * @param length Length in seconds
     * @return Pointer to created clip
     */
    Clip* addClip(const juce::File& audioFile, double startTime, double length);

    /**
     * @brief Remove clip
     * @param clip Clip to remove
     */
    void removeClip(Clip* clip);

    /**
     * @brief Clear all clips from track
     * @note MESSAGE THREAD ONLY
     */
    void clearClips();

    /**
     * @brief Get all clips
     * @return Vector of clip pointers
     */
    const std::vector<std::unique_ptr<Clip>>& getClips() const { return clips; }

    //==========================================================================
    // Mixer Controls
    //==========================================================================

    void setVolume(float newVolume) { volume.store(newVolume); }
    float getVolume() const { return volume.load(); }

    void setPan(float newPan) { pan.store(newPan); }
    float getPan() const { return pan.load(); }

    void setMute(bool shouldMute) { muted.store(shouldMute); }
    bool isMuted() const { return muted.load(); }

    void setSolo(bool shouldSolo) { solo.store(shouldSolo); }
    bool isSolo() const { return solo.load(); }

    //==========================================================================
    // Track Properties
    //==========================================================================

    juce::String getName() const { return name; }
    void setName(const juce::String& newName) { name = newName; }

    bool isAudio() const { return audioTrack; }

private:
    //==========================================================================
    // Member variables
    //==========================================================================

    juce::String name;
    bool audioTrack{true};

    // Clips on this track
    std::vector<std::unique_ptr<Clip>> clips;

    // Mixer state (atomic for thread safety)
    std::atomic<float> volume{0.8f};
    std::atomic<float> pan{0.0f};
    std::atomic<bool> muted{false};
    std::atomic<bool> solo{false};

    // Temporary mix buffer
    juce::AudioBuffer<float> mixBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Track)
};
