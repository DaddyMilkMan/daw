/*
  ==============================================================================

    RoutingValidator.cpp
    Implementation of audio routing validation

  ==============================================================================
*/

#include "RoutingValidator.h"
#include <iostream>
#include <algorithm>

namespace zenith {

//==============================================================================
// RoutingValidator Implementation
//==============================================================================

RoutingValidator::RoutingValidator() {
    std::cout << "RoutingValidator: Initialized" << std::endl;
}

RoutingValidator::~RoutingValidator() {
    std::cout << "RoutingValidator: Shut down ("
              << nodes_.size() << " nodes, "
              << connections_.size() << " connections)" << std::endl;
}

//==============================================================================
void RoutingValidator::addNode(const AudioRoutingNode& node) {
    nodes_[node.id] = node;
}

//==============================================================================
void RoutingValidator::removeNode(const juce::String& nodeId) {
    // Remove all connections to/from this node
    auto it = adjacency_.begin();
    while (it != adjacency_.end()) {
        if (it->first == nodeId || it->second.first == nodeId) {
            // Remove connection
            connections_.erase(it->second.second);
            it = adjacency_.erase(it);
        } else {
            ++it;
        }
    }

    // Remove node
    nodes_.erase(nodeId);
}

//==============================================================================
AudioRoutingNode RoutingValidator::getNode(const juce::String& nodeId) const {
    auto it = nodes_.find(nodeId);
    if (it != nodes_.end()) {
        return it->second;
    }
    return AudioRoutingNode{};  // Return empty node
}

//==============================================================================
bool RoutingValidator::hasNode(const juce::String& nodeId) const {
    return nodes_.find(nodeId) != nodes_.end();
}

//==============================================================================
RoutingValidationResult RoutingValidator::addConnection(
    const AudioRoutingConnection& connection)
{
    RoutingValidationResult result = validateConnection(connection);

    if (!result.isValid) {
        return result;  // Don't add if invalid
    }

    // Add connection
    connections_[connection.id] = connection;
    adjacency_.insert({connection.sourceNodeId,
                      {connection.destNodeId, connection.id}});

    result.isValid = true;
    result.summary = "Connection added successfully";
    return result;
}

//==============================================================================
void RoutingValidator::removeConnection(const juce::String& connectionId) {
    // Remove from adjacency list
    for (auto it = adjacency_.begin(); it != adjacency_.end(); ++it) {
        if (it->second.second == connectionId) {
            adjacency_.erase(it);
            break;
        }
    }

    // Remove connection
    connections_.erase(connectionId);
}

//==============================================================================
RoutingValidationResult RoutingValidator::validateConnection(
    const AudioRoutingConnection& connection) const
{
    RoutingValidationResult result;
    result.isValid = true;

    // Check source exists
    if (!hasNode(connection.sourceNodeId)) {
        RoutingValidationIssue issue;
        issue.type = RoutingValidationIssue::InvalidSource;
        issue.description = "Source node '" + connection.sourceNodeId +
                           "' does not exist";
        issue.connectionId = connection.id;
        issue.severity = 9;
        result.issues.push_back(issue);
        result.isValid = false;
    }

    // Check destination exists
    if (!hasNode(connection.destNodeId)) {
        RoutingValidationIssue issue;
        issue.type = RoutingValidationIssue::InvalidDestination;
        issue.description = "Destination node '" + connection.destNodeId +
                           "' does not exist";
        issue.connectionId = connection.id;
        issue.severity = 9;
        result.issues.push_back(issue);
        result.isValid = false;
    }

    // Check for self-connection
    if (connection.sourceNodeId == connection.destNodeId) {
        RoutingValidationIssue issue;
        issue.type = RoutingValidationIssue::SelfConnection;
        issue.description = "Node cannot connect to itself";
        issue.connectionId = connection.id;
        issue.severity = 7;
        result.issues.push_back(issue);
        result.isValid = false;
    }

    // Check for duplicate connection
    for (const auto& pair : connections_) {
        const auto& conn = pair.second;
        if (conn.sourceNodeId == connection.sourceNodeId &&
            conn.destNodeId == connection.destNodeId &&
            conn.sourceChannel == connection.sourceChannel &&
            conn.destChannel == connection.destChannel) {
            RoutingValidationIssue issue;
            issue.type = RoutingValidationIssue::DuplicateConnection;
            issue.description = "Connection already exists";
            issue.connectionId = connection.id;
            issue.severity = 5;
            result.issues.push_back(issue);
            // Don't fail for duplicates, just warn
        }
    }

    // Check for loop
    if (wouldCreateLoop(connection.sourceNodeId, connection.destNodeId)) {
        RoutingValidationIssue issue;
        issue.type = RoutingValidationIssue::LoopDetected;
        issue.description = "Connection would create a routing loop";
        issue.connectionId = connection.id;
        issue.severity = 8;
        result.issues.push_back(issue);
        result.isValid = false;
    }

    // Check channel compatibility
    if (hasNode(connection.sourceNodeId) && hasNode(connection.destNodeId)) {
        auto sourceNode = getNode(connection.sourceNodeId);
        auto destNode = getNode(connection.destNodeId);

        // Validate layouts
        auto& validator = ChannelLayoutValidatorHolder::getInstance();
        auto layoutResult = validator.validateCompatibility(
            sourceNode.channelLayout,
            destNode.channelLayout);

        if (!layoutResult.isValid) {
            RoutingValidationIssue issue;
            issue.type = RoutingValidationIssue::LayoutIncompatible;
            issue.description = "Channel layouts are incompatible: " +
                               layoutResult.summary;
            issue.connectionId = connection.id;
            issue.severity = 7;
            result.issues.push_back(issue);
            result.isValid = false;
        }
    }

    // Generate summary
    if (result.isValid) {
        result.summary = "Connection is valid";
    } else {
        result.summary = "Connection has " +
                        juce::String(result.issues.size()) +
                        " validation issues";
    }

    return result;
}

//==============================================================================
RoutingValidationResult RoutingValidator::validateGraph() const {
    RoutingValidationResult result;
    result.isValid = true;

    // Check all nodes for loops
    for (const auto& pair : nodes_) {
        const juce::String& nodeId = pair.first;
        auto loop = detectLoop(nodeId);

        if (!loop.empty()) {
            RoutingValidationIssue issue;
            issue.type = RoutingValidationIssue::LoopDetected;
            issue.description = "Routing loop detected: " +
                               juce::String(loop.size()) + " nodes involved";
            issue.severity = 8;
            result.issues.push_back(issue);
            result.isValid = false;
        }
    }

    // Validate all connections
    for (const auto& pair : connections_) {
        auto connResult = validateConnection(pair.second);
        if (!connResult.isValid) {
            result.issues.insert(result.issues.end(),
                               connResult.issues.begin(),
                               connResult.issues.end());
            result.isValid = false;
        }
    }

    // Generate summary
    if (result.isValid) {
        result.summary = "Routing graph is valid (" +
                        juce::String(nodes_.size()) + " nodes, " +
                        juce::String(connections_.size()) + " connections)";
    } else {
        result.summary = "Routing graph has " +
                        juce::String(result.issues.size()) +
                        " validation issues";
    }

    return result;
}

//==============================================================================
std::vector<juce::String> RoutingValidator::detectLoop(
    const juce::String& startNodeId) const
{
    std::set<juce::String> visited;
    std::set<juce::String> recStack;
    std::vector<juce::String> path;

    if (detectCycleDFS(startNodeId, visited, recStack, path)) {
        return path;
    }

    return {};
}

//==============================================================================
bool RoutingValidator::wouldCreateLoop(const juce::String& sourceId,
                                       const juce::String& destId) const
{
    // Temporarily add the connection and check for cycles
    std::multimap<juce::String, std::pair<juce::String, juce::String>> tempAdj = adjacency_;
    tempAdj.insert({sourceId, {destId, "temp"}});

    std::set<juce::String> visited;
    std::set<juce::String> recStack;
    std::vector<juce::String> path;

    // Check from source node
    return detectCycleDFS(sourceId, visited, recStack, path);
}

//==============================================================================
std::vector<AudioRoutingConnection> RoutingValidator::getConnectionsFrom(
    const juce::String& nodeId) const
{
    std::vector<AudioRoutingConnection> result;

    auto range = adjacency_.equal_range(nodeId);
    for (auto it = range.first; it != range.second; ++it) {
        const juce::String& connectionId = it->second.second;
        auto connIt = connections_.find(connectionId);
        if (connIt != connections_.end()) {
            result.push_back(connIt->second);
        }
    }

    return result;
}

//==============================================================================
std::vector<AudioRoutingConnection> RoutingValidator::getConnectionsTo(
    const juce::String& nodeId) const
{
    std::vector<AudioRoutingConnection> result;

    for (const auto& pair : connections_) {
        if (pair.second.destNodeId == nodeId) {
            result.push_back(pair.second);
        }
    }

    return result;
}

//==============================================================================
std::vector<AudioRoutingNode> RoutingValidator::getAllNodes() const {
    std::vector<AudioRoutingNode> result;
    for (const auto& pair : nodes_) {
        result.push_back(pair.second);
    }
    return result;
}

//==============================================================================
std::vector<AudioRoutingConnection> RoutingValidator::getAllConnections() const {
    std::vector<AudioRoutingConnection> result;
    for (const auto& pair : connections_) {
        result.push_back(pair.second);
    }
    return result;
}

//==============================================================================
void RoutingValidator::clear() {
    nodes_.clear();
    connections_.clear();
    adjacency_.clear();
}

//==============================================================================
// Private Methods
//==============================================================================

bool RoutingValidator::detectCycleDFS(
    const juce::String& nodeId,
    std::set<juce::String>& visited,
    std::set<juce::String>& recStack,
    std::vector<juce::String>& path) const
{
    if (recStack.count(nodeId)) {
        // Found a cycle
        path.push_back(nodeId);
        return true;
    }

    if (visited.count(nodeId)) {
        // Already visited, no cycle from this path
        return false;
    }

    visited.insert(nodeId);
    recStack.insert(nodeId);
    path.push_back(nodeId);

    // Visit all neighbors
    auto range = adjacency_.equal_range(nodeId);
    for (auto it = range.first; it != range.second; ++it) {
        const juce::String& neighborId = it->second.first;
        if (detectCycleDFS(neighborId, visited, recStack, path)) {
            return true;
        }
    }

    // Backtrack
    recStack.erase(nodeId);
    path.pop_back();

    return false;
}

} // namespace zenith
