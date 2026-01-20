/*
  ==============================================================================
    agents/ClockSyncAgent/protocols/PTPProtocol.h
    PTP (IEEE 1588) Slave implementation (Basic).
  ==============================================================================
*/

#pragma once

#include "SyncProtocol.h"
#include <atomic>
#include <mutex>
#include <vector>

namespace zenith {
namespace agents {
namespace protocols {

/**
    Implements a basic PTP (Precision Time Protocol) slave.
    Listens to Sync and Follow_Up messages to calculate offset.
    Does not currently implement full Best Master Clock Algorithm (BMCA) or Delay Request mechanism.
*/
class PTPProtocol : public SyncProtocol,
                    private juce::Thread {
public:
    PTPProtocol();
    ~PTPProtocol() override;

    // SyncProtocol overrides
    void start() override;
    void stop() override;
    int64_t getOffset() const override;
    bool isSynchronized() const override;
    double getDrift() const override;

private:
    void run() override;
    void processEventMessage(const uint8_t* data, int size, int64_t rxTimestamp);
    void processGeneralMessage(const uint8_t* data, int size);

    // Sockets for PTP
    juce::DatagramSocket eventSocket_;   // Port 319
    juce::DatagramSocket generalSocket_; // Port 320

    std::atomic<int64_t> offsetNs_{0};
    std::atomic<bool> synchronized_{false};

    // State for Sync/FollowUp matching
    struct {
        uint16_t sequenceId{0};
        int64_t syncTimestamp{0}; // T2 (Ingress time of Sync)
        bool waitingForFollowUp{false};
    } syncState_;

    std::mutex stateMutex_;
};

} // namespace protocols
} // namespace agents
} // namespace zenith
