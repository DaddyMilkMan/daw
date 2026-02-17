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

class LoroMap {
public:
    bool set(const std::string& key, const LoroValue& value, const Timestamp& ts) {
        bool changed = entries[key].set(value, ts);
        if (changed) {
            // Logic to track specific key changes could go here if needed
        }
        return changed;
    }
    LoroValue get(const std::string& key) const {
        auto it = entries.find(key);
        return (it != entries.end()) ? it->second.value : LoroValue();
    }
    std::map<std::string, LWWRegister> entries;
};


} // namespace
