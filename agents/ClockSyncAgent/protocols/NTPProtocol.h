/*
  ==============================================================================
    agents/ClockSyncAgent/protocols/NTPProtocol.h
    NTP Client implementation.
  ==============================================================================
*/

#pragma once

#include "SyncProtocol.h"
#include <atomic>
#include <mutex>

namespace zenith {
namespace agents {
namespace protocols {

/**
    Implements NTP (Network Time Protocol) client synchronization.
    Uses SNTP (Simple Network Time Protocol) logic.
*/
class NTPProtocol : public SyncProtocol,
                    private juce::Thread {
public:
    NTPProtocol();
    ~NTPProtocol() override;

    // SyncProtocol overrides
    void start() override;
    void stop() override;
    int64_t getOffset() const override;
    bool isSynchronized() const override;
    double getDrift() const override;
    void forceSync() override;

    /** Sets the NTP server address (default: pool.ntp.org). */
    void setServerAddress(const juce::String& address);

    /** Sets the polling interval in milliseconds. */
    void setPollingInterval(int intervalMs);

private:
    void run() override;
    bool sendNTPRequest();
    bool receiveNTPResponse();

    juce::DatagramSocket socket_;
    juce::String serverAddress_{"pool.ntp.org"};
    int port_{123};
    int pollingIntervalMs_{10000}; // 10 seconds default

    std::atomic<int64_t> offsetNs_{0};
    std::atomic<bool> synchronized_{false};
    std::atomic<double> drift_{1.0};

    // Protects socket access if needed (though mainly used in thread)
    std::mutex socketMutex_;
};

} // namespace protocols
} // namespace agents
} // namespace zenith
