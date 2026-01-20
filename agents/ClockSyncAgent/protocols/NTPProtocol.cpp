/*
  ==============================================================================
    agents/ClockSyncAgent/protocols/NTPProtocol.cpp
    NTP Client implementation.
  ==============================================================================
*/

#include "NTPProtocol.h"
#include <iostream>

namespace zenith {
namespace agents {
namespace protocols {

// NTP Timestamp format: 64-bit fixed point
// 32 bits seconds since Jan 1 1900
// 32 bits fraction
struct NTPPacket {
    uint8_t li_vn_mode;      // Leap Indicator, Version, Mode
    uint8_t stratum;         // Stratum level
    uint8_t poll;            // Poll interval
    uint8_t precision;       // Precision
    uint32_t rootDelay;      // Root delay
    uint32_t rootDispersion; // Root dispersion
    uint32_t refId;          // Reference ID
    uint32_t refTm_s;        // Reference Timestamp seconds
    uint32_t refTm_f;        // Reference Timestamp fraction
    uint32_t origTm_s;       // Originate Timestamp seconds
    uint32_t origTm_f;       // Originate Timestamp fraction
    uint32_t rxTm_s;         // Receive Timestamp seconds
    uint32_t rxTm_f;         // Receive Timestamp fraction
    uint32_t txTm_s;         // Transmit Timestamp seconds
    uint32_t txTm_f;         // Transmit Timestamp fraction
};

// NTP Epoch (1900) to Unix Epoch (1970) offset in seconds
static const uint32_t NTP_TIMESTAMP_DELTA = 2208988800u;

NTPProtocol::NTPProtocol() : Thread("NTPProtocolThread") {
}

NTPProtocol::~NTPProtocol() {
    stop();
}

void NTPProtocol::start() {
    if (!isThreadRunning()) {
        startThread();
    }
}

void NTPProtocol::stop() {
    signalThreadShouldExit();
    // Wake up socket if blocked?
    // DatagramSocket doesn't have a reliable interrupt if blocking on read,
    // but we'll use waitUntilReady with timeout in the loop.
    waitForThreadToExit(2000);
    socket_.shutdown();
}

int64_t NTPProtocol::getOffset() const {
    return offsetNs_.load(std::memory_order_acquire);
}

bool NTPProtocol::isSynchronized() const {
    return synchronized_.load(std::memory_order_acquire);
}

double NTPProtocol::getDrift() const {
    return drift_.load(std::memory_order_acquire);
}

void NTPProtocol::setServerAddress(const juce::String& address) {
    serverAddress_ = address;
}

void NTPProtocol::setPollingInterval(int intervalMs) {
    pollingIntervalMs_ = intervalMs;
}

void NTPProtocol::run() {
    if (!socket_.bindToPort(0)) { // Bind to any available port
        std::cerr << "NTPProtocol: Failed to bind to port 0." << std::endl;
        return;
    }

    while (!threadShouldExit()) {
        if (sendNTPRequest()) {
            if (receiveNTPResponse()) {
                bool wasSync = synchronized_.exchange(true, std::memory_order_release);
                if (!wasSync && onSyncStateChanged) onSyncStateChanged(true);
            }
        }

        wait(pollingIntervalMs_);
    }
}

bool NTPProtocol::sendNTPRequest() {
    NTPPacket packet = { 0 };

    // LI = 0 (no warning), VN = 3 (IPv4 only), Mode = 3 (Client)
    packet.li_vn_mode = (0 << 6) | (3 << 3) | 3;

    // We should ideally put our current time in origTm (T1), but for simple SNTP
    // we can track T1 locally when we send.
    // However, to follow protocol, let's put it in Transmit Timestamp field?
    // Actually, client sets Transmit Timestamp (T1) in request.

    // Get current time (Unix epoch)
    auto now = std::chrono::system_clock::now();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    auto micros = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count() % 1000000;

    // Convert to NTP time
    packet.txTm_s = juce::ByteOrder::swap((uint32_t)(seconds + NTP_TIMESTAMP_DELTA));
    packet.txTm_f = juce::ByteOrder::swap((uint32_t)((micros / 1000000.0) * 4294967296.0));

    // Send
    std::lock_guard<std::mutex> lock(socketMutex_);
    int bytesSent = socket_.write(serverAddress_, port_, &packet, sizeof(packet));

    return bytesSent == sizeof(packet);
}

bool NTPProtocol::receiveNTPResponse() {
    NTPPacket packet;
    int bytesRead = 0;

    // Wait for response (timeout 1s)
    int ready = socket_.waitUntilReady(true, 1000);
    if (ready > 0) {
        std::lock_guard<std::mutex> lock(socketMutex_);
        bytesRead = socket_.read(&packet, sizeof(packet), false);
    } else {
        return false;
    }

    if (bytesRead != sizeof(packet)) {
        return false;
    }

    // T4: Destination Timestamp (Time of arrival)
    auto now = std::chrono::high_resolution_clock::now();
    // We need this in system time reference to compare with NTP timestamps which are absolute.
    // However, calculating offset is relative.
    // Let's stick to calculating offset relative to system clock.

    // Timestamps from packet are Big Endian
    uint32_t rxTm_s = juce::ByteOrder::swap(packet.rxTm_s); // T2 seconds
    uint32_t rxTm_f = juce::ByteOrder::swap(packet.rxTm_f); // T2 fraction
    uint32_t txTm_s = juce::ByteOrder::swap(packet.txTm_s); // T3 seconds
    uint32_t txTm_f = juce::ByteOrder::swap(packet.txTm_f); // T3 fraction
    uint32_t origTm_s = juce::ByteOrder::swap(packet.origTm_s); // T1 seconds
    uint32_t origTm_f = juce::ByteOrder::swap(packet.origTm_f); // T1 fraction

    // Convert to nanoseconds since NTP epoch
    auto toNanos = [](uint32_t s, uint32_t f) -> int64_t {
        double frac = (double)f / 4294967296.0;
        return (int64_t)s * 1000000000LL + (int64_t)(frac * 1e9);
    };

    int64_t t1 = toNanos(origTm_s, origTm_f);
    int64_t t2 = toNanos(rxTm_s, rxTm_f);
    int64_t t3 = toNanos(txTm_s, txTm_f);

    // Convert T4 to NTP timeframe
    auto nowSys = std::chrono::system_clock::now();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(nowSys.time_since_epoch()).count();
    auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(nowSys.time_since_epoch()).count() % 1000000000LL;

    int64_t t4 = (seconds + NTP_TIMESTAMP_DELTA) * 1000000000LL + nanos;

    // Offset = ((T2 - T1) + (T3 - T4)) / 2
    int64_t offset = ((t2 - t1) + (t3 - t4)) / 2;

    offsetNs_.store(offset, std::memory_order_release);
    if (onOffsetChanged) onOffsetChanged(offset);

    return true;
}

} // namespace protocols
} // namespace agents
} // namespace zenith
