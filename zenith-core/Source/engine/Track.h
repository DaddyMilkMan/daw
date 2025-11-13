/**
 * @file Track.h
 * @brief Audio track for Zenith DAW
 *
 * Responsibilities:
 * - Manage track-level state (gain, pan, mute, solo)
 * - Render clips into audio buffer
 * - Process FX chain (W11.0: internal nodes, later VST3)
 * - No allocations in processBlock() (real-time safe)
 */

#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <vector>
#include <array>
#include <memory>
#include "Clip.h"
#include "IDspNode.h"

namespace zenith {

/**
 * @class Track
 * @brief Single audio track with clips
 *
 * Thread Safety:
 * - processBlock() runs on AUDIO THREAD (real-time safe!)
 * - All setters run on MESSAGE THREAD
 * - Use std::atomic for cross-thread communication
 */
class Track
{
public:
    Track();
    ~Track();

    //==========================================================================
    // Lifecycle (MESSAGE THREAD)
    //==========================================================================

    /**
     * @brief Prepare track for playback (pre-allocate buffers)
     * @param blockSize Maximum block size
     * @param sampleRate Sample rate
     */
    void prepareToPlay(int blockSize, double sampleRate);

    /**
     * @brief Release resources (called when audio stops)
     */
    void releaseResources();

    //==========================================================================
    // Audio Processing (AUDIO THREAD)
    //==========================================================================

    /**
     * @brief Render track into mix buffer
     * @param mixBuffer Buffer to render into (additive mix)
     * @param numSamples Number of samples to render
     * @param transportPosition Current transport position in samples
     *
     * @note AUDIO THREAD - real-time safe!
     * @note Does NOT clear buffer - mixes additively
     */
    void processBlock(juce::AudioBuffer<float>& mixBuffer,
                      int numSamples,
                      juce::int64 transportPosition);

    //==========================================================================
    // Track State (MESSAGE THREAD)
    //==========================================================================

    void setName(const juce::String& name) { trackName = name; }
    juce::String getName() const { return trackName; }

    void setGain(float g) { gain_.store(juce::jlimit(0.0f, 2.0f, g)); }
    float getGain() const { return gain_.load(); }

    void setPan(float p) { pan_.store(juce::jlimit(-1.0f, 1.0f, p)); }
    float getPan() const { return pan_.load(); }

    void setMuted(bool m) { muted_.store(m); }
    bool isMuted() const { return muted_.load(); }

    void setSoloed(bool s) { soloed_.store(s); }
    bool isSoloed() const { return soloed_.load(); }

    //==========================================================================
    // W13: Clip Management (MESSAGE THREAD)
    //==========================================================================

    /**
     * @brief Add clip to track (MESSAGE THREAD)
     * @param clip Clip to add (must be valid)
     *
     * @note Clips are automatically sorted by startSample
     * @note Do NOT call while audio is playing (static timeline assumption)
     */
    void addClip(const Clip& clip);

    /**
     * @brief Get number of clips
     */
    int getNumClips() const { return static_cast<int>(clips_.size()); }

    /**
     * @brief Clear all clips (MESSAGE THREAD)
     */
    void clearClips();

    //==========================================================================
    // W11.0: FX Chain Management (MESSAGE THREAD)
    //==========================================================================

    /**
     * @brief Get number of FX slots per track (fixed at 5)
     */
    static constexpr int getNumFxSlots() { return 5; }

    /**
     * @brief Set FX node in slot (MESSAGE THREAD)
     * @param slotIndex Slot index [0..4]
     * @param node DSP node (ownership transferred to Track)
     *
     * @note Replaces existing node in slot (if any)
     * @note Node will be prepared if track is already prepared
     */
    void setFxNode(int slotIndex, std::unique_ptr<IDspNode> node);

    /**
     * @brief Clear FX node from slot (MESSAGE THREAD)
     * @param slotIndex Slot index [0..4]
     */
    void clearFxNode(int slotIndex);

    /**
     * @brief Get FX node from slot (for parameter access)
     * @param slotIndex Slot index [0..4]
     * @return Pointer to node, or nullptr if slot is empty
     */
    IDspNode* getFxNode(int slotIndex);

    /**
     * @brief Set FX bypass state (MESSAGE THREAD)
     * @param slotIndex Slot index [0..4]
     * @param shouldBypass true = bypass, false = active
     */
    void setFxBypassed(int slotIndex, bool shouldBypass);

    /**
     * @brief Check if FX is bypassed
     * @param slotIndex Slot index [0..4]
     * @return true if bypassed or slot is empty
     */
    bool isFxBypassed(int slotIndex) const;

private:
    //==========================================================================
    // Member Variables
    //==========================================================================

    // Track metadata
    juce::String trackName;

    // Track state (atomics for thread-safe access)
    std::atomic<float> gain_{1.0f};     // [0..2], default 1.0
    std::atomic<float> pan_{0.0f};      // [-1..1], L..R, default center
    std::atomic<bool> muted_{false};
    std::atomic<bool> soloed_{false};

    // Audio settings
    double currentSampleRate{44100.0};
    int currentBlockSize{512};

    // Pre-allocated scratch buffer for track processing
    juce::AudioBuffer<float> trackBuffer;

    // W13: Clip list (sorted by startSample, MESSAGE THREAD access only)
    std::vector<Clip> clips_;

    // W11.0: FX chain (fixed 5 slots, MESSAGE THREAD access only)
    struct FxSlot
    {
        std::unique_ptr<IDspNode> node;
    };

    std::array<FxSlot, 5> fxSlots_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Track)
};

} // namespace zenith
