/**
 * ClockSyncAgent Test Scaffold
 */

#include <iostream>
#include <thread>
#include <chrono>
#include <cassert>
#include "../ClockSyncAgent.h"

int main() {
    std::cout << "Starting ClockSyncAgent Test..." << std::endl;

    zenith::agents::ClockSyncAgent agent;

    // ==========================================
    // 1. Test Initial State (LocalClock)
    // ==========================================
    auto status = agent.getSyncStatus();
    if (status.currentSource != zenith::agents::ClockSyncAgent::TimeSource::LocalClock) {
        std::cerr << "FAIL: Initial source should be LocalClock" << std::endl;
        return 1;
    }
    if (!status.synchronized) {
        std::cerr << "FAIL: LocalClock should be synchronized initially" << std::endl;
        return 1;
    }
    std::cout << "LocalClock: OK" << std::endl;

    // ==========================================
    // 2. Test Latency Calculation (from PR #452)
    // ==========================================
    // NTP Scenario:
    // T1: Client Sent Request (100)
    // T2: Server Received (110)
    // T3: Server Sent Response (120)
    // T4: Client Received (140)
    //
    // RTT = (T4 - T1) - (T3 - T2) = (140 - 100) - (120 - 110) = 40 - 10 = 30
    // Latency (One Way) = RTT / 2 = 15
    // Offset = ((T2 - T1) + (T3 - T4)) / 2 = ((110 - 100) + (120 - 140)) / 2 = (10 + -20) / 2 = -10 / 2 = -5
    
    using namespace std::chrono;
    
    auto t1 = nanoseconds(100000000); // 100ms
    auto t2 = nanoseconds(110000000); // 110ms
    auto t3 = nanoseconds(120000000); // 120ms
    auto t4 = nanoseconds(140000000); // 140ms
    
    agent.updateNetworkMetrics(t1, t2, t3, t4);
    
    status = agent.getSyncStatus();
    
    double expectedLatency = 15.0;
    int64_t expectedOffset = -5000000;

    std::cout << "Calculated Latency: " << status.latencyMs << " ms" << std::endl;
    std::cout << "Calculated Offset: " << status.offsetNanoseconds << " ns" << std::endl;

    if (std::abs(status.latencyMs - expectedLatency) > 0.001) {
        std::cerr << "FAIL: Latency calculation incorrect. Expected " << expectedLatency << ", got " << status.latencyMs << std::endl;
        return 1;
    }
    
    if (status.offsetNanoseconds != expectedOffset) {
         std::cerr << "FAIL: Offset calculation incorrect. Expected " << expectedOffset << ", got " << status.offsetNanoseconds << std::endl;
         return 1;
    }
    std::cout << "Latency Calculation: OK" << std::endl;

    // ==========================================
    // 3. Test NTP Source Switching
    // ==========================================
    std::cout << "Switching to NTP..." << std::endl;
    agent.setTimeSource(zenith::agents::ClockSyncAgent::TimeSource::NetworkNTP);

    status = agent.getSyncStatus();
    if (status.currentSource != zenith::agents::ClockSyncAgent::TimeSource::NetworkNTP) {
        std::cerr << "FAIL: Source should be NetworkNTP" << std::endl;
        return 1;
    }
    // Note: Synchronization might take time, so we don't expect it to be true immediately.
    // But it should be false initially.
    if (status.synchronized) {
        std::cout << "WARNING: NTP reports synchronized immediately (unexpected but possible if mocked)" << std::endl;
    } else {
        std::cout << "NTP initialized (not yet synced): OK" << std::endl;
    }

    // ==========================================
    // 4. Test PTP Source Switching
    // ==========================================
    std::cout << "Switching to PTP..." << std::endl;
    agent.setTimeSource(zenith::agents::ClockSyncAgent::TimeSource::NetworkPTP);

    status = agent.getSyncStatus();
    if (status.currentSource != zenith::agents::ClockSyncAgent::TimeSource::NetworkPTP) {
        std::cerr << "FAIL: Source should be NetworkPTP" << std::endl;
        return 1;
    }
    std::cout << "PTP initialized: OK" << std::endl;

    // ==========================================
    // 5. Test Resynchronize (LocalClock)
    // ==========================================
    std::cout << "Testing Resynchronize (LocalClock)..." << std::endl;
    agent.setTimeSource(zenith::agents::ClockSyncAgent::TimeSource::LocalClock);

    // Artificially inject an offset
    agent.updateNetworkMetrics(
        std::chrono::nanoseconds(100),
        std::chrono::nanoseconds(110),
        std::chrono::nanoseconds(120),
        std::chrono::nanoseconds(130)
    );
    // Offset should be non-zero
    status = agent.getSyncStatus();
    if (status.offsetNanoseconds == 0) {
        std::cerr << "FAIL: Failed to inject artificial offset for testing" << std::endl;
        return 1;
    }

    agent.resynchronize();
    status = agent.getSyncStatus();

    if (status.offsetNanoseconds != 0) {
        std::cerr << "FAIL: Resynchronize (LocalClock) did not reset offset" << std::endl;
        return 1;
    }
    if (!status.synchronized) {
        std::cerr << "FAIL: Resynchronize (LocalClock) should set synchronized to true" << std::endl;
        return 1;
    }
    std::cout << "Resynchronize (LocalClock): OK" << std::endl;

    // ==========================================
    // 6. Test Resynchronize (MIDIClock)
    // ==========================================
    std::cout << "Testing Resynchronize (MIDIClock)..." << std::endl;
    agent.setTimeSource(zenith::agents::ClockSyncAgent::TimeSource::MIDIClock);

    // Simulate synchronized state (manually, as we can't easily pump MIDI here without complexity)
    // But resynchronize should force synchronized to false.
    // Note: setTimeSource(MIDI) already sets sync=false, but let's assume it was true.
    // Since we can't easily force sync=true without private access or pumping messages,
    // we verify that resynchronize ensures sync=false and offset=0.

    agent.resynchronize();
    status = agent.getSyncStatus();

    if (status.synchronized) {
        std::cerr << "FAIL: Resynchronize (MIDIClock) should set synchronized to false" << std::endl;
        return 1;
    }
    if (status.offsetNanoseconds != 0) {
        std::cerr << "FAIL: Resynchronize (MIDIClock) should reset offset" << std::endl;
        return 1;
    }
    std::cout << "Resynchronize (MIDIClock): OK" << std::endl;

    // ==========================================
    // 7. Test Resynchronize (Network Protocols)
    // ==========================================
    std::cout << "Testing Resynchronize (NTP)..." << std::endl;
    agent.setTimeSource(zenith::agents::ClockSyncAgent::TimeSource::NetworkNTP);
    agent.resynchronize(); // Should not crash
    std::cout << "Resynchronize (NTP): OK" << std::endl;

    std::cout << "Testing Resynchronize (PTP)..." << std::endl;
    agent.setTimeSource(zenith::agents::ClockSyncAgent::TimeSource::NetworkPTP);
    agent.resynchronize(); // Should not crash

    // For PTP, we know it resets sync state.
    status = agent.getSyncStatus();
    if (status.synchronized) {
        std::cerr << "FAIL: Resynchronize (PTP) should set synchronized to false" << std::endl;
        return 1;
    }
    std::cout << "Resynchronize (PTP): OK" << std::endl;

    // ==========================================
    // Cleanup
    // ==========================================
    std::cout << "Testing cleanup..." << std::endl;
    // Agent goes out of scope here.

    std::cout << "ClockSyncAgent Test - PASS" << std::endl;
    return 0;
}
