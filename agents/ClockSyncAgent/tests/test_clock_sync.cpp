/**
 * ClockSyncAgent Test Scaffold
 */

#include <iostream>
#include <thread>
#include <chrono>
#include "../ClockSyncAgent.h"

int main() {
    std::cout << "Starting ClockSyncAgent Test..." << std::endl;

    zenith::agents::ClockSyncAgent agent;

    // Test Initial State (LocalClock)
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

    // Test NTP Source
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

    // Test PTP Source
    std::cout << "Switching to PTP..." << std::endl;
    agent.setTimeSource(zenith::agents::ClockSyncAgent::TimeSource::NetworkPTP);

    status = agent.getSyncStatus();
    if (status.currentSource != zenith::agents::ClockSyncAgent::TimeSource::NetworkPTP) {
        std::cerr << "FAIL: Source should be NetworkPTP" << std::endl;
        return 1;
    }
    std::cout << "PTP initialized: OK" << std::endl;

    // Test Cleanup (Destructor)
    std::cout << "Testing cleanup..." << std::endl;
    // Agent goes out of scope here.

    std::cout << "ClockSyncAgent Test - PASS" << std::endl;
    return 0;
}
