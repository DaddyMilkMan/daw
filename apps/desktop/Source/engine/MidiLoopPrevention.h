/*
  ==============================================================================

    MidiLoopPrevention.h
    Created: 2026-02-18
    Author:  Zenith DAW - Month 6: MIDI Safety (Gap #3)

    Prevents and resolves MIDI feedback loops.

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <vector>
#include <map>
#include <set>
#include <functional>

namespace zenith {

//==============================================================================
/**
 * @brief MIDI device/port identifier
 */
struct MidiPort {
    juce::String id;               // Unique port ID
    juce::String name;             // Display name
    bool isInput = false;          // True = input, False = output
    int deviceIndex = -1;          // Device index

    juce::String toString() const {
        return name + " (" + (isInput ? "Input" : "Output") + ")";
    }
};

//==============================================================================
/**
 * @brief MIDI routing connection
 */
struct MidiConnection {
    juce::String id;               // Unique connection ID
    MidiPort source;               // Source port
    MidiPort destination;          // Destination port
    bool isActive = true;          // Is connection active

    juce::String toString() const {
        return source.toString() + " -> " + destination.toString();
    }
};

//==============================================================================
/**
 * @brief MIDI loop event
 */
struct MidiLoopEvent {
    enum Type {
        DirectLoop,                // A -> B -> A
        IndirectLoop,              // A -> B -> C -> A
        ThruLoop,                 // MIDI thru loop
        SelfLoop                   // Output routed to own input
    };

    Type type;
    juce::String description;
    std::vector<juce::String> connectionIds;  // Connections involved
    juce::String startPort;       // Where loop starts
    double severity = 0.0;         // 0-10

    juce::String toString() const {
        juce::String typeStr;
        switch (type) {
            case DirectLoop: typeStr = "Direct Loop"; break;
            case IndirectLoop: typeStr = "Indirect Loop"; break;
            case ThruLoop: typeStr = "Thru Loop"; break;
            case SelfLoop: typeStr = "Self Loop"; break;
        }
        return "[" + typeStr + "] " + description +
               " (start: " + startPort + ")";
    }
};

//==============================================================================
/**
 * @brief Loop resolution result
 */
struct LoopResolutionResult {
    bool success = true;
    std::vector<MidiLoopEvent> loopsDetected;
    int loopsBroken = 0;
    std::vector<juce::String> disabledConnections;

    juce::String toString() const {
        return "Loop Resolution: " + juce::String(success ? "SUCCESS" : "FAILED") +
               " (" + juce::String(loopsBroken) + " loops broken, " +
               juce::String(disabledConnections.size()) + " connections disabled)";
    }
};

//==============================================================================
/**
 * @brief MIDI routing graph
 */
class MidiRoutingGraph {
public:
    //==========================================================================
    void addConnection(const MidiConnection& connection);
    void removeConnection(const juce::String& connectionId);
    void setConnectionActive(const juce::String& connectionId, bool isActive);

    //==========================================================================
    std::vector<MidiConnection> getAllConnections() const;
    std::vector<MidiConnection> getConnectionsFromPort(const MidiPort& port) const;
    std::vector<MidiConnection> getConnectionsToPort(const MidiPort& port) const;

    //==========================================================================
    bool hasPath(const MidiPort& from, const MidiPort& to) const;

private:
    std::map<juce::String, MidiConnection> connections_;
    std::map<juce::String, std::vector<MidiConnection>> connectionsFrom_;
    std::map<juce::String, std::vector<MidiConnection>> connectionsTo_;
};

//==============================================================================
/**
 * @brief Prevents and resolves MIDI feedback loops
 *
 * Features:
 * - MIDI path graph tracking
 * - Loop detection algorithm (DFS)
 * - Automatic loop breaking
 * - Message source tagging
 * - Thru path validation
 * - Visual routing diagram
 */
class MidiLoopPrevention {
public:
    //==========================================================================
    MidiLoopPrevention();
    ~MidiLoopPrevention();

    //==========================================================================
    /**
     * @brief Add a MIDI connection
     * @param connection Connection to add
     * @return Result with any loops detected
     */
    LoopResolutionResult addConnection(const MidiConnection& connection);

    //==========================================================================
    /**
     * @brief Remove a MIDI connection
     * @param connectionId Connection to remove
     * @return true if removed
     */
    bool removeConnection(const juce::String& connectionId);

    //==========================================================================
    /**
     * @brief Activate or deactivate a connection
     * @param connectionId Connection ID
     * @param isActive Active state
     * @return Result with any loops detected on activation
     */
    LoopResolutionResult setConnectionActive(const juce::String& connectionId,
                                              bool isActive);

    //==========================================================================
    /**
     * @brief Detect all loops in routing
     * @return List of loops detected
     */
    std::vector<MidiLoopEvent> detectAllLoops() const;

    //==========================================================================
    /**
     * @brief Break all detected loops
     * @param startFrom Starting port for loop detection
     * @return Resolution result
     */
    LoopResolutionResult breakAllLoops(const juce::String& startFrom = "");

    //==========================================================================
    /**
     * @brief Validate MIDI thru path
     * @param inputPort Input port
     * @param outputPort Output port
     * @return true if path is valid (no loops)
     */
    bool validateThruPath(const MidiPort& inputPort,
                          const MidiPort& outputPort) const;

    //==========================================================================
    /**
     * @brief Get all connections
     * @return List of all connections
     */
    std::vector<MidiConnection> getAllConnections() const;

    //==========================================================================
    /**
     * @brief Get routing graph (for visualization)
     * @return Graph representation
     */
    juce::String getRoutingGraph() const;

    //==========================================================================
    /**
     * @brief Enable/disable automatic loop breaking
     * @param enable true to automatically break loops
     */
    void setAutoLoopBreakingEnabled(bool enable) {
        autoBreakLoops_ = enable;
    }

    /**
     * @brief Check if auto loop breaking is enabled
     * @return true if enabled
     */
    bool isAutoLoopBreakingEnabled() const { return autoBreakLoops_; }

    //==========================================================================
    /**
     * @brief Register callback for loop detection
     * @param callback Function to call when loop detected
     */
    void setLoopCallback(std::function<void(const MidiLoopEvent&)> callback) {
        loopCallback_ = callback;
    }

private:
    //==========================================================================
    bool hasPath(const MidiPort& from, const MidiPort& to) const;
    std::vector<juce::String> findPath(const MidiPort& from,
                                       const MidiPort& to) const;

    std::vector<MidiLoopEvent> detectLoopsFrom(const MidiPort& startPort) const;
    void disableConnection(const juce::String& connectionId);

    //==========================================================================
    // Routing graph
    MidiRoutingGraph graph_;

    // Settings
    bool autoBreakLoops_ = true;

    // Callbacks
    std::function<void(const MidiLoopEvent&)> loopCallback_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiLoopPrevention)
};

//==============================================================================
/**
 * @brief Singleton accessor for MIDI loop prevention
 */
class MidiLoopPreventionHolder {
public:
    static MidiLoopPrevention& getInstance() {
        static MidiLoopPrevention instance;
        return instance;
    }

    MidiLoopPreventionHolder(const MidiLoopPreventionHolder&) = delete;
    MidiLoopPreventionHolder& operator=(const MidiLoopPreventionHolder&) = delete;

private:
    MidiLoopPreventionHolder() = default;
    ~MidiLoopPreventionHolder() = default;
};

} // namespace zenith
