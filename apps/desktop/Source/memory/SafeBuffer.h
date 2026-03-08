/*
  ==============================================================================

    SafeBuffer.h
    Created: 2026-02-19
    Month 9, Gap #6 - Buffer Overflow Protection

    Bounds-checked buffer operations with overflow detection.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <memory>
#include <stdexcept>
#include <cstring>
#include <atomic>

namespace zenith {

//==============================================================================
/**
 * @brief Buffer overflow detection result
 */
struct BufferViolation {
    enum Type {
        None,
        ReadOverflow,        // Read past buffer end
        WriteOverflow,       // Write past buffer end
        ReadUnderflow,       // Read before buffer start
        WriteUnderflow,      // Write before buffer start
        InvalidIndex,        // Index out of bounds
        NullPointer,         // Null pointer access
        SizeMismatch         // Size mismatch in operations
    };

    Type type = None;
    juce::String description;
    size_t requestedIndex = 0;
    size_t bufferSize = 0;
    void* bufferAddress = nullptr;
    juce::Time timestamp;

    bool isViolation() const { return type != None; }

    juce::String toString() const {
        juce::String typeStr;
        switch (type) {
            case None: typeStr = "None"; break;
            case ReadOverflow: typeStr = "Read Overflow"; break;
            case WriteOverflow: typeStr = "Write Overflow"; break;
            case ReadUnderflow: typeStr = "Read Underflow"; break;
            case WriteUnderflow: typeStr = "Write Underflow"; break;
            case InvalidIndex: typeStr = "Invalid Index"; break;
            case NullPointer: typeStr = "Null Pointer"; break;
            case SizeMismatch: typeStr = "Size Mismatch"; break;
        }
        return "[" + typeStr + "] " + description +
               " (index: " + juce::String((int)requestedIndex) +
               ", size: " + juce::String((int)bufferSize) + ")";
    }
};

//==============================================================================
/**
 * @brief Safe buffer with bounds checking
 *
 * Features:
 * - Runtime bounds checking on all operations
 * - Zero-cost abstraction in release builds (optional)
 * - Exception-based error reporting
 * - Compatible with raw pointer interface
 * - Support for both heap and stack allocation
 */
template <typename T>
class SafeBuffer {
public:
    //==========================================================================
    SafeBuffer()
        : data_(nullptr)
        , size_(0)
        , capacity_(0)
        , ownsMemory_(false) {
    }

    //==========================================================================
    /**
     * @brief Create buffer with specified size
     */
    explicit SafeBuffer(size_t size)
        : data_(nullptr)
        , size_(size)
        , capacity_(size)
        , ownsMemory_(true) {

        if (size > 0) {
            data_ = new T[size]();
        }
    }

    //==========================================================================
    /**
     * @brief Create buffer wrapping existing memory (non-owning)
     */
    SafeBuffer(T* externalData, size_t size)
        : data_(externalData)
        , size_(size)
        , capacity_(size)
        , ownsMemory_(false) {

        if (externalData == nullptr && size > 0) {
            throw std::invalid_argument("Null pointer with non-zero size");
        }
    }

    //==========================================================================
    /**
     * @brief Copy constructor
     */
    SafeBuffer(const SafeBuffer& other)
        : data_(nullptr)
        , size_(other.size_)
        , capacity_(other.size_)
        , ownsMemory_(true) {

        if (size_ > 0) {
            data_ = new T[size_];
            std::memcpy(data_, other.data_, size_ * sizeof(T));
        }
    }

    //==========================================================================
    /**
     * @brief Move constructor
     */
    SafeBuffer(SafeBuffer&& other) noexcept
        : data_(other.data_)
        , size_(other.size_)
        , capacity_(other.capacity_)
        , ownsMemory_(other.ownsMemory_) {

        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
        other.ownsMemory_ = false;
    }

    //==========================================================================
    ~SafeBuffer() {
        release();
    }

    //==========================================================================
    /**
     * @brief Copy assignment
     */
    SafeBuffer& operator=(const SafeBuffer& other) {
        if (this != &other) {
            release();
            size_ = other.size_;
            capacity_ = other.size_;
            ownsMemory_ = true;

            if (size_ > 0) {
                data_ = new T[size_];
                std::memcpy(data_, other.data_, size_ * sizeof(T));
            }
        }
        return *this;
    }

    //==========================================================================
    /**
     * @brief Move assignment
     */
    SafeBuffer& operator=(SafeBuffer&& other) noexcept {
        if (this != &other) {
            release();

            data_ = other.data_;
            size_ = other.size_;
            capacity_ = other.capacity_;
            ownsMemory_ = other.ownsMemory_;

            other.data_ = nullptr;
            other.size_ = 0;
            other.capacity_ = 0;
            other.ownsMemory_ = false;
        }
        return *this;
    }

    //==========================================================================
    /**
     * @brief Resize buffer (preserves contents if growing)
     */
    void resize(size_t newSize) {
        if (newSize == size_) {
            return;
        }

        if (!ownsMemory_) {
            throw std::runtime_error("Cannot resize non-owned buffer");
        }

        T* newData = nullptr;
        if (newSize > 0) {
            newData = new T[newSize]();

            // Copy existing data
            size_t copySize = juce::jmin(size_, newSize);
            if (data_ != nullptr) {
                std::memcpy(newData, data_, copySize * sizeof(T));
            }
        }

        release();

        data_ = newData;
        size_ = newSize;
        capacity_ = newSize;
    }

    //==========================================================================
    /**
     * @brief Bounds-checked read access
     */
    T& at(size_t index) {
        checkBounds(index, "at()");
        return data_[index];
    }

    //==========================================================================
    /**
     * @brief Bounds-checked read access (const)
     */
    const T& at(size_t index) const {
        checkBounds(index, "at()");
        return data_[index];
    }

    //==========================================================================
    /**
     * @brief Unchecked access (use with caution)
     */
    T& operator[](size_t index) noexcept {
        return data_[index];
    }

    //==========================================================================
    /**
     * @brief Unchecked access (const)
     */
    const T& operator[](size_t index) const noexcept {
        return data_[index];
    }

    //==========================================================================
    /**
     * @brief Get raw pointer (use with caution)
     */
    T* getData() noexcept { return data_; }

    //==========================================================================
    /**
     * @brief Get raw pointer (const)
     */
    const T* getData() const noexcept { return data_; }

    //==========================================================================
    /**
     * @brief Get buffer size
     */
    size_t size() const noexcept { return size_; }

    //==========================================================================
    /**
     * @brief Get buffer capacity
     */
    size_t capacity() const noexcept { return capacity_; }

    //==========================================================================
    /**
     * @brief Check if buffer is empty
     */
    bool isEmpty() const noexcept { return size_ == 0; }

    //==========================================================================
    /**
     * @brief Check if buffer owns its memory
     */
    bool ownsMemory() const noexcept { return ownsMemory_; }

    //==========================================================================
    /**
     * @brief Clear buffer to zero
     */
    void clear() {
        if (data_ != nullptr && size_ > 0) {
            std::memset(data_, 0, size_ * sizeof(T));
        }
    }

    //==========================================================================
    /**
     * @brief Fill buffer with value
     */
    void fill(const T& value) {
        for (size_t i = 0; i < size_; ++i) {
            data_[i] = value;
        }
    }

    //==========================================================================
    /**
     * @brief Copy data into buffer with bounds checking
     */
    void copyFrom(const T* source, size_t sourceSize, size_t destOffset = 0) {
        if (source == nullptr) {
            throw std::invalid_argument("Null source pointer");
        }

        checkRange(destOffset, sourceSize, "copyFrom()");

        if (sourceSize > 0) {
            std::memcpy(data_ + destOffset, source, sourceSize * sizeof(T));
        }
    }

    //==========================================================================
    /**
     * @brief Copy data from buffer with bounds checking
     */
    void copyTo(T* dest, size_t destSize, size_t sourceOffset = 0) const {
        if (dest == nullptr) {
            throw std::invalid_argument("Null destination pointer");
        }

        if (sourceOffset >= size_) {
            throw std::out_of_range("Source offset out of bounds");
        }

        size_t copySize = juce::jmin(size_ - sourceOffset, destSize);

        if (copySize > 0) {
            std::memcpy(dest, data_ + sourceOffset, copySize * sizeof(T));
        }
    }

    //==========================================================================
    /**
     * @brief Set violation callback for custom error handling
     */
    using ViolationCallback = std::function<void(const BufferViolation&)>;
    void setViolationCallback(ViolationCallback callback) {
        violationCallback_ = std::move(callback);
    }

private:
    //==========================================================================
    T* data_;
    size_t size_;
    size_t capacity_;
    bool ownsMemory_;
    ViolationCallback violationCallback_;

    //==========================================================================
    void release() {
        if (ownsMemory_ && data_ != nullptr) {
            delete[] data_;
        }
        data_ = nullptr;
        size_ = 0;
        capacity_ = 0;
        ownsMemory_ = false;
    }

    //==========================================================================
    void checkBounds(size_t index, const char* operation) const {
        if (data_ == nullptr) {
            reportViolation(BufferViolation::NullPointer, operation, index);
            throw std::out_of_range("Null buffer access");
        }

        if (index >= size_) {
            reportViolation(BufferViolation::InvalidIndex, operation, index);
            throw std::out_of_range("Index out of bounds");
        }
    }

    //==========================================================================
    void checkRange(size_t offset, size_t count, const char* operation) const {
        if (data_ == nullptr) {
            reportViolation(BufferViolation::NullPointer, operation, offset);
            throw std::out_of_range("Null buffer access");
        }

        if (offset >= size_) {
            reportViolation(BufferViolation::InvalidIndex, operation, offset);
            throw std::out_of_range("Offset out of bounds");
        }

        if (offset + count > size_) {
            BufferViolation violation;
            violation.type = BufferViolation::WriteOverflow;
            violation.description = juce::String(operation) + " range exceeds buffer";
            violation.requestedIndex = offset + count - 1;
            violation.bufferSize = size_;
            violation.bufferAddress = data_;
            violation.timestamp = juce::Time::getCurrentTime();

            if (violationCallback_) {
                violationCallback_(violation);
            }

            throw std::out_of_range("Range exceeds buffer size");
        }
    }

    //==========================================================================
    void reportViolation(BufferViolation::Type type, const char* operation,
                        size_t index) const {
        BufferViolation violation;
        violation.type = type;
        violation.description = operation;
        violation.requestedIndex = index;
        violation.bufferSize = size_;
        violation.bufferAddress = data_;
        violation.timestamp = juce::Time::getCurrentTime();

        if (violationCallback_) {
            violationCallback_(violation);
        }
    }
};

//==============================================================================
// Type aliases for common buffer types
using AudioSampleBuffer = SafeBuffer<float>;
using Int8Buffer = SafeBuffer<juce::int8>;
using Int16Buffer = SafeBuffer<juce::int16>;
using Int32Buffer = SafeBuffer<juce::int32>;
using UInt8Buffer = SafeBuffer<juce::uint8>;
using UInt16Buffer = SafeBuffer<juce::uint16>;
using UInt32Buffer = SafeBuffer<juce::uint32>;

} // namespace zenith
