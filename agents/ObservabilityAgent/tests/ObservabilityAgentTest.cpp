/*
  ==============================================================================
    agents/ObservabilityAgent/tests/ObservabilityAgentTest.cpp
    Test suite for ObservabilityAgent.
  ==============================================================================
*/

#include <iostream>
#include <vector>
#include <cassert>
#include <cmath>
#include "../ObservabilityAgent.h"

// Standalone test for ObservabilityAgent

void runTest() {
    using namespace zenith::agents;

    std::cout << "Starting ObservabilityAgent Test..." << std::endl;

    ObservabilityAgent agent;

    // Test 1: Record Gauge
    agent.recordGauge("cpu_load", 45.5);

    auto metrics = agent.getMetrics();
    assert(metrics.size() == 1);
    assert(metrics[0].name == "cpu_load");
    assert(metrics[0].type == ObservabilityAgent::MetricType::Gauge);
    assert(std::abs(metrics[0].value - 45.5) < 0.001);

    std::cout << "Test 1 Passed: Record Gauge" << std::endl;

    // Test 2: Record Counter
    agent.recordCounter("notes_played", 1.0);
    agent.recordCounter("notes_played", 1.0);

    metrics = agent.getMetrics(); // Should fetch new metrics
    assert(metrics.size() == 2);
    assert(metrics[0].name == "notes_played");
    assert(metrics[0].type == ObservabilityAgent::MetricType::Counter);

    std::cout << "Test 2 Passed: Record Counter" << std::endl;

    // Test 3: Timer
    auto start = agent.startTimer();
    // Simulate tiny delay
    for(volatile int i=0; i<1000; ++i);
    agent.endTimer("processing_time", start);

    metrics = agent.getMetrics();
    assert(metrics.size() == 1);
    assert(metrics[0].name == "processing_time");
    assert(metrics[0].type == ObservabilityAgent::MetricType::Timer);

    std::cout << "Test 3 Passed: Timer" << std::endl;

    // Test 4: Ring Buffer Capacity
    agent.clearMetrics();

    int written = 0;
    // Fill buffer beyond capacity (4096)
    for (int i = 0; i < 5000; ++i) {
        agent.recordGauge("stress_test", (double)i);
        written++;
    }

    metrics = agent.getMetrics();

    std::cout << "Written 5000 items. Read: " << metrics.size() << " items." << std::endl;

    // Expect metrics.size() to be capacity (4096) or capacity-1
    assert(metrics.size() <= 4096);
    assert(metrics.size() >= 4095); // Should be full

    // Verify FIFO behavior (drop newest or blocking? Implementation uses prepareToWrite(1))
    // juce::AbstractFifo::prepareToWrite(1) returns 0 blocks if full.
    // So if full, we drop the new event.
    // Thus we expect the FIRST 4096 events to be in the buffer, and the rest dropped.

    assert(metrics[0].value == 0.0);
    assert(metrics[1].value == 1.0);

    std::cout << "Test 4 Passed: Buffer Capacity" << std::endl;

    std::cout << "All Tests Passed!" << std::endl;
}

int main() {
    runTest();
    return 0;
}
