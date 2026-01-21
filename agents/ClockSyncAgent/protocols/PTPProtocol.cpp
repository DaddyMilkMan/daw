/*
  ==============================================================================
    agents/ClockSyncAgent/protocols/PTPProtocol.cpp
    PTP (IEEE 1588) Slave implementation.
  ==============================================================================
*/

#include "PTPProtocol.h"
#include <iostream>

namespace zenith {
namespace agents {
namespace protocols {

// PTP Constants
constexpr int PTP_EVENT_PORT = 319;
constexpr int PTP_GENERAL_PORT = 320;
constexpr const char* PTP_MULTICAST_IP = "224.0.1.129";

// Message Types
constexpr uint8_t MSG_TYPE_SYNC = 0x0;
constexpr uint8_t MSG_TYPE_FOLLOW_UP = 0x8;

// Flags
constexpr uint16_t FLAG_TWO_STEP = 0x0200;

struct PTPHeader {
    uint8_t transportSpecific_messageType;
    uint8_t versionPTP_reserved;
    uint16_t messageLength;
    uint8_t domainNumber;
    uint8_t reserved1;
    uint16_t flagField;
    int64_t correctionField;
    uint32_t reserved2;
    uint8_t sourcePortIdentity[10];
    uint16_t sequenceId;
    uint8_t controlField;
    uint8_t logMessageInterval;
};

// Timestamp format in PTP messages: 48 bits seconds, 32 bits nanoseconds
struct PTPTimestamp {
    uint8_t seconds[6];
    uint32_t nanoseconds;
};

PTPProtocol::PTPProtocol() : Thread("PTPProtocolThread") {
}

PTPProtocol::~PTPProtocol() {
    stop();
}

void PTPProtocol::start() {
    if (!isThreadRunning()) {
        startThread();
    }
}

void PTPProtocol::stop() {
    signalThreadShouldExit();
    waitForThreadToExit(2000);
    eventSocket_.shutdown();
    generalSocket_.shutdown();
}

int64_t PTPProtocol::getOffset() const {
    return offsetNs_.load(std::memory_order_acquire);
}

bool PTPProtocol::isSynchronized() const {
    return synchronized_.load(std::memory_order_acquire);
}

double PTPProtocol::getDrift() const {
    // Basic implementation assumes 1.0 (no drift measured yet)
    return 1.0;
}

void PTPProtocol::run() {
    // Bind sockets and join multicast group
    if (!eventSocket_.bindToPort(PTP_EVENT_PORT) ||
        !generalSocket_.bindToPort(PTP_GENERAL_PORT)) {
        std::cerr << "PTPProtocol: Failed to bind to PTP ports (319/320). Check permissions." << std::endl;
        return;
    }

    eventSocket_.joinMulticast(PTP_MULTICAST_IP);
    generalSocket_.joinMulticast(PTP_MULTICAST_IP);

    std::vector<uint8_t> buffer(1024);

    while (!threadShouldExit()) {
        bool activity = false;

        // Check Event Socket (Sync)
        int ready = eventSocket_.waitUntilReady(true, 10);
        if (ready > 0) {
            // Capture T2 (RX Timestamp) immediately
            auto now = std::chrono::system_clock::now();
            auto seconds = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
            auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count() % 1000000000LL;
            int64_t rxTimestamp = seconds * 1000000000LL + nanos;

            int bytesRead = eventSocket_.read(buffer.data(), (int)buffer.size(), false);
            if (bytesRead >= 34) { // Minimum PTP header size
                processEventMessage(buffer.data(), bytesRead, rxTimestamp);
                activity = true;
            }
        }

        // Check General Socket (Follow_Up)
        ready = generalSocket_.waitUntilReady(true, 10);
        if (ready > 0) {
            int bytesRead = generalSocket_.read(buffer.data(), (int)buffer.size(), false);
            if (bytesRead >= 34) {
                processGeneralMessage(buffer.data(), bytesRead);
                activity = true;
            }
        }

        if (!activity) {
            wait(10); // Sleep briefly to avoid spinning too hot
        }
    }
}

void PTPProtocol::processEventMessage(const uint8_t* data, int size, int64_t rxTimestamp) {
    uint8_t msgType = data[0] & 0x0F;
    uint16_t flags = (uint16_t(data[6]) << 8) | data[7];
    uint16_t seqId = (uint16_t(data[30]) << 8) | data[31];

    if (msgType == MSG_TYPE_SYNC) {
        std::lock_guard<std::mutex> lock(stateMutex_);
        syncState_.sequenceId = seqId;
        syncState_.syncTimestamp = rxTimestamp; // T2
        syncState_.waitingForFollowUp = (flags & FLAG_TWO_STEP) != 0;

        if (!syncState_.waitingForFollowUp) {
            // One-step: Timestamp is in the Sync message itself (bytes 34-43)
            // Implementation skipped for brevity (Focus on Two-Step which is common)
        }
    }
}

void PTPProtocol::processGeneralMessage(const uint8_t* data, int size) {
    uint8_t msgType = data[0] & 0x0F;
    uint16_t seqId = (uint16_t(data[30]) << 8) | data[31];

    if (msgType == MSG_TYPE_FOLLOW_UP) {
        std::lock_guard<std::mutex> lock(stateMutex_);

        if (syncState_.waitingForFollowUp && seqId == syncState_.sequenceId) {
            // Parse preciseOriginTimestamp (bytes 34-43)
            if (size < 44) return;

            // 48-bit seconds (big endian)
            uint64_t seconds = 0;
            for (int i = 0; i < 6; ++i) {
                seconds = (seconds << 8) | data[34 + i];
            }

            // 32-bit nanoseconds (big endian)
            uint32_t nanos = 0;
            for (int i = 0; i < 4; ++i) {
                nanos = (nanos << 8) | data[40 + i];
            }

            int64_t t1 = (int64_t)seconds * 1000000000LL + nanos;
            int64_t t2 = syncState_.syncTimestamp;

            // Offset = T2 (Slave Receive) - T1 (Master Send) - Delay
            // Assuming Delay = 0 for now (or positive offset means Slave is ahead?)
            // Wait.
            // If Master sends at T1=100.
            // Slave receives at T2=110.
            // Real delay = 5.
            // If Slave clock is perfect: T2 should be 105.
            // Slave reads 110. Slave is FAST (ahead) by 5?
            // No.
            // Master Time @ Event = 100.
            // Slave Time @ Event = 110.
            // Slave - Master = 10. (Offset).
            // So Offset = T2 - T1 - Delay.
            // If Delay is ignored (0), Offset = 10.
            // We want to correct Slave. SlaveNew = SlaveOld + Offset.

            int64_t offset = t1 - t2; // Offset = Master(T1) - Slave(T2)

            offsetNs_.store(offset, std::memory_order_release);
            if (onOffsetChanged) onOffsetChanged(offset);

            bool wasSync = synchronized_.exchange(true, std::memory_order_release);
            if (!wasSync && onSyncStateChanged) onSyncStateChanged(true);

            syncState_.waitingForFollowUp = false;
        }
    }
}

} // namespace protocols
} // namespace agents
} // namespace zenith
