/*
  ==============================================================================

    SysExTransferSafetyManager.h
    Created: 2026-02-18
    Author:  Zenith DAW - Month 6: MIDI Safety (Gap #7)

    Ensures safe SysEx (System Exclusive) transfers.

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <vector>
#include <map>

namespace zenith {

//==============================================================================
/**
 * @brief SysEx transfer state
 */
enum class SysExTransferState {
    Idle,                        // Not transferring
    Receiving,                   // Receiving SysEx
    Sending,                     // Sending SysEx
    Complete,                    // Transfer complete
    TimedOut,                    // Transfer timeout
    Failed                       // Transfer failed
};

//==============================================================================
/**
 * @brief SysEx transfer issue
 */
struct SysExTransferIssue {
    enum Type {
        ChecksumMismatch,         // SysEx checksum invalid
        IncompleteTransfer,        // Transfer incomplete
        Timeout,                   // Transfer timed out
        BufferOverflow,            // Buffer too small
        InvalidManufacturer,       // Unknown manufacturer ID
        InvalidFormat,             // Invalid SysEx format
        TransferInterrupted        // Transfer was interrupted
    };

    Type type;
    juce::String description;
    int bytesTransferred = 0;
    int expectedBytes = 0;
    double severity = 0.0;         // 0-10

    juce::String toString() const {
        juce::String typeStr;
        switch (type) {
            case ChecksumMismatch: typeStr = "Checksum Mismatch"; break;
            case IncompleteTransfer: typeStr = "Incomplete"; break;
            case Timeout: typeStr = "Timeout"; break;
            case BufferOverflow: typeStr = "Buffer Overflow"; break;
            case InvalidManufacturer: typeStr = "Invalid Manufacturer"; break;
            case InvalidFormat: typeStr = "Invalid Format"; break;
            case TransferInterrupted: typeStr = "Interrupted"; break;
        }
        return "[" + typeStr + "] " + description +
               " (" + juce::String(bytesTransferred) + "/" +
               juce::String(expectedBytes) + " bytes)";
    }
};

//==============================================================================
/**
 * @brief SysEx transfer information
 */
struct SysExTransfer {
    juce::String id;               // Transfer ID
    SysExTransferState state = SysExTransferState::Idle;
    std::vector<juce::uint8> data;       // SysEx data
    juce::String manufacturerId;   // 3-byte manufacturer ID
    int expectedLength = 0;        // Expected total length
    int receivedLength = 0;        // Bytes received so far
    juce::Time startTime;          // Transfer start time

    juce::String toString() const {
        return "SysEx[" + id + "]: " +
               juce::String(receivedLength) + "/" +
               juce::String(expectedLength) + " bytes";
    }
};

//==============================================================================
/**
 * @brief SysEx transfer statistics
 */
struct SysExTransferStatistics {
    int totalTransfers = 0;
    int successfulTransfers = 0;
    int failedTransfers = 0;
    int timedOutTransfers = 0;
    double averageTransferTime = 0.0;
    int totalBytesTransferred = 0;

    juce::String toString() const {
        return "SysEx Transfers: " +
               juce::String(successfulTransfers) + "/" +
               juce::String(totalTransfers) + " successful";
    }
};

//==============================================================================
/**
 * @brief Manages SysEx transfer safety
 *
 * Features:
 * - SysEx checksum verification
 * - Transfer timeout detection
 * - Incomplete transfer handling
 * - Buffer size validation
 * - Manufacturer ID filtering
 * - Multi-packet transfer reassembly
 */
class SysExTransferSafetyManager {
public:
    //==========================================================================
    SysExTransferSafetyManager();
    ~SysExTransferSafetyManager();

    //==========================================================================
    /**
     * @brief Start receiving SysEx
     * @param transferId Transfer identifier
     * @param manufacturerId Manufacturer ID (if known)
     * @param expectedLength Expected length in bytes (0 if unknown)
     * @return true if started successfully
     */
    bool startReceiving(const juce::String& transferId,
                       const juce::String& manufacturerId = "",
                       int expectedLength = 0);

    //==========================================================================
    /**
     * @brief Add SysEx data during transfer
     * @param transferId Transfer identifier
     * @param data Data bytes
     * @param length Data length
     * @return true if data added successfully
     */
    bool addSysExData(const juce::String& transferId,
                      const juce::uint8* data,
                      int length);

    //==========================================================================
    /**
     * @brief Complete SysEx transfer
     * @param transferId Transfer identifier
     * @return List of issues found (empty if successful)
     */
    std::vector<SysExTransferIssue> completeTransfer(const juce::String& transferId);

    //==========================================================================
    /**
     * @brief Check for timed out transfers
     * @param timeoutSeconds Timeout threshold
     * @return List of timed out transfers
     */
    std::vector<juce::String> checkForTimeouts(double timeoutSeconds = 5.0) const;

    //==========================================================================
    /**
     * @brief Verify SysEx checksum
     * @param data SysEx data
     * @param length Data length
     * @param checksumLocation Index of checksum byte
     * @return true if checksum valid
     */
    static bool verifyChecksum(const juce::uint8* data,
                               int length,
                               int checksumLocation = -1);

    //==========================================================================
    /**
     * @brief Validate SysEx format
     * @param data SysEx data
     * @param length Data length
     * @return List of validation issues
     */
    std::vector<SysExTransferIssue> validateSysEx(const juce::uint8* data,
                                                   int length) const;

    //==========================================================================
    /**
     * @brief Get transfer by ID
     * @param transferId Transfer identifier
     * @return Transfer (invalid if not found)
     */
    SysExTransfer getTransfer(const juce::String& transferId) const;

    //==========================================================================
    /**
     * @brief Reassemble multi-packet SysEx
     * @param packets Vector of SysEx packets
     * @return Reassembled SysEx data
     */
    static std::vector<juce::uint8> reassembleMultiPacket(
        const std::vector<std::vector<juce::uint8>>& packets);

    //==========================================================================
    /**
     * @brief Set transfer timeout
     * @param timeoutSeconds Timeout in seconds
     */
    void setTransferTimeout(double timeoutSeconds) {
        transferTimeout_ = juce::jmax(1.0, timeoutSeconds);
    }

    //==========================================================================
    /**
     * @brief Add manufacturer ID filter
     * @param manufacturerId 3-byte manufacturer ID
     * @param allowed true to allow, false to block
     */
    void setManufacturerAllowed(const juce::String& manufacturerId,
                                bool allowed) {
        allowedManufacturers_[manufacturerId] = allowed;
    }

    //==========================================================================
    /**
     * @brief Get transfer statistics
     * @return Current statistics
     */
    SysExTransferStatistics getStatistics() const {
        return statistics_;
    }

    /**
     * @brief Reset statistics
     */
    void resetStatistics() {
        statistics_ = SysExTransferStatistics{};
    }

private:
    //==========================================================================
    bool isManufacturerAllowed(const juce::String& manufacturerId) const;
    std::vector<SysExTransferIssue> validateTransfer(const SysExTransfer& transfer) const;

    //==========================================================================
    // Active transfers
    std::map<juce::String, SysExTransfer> activeTransfers_;

    // Settings
    double transferTimeout_ = 5.0;
    std::map<juce::String, bool> allowedManufacturers_;

    // Statistics
    SysExTransferStatistics statistics_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SysExTransferSafetyManager)
};

//==============================================================================
/**
 * @brief Singleton accessor for SysEx transfer safety manager
 */
class SysExTransferSafetyManagerHolder {
public:
    static SysExTransferSafetyManager& getInstance() {
        static SysExTransferSafetyManager instance;
        return instance;
    }

    SysExTransferSafetyManagerHolder(const SysExTransferSafetyManagerHolder&) = delete;
    SysExTransferSafetyManagerHolder& operator=(const SysExTransferSafetyManagerHolder&) = delete;

private:
    SysExTransferSafetyManagerHolder() = default;
    ~SysExTransferSafetyManagerHolder() = default;
};

} // namespace zenith
