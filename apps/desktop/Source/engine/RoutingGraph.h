/*
  ==============================================================================

    RoutingGraph.h
    Created: 2025-12-03
    Author:  Zenith DAW

    Manages the audio signal flow graph, including track-to-bus routing,
    sends, and sidechains.
    
    THREAD SAFETY FIX: Uses lock-free RCU-style snapshot pattern for reads.
    - Writes (add/remove/connect) use lock and are message-thread-only
    - Reads use atomic snapshot mechanism - safe to call from any thread

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <unordered_map>
#include <vector>
#include <string>
#include <atomic>
#include <memory>

namespace zenith {

class RoutingGraph
{
public:
    friend class AudioRenderer; // Allow AudioRenderer to access snapshots

    //==============================================================================
    enum class NodeType
    {
        Track,
        Bus,
        Master,
        PluginSidechain
    };

    // Forward declarations for pointers
    class Track;
    class AuxBus;

    struct Node
    {
        juce::String id;
        NodeType type;
        juce::String name;
        int channelCount = 2;
    };

    struct Connection
    {
        juce::String sourceId;
        juce::String destId;
        float gain = 1.0f;
        int sourceChannelIndex = 0; // For multi-channel routing
        int destChannelIndex = 0;
    };

    //==============================================================================
    RoutingGraph();
    ~RoutingGraph();

    //==============================================================================
    // Graph Modification (MESSAGE THREAD ONLY)
    void addNode(const Node& node);
    void removeNode(const juce::String& nodeId);
    
    bool connect(const juce::String& sourceId, const juce::String& destId, float gain = 1.0f);
    bool disconnect(const juce::String& sourceId, const juce::String& destId);
    
    //==============================================================================
    // Lock-free Queries (ANY THREAD - RT-SAFE)
    bool hasNode(const juce::String& nodeId) const;
    const Node* getNode(const juce::String& nodeId) const;
    std::vector<Connection> getConnectionsFrom(const juce::String& sourceId) const;
    std::vector<Connection> getConnectionsTo(const juce::String& destId) const;
    std::vector<juce::String> getProcessingOrder() const;

    /**
     * @brief Update snapshot with direct pointers (MESSAGE THREAD ONLY)
     */
    void updateSnapshotWithPointers(
        const std::unordered_map<juce::String, Track*>& trackMap,
        const std::unordered_map<juce::String, AuxBus*>& auxBusMap);
    
    //==============================================================================
    // Serialization (MESSAGE THREAD ONLY)
    juce::var toVar() const;
    void fromVar(const juce::var& data);
    
    juce::ValueTree toValueTree() const;
    void fromValueTree(const juce::ValueTree& state);

private:
    //==============================================================================
    // Snapshot for lock-free read access
    struct Snapshot
    {
        std::unordered_map<std::string, Node> nodes;
        std::vector<Connection> connections;
        std::vector<juce::String> processingOrder;
        
        // Fast lookup maps (populated by RoutingGraph::updateSnapshot)
        std::unordered_map<juce::String, Track*> trackLookup;
        std::unordered_map<juce::String, AuxBus*> auxBusLookup;

        Snapshot() = default;
        Snapshot(const std::unordered_map<std::string, Node>& n, 
                 const std::vector<Connection>& c,
                 const std::vector<juce::String>& order)
            : nodes(n), connections(c), processingOrder(order) {}
    };
    
    // Owning data (message thread only, protected by lock)
    std::unordered_map<std::string, Node> nodes_;
    std::vector<Connection> connections_;

    // Lock for modifications (message thread only)
    mutable juce::CriticalSection writeLock_;
    
    // Lock-free snapshot for reads (any thread)
    std::atomic<const Snapshot*> activeSnapshot_{nullptr};
    std::shared_ptr<Snapshot> currentSnapshot_;
    std::vector<std::shared_ptr<Snapshot>> snapshotTrash_;
    
    // Helper to update snapshot after modification
    void updateSnapshot();
    
    // Get current snapshot (lock-free)
    const Snapshot* getSnapshot() const { 
        return activeSnapshot_.load(std::memory_order_acquire); 
    }
};

} // namespace zenith
