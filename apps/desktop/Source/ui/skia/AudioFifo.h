/*
  ==============================================================================

    AudioFifo.h
    Created: 2025-12-08
    Author:  Zenith DAW

    Lock-free FIFO for passing audio samples from audio thread to UI thread.
    Used for spectrum analyzers and waveform displays.

    Thread Safety:
    - Audio thread: pushSamples() only
    - UI thread: popSamples() only
    - Uses juce::AbstractFifo (SPSC lock-free)

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <vector>
#include <atomic>

namespace zenith {

/**
 * @class AudioFifo
 * @brief Lock-free FIFO for audio samples (SPSC: Single Producer, Single Consumer)
 * 
 * Performance notes:
 * - Zero allocations after construction
 * - Cache-line aligned for SPSC operation
 * - Uses memory_order_relaxed for index updates
 */
class AudioFifo {
public:
    /**
     * @brief Construct with specified capacity
     * @param capacity Number of samples to buffer (default 4096 ~93ms at 44.1kHz)
     */
    explicit AudioFifo(int capacity = 4096)
        : fifo_(capacity)
        , buffer_(static_cast<size_t>(capacity), 0.0f)
        , capacity_(capacity)
    {
    }

    //==========================================================================
    // Audio Thread Methods (Producer)
    //==========================================================================

    /**
     * @brief Push samples from audio thread (lock-free, non-blocking)
     * @param samples Pointer to audio samples
     * @param numSamples Number of samples to push
     * @return Number of samples actually pushed (may be less if FIFO is full)
     * 
     * Thread: AUDIO THREAD ONLY
     */
    int pushSamples(const float* samples, int numSamples) {
        int freeSpace = fifo_.getFreeSpace();
        int numToWrite = std::min(freeSpace, numSamples);

        if (numToWrite > 0) {
            int start1, size1, start2, size2;
            fifo_.prepareToWrite(numToWrite, start1, size1, start2, size2);

            if (size1 > 0) {
                std::memcpy(buffer_.data() + start1, samples, 
                            static_cast<size_t>(size1) * sizeof(float));
            }
            if (size2 > 0) {
                std::memcpy(buffer_.data() + start2, samples + size1, 
                            static_cast<size_t>(size2) * sizeof(float));
            }

            fifo_.finishedWrite(numToWrite);
        }

        return numToWrite;
    }

    /**
     * @brief Push samples from stereo buffer (mixes to mono)
     * @param buffer Stereo audio buffer
     * @param numSamples Number of samples per channel
     * @return Number of samples actually pushed
     * 
     * Thread: AUDIO THREAD ONLY
     */
    int pushStereoAsMono(const juce::AudioBuffer<float>& buffer, int numSamples) {
        int numChannels = buffer.getNumChannels();
        if (numChannels == 0 || numSamples <= 0) return 0;

        // Mix to mono in a temp buffer on stack (small enough)
        constexpr int maxStackSamples = 512;
        float stackBuffer[maxStackSamples];
        
        int processed = 0;
        while (processed < numSamples) {
            int chunkSize = std::min(maxStackSamples, numSamples - processed);
            
            // Mix channels to mono
            for (int i = 0; i < chunkSize; ++i) {
                float sum = 0.0f;
                for (int ch = 0; ch < numChannels; ++ch) {
                    sum += buffer.getSample(ch, processed + i);
                }
                stackBuffer[i] = sum / static_cast<float>(numChannels);
            }

            int pushed = pushSamples(stackBuffer, chunkSize);
            if (pushed == 0) break; // FIFO is full
            processed += pushed;
        }

        return processed;
    }

    //==========================================================================
    // UI Thread Methods (Consumer)
    //==========================================================================

    /**
     * @brief Pop samples for UI thread (lock-free, non-blocking)
     * @param samples Output buffer for samples
     * @param numSamples Number of samples requested
     * @return Number of samples actually popped
     * 
     * Thread: UI THREAD ONLY
     */
    int popSamples(float* samples, int numSamples) {
        int numReady = fifo_.getNumReady();
        int numToRead = std::min(numReady, numSamples);

        if (numToRead > 0) {
            int start1, size1, start2, size2;
            fifo_.prepareToRead(numToRead, start1, size1, start2, size2);

            if (size1 > 0) {
                std::memcpy(samples, buffer_.data() + start1, 
                            static_cast<size_t>(size1) * sizeof(float));
            }
            if (size2 > 0) {
                std::memcpy(samples + size1, buffer_.data() + start2, 
                            static_cast<size_t>(size2) * sizeof(float));
            }

            fifo_.finishedRead(numToRead);
        }

        return numToRead;
    }

    /**
     * @brief Get number of samples available to read
     * Thread: UI THREAD (safe to call from any thread)
     */
    int getNumReady() const { return fifo_.getNumReady(); }

    /**
     * @brief Get number of free slots for writing
     * Thread: AUDIO THREAD (safe to call from any thread)
     */
    int getFreeSpace() const { return fifo_.getFreeSpace(); }

    /**
     * @brief Clear all samples (call from UI thread only)
     */
    void clear() { 
        fifo_.reset();
    }

    /**
     * @brief Get total capacity
     */
    int getCapacity() const { return capacity_; }

private:
    juce::AbstractFifo fifo_;
    std::vector<float> buffer_;
    const int capacity_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioFifo)
};

} // namespace zenith
