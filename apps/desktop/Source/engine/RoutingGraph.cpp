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
#include "ZenithLogger.h"
#include <unordered_set>

//==============================================================================
namespace {
constexpr int kMaxSnapshotHistory = 10;
}
//==============================================================================

namespace zenith {

RoutingGraph::RoutingGraph() {
  // Initialize with empty snapshot
  currentSnapshot_ = std::make_shared<Snapshot>();
  activeSnapshot_.store(currentSnapshot_.get(), std::memory_order_release);
}

RoutingGraph::~RoutingGraph() {}

//==============================================================================
// Snapshot Management
//==============================================================================

//==============================================================================
// Snapshot Management
//==============================================================================

//==============================================================================
// Snapshot Management & Sort
//==============================================================================

void RoutingGraph::updateSnapshot() {
  // 1. Reset Feedback Flags
  for (auto &conn : connections_) {
    conn.isFeedback = false;
  }

  // 2. Build Adjacency List for Cycle Detection
  std::unordered_map<std::string, std::vector<std::string>> adjList;
  std::unordered_map<std::string, int> inDegree;

  // Initialize
  for (const auto &pair : nodes_) {
    inDegree[pair.first] = 0;
    adjList[pair.first] = {};
  }

  for (const auto &c : connections_) {
    std::string src = c.sourceId.toStdString();
    std::string dst = c.destId.toStdString();
    if (nodes_.count(src) && nodes_.count(dst)) {
      adjList[src].push_back(dst);
      inDegree[dst]++;
    }
  }

  // 3. Detect Cycles (DFS)
  std::unordered_set<std::string> visited;
  std::unordered_set<std::string> recursionStack;
  std::vector<std::string> cycleNodes;

  std::function<bool(const std::string &)> hasCycle =
      [&](const std::string &u) -> bool {
    visited.insert(u);
    recursionStack.insert(u);

    for (const auto &v : adjList[u]) {
      if (recursionStack.count(v)) {
        // Cycle detected: u -> v is a back edge
        ZENITH_LOG_WARNING("Cycle detected: " + nodes_[u].name + " -> " +
                           nodes_[v].name);

        // Mark connection as feedback (break the cycle)
        for (auto &conn : connections_) {
          if (conn.sourceId.toStdString() == u &&
              conn.destId.toStdString() == v) {
            conn.isFeedback = true;
            break;
          }
        }
        return true;
      }
      if (!visited.count(v)) {
        if (hasCycle(v))
          return true;
      }
    }

    recursionStack.erase(u);
    return false;
  };

  // Run DFS on all nodes to mark feedback edges
  for (const auto &pair : nodes_) {
    if (!visited.count(pair.first)) {
      hasCycle(pair.first);
    }
  }

  // 4. Re-calculate degrees ignoring feedback edges (Kahn's Algo)
  inDegree.clear();
  adjList.clear();

  for (const auto &pair : nodes_) {
    inDegree[pair.first] = 0;
  }

  for (const auto &c : connections_) {
    if (c.isFeedback)
      continue; // Ignore feedback edges for topological sort

    std::string src = c.sourceId.toStdString();
    std::string dst = c.destId.toStdString();

    if (nodes_.count(src) && nodes_.count(dst)) {
      adjList[src].push_back(dst);
      inDegree[dst]++;
    }
  }

  // 5. Run Kahn's Algorithm
  std::vector<juce::String> processingOrder;
  std::vector<std::string> queue;

  for (const auto &pair : inDegree) {
    if (pair.second == 0)
      queue.push_back(pair.first);
  }

  size_t idx = 0;
  while (idx < queue.size()) {
    std::string u = queue[idx++];
    processingOrder.push_back(u);

    for (const auto &v : adjList[u]) {
      inDegree[v]--;
      if (inDegree[v] == 0) {
        queue.push_back(v);
      }
    }
  }

  // Verify completeness
  if (processingOrder.size() < nodes_.size()) {
    ZENITH_LOG_ERROR(
        "Topological sort failed despite cycle breaking! Disconnected graph?");
    // Fallback: Add remaining nodes (islands)
    for (const auto &pair : nodes_) {
      bool found = false;
      for (const auto &id : processingOrder)
        if (id.toStdString() == pair.first)
          found = true;
      if (!found)
        processingOrder.push_back(pair.first);
    }
  }

  // 6. Verify Integrity
  if (!verifyGraphIntegrity()) {
    ZENITH_LOG_ERROR("Graph integrity check failed after update!");
  }

  // 7. Swap Snapshot
  auto newSnapshot =
      std::make_shared<Snapshot>(nodes_, connections_, processingOrder);
  activeSnapshot_.store(newSnapshot.get(), std::memory_order_release);

  snapshotTrash_.push_back(currentSnapshot_);
  currentSnapshot_ = newSnapshot;

  while (snapshotTrash_.size() > kMaxSnapshotHistory) {
    snapshotTrash_.erase(snapshotTrash_.begin());
  }
}

bool RoutingGraph::verifyGraphIntegrity() const {
  // 1. Check for orphaned nodes (nodes with connections to non-existent
  // targets)
  for (const auto &conn : connections_) {
    if (nodes_.find(conn.sourceId.toStdString()) == nodes_.end()) {
      ZENITH_LOG_ERROR("Integrity Fail: Connection source " + conn.sourceId +
                       " does not exist.");
      return false;
    }
    if (nodes_.find(conn.destId.toStdString()) == nodes_.end()) {
      ZENITH_LOG_ERROR("Integrity Fail: Connection dest " + conn.destId +
                       " does not exist.");
      return false;
    }
  }
  return true;
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

bool RoutingGraph::connectModulation(const juce::String &sourceId,
                                     const juce::String &destId,
                                     int pluginIndex, int paramIndex,
                                     float intensity) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  const juce::ScopedLock sl(writeLock_);

  // Validate nodes
  if (nodes_.find(sourceId.toStdString()) == nodes_.end() ||
      nodes_.find(destId.toStdString()) == nodes_.end()) {
    return false;
  }

  // Check duplicates
  for (const auto &c : connections_) {
    if (c.sourceId == sourceId && c.destId == destId &&
        c.type == Connection::Type::Modulation &&
        c.targetPluginIndex == pluginIndex &&
        c.targetParamIndex == paramIndex) {
      return true;
    }
  }

  Connection c;
  c.type = Connection::Type::Modulation;
  c.sourceId = sourceId;
  c.destId = destId;
  c.gain = intensity; // Use gain as intensity
  c.targetPluginIndex = pluginIndex;
  c.targetParamIndex = paramIndex;

  connections_.push_back(c);
  updateSnapshot();
  return true;
}

//==============================================================================
// Verification
//==============================================================================

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

const std::vector<RoutingGraph::Connection> &
RoutingGraph::getConnectionsFrom(const juce::String &sourceId) const {
  // RT-SAFE: Uses atomic snapshot load
  const auto *snapshot = getSnapshot();
  static const std::vector<Connection> empty;
  if (!snapshot)
    return empty;

  auto it = snapshot->outgoingConnections.find(sourceId.toStdString());
  if (it != snapshot->outgoingConnections.end())
    return it->second;

  return empty;
}

std::vector<RoutingGraph::Connection>
RoutingGraph::getConnectionsTo(const juce::String &destId) const {
  const auto *snapshot = getSnapshot();
  if (!snapshot)
    return {};

  std::vector<Connection> result;
  result.reserve(snapshot->connections.size());
  for (const auto &c : snapshot->connections) {
    if (c.destId == destId)
      result.push_back(c);
  }
  return result;
}

const std::vector<juce::String> &RoutingGraph::getProcessingOrder() const {
  const auto *snapshot = getSnapshot();
  static const std::vector<juce::String> empty;
  if (!snapshot)
    return empty;
  return snapshot->processingOrder;
}

//==============================================================================
// Serialization (MESSAGE THREAD ONLY)
//==============================================================================

juce::var RoutingGraph::toVar() const {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  const juce::ScopedLock sl(writeLock_);
  auto *obj = new juce::DynamicObject();

  // Serialize Nodes
  juce::var nodesArray;
  for (const auto &pair : nodes_) {
    auto *nodeObj = new juce::DynamicObject();
    nodeObj->setProperty("id", pair.second.id);
    nodeObj->setProperty("name", pair.second.name);
    nodeObj->setProperty("type", (int)pair.second.type);
    nodesArray.append(nodeObj);
  }
  obj->setProperty("nodes", nodesArray);

  // Serialize Connections
  juce::var connsArray;
  for (const auto &c : connections_) {
    auto *connObj = new juce::DynamicObject();
    connObj->setProperty("source", c.sourceId);
    connObj->setProperty("dest", c.destId);
    connObj->setProperty("gain", c.gain);

    // Type specific
    connObj->setProperty(
        "type", c.type == Connection::Type::Modulation ? "mod" : "audio");
    if (c.type == Connection::Type::Modulation) {
      connObj->setProperty("pluginIdx", c.targetPluginIndex);
      connObj->setProperty("paramIdx", c.targetParamIndex);
    }

    connsArray.append(connObj);
  }
  obj->setProperty("connections", connsArray);

  return juce::var(obj);
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

          if (connObj->getProperty("type").toString() == "mod") {
            c.type = Connection::Type::Modulation;
            c.targetPluginIndex = connObj->getProperty("pluginIdx");
            c.targetParamIndex = connObj->getProperty("paramIdx");
          } else {
            c.type = Connection::Type::Audio;
          }

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

  for (const auto &c : connections_) {
    juce::ValueTree conn("Connection");
    conn.setProperty("source", c.sourceId, nullptr);
    conn.setProperty("dest", c.destId, nullptr);
    conn.setProperty("gain", c.gain, nullptr);
    conn.setProperty("type",
                     c.type == Connection::Type::Modulation ? "mod" : "audio",
                     nullptr);
    if (c.type == Connection::Type::Modulation) {
      conn.setProperty("pluginIdx", c.targetPluginIndex, nullptr);
      conn.setProperty("paramIdx", c.targetParamIndex, nullptr);
    }
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

        if (child.getProperty("type").toString() == "mod") {
          c.type = Connection::Type::Modulation;
          c.targetPluginIndex = child.getProperty("pluginIdx");
          c.targetParamIndex = child.getProperty("paramIdx");
        } else {
          c.type = Connection::Type::Audio;
        }

        connections_.push_back(c);
      }
    }
  }

  updateSnapshot();
}

} // namespace zenith
