/*
  ==============================================================================
    transport_protocol.cpp
    TransportProtocolAgent - Minimal scaffold implementation
  ==============================================================================
*/

#include "transport_protocol.h"

namespace zenith {

/**
 * @brief Construct a new Transport Protocol Agent
 */
TransportProtocolAgent::TransportProtocolAgent() {
    printName();
}

/**
 * @brief Destroy the Transport Protocol Agent
 */
TransportProtocolAgent::~TransportProtocolAgent() = default;

/**
 * @brief Print the agent name
 */
void TransportProtocolAgent::printName() const {
    std::cout << "TransportProtocolAgent v0.1.0" << std::endl;
    std::cout << "Purpose: Transport protocol communication and synchronization" << std::endl;
}

/**
 * @brief Get the agent name
 * @return Agent name as string
 */
std::string TransportProtocolAgent::getName() const {
    return "TransportProtocolAgent";
}

/**
 * @brief Get the agent version
 * @return Version string
 */
std::string TransportProtocolAgent::getVersion() const {
    return "0.1.0";
}

} // namespace zenith
