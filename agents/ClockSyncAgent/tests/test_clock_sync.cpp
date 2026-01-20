/**
 * ClockSyncAgent Test Scaffold
 */

#include <iostream>
#include <cassert>
#include "../ClockSyncAgent.h"

int main() {
    std::cout << "Starting ClockSyncAgent Latency Test..." << std::endl;
    
    zenith::agents::ClockSyncAgent agent;
    
    // 1. Test Latency Calculation
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
    
    auto status = agent.getSyncStatus();
    
    // RTT = 30ms -> Latency = 15ms
    double expectedLatency = 15.0;
    // Offset = -5ms = -5,000,000ns
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

    std::cout << "ClockSyncAgent Latency Test - PASS" << std::endl;
    return 0;
}