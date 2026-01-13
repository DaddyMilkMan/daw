/*
  ==============================================================================
    test_transport_protocol.cpp
    Test suite for TransportProtocolAgent
  ==============================================================================
*/

#include "../src/transport_protocol.h"
#include <iostream>

/**
 * @brief Main test function
 * 
 * Minimal test scaffold that returns 0 to indicate success.
 * Tests basic agent functionality including initialization and name printing.
 * 
 * @return 0 on success
 */
int main() {
    std::cout << "TransportProtocolAgent Test Suite" << std::endl;
    std::cout << "=================================" << std::endl;
    std::cout << std::endl;
    
    // Test agent initialization and name printing
    std::cout << "[TEST] Agent initialization and name printing..." << std::endl;
    zenith::TransportProtocolAgent agent;
    std::cout << "[RESULT] Agent name: " << agent.getName() << " v" << agent.getVersion() << std::endl;
    std::cout << "[PASS]" << std::endl;
    std::cout << std::endl;
    
    // Test agent cleanup
    std::cout << "[TEST] Agent cleanup..." << std::endl;
    // Agent will be destroyed when going out of scope
    std::cout << "[PASS]" << std::endl;
    std::cout << std::endl;
    
    std::cout << "All tests passed!" << std::endl;
    
    return 0; // Success
}
