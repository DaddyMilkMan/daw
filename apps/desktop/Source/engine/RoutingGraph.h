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
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace zenith {

class RoutingGraph {
public:
  //==============================================================================
  enum class NodeType { Track, Bus, Master, PluginSidechain };

  struct Node {
    juce::String id;
    NodeType type;
    juce::String name;
    int channelCount = 2;
  };

  struct Connection {
    enum class Type { Audio, Modulation };
    Type type = Type::Audio;

    juce::String sourceId;
    juce::String destId;
    float gain = 1.0f;

    // Audio Routing
    int sourceChannelIndex = 0;
    int destChannelIndex = 0;
    bool isFeedback = false;

    // Modulation Routing
    int targetPluginIndex = -1; // -1 for track params, >=0 for plugin params
    int targetParamIndex = -1;  // Parameter index
    juce::String targetParamId; // Parameter ID (for robust binding)

    bool operator==(const Connection &other) const {
      return sourceId == other.sourceId && destId == other.destId &&
             type == other.type &&
             targetPluginIndex == other.targetPluginIndex &&
             targetParamIndex == other.targetParamIndex;
    }
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
  bool connectModulation(const juce::String &sourceId,
                         const juce::String &destId, int pluginIndex,
                         int paramIndex, float intensity = 1.0f);
  bool disconnect(const juce::String &sourceId, const juce::String &destId);

  //==============================================================================
  // Verification
  bool verifyGraphIntegrity() const;

  //==============================================================================
  // Lock-free Queries (ANY THREAD - RT-SAFE)
  bool hasNode(const juce::String &nodeId) const;
  const Node *getNode(const juce::String &nodeId) const;
  const std::vector<Connection> &
  getConnectionsFrom(const juce::String &sourceId) const;
  std::vector<Connection> getConnectionsTo(const juce::String &destId) const;
  const std::vector<juce::String> &getProcessingOrder() const;

  //==============================================================================
  // Serialization (MESSAGE THREAD ONLY)
  juce::var toVar() const;
  void fromVar(const juce::var &data);

  juce::ValueTree toValueTree() const;
  void fromValueTree(const juce::ValueTree &state);

private:
  //==============================================================================
  // Snapshot for lock-free read access
  struct Snapshot {
    std::unordered_map<std::string, Node> nodes;
    std::vector<Connection> connections;
    std::vector<juce::String> processingOrder;

    // Optimized lookup for RT thread
    std::unordered_map<std::string, std::vector<Connection>>
        outgoingConnections;

    Snapshot() = default;
    Snapshot(const std::unordered_map<std::string, Node> &n,
             const std::vector<Connection> &c,
             const std::vector<juce::String> &order)
        : nodes(n), connections(c), processingOrder(order) {
      // Build optimized lookup
      for (const auto &conn : connections) {
        outgoingConnections[conn.sourceId.toStdString()].push_back(conn);
      }
    }
  };

  // Owning data (message thread only, protected by lock)
  std::unordered_map<std::string, Node> nodes_;
  std::vector<Connection> connections_;

  // Lock for modifications (message thread only)
  mutable juce::CriticalSection writeLock_;

  // Lock-free snapshot for reads (any thread)
  std::atomic<const Snapshot *> activeSnapshot_{nullptr};
  std::shared_ptr<Snapshot> currentSnapshot_;
  std::vector<std::shared_ptr<Snapshot>> snapshotTrash_;

  // Helper to update snapshot after modification
  void updateSnapshot();

  // Get current snapshot (lock-free)
  const Snapshot *getSnapshot() const {
    return activeSnapshot_.load(std::memory_order_acquire);
  }
};

} // namespace zenith
