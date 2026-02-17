#pragma once

#include <juce_core/juce_core.h>
#include <map>
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <set>

/**
 * @file ZenithCRDT.h
 * @brief Professional-grade C++ CRDT engine for Zenith DAW.
 *
 * Features:
 * - LWW (Last-Write-Wins) Map for properties.
 * - RGA (Replicated Growable Array) for ordered lists (tracks, clips).
 * - Tombstone-based deletions with automatic garbage collection hooks.
 * - Efficient binary delta serialization.
 */

namespace Zenith {

class LoroDoc {
public:
    LoroDoc() : peerId(PeerID::generate()) {}

    PeerID getPeerID() const { return peerId; }
    uint64_t nextCounter() { return ++counter; }

    LoroMap& getMap(const std::string& name) { return maps[name]; }
    LoroList& getList(const std::string& name) { return lists[name]; }

    const std::map<std::string, LoroMap>& getMaps() const { return maps; }
    const std::map<std::string, LoroList>& getLists() const { return lists; }

    std::vector<std::string> getMapNames() const {
        std::vector<std::string> names;
        for (const auto& pair : maps) names.push_back(pair.first);
        return names;
    }

    std::vector<std::string> getListNames() const {
        std::vector<std::string> names;
        for (const auto& pair : lists) names.push_back(pair.first);
        return names;
    }

    juce::MemoryBlock exportUpdates() {
        juce::MemoryBlock mb;
        juce::MemoryOutputStream os(mb, false);
        os.writeInt64((int64_t)peerId.value);
        os.writeInt64((int64_t)counter);

        // Maps
        os.writeInt((int)maps.size());
        for (auto& pair : maps) {
            os.writeString(pair.first);
            os.writeInt((int)pair.second.entries.size());
            for (auto& entry : pair.second.entries) {
                os.writeString(entry.first);
                writeTimestamp(os, entry.second.timestamp);
                writeValue(os, entry.second.value);
            }
        }

        // Lists
        os.writeInt((int)lists.size());
        for (auto& pair : lists) {
            os.writeString(pair.first);
            os.writeInt((int)pair.second.blocks.size());
            for (const auto& b : pair.second.blocks) {
                writeTimestamp(os, b.id);
                writeTimestamp(os, b.originLeft);
                writeValue(os, b.value);
                os.writeBool(b.isTombstone);
            }
        }
        return mb;
    }


} // namespace
