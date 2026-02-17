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

class LoroList {
public:
    bool insert(const LoroValue& val, Timestamp id, Timestamp origin) {
        if (seenIds.count(id)) return false;
        seenIds.insert(id);

        RGABlock block { id, origin, val, false };

        // Find position to insert
        auto it = blocks.begin();
        if (!(origin == Timestamp{0, {0}})) {
            while (it != blocks.end() && !(it->id == origin)) ++it;
            if (it != blocks.end()) ++it; // Insert AFTER origin
        }

        // Handle concurrent siblings
        while (it != blocks.end() && it->originLeft == origin && id < it->id) {
            ++it;
        }

        blocks.insert(it, block);
        return true;
    }

    void remove(Timestamp id) {
        for (auto& b : blocks) {
            if (b.id == id) {
                b.isTombstone = true;
                return;
            }
        }
    }

    std::vector<LoroValue> getActiveValues() const {
        std::vector<LoroValue> result;
        for (const auto& b : blocks) {
            if (!b.isTombstone) result.push_back(b.value);
        }
        return result;
    }

    std::vector<RGABlock> blocks;
    std::set<Timestamp> seenIds;
};


} // namespace
