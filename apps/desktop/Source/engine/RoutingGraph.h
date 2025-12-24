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

#include <atomic>
#include <deque>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace zenith {

// Forward declarations
class Track;
class AuxBus;

class RoutingGraph {
public:
  friend class AudioRenderer; // Allow AudioRenderer to access snapshots

  //==============================================================================
  enum class NodeType { Track, Bus, Master, PluginSidechain };

  struct Node {
    juce::String id;
    NodeType type;
    juce::String name;
    int channelCount = 2;
  };

  struct Connection {
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
  void addNode(const Node &node);
  void removeNode(const juce::String &nodeId);

  bool connect(const juce::String &sourceId, const juce::String &destId,
               float gain = 1.0f);
  bool disconnect(const juce::String &sourceId, const juce::String &destId);

  //==============================================================================
  // Lock-free Queries (ANY THREAD - RT-SAFE)
  bool hasNode(const juce::String &nodeId) const;
  const Node *getNode(const juce::String &nodeId) const;
  std::vector<Connection>
  getConnectionsFrom(const juce::String &sourceId) const;
  std::vector<Connection> getConnectionsTo(const juce::String &destId) const;
  std::vector<juce::String> getProcessingOrder() const;

  /**
   * @brief Update snapshot with direct pointers (MESSAGE THREAD ONLY)
   */
  void updateSnapshotWithPointers(const std::vector<Track *> &tracks,
                                  const std::vector<AuxBus *> &auxBuses);

  //==============================================================================
  // Serialization (MESSAGE THREAD ONLY)
  juce::var toVar() const;
  void fromVar(const juce::var &data);

  juce::ValueTree toValueTree() const;
  void fromValueTree(const juce::ValueTree &state);

private:
  //==============================================================================
  // Snapshot for lock-free read access
  struct Topology {
    std::vector<Connection> connections;
    std::vector<juce::String> processingOrder;
    int version = 0;
  };

  struct Snapshot {
    std::unordered_map<std::string, Node> nodes;
    std::shared_ptr<Topology> topology;

    enum class ProcessorType { Track, Bus };
    struct ProcessorNode {
      ProcessorType type;
      Track *track = nullptr;
      AuxBus *bus = nullptr;
      float masterGain = 1.0f;
      int trackIndex = -1; // Cached for fast buffer access
      int busIndex = -1;
    };

    std::vector<ProcessorNode> processingSequence;

    // Fast lookup maps (still used for message thread queries)
    std::unordered_map<juce::String, Track *> trackLookup;
    std::unordered_map<juce::String, AuxBus *> auxBusLookup;

    Snapshot() : topology(std::make_shared<Topology>()) {}
    Snapshot(const std::unordered_map<std::string, Node> &n,
             std::shared_ptr<Topology> t)
        : nodes(n), topology(t) {}
  };

  // Owning data (message thread only, protected by lock)
  std::unordered_map<std::string, Node> nodes_;
  std::vector<Connection> connections_;
  std::shared_ptr<Topology> currentTopology_;
  int nextTopologyVersion_ = 1;

  // Lock for modifications (message thread only)
  mutable juce::CriticalSection writeLock_;

  // Lock-free snapshot for reads (any thread)
  // Audio thread reads activeSnapshot_ (atomic raw pointer)
  std::atomic<Snapshot*> activeSnapshot_{nullptr};

  // Message thread owns currentSnapshot_
  std::shared_ptr<Snapshot> currentSnapshot_;

  // Helper to update snapshot after modification
  void updateSnapshot();

  // Get current snapshot (lock-free)
  const Snapshot* getSnapshot() const {
    return activeSnapshot_.load(std::memory_order_acquire);
  }
};

} // namespace zenith
