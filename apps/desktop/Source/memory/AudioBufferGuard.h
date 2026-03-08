/*
  ==============================================================================

    AudioBufferGuard.h
    Created: 2026-02-19
    Month 9, Gap #6 - Audio Buffer Safety

    JUCE AudioBuffer wrapper with comprehensive safety checks.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "SafeBuffer.h"
#include <stdexcept>
#include <functional>

namespace zenith {

//==============================================================================
/**
 * @brief Audio buffer violation information
 */
struct AudioBufferViolation {
    enum Type {
        None,
        InvalidChannel,       // Channel index out of range
        InvalidSample,        // Sample index out of range
        ChannelCountMismatch, // Channel count mismatch
        SampleCountMismatch,  // Sample count mismatch
        NullBuffer,           // Null buffer access
        SizeOverflow          // Operation would overflow
    };

    Type type = None;
    juce::String description;
    int requestedChannel = -1;
    int requestedSample = -1;
    int numChannels = 0;
    int numSamples = 0;
    juce::Time timestamp;

    juce::String toString() const {
        juce::String typeStr;
        switch (type) {
            case None: typeStr = "None"; break;
            case InvalidChannel: typeStr = "Invalid Channel"; break;
            case InvalidSample: typeStr = "Invalid Sample"; break;
            case ChannelCountMismatch: typeStr = "Channel Count Mismatch"; break;
            case SampleCountMismatch: typeStr = "Sample Count Mismatch"; break;
            case NullBuffer: typeStr = "Null Buffer"; break;
            case SizeOverflow: typeStr = "Size Overflow"; break;
        }
        return "[" + typeStr + "] " + description +
               " (ch: " + juce::String(requestedChannel) +
               ", samp: " + juce::String(requestedSample) + ")";
    }
};

//==============================================================================
/**
 * @brief Guard wrapper for JUCE AudioBuffer with safety checks
 *
 * Features:
 * - Channel and sample bounds checking
 * - Size validation for operations
 * - Safe copy operations
 * - Optional violation callbacks
 * - Compatible with JUCE AudioBuffer interface
 */
template <typename SampleType>
class AudioBufferGuard {
public:
    //==========================================================================
    AudioBufferGuard()
        : buffer_(nullptr)
        , ownsBuffer_(false) {
    }

    //==========================================================================
    /**
     * @brief Create guard for new AudioBuffer
     */
    AudioBufferGuard(int numChannels, int numSamples)
        : ownsBuffer_(true) {

        buffer_ = new juce::AudioBuffer<SampleType>(numChannels, numSamples);
    }

    //==========================================================================
    /**
     * @brief Create guard for existing AudioBuffer (non-owning)
     */
    explicit AudioBufferGuard(juce::AudioBuffer<SampleType>& externalBuffer)
        : buffer_(&externalBuffer)
        , ownsBuffer_(false) {
    }

    //==========================================================================
    /**
     * @brief Create guard for existing AudioBuffer pointer (non-owning)
     */
    explicit AudioBufferGuard(juce::AudioBuffer<SampleType>* externalBuffer)
        : buffer_(externalBuffer)
        , ownsBuffer_(false) {

        if (buffer_ == nullptr) {
            throw std::invalid_argument("Null buffer pointer");
        }
    }

    //==========================================================================
    /**
     * @brief Copy constructor (deep copy)
     */
    AudioBufferGuard(const AudioBufferGuard& other)
        : ownsBuffer_(true) {

        if (other.buffer_ != nullptr) {
            buffer_ = new juce::AudioBuffer<SampleType>(*other.buffer_);
        } else {
            buffer_ = nullptr;
        }
    }

    //==========================================================================
    ~AudioBufferGuard() {
        release();
    }

    //==========================================================================
    /**
     * @brief Copy assignment
     */
    AudioBufferGuard& operator=(const AudioBufferGuard& other) {
        if (this != &other) {
            release();

            if (other.buffer_ != nullptr) {
                buffer_ = new juce::AudioBuffer<SampleType>(*other.buffer_);
                ownsBuffer_ = true;
            }
        }
        return *this;
    }

    //==========================================================================
    /**
     * @brief Get number of channels
     */
    int getNumChannels() const noexcept {
        return buffer_ != nullptr ? buffer_->getNumChannels() : 0;
    }

    //==========================================================================
    /**
     * @brief Get number of samples
     */
    int getNumSamples() const noexcept {
        return buffer_ != nullptr ? buffer_->getNumSamples() : 0;
    }

    //==========================================================================
    /**
     * @brief Get total size in samples
     */
    int getTotalSize() const noexcept {
        return getNumChannels() * getNumSamples();
    }

    //==========================================================================
    /**
     * @brief Check if buffer is valid
     */
    bool isValid() const noexcept {
        return buffer_ != nullptr;
    }

    //==========================================================================
    /**
     * @brief Check if buffer owns its memory
     */
    bool ownsMemory() const noexcept {
        return ownsBuffer_;
    }

    //==========================================================================
    /**
     * @brief Clear all channels to zero
     */
    void clear() {
        if (buffer_ != nullptr) {
            buffer_->clear();
        }
    }

    //==========================================================================
    /**
     * @brief Clear specific range
     */
    void clear(int startSample, int numSamples) {
        validateSampleRange(startSample, numSamples, "clear()");
        if (buffer_ != nullptr) {
            buffer_->clear(startSample, numSamples);
        }
    }

    //==========================================================================
    /**
     * @brief Clear specific channel range
     */
    void clear(int channel, int startSample, int numSamples) {
        validateChannel(channel, "clear()");
        validateSampleRange(startSample, numSamples, "clear()");

        if (buffer_ != nullptr) {
            buffer_->clear(channel, startSample, numSamples);
        }
    }

    //==========================================================================
    /**
     * @brief Safe sample access with bounds checking
     */
    SampleType getSample(int channel, int sample) const {
        validateChannel(channel, "getSample()");
        validateSample(sample, "getSample()");

        return buffer_->getSample(channel, sample);
    }

    //==========================================================================
    /**
     * @brief Safe sample write with bounds checking
     */
    void setSample(int channel, int sample, SampleType value) {
        validateChannel(channel, "setSample()");
        validateSample(sample, "setSample()");

        buffer_->setSample(channel, sample, value);
    }

    //==========================================================================
    /**
     * @brief Add to sample with bounds checking
     */
    void addSample(int channel, int sample, SampleType value) {
        validateChannel(channel, "addSample()");
        validateSample(sample, "addSample()");

        buffer_->addSample(channel, sample, value);
    }

    //==========================================================================
    /**
     * @brief Get read pointer for channel with validation
     */
    const SampleType* getReadPointer(int channel) const {
        validateChannel(channel, "getReadPointer()");
        return buffer_->getReadPointer(channel);
    }

    //==========================================================================
    /**
     * @brief Get read pointer with offset
     */
    const SampleType* getReadPointer(int channel, int sampleOffset) const {
        validateChannel(channel, "getReadPointer()");
        validateSample(sampleOffset, "getReadPointer()");
        return buffer_->getReadPointer(channel, sampleOffset);
    }

    //==========================================================================
    /**
     * @brief Get write pointer for channel with validation
     */
    SampleType* getWritePointer(int channel) {
        validateChannel(channel, "getWritePointer()");
        return buffer_->getWritePointer(channel);
    }

    //==========================================================================
    /**
     * @brief Get write pointer with offset
     */
    SampleType* getWritePointer(int channel, int sampleOffset) {
        validateChannel(channel, "getWritePointer()");
        validateSample(sampleOffset, "getWritePointer()");
        return buffer_->getWritePointer(channel, sampleOffset);
    }

    //==========================================================================
    /**
     * @brief Copy from another buffer with size validation
     */
    void copyFrom(int destChannel, const AudioBufferGuard<SampleType>& source,
                  int sourceChannel, int numSamples) {

        validateChannel(destChannel, "copyFrom()");
        source.validateChannel(sourceChannel, "copyFrom()");

        if (this->getNumSamples() < numSamples || source.getNumSamples() < numSamples) {
            reportViolation(AudioBufferViolation::SampleCountMismatch, "copyFrom()");
            throw std::invalid_argument("Sample count mismatch");
        }

        buffer_->copyFrom(destChannel, 0, *source.buffer_, sourceChannel, 0, numSamples);
    }

    //==========================================================================
    /**
     * @brief Copy from raw pointer with bounds checking
     */
    void copyFrom(int destChannel, const SampleType* source, int numSamples) {
        validateChannel(destChannel, "copyFrom()");

        if (source == nullptr) {
            throw std::invalid_argument("Null source pointer");
        }

        if (getNumSamples() < numSamples) {
            reportViolation(AudioBufferViolation::SampleCountMismatch, "copyFrom()");
            throw std::invalid_argument("Sample count exceeds buffer size");
        }

        buffer_->copyFrom(destChannel, 0, source, numSamples);
    }

    //==========================================================================
    /**
     * @brief Copy to another buffer with size validation
     */
    void copyTo(int sourceChannel, AudioBufferGuard<SampleType>& dest,
                int destChannel, int numSamples) const {

        validateChannel(sourceChannel, "copyTo()");
        dest.validateChannel(destChannel, "copyTo()");

        if (this->getNumSamples() < numSamples || dest.getNumSamples() < numSamples) {
            reportViolation(AudioBufferViolation::SampleCountMismatch, "copyTo()");
            throw std::invalid_argument("Sample count mismatch");
        }

        dest.buffer_->copyFrom(destChannel, 0, *buffer_, sourceChannel, 0, numSamples);
    }

    //==========================================================================
    /**
     * @brief Resize buffer (only works if we own it)
     */
    void resize(int newNumChannels, int newNumSamples) {
        if (!ownsBuffer_) {
            throw std::runtime_error("Cannot resize non-owned buffer");
        }

        if (buffer_ == nullptr) {
            buffer_ = new juce::AudioBuffer<SampleType>(newNumChannels, newNumSamples);
        } else {
            buffer_->setSize(newNumChannels, newNumSamples, true);
        }
    }

    //==========================================================================
    /**
     * @brief Apply gain to range
     */
    void applyGain(int channel, int startSample, int numSamples, SampleType gain) {
        validateChannel(channel, "applyGain()");
        validateSampleRange(startSample, numSamples, "applyGain()");

        buffer_->applyGain(channel, startSample, numSamples, gain);
    }

    //==========================================================================
    /**
     * @brief Apply gain to all channels
     */
    void applyGain(int startSample, int numSamples, SampleType gain) {
        validateSampleRange(startSample, numSamples, "applyGain()");
        buffer_->applyGain(startSample, numSamples, gain);
    }

    //==========================================================================
    /**
     * @brief Get underlying buffer (use with caution)
     */
    juce::AudioBuffer<SampleType>* getBuffer() noexcept { return buffer_; }

    //==========================================================================
    /**
     * @brief Get underlying buffer (const)
     */
    const juce::AudioBuffer<SampleType>* getBuffer() const noexcept { return buffer_; }

    //==========================================================================
    /**
     * @brief Set violation callback
     */
    using ViolationCallback = std::function<void(const AudioBufferViolation&)>;
    void setViolationCallback(ViolationCallback callback) {
        violationCallback_ = std::move(callback);
    }

private:
    //==========================================================================
    juce::AudioBuffer<SampleType>* buffer_;
    bool ownsBuffer_;
    ViolationCallback violationCallback_;

    //==========================================================================
    void release() {
        if (ownsBuffer_ && buffer_ != nullptr) {
            delete buffer_;
        }
        buffer_ = nullptr;
        ownsBuffer_ = false;
    }

    //==========================================================================
    void validateChannel(int channel, const char* operation) const {
        if (buffer_ == nullptr) {
            AudioBufferViolation violation;
            violation.type = AudioBufferViolation::NullBuffer;
            violation.description = operation;
            violation.timestamp = juce::Time::getCurrentTime();

            if (violationCallback_) {
                violationCallback_(violation);
            }

            throw std::runtime_error("Null buffer access");
        }

        if (channel < 0 || channel >= buffer_->getNumChannels()) {
            AudioBufferViolation violation;
            violation.type = AudioBufferViolation::InvalidChannel;
            violation.description = operation;
            violation.requestedChannel = channel;
            violation.numChannels = buffer_->getNumChannels();
            violation.timestamp = juce::Time::getCurrentTime();

            if (violationCallback_) {
                violationCallback_(violation);
            }

            throw std::out_of_range("Channel index out of range");
        }
    }

    //==========================================================================
    void validateSample(int sample, const char* operation) const {
        if (buffer_ == nullptr) {
            AudioBufferViolation violation;
            violation.type = AudioBufferViolation::NullBuffer;
            violation.description = operation;
            violation.timestamp = juce::Time::getCurrentTime();

            if (violationCallback_) {
                violationCallback_(violation);
            }

            throw std::runtime_error("Null buffer access");
        }

        if (sample < 0 || sample >= buffer_->getNumSamples()) {
            AudioBufferViolation violation;
            violation.type = AudioBufferViolation::InvalidSample;
            violation.description = operation;
            violation.requestedSample = sample;
            violation.numSamples = buffer_->getNumSamples();
            violation.timestamp = juce::Time::getCurrentTime();

            if (violationCallback_) {
                violationCallback_(violation);
            }

            throw std::out_of_range("Sample index out of range");
        }
    }

    //==========================================================================
    void validateSampleRange(int startSample, int numSamples, const char* operation) const {
        if (buffer_ == nullptr) {
            AudioBufferViolation violation;
            violation.type = AudioBufferViolation::NullBuffer;
            violation.description = operation;
            violation.timestamp = juce::Time::getCurrentTime();

            if (violationCallback_) {
                violationCallback_(violation);
            }

            throw std::runtime_error("Null buffer access");
        }

        if (startSample < 0 || numSamples < 0 ||
            startSample + numSamples > buffer_->getNumSamples()) {
            AudioBufferViolation violation;
            violation.type = AudioBufferViolation::InvalidSample;
            violation.description = operation;
            violation.requestedSample = startSample + numSamples - 1;
            violation.numSamples = buffer_->getNumSamples();
            violation.timestamp = juce::Time::getCurrentTime();

            if (violationCallback_) {
                violationCallback_(violation);
            }

            throw std::out_of_range("Sample range out of bounds");
        }
    }

    //==========================================================================
    void reportViolation(AudioBufferViolation::Type type, const char* operation) const {
        AudioBufferViolation violation;
        violation.type = type;
        violation.description = operation;
        violation.numChannels = getNumChannels();
        violation.numSamples = getNumSamples();
        violation.timestamp = juce::Time::getCurrentTime();

        if (violationCallback_) {
            violationCallback_(violation);
        }
    }
};

//==============================================================================
// Type aliases for common audio buffer types
using AudioBufferGuardFloat = AudioBufferGuard<float>;
using AudioBufferGuardDouble = AudioBufferGuard<double>;

} // namespace zenith
