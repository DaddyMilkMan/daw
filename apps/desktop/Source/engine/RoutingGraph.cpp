/*
  ==============================================================================

    RoutingGraph.cpp
    Created: 2025-12-03
    Author:  Zenith DAW

    THREAD SAFETY FIX: Implements lock-free RCU-style snapshot pattern.
    - All modifications use writeLock_ and update snapshot atomically
    - All reads use atomic snapshot load - fully RT-safe

  ==============================================================================
*/

#include "RoutingGraph.h"
#include "RealTimeGarbageCollector.h"
#include "Track.h"
#include "AuxBus.h"

namespace zenith {

RoutingGraph::RoutingGraph() {
  // Initialize with empty snapshot and topology
  currentTopology_ = std::make_shared<Topology>();
  currentSnapshot_ = std::make_shared<Snapshot>(nodes_, currentTopology_);
  activeSnapshot_.store(currentSnapshot_.get(), std::memory_order_release);
}

RoutingGraph::~RoutingGraph() {}

//==============================================================================
// Snapshot Management
//==============================================================================

void RoutingGraph::updateSnapshot() {
  // Called from message thread while holding writeLock_

  // 1. Create a new topology structure
  auto nextTopology = std::make_shared<Topology>();
  nextTopology->version = nextTopologyVersion_++;
  nextTopology->connections = connections_; // Copy master connections list

  // Reset feedback flags in the topology copy
  for (auto &c : nextTopology->connections) {
    c.isFeedback = false;
  }

  // 2. Build Adjacency List for Cycle Detection
  // We sort node IDs to ensure deterministic traversal order
  std::vector<std::string> sortedNodeIds;
  sortedNodeIds.reserve(nodes_.size());
  for (const auto &pair : nodes_) {
    sortedNodeIds.push_back(pair.first);
  }
  std::sort(sortedNodeIds.begin(), sortedNodeIds.end());

  std::map<std::string, std::vector<std::string>> adjList;
  for (const auto &c : nextTopology->connections) {
    adjList[c.sourceId.toStdString()].push_back(c.destId.toStdString());
  }

  // Ensure adjacency lists are also sorted for determinism
  for (auto &pair : adjList) {
    std::sort(pair.second.begin(), pair.second.end());
  }

  // 3. DFS for Cycle Detection
  // 0 = White (Unvisited), 1 = Gray (Visiting), 2 = Black (Visited)
  std::unordered_map<std::string, int> visitState;
  
  // Helper DFS function
  std::function<void(const std::string&)> dfs = 
      [&](const std::string& u) {
    visitState[u] = 1; // Mark Gray

    for (const auto& v : adjList[u]) {
      int vState = visitState[v];
      if (vState == 1) {
        // Gray -> Gray: Back-edge (Cycle) detected!
        // Mark all connections u->v as feedback
        for (auto &c : nextTopology->connections) {
          if (c.sourceId.toStdString() == u && c.destId.toStdString() == v) {
            c.isFeedback = true;
          }
        }
      } else if (vState == 0) {
        // White: Recurse
        dfs(v);
      }
    }

    visitState[u] = 2; // Mark Black
  };

  // Run DFS from each node (if not visited)
  for (const auto &id : sortedNodeIds) {
    if (visitState[id] == 0) {
      dfs(id);
    }
  }

  // 4. Robust Topological Sort (Kahn's Algorithm)
  // Now we treat 'isFeedback' edges as non-existent for the sort
  std::unordered_map<std::string, int> inDegree;
  std::map<std::string, std::vector<std::string>> dagAdjList; // DAG only

  // Initialize in-degrees
  for (const auto &id : sortedNodeIds) {
    inDegree[id] = 0;
  }

  // Build DAG (ignoring feedback edges)
  for (const auto &c : nextTopology->connections) {
    if (!c.isFeedback) {
      std::string src = c.sourceId.toStdString();
      std::string dst = c.destId.toStdString();
      
      if (nodes_.count(src) && nodes_.count(dst)) {
        dagAdjList[src].push_back(dst);
        inDegree[dst]++;
      }
    }
  }

  // Sort adjacency for determinism in queue adds
  for (auto &pair : dagAdjList) {
    std::sort(pair.second.begin(), pair.second.end());
  }

  // Queue for nodes with 0 in-degree
  // Use a std::vector as a queue but sort it or process in deterministic order?
  // Since we iterate sortedNodeIds, the initial fill is deterministic.
  std::vector<std::string> queue;
  queue.reserve(nodes_.size());
  
  for (const auto &id : sortedNodeIds) {
    if (inDegree[id] == 0) {
      queue.push_back(id);
    }
  }

  // Process queue
  size_t queueIndex = 0;
  while (queueIndex < queue.size()) {
    std::string u = queue[queueIndex++];
    nextTopology->processingOrder.push_back(u);

    // Neighbors in the DAG
    for (const auto &v : dagAdjList[u]) {
      inDegree[v]--;
      if (inDegree[v] == 0) {
        queue.push_back(v);
      }
    }
  }

  // If graph had nodes (disconnected or cycles), they should be covered now 
  // because we broke all cycles. 
  // Any remaining nodes not in processingOrder? 
  // Should not happen if DFS correctly broke all cycles.
  // But just in case of logic error, append remaining safely.
  if (nextTopology->processingOrder.size() < nodes_.size()) {
     for (const auto &id : sortedNodeIds) {
        bool found = false;
        for (const auto &processed : nextTopology->processingOrder) {
            if (processed == juce::String(id)) { found = true; break; }
        }
        if (!found) nextTopology->processingOrder.push_back(id);
     }
  }

  // Update state
  currentTopology_ = nextTopology;

  // Create new snapshot
  auto newSnapshot = std::make_shared<Snapshot>(nodes_, currentTopology_);

  // Atomic swap
  activeSnapshot_.store(newSnapshot.get(), std::memory_order_release);
  RealTimeGarbageCollector::getInstance().deferDelete(currentSnapshot_);
  currentSnapshot_ = newSnapshot;
}

void RoutingGraph::updateSnapshotWithPointers(
    const std::unordered_map<juce::String, std::shared_ptr<Track>> &trackMap,
    const std::unordered_map<juce::String, std::shared_ptr<AuxBus>> &auxBusMap) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
  const juce::ScopedLock sl(writeLock_);

  // 10/10 Optimization: Reuse the currentTopology_ (no re-sort, no copy of
  // connections)
  auto newSnapshot = std::make_shared<Snapshot>(nodes_, currentTopology_);
  
  // Convert shared_ptr to weak_ptr for non-owning references
  // This prevents potential reference cycles and ensures proper cleanup
  for (const auto& [key, value] : trackMap) {
    newSnapshot->trackLookup[key] = value;  // Implicit shared_ptr -> weak_ptr
  }
  for (const auto& [key, value] : auxBusMap) {
    newSnapshot->auxBusLookup[key] = value;  // Implicit shared_ptr -> weak_ptr
  }

  // OPTIMIZATION: Build linear render list
  // Pre-resolve all pointers so the audio thread doesn't have to do hash lookups
  if (currentTopology_) {
      newSnapshot->linearRenderOrder.reserve(currentTopology_->processingOrder.size());
      
      for (const auto& nodeId : currentTopology_->processingOrder) {
          Snapshot::RenderNode node;
          node.id = nodeId;
          
          // Try Track
          auto trackIt = newSnapshot->trackLookup.find(nodeId);
          if (trackIt != newSnapshot->trackLookup.end()) {
              if (auto trackPtr = trackIt->second.lock()) {
                  node.track = trackPtr.get();
                  newSnapshot->linearRenderOrder.push_back(node);
                  continue; 
              }
          }
          
          // Try AuxBus
          auto busIt = newSnapshot->auxBusLookup.find(nodeId);
          if (busIt != newSnapshot->auxBusLookup.end()) {
              if (auto busPtr = busIt->second.lock()) {
                  node.bus = busPtr.get();
                  newSnapshot->linearRenderOrder.push_back(node);
                  continue;
              }
          }
      }
  }

  // Atomic swap
  activeSnapshot_.store(newSnapshot.get(), std::memory_order_release);
  RealTimeGarbageCollector::getInstance().deferDelete(currentSnapshot_);
  currentSnapshot_ = newSnapshot;
}

//==============================================================================
// Graph Modification (MESSAGE THREAD ONLY)
//==============================================================================

void RoutingGraph::addNode(const Node &node) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  const juce::ScopedLock sl(writeLock_);
  nodes_[node.id.toStdString()] = node;
  updateSnapshot();
}

void RoutingGraph::removeNode(const juce::String &nodeId) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  const juce::ScopedLock sl(writeLock_);

  // Remove node
  nodes_.erase(nodeId.toStdString());

  // Remove associated connections
  connections_.erase(std::remove_if(connections_.begin(), connections_.end(),
                                    [&](const Connection &c) {
                                      return c.sourceId == nodeId ||
                                             c.destId == nodeId;
                                    }),
                     connections_.end());

  updateSnapshot();
}

bool RoutingGraph::detectCycle(const juce::String& sourceId, const juce::String& destId) {
    // Basic idea: If we can reach sourceId starting from destId, then adding sourceId->destId creates a cycle.
    // We use the existing connections plus the potential new one.
    
    std::unordered_map<std::string, std::vector<std::string>> adj;
    for (const auto& c : connections_) {
        adj[c.sourceId.toStdString()].push_back(c.destId.toStdString());
    }
    
    // BFS queue
    std::vector<std::string> queue;
    std::unordered_set<std::string> visited;
    
    queue.push_back(destId.toStdString());
    visited.insert(destId.toStdString());
    
    size_t head = 0;
    while(head < queue.size()) {
        std::string u = queue[head++];
        
        if (u == sourceId.toStdString()) return true; // Found path back to source
        
        for (const auto& v : adj[u]) {
            if (visited.find(v) == visited.end()) {
                visited.insert(v);
                queue.push_back(v);
            }
        }
    }
    
    return false;
}

bool RoutingGraph::connect(const juce::String &sourceId,
                           const juce::String &destId, float gain,
                           bool isSidechain, bool isFeedback) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  const juce::ScopedLock sl(writeLock_);

  // Validate nodes exist
  if (nodes_.find(sourceId.toStdString()) == nodes_.end() ||
      nodes_.find(destId.toStdString()) == nodes_.end()) {
    return false;
  }

  // Check if connection already exists
  for (const auto &c : connections_) {
    if (c.sourceId == sourceId && c.destId == destId &&
        c.isSidechain == isSidechain)
      return true; // Already connected
  }
  
  // Proactive Cycle Detection (unless explicitly marked as feedback)
  if (!isFeedback && detectCycle(sourceId, destId)) {
      // Cycle detected!
      // In a real scenario, we might want to allow this if 'isFeedback' was passed as true,
      // but for standard routing, we block it to prevent infinite recursion in the renderer.
      // If the user *intends* a feedback loop, they should use a specific feedback device, 
      // but standard track routing should be DAG.
      DBG("RoutingGraph: Cycle detected! Connection " + sourceId + " -> " + destId + " rejected.");
      return false;
  }

  Connection c;
  c.sourceId = sourceId;
  c.destId = destId;
  c.gain = gain;
  c.isSidechain = isSidechain;
  c.isFeedback = isFeedback;
  connections_.push_back(c);

  updateSnapshot();
  return true;
}

bool RoutingGraph::disconnect(const juce::String &sourceId,
                              const juce::String &destId) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  const juce::ScopedLock sl(writeLock_);

  auto it = std::remove_if(
      connections_.begin(), connections_.end(), [&](const Connection &c) {
        return c.sourceId == sourceId && c.destId == destId;
      });

  if (it != connections_.end()) {
    connections_.erase(it, connections_.end());
    updateSnapshot();
    return true;
  }

  return false;
}

//==============================================================================
// Lock-free Queries (ANY THREAD - RT-SAFE)
//==============================================================================

bool RoutingGraph::hasNode(const juce::String &nodeId) const {
  // RT-SAFE: Uses atomic snapshot load
  const auto *snapshot = getSnapshot();
  if (!snapshot)
    return false;
  return snapshot->nodes.find(nodeId.toStdString()) != snapshot->nodes.end();
}

const RoutingGraph::Node *
RoutingGraph::getNode(const juce::String &nodeId) const {
  // RT-SAFE: Uses atomic snapshot load
  const auto *snapshot = getSnapshot();
  if (!snapshot)
    return nullptr;

  auto it = snapshot->nodes.find(nodeId.toStdString());
  if (it != snapshot->nodes.end())
    return &it->second;
  return nullptr;
}

std::vector<RoutingGraph::Connection>
RoutingGraph::getConnectionsFrom(const juce::String &sourceId) const {
  // RT-SAFE: Uses atomic snapshot load
  const auto *snapshot = getSnapshot();
  if (!snapshot || !snapshot->topology)
    return {};

  std::vector<Connection> result;
  result.reserve(
      snapshot->topology->connections.size()); // Over-reserve to avoid realloc
  for (const auto &c : snapshot->topology->connections) {
    if (c.sourceId == sourceId)
      result.push_back(c);
  }
  return result;
}

std::vector<RoutingGraph::Connection>
RoutingGraph::getConnectionsTo(const juce::String &destId) const {
  // RT-SAFE: Uses atomic snapshot load
  const auto *snapshot = getSnapshot();
  if (!snapshot || !snapshot->topology)
    return {};

  std::vector<Connection> result;
  result.reserve(
      snapshot->topology->connections.size()); // Over-reserve to avoid realloc
  for (const auto &c : snapshot->topology->connections) {
    if (c.destId == destId)
      result.push_back(c);
  }
  return result;
}

std::vector<juce::String> RoutingGraph::getProcessingOrder() const {
  // RT-SAFE: Uses atomic snapshot load
  const auto *snapshot = getSnapshot();
  if (!snapshot || !snapshot->topology)
    return {};
  return snapshot->topology->processingOrder;
}

//==============================================================================
// Serialization (MESSAGE THREAD ONLY)
//==============================================================================

juce::var RoutingGraph::toVar() const {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  const juce::ScopedLock sl(writeLock_);
  juce::DynamicObject::Ptr obj = new juce::DynamicObject();

  // Serialize Nodes
  juce::var nodesArray;
  for (const auto &pair : nodes_) {
    juce::DynamicObject::Ptr nodeObj = new juce::DynamicObject();
    nodeObj->setProperty("id", pair.second.id);
    nodeObj->setProperty("name", pair.second.name);
    nodeObj->setProperty("type", (int)pair.second.type);
    nodesArray.append(juce::var(nodeObj.get()));
  }
  obj->setProperty("nodes", nodesArray);

  // Serialize Connections
  juce::var connsArray;
  for (const auto &c : connections_) {
    juce::DynamicObject::Ptr connObj = new juce::DynamicObject();
    connObj->setProperty("source", c.sourceId);
    connObj->setProperty("dest", c.destId);
    connObj->setProperty("gain", c.gain);
    connObj->setProperty("isSidechain", c.isSidechain);
    connsArray.append(juce::var(connObj.get()));
  }
  obj->setProperty("connections", connsArray);

  return juce::var(obj.get());
}

void RoutingGraph::fromVar(const juce::var &data) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  const juce::ScopedLock sl(writeLock_);

  // Clear existing data
  nodes_.clear();
  connections_.clear();

  if (auto *obj = data.getDynamicObject()) {
    // Deserialize nodes
    auto nodesVar = obj->getProperty("nodes");
    if (auto *nodesArray = nodesVar.getArray()) {
      for (const auto &nodeVar : *nodesArray) {
        if (auto *nodeObj = nodeVar.getDynamicObject()) {
          Node node;
          node.id = nodeObj->getProperty("id").toString();
          node.name = nodeObj->getProperty("name").toString();
          node.type = static_cast<NodeType>(
              static_cast<int>(nodeObj->getProperty("type")));
          nodes_[node.id.toStdString()] = node;
        }
      }
    }

    // Deserialize connections
    auto connsVar = obj->getProperty("connections");
    if (auto *connsArray = connsVar.getArray()) {
      for (const auto &connVar : *connsArray) {
        if (auto *connObj = connVar.getDynamicObject()) {
          Connection c;
          c.sourceId = connObj->getProperty("source").toString();
          c.destId = connObj->getProperty("dest").toString();
          c.gain = connObj->getProperty("gain");
          c.isSidechain = connObj->getProperty("isSidechain");
          connections_.push_back(c);
        }
      }
    }
  }

  updateSnapshot();
}

juce::ValueTree RoutingGraph::toValueTree() const {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  const juce::ScopedLock sl(writeLock_);
  juce::ValueTree tree("RoutingGraph");

  for (const auto &c : currentTopology_->connections) {
    juce::ValueTree conn("Connection");
    conn.setProperty("source", c.sourceId, nullptr);
    conn.setProperty("dest", c.destId, nullptr);
    conn.setProperty("gain", c.gain, nullptr);
    conn.setProperty("isSidechain", c.isSidechain, nullptr);
    tree.appendChild(conn, nullptr);
  }

  return tree;
}

void RoutingGraph::fromValueTree(const juce::ValueTree &state) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  const juce::ScopedLock sl(writeLock_);
  connections_.clear();

  if (state.hasType("RoutingGraph")) {
    for (const auto &child : state) {
      if (child.hasType("Connection")) {
        Connection c;
        c.sourceId = child.getProperty("source");
        c.destId = child.getProperty("dest");
        c.gain = child.getProperty("gain", 1.0f);
        c.isSidechain = child.getProperty("isSidechain", false);
        connections_.push_back(c);
      }
    }
  }

  updateSnapshot();
}

} // namespace zenith
