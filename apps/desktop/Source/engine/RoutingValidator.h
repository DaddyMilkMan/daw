/*
  ==============================================================================

    RoutingValidator.h
    Created: 2026-02-18
    Author:  Zenith DAW - Month 7: Audio Engine Safety (Gap #3)

    Validates audio routing graphs and prevents routing errors.

  ==============================================================================
*/

#pragma once

#include "ChannelLayoutValidator.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <vector>
#include <map>
#include <set>

namespace zenith {

//==============================================================================
/**
 * @brief Audio routing node (track, bus, output, etc.)
 */
struct AudioRoutingNode {
    enum Type {
        Track,           // Audio track
        Bus,             // Mix bus
        Output,          // Hardware output
        Input,           // Hardware input
        Plugin,          // Plugin insert
        Folder,          // Folder track
        Unknown
    };

    juce::String id;
    Type type = Unknown;
    juce::AudioChannelSet channelLayout;
    int inputChannels = 0;
    int outputChannels = 0;

    juce::String toString() const {
        juce::String typeStr;
        switch (type) {
            case Track: typeStr = "Track"; break;
            case Bus: typeStr = "Bus"; break;
            case Output: typeStr = "Output"; break;
            case Input: typeStr = "Input"; break;
            case Plugin: typeStr = "Plugin"; break;
            case Folder: typeStr = "Folder"; break;
            case Unknown: typeStr = "Unknown"; break;
        }
        return "[" + typeStr + "] " + id +
               " (" + juce::String(inputChannels) + "->" +
               juce::String(outputChannels) + ")";
    }
};

//==============================================================================
/**
 * @brief Audio routing connection
 */
struct AudioRoutingConnection {
    juce::String id;
    juce::String sourceNodeId;
    juce::String destNodeId;
    int sourceChannel = -1;      // -1 for all channels
    int destChannel = -1;        // -1 for all channels
    float gain = 1.0f;
    bool enabled = true;

    juce::String toString() const {
        return sourceNodeId + " -> " + destNodeId +
               (sourceChannel >= 0 ? " [ch:" + juce::String(sourceChannel) + "]" : "") +
               (gain != 1.0f ? " (" + juce::String(gain, 2) + " dB)" : "");
    }
};

//==============================================================================
/**
 * @brief Routing validation issue
 */
struct RoutingValidationIssue {
    enum Type {
        InvalidSource,          // Source node doesn't exist
        InvalidDestination,     // Destination node doesn't exist
        ChannelMismatch,        // Channel counts don't match
        LoopDetected,           // Routing loop would be created
        InvalidConnection,      // Connection itself is invalid
        DuplicateConnection,    // Connection already exists
        SelfConnection,         // Node connected to itself
        Overflow,               // Routing overflow
        LayoutIncompatible      // Channel layouts incompatible
    };

    Type type;
    juce::String description;
    juce::String connectionId;
    int severity = 0;  // 0-10

    juce::String toString() const {
        juce::String typeStr;
        switch (type) {
            case InvalidSource: typeStr = "Invalid Source"; break;
            case InvalidDestination: typeStr = "Invalid Dest"; break;
            case ChannelMismatch: typeStr = "Channel Mismatch"; break;
            case LoopDetected: typeStr = "Loop Detected"; break;
            case InvalidConnection: typeStr = "Invalid Connection"; break;
            case DuplicateConnection: typeStr = "Duplicate"; break;
            case SelfConnection: typeStr = "Self Connection"; break;
            case Overflow: typeStr = "Overflow"; break;
            case LayoutIncompatible: typeStr = "Incompatible Layout"; break;
        }
        return "[" + typeStr + "] " + description;
    }
};

//==============================================================================
/**
 * @brief Routing validation result
 */
struct RoutingValidationResult {
    bool isValid = false;
    std::vector<RoutingValidationIssue> issues;
    std::vector<juce::String> warnings;
    juce::String summary;

    juce::String toString() const {
        if (isValid) {
            return "Routing is valid";
        }
        juce::String result = "Routing validation failed:";
        for (const auto& issue : issues) {
            result += "\n  - " + issue.toString();
        }
        for (const auto& warning : warnings) {
            result += "\n  [WARNING] " + warning;
        }
        return result;
    }
};

//==============================================================================
/**
 * @brief Audio routing validator
 *
 * Features:
 * - Validates routing connections
 * - Detects routing loops
 * - Checks channel compatibility
 * - Validates routing graphs
 * - Prevents self-connections
 * - Duplicate detection
 */
class RoutingValidator {
public:
    //==========================================================================
    RoutingValidator();
    ~RoutingValidator();

    //==========================================================================
    /**
     * @brief Add routing node
     */
    void addNode(const AudioRoutingNode& node);

    //==========================================================================
    /**
     * @brief Remove routing node
     */
    void removeNode(const juce::String& nodeId);

    //==========================================================================
    /**
     * @brief Get routing node
     */
    AudioRoutingNode getNode(const juce::String& nodeId) const;

    //==========================================================================
    /**
     * @brief Check if node exists
     */
    bool hasNode(const juce::String& nodeId) const;

    //==========================================================================
    /**
     * @brief Add routing connection
     * @return Validation result (check isValid before using)
     */
    RoutingValidationResult addConnection(const AudioRoutingConnection& connection);

    //==========================================================================
    /**
     * @brief Remove routing connection
     */
    void removeConnection(const juce::String& connectionId);

    //==========================================================================
    /**
     * @brief Validate routing connection
     * Use this before adding to check if it would be valid
     */
    RoutingValidationResult validateConnection(
        const AudioRoutingConnection& connection) const;

    //==========================================================================
    /**
     * @brief Validate entire routing graph
     * Checks for loops, inconsistencies, etc.
     */
    RoutingValidationResult validateGraph() const;

    //==========================================================================
    /**
     * @brief Check for routing loops from a node
     * Returns list of nodes in the loop (empty if no loop)
     */
    std::vector<juce::String> detectLoop(const juce::String& startNodeId) const;

    //==========================================================================
    /**
     * @brief Check if adding a connection would create a loop
     */
    bool wouldCreateLoop(const juce::String& sourceId,
                        const juce::String& destId) const;

    //==========================================================================
    /**
     * @brief Get all connections from a node
     */
    std::vector<AudioRoutingConnection> getConnectionsFrom(
        const juce::String& nodeId) const;

    //==========================================================================
    /**
     * @brief Get all connections to a node
     */
    std::vector<AudioRoutingConnection> getConnectionsTo(
        const juce::String& nodeId) const;

    //==========================================================================
    /**
     * @brief Get all nodes
     */
    std::vector<AudioRoutingNode> getAllNodes() const;

    //==========================================================================
    /**
     * @brief Get all connections
     */
    std::vector<AudioRoutingConnection> getAllConnections() const;

    //==========================================================================
    /**
     * @brief Clear all nodes and connections
     */
    void clear();

    //==========================================================================
    /**
     * @brief Get node count
     */
    int getNodeCount() const {
        return static_cast<int>(nodes_.size());
    }

    //==========================================================================
    /**
     * @brief Get connection count
     */
    int getConnectionCount() const {
        return static_cast<int>(connections_.size());
    }

private:
    //==========================================================================
    bool detectCycleDFS(const juce::String& nodeId,
                       std::set<juce::String>& visited,
                       std::set<juce::String>& recStack,
                       std::vector<juce::String>& path) const;

    //==========================================================================
    // Routing graph
    std::map<juce::String, AudioRoutingNode> nodes_;
    std::map<juce::String, AudioRoutingConnection> connections_;

    // Adjacency list for graph traversal
    // sourceNodeId -> {destNodeId, connectionId}
    std::multimap<juce::String, std::pair<juce::String, juce::String>> adjacency_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RoutingValidator)
};

//==============================================================================
/**
 * @brief Singleton accessor for routing validator
 */
class RoutingValidatorHolder {
public:
    static RoutingValidator& getInstance() {
        static RoutingValidator instance;
        return instance;
    }

    RoutingValidatorHolder(const RoutingValidatorHolder&) = delete;
    RoutingValidatorHolder& operator=(const RoutingValidatorHolder&) = delete;

private:
    RoutingValidatorHolder() = default;
    ~RoutingValidatorHolder() = default;
};

} // namespace zenith
