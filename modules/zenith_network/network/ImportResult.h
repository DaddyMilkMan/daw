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

    struct ImportResult {
        std::set<std::string> modifiedMaps;
        std::set<std::string> modifiedLists;
    };

    ImportResult importUpdates(const juce::MemoryBlock& updates) {
        ImportResult result;
        juce::MemoryInputStream is(updates, false);
        if (is.getTotalLength() == 0) return result;

        is.readInt64(); // Peer ID
        uint64_t remoteCounter = (uint64_t)is.readInt64();
        if (remoteCounter > counter) counter = remoteCounter;

        int numMaps = is.readInt();
        for (int i = 0; i < numMaps; ++i) {
            juce::String name = is.readString();
            auto& map = maps[name.toStdString()];
            int numEntries = is.readInt();
            bool changed = false;
            for (int k = 0; k < numEntries; ++k) {
                juce::String key = is.readString();
                Timestamp ts = readTimestamp(is);
                LoroValue val = readValue(is);
                if (map.set(key.toStdString(), val, ts)) changed = true;
            }
            if (changed) result.modifiedMaps.insert(name.toStdString());
        }

        int numLists = is.readInt();
        for (int i = 0; i < numLists; ++i) {
            juce::String name = is.readString();
            auto& list = lists[name.toStdString()];
            int numBlocks = is.readInt();
            bool changed = false;
            for (int k = 0; k < numBlocks; ++k) {
                Timestamp id = readTimestamp(is);
                Timestamp origin = readTimestamp(is);
                LoroValue val = readValue(is);
                bool isTombstone = is.readBool();
                if (list.insert(val, id, origin)) changed = true;
                if (isTombstone) list.remove(id);
            }
            if (changed) result.modifiedLists.insert(name.toStdString());
        }
        return result;
    }

private:
    PeerID peerId;
    uint64_t counter = 0;
    std::map<std::string, LoroMap> maps;
    std::map<std::string, LoroList> lists;

    void writeTimestamp(juce::MemoryOutputStream& os, const Timestamp& ts) {
        os.writeInt64((int64_t)ts.counter);
        os.writeInt64((int64_t)ts.peer.value);
    }
    Timestamp readTimestamp(juce::MemoryInputStream& is) {
        uint64_t c = (uint64_t)is.readInt64();
        uint64_t p = (uint64_t)is.readInt64();
        return { c, { p } };
    }
    void writeValue(juce::MemoryOutputStream& os, const LoroValue& v) {
        os.writeInt((int)v.type);
        switch (v.type) {
            case LoroValue::Type::Bool: os.writeBool(v.bVal); break;
            case LoroValue::Type::Int: os.writeInt64(v.iVal); break;
            case LoroValue::Type::Float: os.writeDouble(v.fVal); break;
            case LoroValue::Type::String: os.writeString(v.sVal); break;
            case LoroValue::Type::Binary: os.writeInt((int)v.binVal.getSize()); os.write(v.binVal.getData(), v.binVal.getSize()); break;
            default: break;
        }
    }
    LoroValue readValue(juce::MemoryInputStream& is) {
        LoroValue v;
        v.type = (LoroValue::Type)is.readInt();
        switch (v.type) {
            case LoroValue::Type::Bool: v.bVal = is.readBool(); break;
            case LoroValue::Type::Int: v.iVal = is.readInt64(); break;
            case LoroValue::Type::Float: v.fVal = is.readDouble(); break;
            case LoroValue::Type::String: v.sVal = is.readString(); break;
            case LoroValue::Type::Binary: { int size = is.readInt(); v.binVal.setSize((size_t)size); is.read(v.binVal.getData(), (size_t)size); } break;
            default: break;
        }
        return v;
    }
};

} // namespace Zenith
