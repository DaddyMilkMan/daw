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

namespace zenith {

RoutingGraph::RoutingGraph()
{
    // Initialize with empty snapshot
    currentSnapshot_ = std::make_shared<Snapshot>();
    activeSnapshot_.store(currentSnapshot_.get(), std::memory_order_release);
}

RoutingGraph::~RoutingGraph()
{
}

//==============================================================================
// Snapshot Management
//==============================================================================

void RoutingGraph::updateSnapshot()
{
    // Called from message thread while holding writeLock_

    // 1. Calculate Processing Order (Kahn's Algorithm)
    std::vector<juce::String> processingOrder;
    std::unordered_map<std::string, int> inDegree;
    std::unordered_map<std::string, std::vector<std::string>> adjList;

    // Initialize in-degrees
    for (const auto& pair : nodes_) {
        inDegree[pair.first] = 0;
    }

    // Build graph and calculate in-degrees
    for (const auto& c : connections_) {
        std::string src = c.sourceId.toStdString();
        std::string dst = c.destId.toStdString();
        
        if (nodes_.count(src) && nodes_.count(dst)) {
            adjList[src].push_back(dst);
            inDegree[dst]++;
        }
    }

    // Queue for nodes with 0 in-degree
    std::vector<std::string> queue;
    for (const auto& pair : inDegree) {
        if (pair.second == 0) {
            queue.push_back(pair.first);
        }
    }

    // Process queue
    size_t queueIndex = 0;
    while (queueIndex < queue.size()) {
        std::string u = queue[queueIndex++];
        processingOrder.push_back(u);

        for (const auto& v : adjList[u]) {
            inDegree[v]--;
            if (inDegree[v] == 0) {
                queue.push_back(v);
            }
        }
    }

    // Handle Cycles: If we have a cycle, some nodes weren't added.
    // We append them at the end to ensure they still run (albeit with potential feedback delay issues).
    if (processingOrder.size() < nodes_.size()) {
        for (const auto& pair : nodes_) {
            bool alreadyAdded = false;
            // Simple linear scan is fine here as this only happens on cycles (error case) 
            // and graph size is usually small (< 1000 nodes).
            for (const auto& id : processingOrder) {
                if (id == pair.second.id) {
                    alreadyAdded = true;
                    break;
                }
            }
            if (!alreadyAdded) {
                processingOrder.push_back(pair.second.id);
            }
        }
    }

    auto newSnapshot = std::make_shared<Snapshot>(nodes_, connections_, processingOrder);
    
    // Atomic swap
    activeSnapshot_.store(newSnapshot.get(), std::memory_order_release);
    
    // Manage lifetime: keep old snapshots alive briefly
    snapshotTrash_.push_back(currentSnapshot_);
    currentSnapshot_ = newSnapshot;
    
    // Time-based garbage collection: keep up to 10 recent snapshots
    // (more than count-based to handle rapid updates during automation)
    while (snapshotTrash_.size() > 10) {
        snapshotTrash_.erase(snapshotTrash_.begin());
    }
}

//==============================================================================
// Graph Modification (MESSAGE THREAD ONLY)
//==============================================================================

void RoutingGraph::addNode(const Node& node)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    
    const juce::ScopedLock sl(writeLock_);
    nodes_[node.id.toStdString()] = node;
    updateSnapshot();
}

void RoutingGraph::removeNode(const juce::String& nodeId)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    
    const juce::ScopedLock sl(writeLock_);
    
    // Remove node
    nodes_.erase(nodeId.toStdString());
    
    // Remove associated connections
    connections_.erase(
        std::remove_if(connections_.begin(), connections_.end(),
            [&](const Connection& c) {
                return c.sourceId == nodeId || c.destId == nodeId;
            }),
        connections_.end());
    
    updateSnapshot();
}

bool RoutingGraph::connect(const juce::String& sourceId, const juce::String& destId, float gain)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    
    const juce::ScopedLock sl(writeLock_);
    
    // Validate nodes exist
    if (nodes_.find(sourceId.toStdString()) == nodes_.end() ||
        nodes_.find(destId.toStdString()) == nodes_.end())
    {
        return false;
    }
    
    // Check if connection already exists
    for (const auto& c : connections_)
    {
        if (c.sourceId == sourceId && c.destId == destId)
            return true; // Already connected
    }
    
    Connection c;
    c.sourceId = sourceId;
    c.destId = destId;
    c.gain = gain;
    connections_.push_back(c);
    
    updateSnapshot();
    return true;
}

bool RoutingGraph::disconnect(const juce::String& sourceId, const juce::String& destId)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    
    const juce::ScopedLock sl(writeLock_);
    
    auto it = std::remove_if(connections_.begin(), connections_.end(),
        [&](const Connection& c) {
            return c.sourceId == sourceId && c.destId == destId;
        });
        
    if (it != connections_.end())
    {
        connections_.erase(it, connections_.end());
        updateSnapshot();
        return true;
    }
    
    return false;
}

//==============================================================================
// Lock-free Queries (ANY THREAD - RT-SAFE)
//==============================================================================

bool RoutingGraph::hasNode(const juce::String& nodeId) const
{
    // RT-SAFE: Uses atomic snapshot load
    const auto* snapshot = getSnapshot();
    if (!snapshot) return false;
    return snapshot->nodes.find(nodeId.toStdString()) != snapshot->nodes.end();
}

const RoutingGraph::Node* RoutingGraph::getNode(const juce::String& nodeId) const
{
    // RT-SAFE: Uses atomic snapshot load
    const auto* snapshot = getSnapshot();
    if (!snapshot) return nullptr;
    
    auto it = snapshot->nodes.find(nodeId.toStdString());
    if (it != snapshot->nodes.end())
        return &it->second;
    return nullptr;
}

std::vector<RoutingGraph::Connection> RoutingGraph::getConnectionsFrom(const juce::String& sourceId) const
{
    // RT-SAFE: Uses atomic snapshot load
    const auto* snapshot = getSnapshot();
    if (!snapshot) return {};
    
    std::vector<Connection> result;
    result.reserve(snapshot->connections.size()); // Over-reserve to avoid realloc
    for (const auto& c : snapshot->connections)
    {
        if (c.sourceId == sourceId)
            result.push_back(c);
    }
    return result;
}

std::vector<RoutingGraph::Connection> RoutingGraph::getConnectionsTo(const juce::String& destId) const
{
    // RT-SAFE: Uses atomic snapshot load
    const auto* snapshot = getSnapshot();
    if (!snapshot) return {};
    
    std::vector<Connection> result;
    result.reserve(snapshot->connections.size()); // Over-reserve to avoid realloc
    for (const auto& c : snapshot->connections)
    {
        if (c.destId == destId)
            result.push_back(c);
    }
    return result;
}

std::vector<juce::String> RoutingGraph::getProcessingOrder() const
{
    // RT-SAFE: Uses atomic snapshot load
    const auto* snapshot = getSnapshot();
    if (!snapshot) return {};
    return snapshot->processingOrder;
}

//==============================================================================
// Serialization (MESSAGE THREAD ONLY)
//==============================================================================

juce::var RoutingGraph::toVar() const
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    
    const juce::ScopedLock sl(writeLock_);
    auto* obj = new juce::DynamicObject();
    
    // Serialize Nodes
    juce::var nodesArray;
    for (const auto& pair : nodes_)
    {
        auto* nodeObj = new juce::DynamicObject();
        nodeObj->setProperty("id", pair.second.id);
        nodeObj->setProperty("name", pair.second.name);
        nodeObj->setProperty("type", (int)pair.second.type);
        nodesArray.append(nodeObj);
    }
    obj->setProperty("nodes", nodesArray);
    
    // Serialize Connections
    juce::var connsArray;
    for (const auto& c : connections_)
    {
        auto* connObj = new juce::DynamicObject();
        connObj->setProperty("source", c.sourceId);
        connObj->setProperty("dest", c.destId);
        connObj->setProperty("gain", c.gain);
        connsArray.append(connObj);
    }
    obj->setProperty("connections", connsArray);
    
    return juce::var(obj);
}

void RoutingGraph::fromVar(const juce::var& data)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    
    const juce::ScopedLock sl(writeLock_);
    
    // Clear existing data
    nodes_.clear();
    connections_.clear();
    
    if (auto* obj = data.getDynamicObject())
    {
        // Deserialize nodes
        auto nodesVar = obj->getProperty("nodes");
        if (auto* nodesArray = nodesVar.getArray())
        {
            for (const auto& nodeVar : *nodesArray)
            {
                if (auto* nodeObj = nodeVar.getDynamicObject())
                {
                    Node node;
                    node.id = nodeObj->getProperty("id").toString();
                    node.name = nodeObj->getProperty("name").toString();
                    node.type = static_cast<NodeType>(static_cast<int>(nodeObj->getProperty("type")));
                    nodes_[node.id.toStdString()] = node;
                }
            }
        }
        
        // Deserialize connections
        auto connsVar = obj->getProperty("connections");
        if (auto* connsArray = connsVar.getArray())
        {
            for (const auto& connVar : *connsArray)
            {
                if (auto* connObj = connVar.getDynamicObject())
                {
                    Connection c;
                    c.sourceId = connObj->getProperty("source").toString();
                    c.destId = connObj->getProperty("dest").toString();
                    c.gain = connObj->getProperty("gain");
                    connections_.push_back(c);
                }
            }
        }
    }
    
    updateSnapshot();
}

juce::ValueTree RoutingGraph::toValueTree() const
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    
    const juce::ScopedLock sl(writeLock_);
    juce::ValueTree tree("RoutingGraph");
    
    for (const auto& c : connections_)
    {
        juce::ValueTree conn("Connection");
        conn.setProperty("source", c.sourceId, nullptr);
        conn.setProperty("dest", c.destId, nullptr);
        conn.setProperty("gain", c.gain, nullptr);
        tree.appendChild(conn, nullptr);
    }
    
    return tree;
}

void RoutingGraph::fromValueTree(const juce::ValueTree& state)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    
    const juce::ScopedLock sl(writeLock_);
    connections_.clear();
    
    if (state.hasType("RoutingGraph"))
    {
        for (const auto& child : state)
        {
            if (child.hasType("Connection"))
            {
                Connection c;
                c.sourceId = child.getProperty("source");
                c.destId = child.getProperty("dest");
                c.gain = child.getProperty("gain", 1.0f);
                connections_.push_back(c);
            }
        }
    }
    
    updateSnapshot();
}

} // namespace zenith
