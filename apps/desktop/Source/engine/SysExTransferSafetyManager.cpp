/*
  ==============================================================================

    SysExTransferSafetyManager.cpp
    Implementation of SysEx transfer safety

  ==============================================================================
*/

#include "SysExTransferSafetyManager.h"
#include <iostream>
#include <algorithm>

namespace zenith {

//==============================================================================
// SysExTransferSafetyManager Implementation
//==============================================================================

SysExTransferSafetyManager::SysExTransferSafetyManager() {
    std::cout << "SysExTransferSafetyManager: Initialized" << std::endl;
}

SysExTransferSafetyManager::~SysExTransferSafetyManager() {
    std::cout << "SysExTransferSafetyManager: Shut down ("
              << statistics_.successfulTransfers << " successful transfers)"
              << std::endl;
}

//==============================================================================
bool SysExTransferSafetyManager::startReceiving(const juce::String& transferId,
                                                  const juce::String& manufacturerId,
                                                  int expectedLength) {

    // Check if manufacturer is allowed
    if (!manufacturerId.isEmpty() && !isManufacturerAllowed(manufacturerId)) {
        std::cerr << "SysExTransferSafetyManager: Manufacturer '" << manufacturerId
                  << "' is not allowed" << std::endl;
        return false;
    }

    // Check if transfer already exists
    if (activeTransfers_.find(transferId) != activeTransfers_.end()) {
        std::cerr << "SysExTransferSafetyManager: Transfer '" << transferId
                  << "' already exists" << std::endl;
        return false;
    }

    // Create new transfer
    SysExTransfer transfer;
    transfer.id = transferId;
    transfer.state = SysExTransferState::Receiving;
    transfer.manufacturerId = manufacturerId;
    transfer.expectedLength = expectedLength;
    transfer.receivedLength = 0;
    transfer.startTime = juce::Time::getCurrentTime();

    activeTransfers_[transferId] = transfer;

    std::cout << "SysExTransferSafetyManager: Started receiving '"
              << transferId << "'" << std::endl;

    return true;
}

//==============================================================================
bool SysExTransferSafetyManager::addSysExData(const juce::String& transferId,
                                               const juce::uint8* data,
                                               int length) {

    auto it = activeTransfers_.find(transferId);
    if (it == activeTransfers_.end()) {
        std::cerr << "SysExTransferSafetyManager: Transfer '" << transferId
                  << "' not found" << std::endl;
        return false;
    }

    SysExTransfer& transfer = it->second;

    if (transfer.state != SysExTransferState::Receiving) {
        std::cerr << "SysExTransferSafetyManager: Transfer '" << transferId
                  << "' not in receiving state" << std::endl;
        return false;
    }

    // Add data to transfer
    transfer.data.insert(transfer.data.end(), data, data + length);
    transfer.receivedLength += length;

    // Check if transfer is complete
    if (transfer.expectedLength > 0 &&
        transfer.receivedLength >= transfer.expectedLength) {
        transfer.state = SysExTransferState::Complete;
        std::cout << "SysExTransferSafetyManager: Transfer '" << transferId
                  << "' complete (" << transfer.receivedLength << " bytes)"
                  << std::endl;
    }

    // Check for end byte (0xF7)
    if (!transfer.data.empty() && transfer.data.back() == 0xF7) {
        transfer.state = SysExTransferState::Complete;
        std::cout << "SysExTransferSafetyManager: Transfer '" << transferId
                  << "' complete (end byte received)" << std::endl;
    }

    return true;
}

//==============================================================================
std::vector<SysExTransferIssue> SysExTransferSafetyManager::completeTransfer(
    const juce::String& transferId) {

    std::vector<SysExTransferIssue> issues;

    auto it = activeTransfers_.find(transferId);
    if (it == activeTransfers_.end()) {
        SysExTransferIssue issue;
        issue.type = SysExTransferIssue::TransferInterrupted;
        issue.description = "Transfer not found";
        issue.severity = 8.0;
        issues.push_back(issue);
        return issues;
    }

    SysExTransfer& transfer = it->second;

    // Validate transfer
    auto validationIssues = validateTransfer(transfer);
    issues.insert(issues.end(), validationIssues.begin(), validationIssues.end());

    // Update statistics
    statistics_.totalTransfers++;
    statistics_.totalBytesTransferred += transfer.receivedLength;

    juce::Time endTime = juce::Time::getCurrentTime();
    juce::RelativeTime duration = endTime - transfer.startTime;
    double transferTime = duration.inSeconds();

    statistics_.averageTransferTime =
        (statistics_.averageTransferTime * (statistics_.totalTransfers - 1) +
         transferTime) / statistics_.totalTransfers;

    if (issues.empty() || std::all_of(issues.begin(), issues.end(),
                                        [](const SysExTransferIssue& i) {
                                            return i.severity < 7.0;
                                        })) {
        transfer.state = SysExTransferState::Complete;
        statistics_.successfulTransfers++;
    } else {
        transfer.state = SysExTransferState::Failed;
        statistics_.failedTransfers++;
    }

    // Remove from active transfers
    activeTransfers_.erase(it);

    return issues;
}

//==============================================================================
std::vector<juce::String> SysExTransferSafetyManager::checkForTimeouts(
    double timeoutSeconds) const {

    std::vector<juce::String> timedOutTransfers;
    juce::Time currentTime = juce::Time::getCurrentTime();

    for (const auto& entry : activeTransfers_) {
        const auto& transfer = entry.second;

        if (transfer.state == SysExTransferState::Receiving) {
            juce::RelativeTime elapsed = currentTime - transfer.startTime;
            double elapsedSeconds = elapsed.inSeconds();

            if (elapsedSeconds > timeoutSeconds) {
                timedOutTransfers.push_back(transfer.id);
            }
        }
    }

    return timedOutTransfers;
}

//==============================================================================
bool SysExTransferSafetyManager::verifyChecksum(const juce::uint8* data,
                                                  int length,
                                                  int checksumLocation) {

    if (length < 2) {
        return false;  // Too short for checksum
    }

    // If checksum location not specified, assume last byte
    if (checksumLocation < 0) {
        checksumLocation = length - 1;
    }

    if (checksumLocation >= length) {
        return false;  // Invalid checksum location
    }

    juce::uint8 expectedChecksum = data[checksumLocation];

    // Calculate checksum (sum of bytes modulo 128)
    juce::uint8 calculatedChecksum = 0;
    for (int i = 0; i < checksumLocation; ++i) {
        calculatedChecksum += data[i];
    }
    calculatedChecksum &= 0x7F;  // Keep to 7 bits

    return calculatedChecksum == expectedChecksum;
}

//==============================================================================
std::vector<SysExTransferIssue> SysExTransferSafetyManager::validateSysEx(
    const juce::uint8* data,
    int length) const {

    std::vector<SysExTransferIssue> issues;

    if (length < 2) {
        SysExTransferIssue issue;
        issue.type = SysExTransferIssue::InvalidFormat;
        issue.description = "SysEx too short";
        issue.bytesTransferred = length;
        issue.expectedBytes = 2;
        issue.severity = 9.0;
        issues.push_back(issue);
        return issues;
    }

    // Check start byte
    if (data[0] != 0xF0) {
        SysExTransferIssue issue;
        issue.type = SysExTransferIssue::InvalidFormat;
        issue.description = "SysEx must start with 0xF0";
        issue.severity = 9.0;
        issues.push_back(issue);
    }

    // Check end byte
    if (data[length - 1] != 0xF7) {
        SysExTransferIssue issue;
        issue.type = SysExTransferIssue::IncompleteTransfer;
        issue.description = "SysEx must end with 0xF7";
        issue.bytesTransferred = length;
        issue.severity = 8.0;
        issues.push_back(issue);
    }

    // Validate manufacturer ID (bytes 1-3)
    if (length >= 4) {
        juce::String manufacturerId =
            juce::String::toHexString(data[1]) +
            juce::String::toHexString(data[2]) +
            juce::String::toHexString(data[3]);

        if (!isManufacturerAllowed(manufacturerId) &&
            !allowedManufacturers_.empty()) {
            SysExTransferIssue issue;
            issue.type = SysExTransferIssue::InvalidManufacturer;
            issue.description = "Manufacturer ID " + manufacturerId +
                               " is not allowed";
            issue.severity = 7.0;
            issues.push_back(issue);
        }
    }

    return issues;
}

//==============================================================================
SysExTransfer SysExTransferSafetyManager::getTransfer(
    const juce::String& transferId) const {

    auto it = activeTransfers_.find(transferId);
    if (it != activeTransfers_.end()) {
        return it->second;
    }

    return SysExTransfer{};  // Invalid transfer
}

//==============================================================================
std::vector<juce::uint8> SysExTransferSafetyManager::reassembleMultiPacket(
    const std::vector<std::vector<juce::uint8>>& packets) {

    std::vector<juce::uint8> reassembled;

    for (const auto& packet : packets) {
        // Remove 0xF0 start and 0xF7 end bytes for intermediate packets
        size_t start = 0;
        size_t end = packet.size();

        if (packet.size() > 0 && packet[0] == 0xF0) {
            start = 1;  // Skip start byte
        }

        if (packet.size() > 0 && packet.back() == 0xF7) {
            end = packet.size() - 1;  // Skip end byte
        }

        // Add packet data
        reassembled.insert(reassembled.end(),
                         packet.begin() + start,
                         packet.begin() + end);
    }

    // Add start and end bytes
    reassembled.insert(reassembled.begin(), 0xF0);
    reassembled.push_back(0xF7);

    return reassembled;
}

//==============================================================================
// Private Methods
//==============================================================================
bool SysExTransferSafetyManager::isManufacturerAllowed(
    const juce::String& manufacturerId) const {

    // If no filters set, allow all
    if (allowedManufacturers_.empty()) {
        return true;
    }

    auto it = allowedManufacturers_.find(manufacturerId);
    if (it != allowedManufacturers_.end()) {
        return it->second;  // Return allowed status
    }

    return false;  // Not in allow list
}

std::vector<SysExTransferIssue> SysExTransferSafetyManager::validateTransfer(
    const SysExTransfer& transfer) const {

    std::vector<SysExTransferIssue> issues;

    if (transfer.data.empty()) {
        SysExTransferIssue issue;
        issue.type = SysExTransferIssue::IncompleteTransfer;
        issue.description = "No data received";
        issue.bytesTransferred = 0;
        issue.expectedBytes = transfer.expectedLength;
        issue.severity = 9.0;
        issues.push_back(issue);
        return issues;
    }

    // Validate format
    auto formatIssues = validateSysEx(transfer.data.data(),
                                     static_cast<int>(transfer.data.size()));
    issues.insert(issues.end(), formatIssues.begin(), formatIssues.end());

    // Check if expected length matches
    if (transfer.expectedLength > 0 &&
        transfer.receivedLength != transfer.expectedLength) {
        SysExTransferIssue issue;
        issue.type = SysExTransferIssue::IncompleteTransfer;
        issue.description = "Length mismatch: expected " +
                           juce::String(transfer.expectedLength) +
                           ", received " + juce::String(transfer.receivedLength);
        issue.bytesTransferred = transfer.receivedLength;
        issue.expectedBytes = transfer.expectedLength;
        issue.severity = 7.0;
        issues.push_back(issue);
    }

    // Verify checksum if present
    if (!transfer.data.empty() && issues.empty()) {
        // Try to verify checksum (last byte)
        if (!verifyChecksum(transfer.data.data(),
                          static_cast<int>(transfer.data.size()),
                          -1)) {
            SysExTransferIssue issue;
            issue.type = SysExTransferIssue::ChecksumMismatch;
            issue.description = "Checksum verification failed";
            issue.bytesTransferred = transfer.receivedLength;
            issue.expectedBytes = static_cast<int>(transfer.data.size());
            issue.severity = 6.0;
            issues.push_back(issue);
        }
    }

    return issues;
}

} // namespace zenith
