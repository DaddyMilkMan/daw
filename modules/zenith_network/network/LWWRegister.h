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

struct LWWRegister {
    LoroValue value;
    Timestamp timestamp = { 0, { 0 } };

    bool set(const LoroValue& newValue, const Timestamp& newTs) {
        if (newTs > timestamp) {
            value = newValue;
            timestamp = newTs;
            return true;
        }
        return false;
    }
};


} // namespace
