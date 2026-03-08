/*
  ==============================================================================

    MidiLoopPrevention.cpp
    Implementation of MIDI loop prevention

  ==============================================================================
*/

#include "MidiLoopPrevention.h"
#include <iostream>
#include <queue>
#include <set>

namespace zenith {

//==============================================================================
// MidiRoutingGraph Implementation
//==============================================================================
void MidiRoutingGraph::addConnection(const MidiConnection& connection) {
    connections_[connection.id] = connection;

    // Update source and destination indices
    connectionsFrom_[connection.source.id].push_back(connection);
    connectionsTo_[connection.destination.id].push_back(connection);
}

void MidiRoutingGraph::removeConnection(const juce::String& connectionId) {
    auto it = connections_.find(connectionId);
    if (it == connections_.end()) {
        return;
    }

    const auto& connection = it->second;

    // Remove from source index
    auto& fromList = connectionsFrom_[connection.source.id];
    fromList.erase(std::remove_if(fromList.begin(), fromList.end(),
                                   [&connectionId](const MidiConnection& c) {
                                       return c.id == connectionId;
                                   }),
                  fromList.end());

    // Remove from destination index
    auto& toList = connectionsTo_[connection.destination.id];
    toList.erase(std::remove_if(toList.begin(), toList.end(),
                                 [&connectionId](const MidiConnection& c) {
                                     return c.id == connectionId;
                                 }),
                 toList.end());

    connections_.erase(it);
}

void MidiRoutingGraph::setConnectionActive(const juce::String& connectionId,
                                            bool isActive) {
    auto it = connections_.find(connectionId);
    if (it != connections_.end()) {
        it->second.isActive = isActive;
    }
}

std::vector<MidiConnection> MidiRoutingGraph::getAllConnections() const {
    std::vector<MidiConnection> all;
    for (const auto& entry : connections_) {
        all.push_back(entry.second);
    }
    return all;
}

std::vector<MidiConnection> MidiRoutingGraph::getConnectionsFromPort(
    const MidiPort& port) const {

    auto it = connectionsFrom_.find(port.id);
    if (it != connectionsFrom_.end()) {
        return it->second;
    }
    return {};
}

std::vector<MidiConnection> MidiRoutingGraph::getConnectionsToPort(
    const MidiPort& port) const {

    auto it = connectionsTo_.find(port.id);
    if (it != connectionsTo_.end()) {
        return it->second;
    }
    return {};
}

bool MidiRoutingGraph::hasPath(const MidiPort& from, const MidiPort& to) const {
    // BFS to find path
    std::queue<juce::String> queue;
    std::set<juce::String> visited;

    queue.push(from.id);
    visited.insert(from.id);

    while (!queue.empty()) {
        juce::String current = queue.front();
        queue.pop();

        if (current == to.id) {
            return true;  // Found path
        }

        // Get all outgoing connections from current port
        auto it = connectionsFrom_.find(current);
        if (it != connectionsFrom_.end()) {
            for (const auto& conn : it->second) {
                if (!conn.isActive) {
                    continue;  // Skip inactive connections
                }

                if (visited.find(conn.destination.id) == visited.end()) {
                    visited.insert(conn.destination.id);
                    queue.push(conn.destination.id);
                }
            }
        }
    }

    return false;  // No path found
}

//==============================================================================
// MidiLoopPrevention Implementation
//==============================================================================

MidiLoopPrevention::MidiLoopPrevention() {
    std::cout << "MidiLoopPrevention: Initialized" << std::endl;
}

MidiLoopPrevention::~MidiLoopPrevention() {
    std::cout << "MidiLoopPrevention: Shut down" << std::endl;
}

//==============================================================================
LoopResolutionResult MidiLoopPrevention::addConnection(
    const MidiConnection& connection) {

    LoopResolutionResult result;
    result.success = true;

    // Check for self-loop (output to own input)
    if (connection.source.id == connection.destination.id) {
        MidiLoopEvent loop;
        loop.type = MidiLoopEvent::SelfLoop;
        loop.description = "Port connected to itself";
        loop.startPort = connection.source.id;
        loop.severity = 10.0;
        loop.connectionIds.push_back(connection.id);
        result.loopsDetected.push_back(loop);

        if (autoBreakLoops_) {
            // Don't add self-loops
            return result;
        }
    }

    // Add to graph
    graph_.addConnection(connection);

    // Check if this creates a loop
    auto loops = detectLoopsFrom(connection.destination);

    if (!loops.empty()) {
        result.loopsDetected = loops;

        if (autoBreakLoops_) {
            // Break loops by disabling this connection
            disableConnection(connection.id);
            result.loopsBroken = static_cast<int>(loops.size());
            result.disabledConnections.push_back(connection.id);

            // Notify callback
            for (const auto& loop : loops) {
                if (loopCallback_) {
                    loopCallback_(loop);
                }
            }

            std::cout << "MidiLoopPrevention: Prevented loop by disabling "
                      << connection.id << std::endl;
        }
    }

    return result;
}

//==============================================================================
bool MidiLoopPrevention::removeConnection(const juce::String& connectionId) {
    // Can't really remove from graph without more complex implementation
    // For now, just deactivate
    graph_.setConnectionActive(connectionId, false);
    return true;
}

//==============================================================================
LoopResolutionResult MidiLoopPrevention::setConnectionActive(
    const juce::String& connectionId,
    bool isActive) {

    LoopResolutionResult result;
    result.success = true;

    graph_.setConnectionActive(connectionId, isActive);

    if (isActive) {
        // Check for loops when activating
        auto connections = graph_.getAllConnections();

        for (const auto& conn : connections) {
            if (conn.id == connectionId && conn.isActive) {
                auto loops = detectLoopsFrom(conn.destination);

                if (!loops.empty()) {
                    result.loopsDetected = loops;

                    if (autoBreakLoops_) {
                        // Disable again
                        graph_.setConnectionActive(connectionId, false);
                        result.loopsBroken = static_cast<int>(loops.size());
                        result.disabledConnections.push_back(connectionId);

                        std::cout << "MidiLoopPrevention: Prevented activation of "
                                  << connectionId << " (would create loop)"
                                  << std::endl;
                    }
                }

                break;
            }
        }
    }

    return result;
}

//==============================================================================
std::vector<MidiLoopEvent> MidiLoopPrevention::detectAllLoops() const {
    std::vector<MidiLoopEvent> allLoops;
    auto connections = graph_.getAllConnections();

    // Check each connection as potential loop start
    for (const auto& conn : connections) {
        if (!conn.isActive) {
            continue;
        }

        auto loops = detectLoopsFrom(conn.destination);
        allLoops.insert(allLoops.end(), loops.begin(), loops.end());
    }

    return allLoops;
}

//==============================================================================
LoopResolutionResult MidiLoopPrevention::breakAllLoops(const juce::String& startFrom) {
    LoopResolutionResult result;
    result.success = true;

    auto loops = detectAllLoops();
    result.loopsDetected = loops;

    if (loops.empty()) {
        return result;
    }

    // Break each loop by disabling the last connection in the loop
    std::set<juce::String> disabledSet;

    for (const auto& loop : loops) {
        // Disable the connection that would break the loop
        for (const auto& connId : loop.connectionIds) {
            if (disabledSet.find(connId) == disabledSet.end()) {
                disableConnection(connId);
                disabledSet.insert(connId);
                result.disabledConnections.push_back(connId);
                result.loopsBroken++;
                break;  // Only need to disable one per loop
            }
        }
    }

    std::cout << "MidiLoopPrevention: Broke " << result.loopsBroken
              << " loops (disabled " << result.disabledConnections.size()
              << " connections)" << std::endl;

    return result;
}

//==============================================================================
bool MidiLoopPrevention::validateThruPath(const MidiPort& inputPort,
                                          const MidiPort& outputPort) const {
    // Check if connecting input to output would create a loop
    // Simulate the connection and check for back-path from output to input

    // Temporarily add connection
    MidiConnection tempConn;
    tempConn.source = inputPort;
    tempConn.destination = outputPort;
    tempConn.isActive = true;

    // Check if there's a path from output back to input (without the new connection)
    return !graph_.hasPath(outputPort, inputPort);
}

//==============================================================================
std::vector<MidiConnection> MidiLoopPrevention::getAllConnections() const {
    return graph_.getAllConnections();
}

//==============================================================================
juce::String MidiLoopPrevention::getRoutingGraph() const {
    juce::String graph = "MIDI Routing Graph:\n";

    auto connections = graph_.getAllConnections();

    for (const auto& conn : connections) {
        graph += "  " + conn.toString();
        if (!conn.isActive) {
            graph += " [DISABLED]";
        }
        graph += "\n";
    }

    return graph;
}

//==============================================================================
// Private Methods
//==============================================================================
bool MidiLoopPrevention::hasPath(const MidiPort& from,
                                  const MidiPort& to) const {
    return graph_.hasPath(from, to);
}

std::vector<juce::String> MidiLoopPrevention::findPath(const MidiPort& from,
                                                       const MidiPort& to) const {
    std::vector<juce::String> path;
    std::map<juce::String, juce::String> parent;
    std::queue<juce::String> queue;
    std::set<juce::String> visited;

    queue.push(from.id);
    visited.insert(from.id);
    parent[from.id] = "";

    while (!queue.empty()) {
        juce::String current = queue.front();
        queue.pop();

        if (current == to.id) {
            // Reconstruct path
            juce::String curr = to.id;
            while (!curr.isEmpty()) {
                path.insert(path.begin(), curr);
                curr = parent[curr];
            }
            return path;
        }

        // Get outgoing connections
        auto connections = graph_.getConnectionsFromPort(
            MidiPort{current, "", false, -1});

        for (const auto& conn : connections) {
            if (!conn.isActive) {
                continue;
            }

            if (visited.find(conn.destination.id) == visited.end()) {
                visited.insert(conn.destination.id);
                parent[conn.destination.id] = current;
                queue.push(conn.destination.id);
            }
        }
    }

    return {};  // No path found
}

std::vector<MidiLoopEvent> MidiLoopPrevention::detectLoopsFrom(
    const MidiPort& startPort) const {

    std::vector<MidiLoopEvent> loops;
    auto connections = graph_.getAllConnections();

    // Use DFS to detect cycles
    std::set<juce::String> visited;
    std::set<juce::String> recStack;
    std::map<juce::String, juce::String> parent;

    std::function<bool(const MidiPort&)> dfs = [&](const MidiPort& port) -> bool {
        visited.insert(port.id);
        recStack.insert(port.id);

        // Get outgoing connections
        auto outgoing = graph_.getConnectionsFromPort(port);

        for (const auto& conn : outgoing) {
            if (!conn.isActive) {
                continue;
            }

            if (recStack.find(conn.destination.id) != recStack.end()) {
                // Found a cycle!
                MidiLoopEvent loop;
                loop.type = MidiLoopEvent::DirectLoop;
                loop.description = "Loop detected: " + conn.toString();
                loop.startPort = startPort.id;
                loop.severity = 8.0;
                loop.connectionIds.push_back(conn.id);
                loops.push_back(loop);
                return true;
            }

            if (visited.find(conn.destination.id) == visited.end()) {
                parent[conn.destination.id] = port.id;
                if (dfs(conn.destination)) {
                    return true;
                }
            }
        }

        recStack.erase(port.id);
        return false;
    };

    dfs(startPort);

    return loops;
}

void MidiLoopPrevention::disableConnection(const juce::String& connectionId) {
    graph_.setConnectionActive(connectionId, false);
}

} // namespace zenith
