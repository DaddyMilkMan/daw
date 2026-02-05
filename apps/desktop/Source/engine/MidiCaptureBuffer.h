/*
  ==============================================================================

    MidiCaptureBuffer.h
    Created: 2025-12-28
    Author:  Zenith DAW

    Lock-free rolling MIDI buffer for retroactive recording (Ableton-style MIDI capture).

    Thread Safety:
    - capture() is AUDIO THREAD SAFE (RT-safe via lock-free ring buffer)
    - captureToClip()/clear() are MESSAGE THREAD ONLY
    - Uses lock-free AbstractFifo for RT-safe recording

    Architecture:
    - Rolling 30-second MIDI buffer (always recording)
    - Capture command converts buffer to MIDI clip
    - Integrates with transport (capture from last stop)

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <atomic>
#include <vector>

#include "EngineConstants.h"
#include "MidiNote.h"

namespace zenith {

// Forward declarations
class ProjectState;
class TempoMap;

//==============================================================================
/**
    Entry for a captured MIDI event in the rolling buffer.
*/
struct MidiCaptureEntry {
    juce::MidiMessage message;
    juce::int64 samplePosition = 0;    ///< Absolute sample position in transport
    int trackIndex = -1;                ///< Source track index (-1 for global input)
    
    MidiCaptureEntry() = default;
    MidiCaptureEntry(const juce::MidiMessage& msg, juce::int64 pos, int track)
        : message(msg), samplePosition(pos), trackIndex(track) {}
};

//==============================================================================
/**
    Lock-free rolling MIDI buffer for retroactive MIDI capture.

    Features:
    - Configurable buffer duration (default 30 seconds)
    - RT-safe MIDI event capture from audio thread
    - "Capture" command converts buffer contents to MIDI clip
    - Transport integration (capture from last stop point)
    - Supports filtering by track index

    Usage:
    1. Call capture() from audio thread to continuously record MIDI
    2. Call captureToClip() from message thread to convert buffer to clip
    3. Call clear() to reset buffer

    @code
    // In audio callback:
    captureBuffer.capture(midiMessage, playheadSamples, trackIndex);

    // User triggers "Capture MIDI" command:
    captureBuffer.captureToClip(projectState, trackId, tempoMap, fromPosition);
    @endcode
*/
class MidiCaptureBuffer {
public:
    //==========================================================================
    /**
     * @brief Default buffer duration in seconds
     */
    static constexpr float kDefaultBufferDurationSeconds = 30.0f;
    
    /**
     * @brief Default buffer size (entries)
     * Assuming ~1000 MIDI events per second max, 30 seconds = ~30000 entries
     */
    static constexpr int kDefaultBufferSize = 32768;

    //==========================================================================
    MidiCaptureBuffer();
    ~MidiCaptureBuffer();

    //==========================================================================
    // Configuration
    //==========================================================================

    /**
     * @brief Prepare the buffer for operation
     * @param sampleRate Current sample rate
     * @note MESSAGE THREAD ONLY
     */
    void prepare(double sampleRate);

    /**
     * @brief Set the buffer duration in seconds
     * @param durationSeconds Duration to keep MIDI events (default 30s)
     * @note MESSAGE THREAD ONLY
     */
    void setBufferDuration(float durationSeconds);

    /**
     * @brief Get the current buffer duration in seconds
     */
    float getBufferDuration() const { return bufferDurationSeconds_.load(); }

    /**
     * @brief Enable/disable MIDI capture
     * @param enabled Whether to capture incoming MIDI
     */
    void setEnabled(bool enabled) { enabled_.store(enabled); }

    /**
     * @brief Check if capture is enabled
     */
    bool isEnabled() const { return enabled_.load(); }

    //==========================================================================
    // MIDI Capture (RT-Safe)
    //==========================================================================

    /**
     * @brief Capture a MIDI message to the rolling buffer
     * @param message MIDI message to capture
     * @param samplePosition Absolute sample position in transport
     * @param trackIndex Source track index (-1 for global input)
     * @note AUDIO THREAD ONLY - RT-safe
     */
    void capture(const juce::MidiMessage& message, juce::int64 samplePosition, 
                 int trackIndex = -1);

    /**
     * @brief Capture all events from a MIDI buffer
     * @param buffer MIDI buffer to capture
     * @param bufferStartSamplePosition Sample position of buffer start
     * @param trackIndex Source track index
     * @note AUDIO THREAD ONLY - RT-safe
     */
    void captureBuffer(const juce::MidiBuffer& buffer, 
                       juce::int64 bufferStartSamplePosition,
                       int trackIndex = -1);

    //==========================================================================
    // Capture to Clip (Message Thread)
    //==========================================================================

    /**
     * @brief Convert captured MIDI to a clip in ProjectState
     * @param projectState ProjectState to add clip to
     * @param trackId Target track ID for the clip
     * @param tempoMap Tempo map for beat conversion
     * @param fromSamplePosition Start position (use last stop if -1)
     * @param toSamplePosition End position (-1 for current)
     * @param filterTrackIndex Only include events from this track (-1 for all)
     * @return true if clip was created successfully
     * @note MESSAGE THREAD ONLY
     */
    bool captureToClip(ProjectState& projectState,
                       const juce::String& trackId,
                       const TempoMap& tempoMap,
                       juce::int64 fromSamplePosition = -1,
                       juce::int64 toSamplePosition = -1,
                       int filterTrackIndex = -1);

    /**
     * @brief Get captured events as a MIDI sequence
     * @param fromSamplePosition Start position (use last stop if -1)
     * @param toSamplePosition End position (-1 for current)
     * @param filterTrackIndex Only include events from this track (-1 for all)
     * @return MIDI message sequence with captured events
     * @note MESSAGE THREAD ONLY
     */
    juce::MidiMessageSequence getCapturedSequence(
        juce::int64 fromSamplePosition = -1,
        juce::int64 toSamplePosition = -1,
        int filterTrackIndex = -1) const;

    //==========================================================================
    // Transport Integration
    //==========================================================================

    /**
     * @brief Mark the current position as the last stop point
     * @param samplePosition Position where transport stopped
     * @note Called automatically by transport controller
     */
    void markStopPosition(juce::int64 samplePosition);

    /**
     * @brief Get the last stop position
     */
    juce::int64 getLastStopPosition() const { return lastStopPosition_.load(); }

    //==========================================================================
    // Buffer Management
    //==========================================================================

    /**
     * @brief Clear all captured MIDI events
     * @note MESSAGE THREAD ONLY
     */
    void clear();

    /**
     * @brief Get the number of events currently in buffer
     * @note Approximate - may include expired events
     */
    int getNumEvents() const { return fifo_.getNumReady(); }

    /**
     * @brief Check if buffer has any captured events
     */
    bool hasEvents() const { return getNumEvents() > 0; }

    /**
     * @brief Get statistics about dropped messages
     */
    uint64_t getDroppedMessageCount() const { return droppedMessages_.load(); }

private:
    //==========================================================================
    // State
    //==========================================================================

    double sampleRate_ = constants::kDefaultSampleRate;
    std::atomic<float> bufferDurationSeconds_{kDefaultBufferDurationSeconds};
    std::atomic<bool> enabled_{true};
    std::atomic<juce::int64> lastStopPosition_{0};

    // Lock-free ring buffer for MIDI capture
    juce::AbstractFifo fifo_{kDefaultBufferSize};
    std::vector<MidiCaptureEntry> buffer_;

    // Statistics
    std::atomic<uint64_t> droppedMessages_{0};

    // Thread safety for message-thread operations
    mutable juce::CriticalSection messageLock_;

    //==========================================================================
    // Internal Methods
    //==========================================================================

    /**
     * @brief Remove events older than buffer duration
     * @param currentSamplePosition Current transport position
     * @note Called internally during capture
     */
    void pruneOldEvents(juce::int64 currentSamplePosition);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiCaptureBuffer)
};

} // namespace zenith
