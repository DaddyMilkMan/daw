/*
  ==============================================================================
    transport_protocol.h
    TransportProtocolAgent - Header file
  ==============================================================================
*/

#pragma once

#include <iostream>
#include <string>

namespace zenith {

/**
 * @brief TransportProtocolAgent class
 * 
 * Minimal scaffold for the Transport Protocol Agent.
 * Handles transport protocol communication and synchronization.
 */
class TransportProtocolAgent {
public:
    /**
     * @brief Construct a new Transport Protocol Agent
     */
    TransportProtocolAgent();

    /**
     * @brief Destroy the Transport Protocol Agent
     */
    ~TransportProtocolAgent();

    /**
     * @brief Print the agent name
     */
    void printName() const;

    /**
     * @brief Get the agent name
     * @return Agent name as string
     */
    std::string getName() const;

    /**
     * @brief Get the agent version
     * @return Version string
     */
    std::string getVersion() const;
};

} // namespace zenith
