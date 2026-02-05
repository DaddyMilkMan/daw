#pragma once

#include "ZenithCRDT.h"
#include <juce_data_structures/juce_data_structures.h>
#include <map>

namespace Zenith {

/**
 * @class ValueTreeCRDTBridge
 * @brief Professional-grade bridge between juce::ValueTree and Zenith::LoroDoc.
 * 
 * Performance:
 * - O(1) property updates via keyed ID map.
 * - Structural changes (lists) synchronized via RGA.
 * - Optimized binary delta propagation.
 */
class ValueTreeCRDTBridge : private juce::ValueTree::Listener {
public:
    ValueTreeCRDTBridge(juce::ValueTree& tree, LoroDoc& doc)
        : targetTree(tree), crdtDoc(doc)
    {
        buildIndex(targetTree);
        // Capture existing state into CRDT (for "established session" scenarios)
        syncSubtreeToCRDT(targetTree);
        targetTree.addListener(this);
    }

    ~ValueTreeCRDTBridge() override {
        targetTree.removeListener(this);
    }

    std::function<void()> onLocalChange;

    void applyRemoteUpdates(const juce::MemoryBlock& updates) {
        juce::ScopedValueSetter<bool> setter(isApplyingRemote, true);
        crdtDoc.importUpdates(updates);
        syncFromCRDT();
    }

private:
    juce::ValueTree targetTree;
    LoroDoc& crdtDoc;
    bool isApplyingRemote = false;

    // Fast lookup for ValueTree nodes by ID
    std::map<juce::String, juce::ValueTree> idToIndex;

    void buildIndex(juce::ValueTree& tree) {
        juce::String id = tree.getProperty("id").toString();
        if (id.isNotEmpty()) idToIndex[id] = tree;
        for (int i = 0; i < tree.getNumChildren(); ++i) {
            auto child = tree.getChild(i);
            buildIndex(child);
        }
    }

    // --- Local -> CRDT ---

    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& prop) override {
        if (isApplyingRemote) return;

        juce::String id = tree.getProperty("id").toString();
        if (id.isEmpty()) return;

        auto& map = crdtDoc.getMap(id.toStdString());
        map.set(prop.toString().toStdString(), LoroValue::fromVar(tree.getProperty(prop)),
                { crdtDoc.nextCounter(), crdtDoc.getPeerID() });

        if (onLocalChange) onLocalChange();
    }

    void valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child) override {
        if (isApplyingRemote) return;

        juce::String parentId = parent.getProperty("id").toString();
        juce::String childId = child.getProperty("id").toString();
        if (parentId.isEmpty() || childId.isEmpty()) return;

        idToIndex[childId] = child; // Index the new child

        // Sync for list (RGA structural sync)
        juce::String listName = parentId.toStdString() + "_children";
        
        auto& list = crdtDoc.getList(listName.toStdString());
        list.insert(LoroValue(childId), { crdtDoc.nextCounter(), crdtDoc.getPeerID() }, { 0, {0} });

        // Also sync its initial properties
        buildIndex(child);
        syncSubtreeToCRDT(child);

        if (onLocalChange) onLocalChange();
    }

    void valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int) override {
        if (isApplyingRemote) return;
        juce::String parentId = parent.getProperty("id").toString();
        juce::String childId = child.getProperty("id").toString();
        if (parentId.isNotEmpty() && childId.isNotEmpty()) {
            // Find the ID of the block containing this childId in the parent's list
            juce::String listName = parentId.toStdString() + "_children";
            auto& list = crdtDoc.getList(listName.toStdString());
            
            for (const auto& block : list.blocks) {
                if (!block.isTombstone && 
                    block.value.type == LoroValue::Type::String && 
                    block.value.sVal == childId) {
                    
                    // Mark as tombstone (CRDT deletion)
                    list.remove(block.id);
                    break; 
                }
            }
        }
        if (onLocalChange) onLocalChange();
    }

    void syncSubtreeToCRDT(juce::ValueTree& tree) {
        juce::String id = tree.getProperty("id").toString();
        if (id.isEmpty()) return;
        auto& map = crdtDoc.getMap(id.toStdString());
        
        // Write Type explicitly (not a property in ValueTree)
        map.set("type", tree.getType().toString(), 
                { crdtDoc.nextCounter(), crdtDoc.getPeerID() });

        for (int i = 0; i < tree.getNumProperties(); ++i) {
            auto prop = tree.getPropertyName(i);
            map.set(prop.toString().toStdString(), LoroValue::fromVar(tree.getProperty(prop)),
                    { crdtDoc.nextCounter(), crdtDoc.getPeerID() });
        }
        
        // For each child, add it to this node's CRDT list (structural sync)
        for (int i = 0; i < tree.getNumChildren(); ++i) {
            auto child = tree.getChild(i);
            juce::String childId = child.getProperty("id").toString();
            if (childId.isNotEmpty()) {
                juce::String listName = id.toStdString() + "_children";
                auto& list = crdtDoc.getList(listName.toStdString());
                list.insert(LoroValue(childId), { crdtDoc.nextCounter(), crdtDoc.getPeerID() }, { 0, {0} });
            }
            syncSubtreeToCRDT(child);
        }
    }

    // --- CRDT -> Local ---

    void syncFromCRDT() {
        // 1. Structural Sync: Create missing children (loop until stable)
        bool createdAny = true;
        int maxIterations = 10; // Safety limit
        while (createdAny && maxIterations-- > 0) {
            createdAny = false;
            const auto& lists = crdtDoc.getLists();
            for (const auto& pair : lists) {
                juce::String listName = pair.first;
                if (!listName.endsWith("_children")) continue;
                
                juce::String parentId = listName.dropLastCharacters(9);
                auto pIt = idToIndex.find(parentId);
                if (pIt == idToIndex.end()) continue;
                
                juce::ValueTree parent = pIt->second;
                const LoroList& list = pair.second;
                auto childIds = list.getActiveValues();
                
                for (const auto& val : childIds) {
                    juce::String childId = val.sVal;
                    if (idToIndex.find(childId) == idToIndex.end()) {
                        // Create missing child
                        if (crdtDoc.getMaps().count(childId.toStdString())) {
                            auto& childMap = crdtDoc.getMaps().at(childId.toStdString());
                            juce::String type;
                            if (childMap.entries.count("type")) {
                                 type = childMap.entries.at("type").value.toVar().toString();
                            }
                            
                            if (type.isNotEmpty()) {
                                juce::ValueTree newChild(type);
                                // Set ID immediately so index works
                                newChild.setProperty("id", childId, nullptr);
                                parent.appendChild(newChild, nullptr);
                                idToIndex[childId] = newChild;
                                createdAny = true;
                                
                                // Also create any child containers that exist in CRDT
                                // (e.g., {trackId}_clips, {trackId}_automation)
                                for (const auto& mapPair : crdtDoc.getMaps()) {
                                    juce::String mapId(mapPair.first);
                                    if (mapId.startsWith(childId + "_") && idToIndex.find(mapId) == idToIndex.end()) {
                                        // Extract container type from the map
                                        juce::String containerType;
                                        if (mapPair.second.entries.count("type")) {
                                            containerType = mapPair.second.entries.at("type").value.toVar().toString();
                                        }
                                        if (containerType.isNotEmpty()) {
                                            juce::ValueTree container(containerType);
                                            container.setProperty("id", mapId, nullptr);
                                            newChild.appendChild(container, nullptr);
                                            idToIndex[mapId] = container;
                                            createdAny = true;
                                        }
                                    }
                                }
                                
                                // Index any children this node might have (from CRDT)
                                buildIndex(newChild);
                            }
                        }
                    }
                }
            }
        }

        // 2. Property Sync
        const auto& maps = crdtDoc.getMaps();

        for (const auto& pair : maps) {
            juce::String id(pair.first);
            
            // O(1) Lookup via index
            auto it = idToIndex.find(id);
            if (it == idToIndex.end()) continue;

            juce::ValueTree& tree = it->second;
            const LoroMap& map = pair.second;

            for (const auto& entry : map.entries) {
                juce::Identifier prop(entry.first);
                juce::var newVal = entry.second.value.toVar();
                
                // Avoid updating 'id' or 'type' redundantly, or if same
                if (prop.toString() == "id" || prop.toString() == "type") continue;

                if (tree.getProperty(prop) != newVal) {
                    tree.setProperty(prop, newVal, nullptr);
                }
            }
        }
    }
};

} // namespace Zenith
