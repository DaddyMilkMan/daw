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

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <atomic>
#include <vector>
#include <memory>

namespace zenith::ui {

//==============================================================================
/**
 * @brief Audio buffer container for real-time analysis
 *
 * This class provides a circular buffer for storing audio data
 * that can be analyzed by the AudioReactiveSystem. It uses
 * lock-free techniques for thread-safe access between the
 * audio thread and UI thread.
 */
class AudioBufferSource {
public:
    //==========================================================================
    // Construction
    //==========================================================================

    AudioBufferSource();
    explicit AudioBufferSource(int numChannels);
    ~AudioBufferSource();

    //==========================================================================
    // Buffer Management
    //==========================================================================

    /**
     * @brief Prepare the buffer for audio processing
     * @param samplesPerBlock Expected block size
     * @param sampleRate Sample rate in Hz
     */
    void prepare(int samplesPerBlock, double sampleRate);

    /**
     * @brief Reset the buffer state
     */
    void reset();

    /**
     * @brief Resize the internal buffer
     * @param numChannels Number of channels
     * @param numSamples Buffer size in samples
     */
    void resize(int numChannels, int numSamples);

    //==========================================================================
    // Audio Input
    //==========================================================================

    /**
     * @brief Write audio data to the circular buffer
     * @param audio Input audio data (interleaved or planar)
     * @param numChannels Number of channels
     * @param numSamples Number of samples per channel
     */
    void writeAudio(const float* const* audio, int numChannels, int numSamples);

    /**
     * @brief Write interleaved audio data to the buffer
     * @param interleavedData Interleaved audio data
     * @param numChannels Number of channels
     * @param numSamples Number of samples per channel
     */
    void writeInterleavedAudio(const float* interleavedData, int numChannels, int numSamples);

    //==========================================================================
    // Audio Output (for analysis)
    //==========================================================================

    /**
     * @brief Read audio data from the buffer
     * @param destBuffer Destination buffer
     * @param numSamples Number of samples to read
     * @param channel Channel to read from
     */
    void readAudio(float* destBuffer, int numSamples, int channel = 0) const;

    /**
     * @brief Get the most recent sample from a channel
     * @param channel Channel index
     * @return Most recent sample value
     */
    float getLatestSample(int channel = 0) const;

    /**
     * @brief Get a read-only view of the internal buffer
     * @return Const pointer to the internal JUCE buffer
     */
    const juce::AudioBuffer<float>& getBuffer() const { return buffer_; }

    //==========================================================================
    // Buffer State
    //==========================================================================

    /**
     * @brief Get the current write position in the buffer
     */
    int getWritePosition() const { return writePosition_.load(std::memory_order_relaxed); }

    /**
     * @brief Get the buffer size in samples
     */
    int getBufferSize() const { return bufferSize_; }

    /**
     * @brief Get the number of channels
     */
    int getNumChannels() const { return numChannels_; }

    /**
     * @brief Get the sample rate
     */
    double getSampleRate() const { return sampleRate_; }

    /**
     * @brief Check if the buffer is ready for use
     */
    bool isPrepared() const { return prepared_; }

private:
    //==========================================================================
    // Member Variables
    //==========================================================================

    juce::AudioBuffer<float> buffer_;
    std::atomic<int> writePosition_{ 0 };
    int bufferSize_ = 4096;
    int numChannels_ = 2;
    double sampleRate_ = 48000.0;
    bool prepared_ = false;

    //==========================================================================
    // Helper Methods
    //==========================================================================

    void incrementWritePosition(int numSamples);
    int wrapPosition(int position) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioBufferSource)
};

//==============================================================================
/**
 * @brief Lock-free audio queue for inter-thread communication
 *
 * Provides a single-producer single-consumer queue for passing
 * audio buffers between the audio thread and UI thread.
 */
class AudioBufferQueue {
public:
    //==========================================================================
    // Construction
    //==========================================================================

    AudioBufferQueue();
    explicit AudioBufferQueue(int numChannels, int numSamples);
    ~AudioBufferQueue();

    //==========================================================================
    // Queue Operations
    //==========================================================================

    /**
     * @brief Push a buffer onto the queue
     * @param audio Audio data
     * @param numChannels Number of channels
     * @param numSamples Number of samples
     * @return True if successful (false if queue full)
     */
    bool push(const float* const* audio, int numChannels, int numSamples);

    /**
     * @brief Pop a buffer from the queue
     * @param dest Destination buffer
     * @param numChannels Number of channels
     * @param numSamples Number of samples
     * @return True if data was available
     */
    bool pop(float* const* dest, int numChannels, int numSamples);

    /**
     * @brief Check if the queue has data available
     */
    bool hasData() const { return available_.load(std::memory_order_acquire) > 0; }

    /**
     * @brief Get the number of available buffers
     */
    int getAvailableCount() const { return available_.load(std::memory_order_acquire); }

private:
    //==========================================================================
    // Member Variables
    //==========================================================================

    static constexpr int kQueueSize = 16;
    std::vector<std::unique_ptr<juce::AudioBuffer<float>>> buffers_;
    std::atomic<int> readIndex_{ 0 };
    std::atomic<int> writeIndex_{ 0 };
    std::atomic<int> available_{ 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioBufferQueue)
};

} // namespace zenith::ui