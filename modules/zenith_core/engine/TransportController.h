/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <juce_core/juce_core.h>
#include <atomic>
#include <functional>

#include "EngineConstants.h"
#include "RTSafetyChecks.h"

namespace zenith {

// Forward declarations
class TempoMap;

//==============================================================================
/**
    Transport state change callback.
*/
using TransportCallback = std::function<void()>;

//==============================================================================
/**
    Transport controller for the engine.
    
    Manages playback state, position, looping, and tempo synchronization.
*/
class TransportController {
public:
    //==========================================================================
    TransportController() = default;
    ~TransportController() = default;

    //==========================================================================
    // Configuration
    //==========================================================================

    /**
     * @brief Set the tempo map for beat/time conversions
     */
    void setTempoMap(const TempoMap* tempoMap) { tempoMap_ = tempoMap; }

    /**
     * @brief Set sample rate for time conversions
     * @note MESSAGE THREAD ONLY — typically called from prepareToPlay
     */
    ZENITH_NONRT_THREAD
    void setSampleRate(double sampleRate) { 
        sampleRate_.store(sampleRate); 
    }

    //==========================================================================
    // Transport Control
    //==========================================================================

    /**
     * @brief Start playback
     * @note MESSAGE THREAD ONLY — triggers onPlay_ callback
     */
    ZENITH_NONRT_THREAD
    void play() {
        isPlaying_.store(true);
        if (onPlay_) onPlay_();
    }

    /**
     * @brief Stop playback
     * @note MESSAGE THREAD ONLY — triggers onStop_ callback
     */
    ZENITH_NONRT_THREAD
    void stop() {
        isPlaying_.store(false);
        if (onStop_) onStop_();
    }

    /**
     * @brief Toggle playback state
     * @note MESSAGE THREAD ONLY
     */
    ZENITH_NONRT_THREAD
    void togglePlayback() {
        if (isPlaying_.load()) {
            stop();
        } else {
            play();
        }
    }

    /**
     * @brief Rewind to start
     * @note MESSAGE THREAD ONLY — triggers onSeek_ callback
     */
    ZENITH_NONRT_THREAD
    void rewind() {
        playheadSamples_.store(0);
        if (onSeek_) onSeek_();
    }

    /**
     * @brief Check if playing
     * @note Thread-safe via atomic load (callable from any thread, including RT)
     */
    ZENITH_RT_SAFE
    bool isPlaying() const { return isPlaying_.load(); }

    //==========================================================================
    // Position
    //==========================================================================

    /**
     * @brief Set playhead position in samples
     * @note MESSAGE THREAD ONLY — triggers onSeek_ callback
     */
    ZENITH_NONRT_THREAD
    void setPlayheadSamples(juce::int64 position) {
        playheadSamples_.store(position);
        if (onSeek_) onSeek_();
    }

    /**
     * @brief Get playhead position in samples
     * @note Thread-safe via atomic load (callable from any thread, including RT)
     */
    ZENITH_RT_SAFE
    juce::int64 getPlayheadSamples() const { 
        return playheadSamples_.load(); 
    }

    /**
     * @brief Advance playhead by specified samples
     * @note AUDIO THREAD ONLY — RT-safe, no callbacks
     */
    ZENITH_RT_THREAD
    void advancePlayhead(int numSamples) {
        if (isLooping_.load()) {
            juce::int64 newPos = playheadSamples_.load() + numSamples;
            juce::int64 loopEnd = loopEndSamples_.load();
            juce::int64 loopStart = loopStartSamples_.load();

            if (loopEnd > loopStart && newPos >= loopEnd) {
                juce::int64 loopLength = loopEnd - loopStart;
                while (newPos >= loopEnd) {
                    newPos -= loopLength;
                }
                if (newPos < loopStart) {
                    newPos = loopStart;
                }
            }
            playheadSamples_.store(newPos);
        } else {
            playheadSamples_.fetch_add(numSamples);
        }
    }

    /**
     * @brief Get playhead position in beats
     * @note Thread-safe (reads atomics only)
     */
    ZENITH_RT_SAFE
    double getPlayheadBeats() const;

    /**
     * @brief Get playhead position in seconds
     * @note Thread-safe via atomic loads (callable from any thread, including RT)
     */
    ZENITH_RT_SAFE
    double getPlayheadSeconds() const {
        double sr = sampleRate_.load();
        return sr > 0.0 ? static_cast<double>(playheadSamples_.load()) / sr : 0.0;
    }

    //==========================================================================
    // Looping
    //==========================================================================

    /**
     * @brief Enable/disable looping
     * @note Thread-safe via atomic store (typically called from message thread)
     */
    void setLooping(bool shouldLoop) {
        isLooping_.store(shouldLoop);
    }

    /**
     * @brief Check if looping is enabled
     * @note Thread-safe via atomic load
     */
    ZENITH_RT_SAFE
    bool isLooping() const { return isLooping_.load(); }

    /**
     * @brief Set loop region in samples
     * @note Thread-safe via atomic stores (typically called from message thread)
     */
    void setLoopRegionSamples(juce::int64 start, juce::int64 end) {
        loopStartSamples_.store(start);
        loopEndSamples_.store(end);
    }

    /**
     * @brief Get loop start in samples
     * @note Thread-safe via atomic load
     */
    ZENITH_RT_SAFE
    juce::int64 getLoopStartSamples() const { return loopStartSamples_.load(); }

    /**
     * @brief Get loop end in samples
     * @note Thread-safe via atomic load
     */
    ZENITH_RT_SAFE
    juce::int64 getLoopEndSamples() const { return loopEndSamples_.load(); }

    /**
     * @brief Check if loop wrap occurs within buffer
     * @return Sample offset of wrap (-1 if no wrap)
     */
    int getLoopWrapOffset() const { return loopWrapOffset_.load(); }

    /**
     * @brief Set loop wrap offset for current buffer
     * @note AUDIO THREAD only
     */
    void setLoopWrapOffset(int offset) { loopWrapOffset_.store(offset); }

    //==========================================================================
    // Tempo
    //==========================================================================

    /**
     * @brief Set tempo in BPM
     */
    void setTempo(double bpm) {
        tempo_.store(juce::jlimit(1.0, 999.0, bpm));
    }

    /**
     * @brief Get tempo in BPM
     */
    double getTempo() const { return tempo_.load(); }

    /**
     * @brief Set time signature
     */
    void setTimeSignature(int numerator, int denominator) {
        timeSigNumerator_.store(numerator);
        timeSigDenominator_.store(denominator);
    }

    /**
     * @brief Get time signature numerator
     */
    int getTimeSigNumerator() const { return timeSigNumerator_.load(); }

    /**
     * @brief Get time signature denominator
     */
    int getTimeSigDenominator() const { return timeSigDenominator_.load(); }

    //==========================================================================
    // Callbacks
    //==========================================================================

    void setOnPlay(TransportCallback cb) { onPlay_ = cb; }
    void setOnStop(TransportCallback cb) { onStop_ = cb; }
    void setOnSeek(TransportCallback cb) { onSeek_ = cb; }

    //==========================================================================
    // Metronome
    //==========================================================================

    void setMetronomeEnabled(bool enabled) { metronomeEnabled_.store(enabled); }
    bool isMetronomeEnabled() const { return metronomeEnabled_.load(); }
    void setMetronomeLevel(float level) { metronomeLevel_.store(level); }
    float getMetronomeLevel() const { return metronomeLevel_.load(); }

private:
    //==========================================================================
    // State
    //==========================================================================
    
    // Metronome state
    std::atomic<bool> metronomeEnabled_{false};
    std::atomic<float> metronomeLevel_{0.5f};

    // Tempo map (owned by Engine)
    const TempoMap* tempoMap_ = nullptr;

    // Sample rate
    std::atomic<double> sampleRate_{constants::kDefaultSampleRate};

    // Playback state
    std::atomic<bool> isPlaying_{false};
    std::atomic<juce::int64> playheadSamples_{0};

    // Loop state
    std::atomic<bool> isLooping_{false};
    std::atomic<juce::int64> loopStartSamples_{0};
    std::atomic<juce::int64> loopEndSamples_{0};
    std::atomic<int> loopWrapOffset_{-1};

    // Tempo state
    std::atomic<double> tempo_{120.0};
    std::atomic<int> timeSigNumerator_{4};
    std::atomic<int> timeSigDenominator_{4};

    // Callbacks
    TransportCallback onPlay_;
    TransportCallback onStop_;
    TransportCallback onSeek_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportController)
};

} // namespace zenith
