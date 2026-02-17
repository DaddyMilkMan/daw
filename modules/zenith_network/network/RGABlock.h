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

struct RGABlock {
    Timestamp id;
    Timestamp originLeft; // Null if at start
    LoroValue value;
    bool isTombstone = false;

    bool operator<(const RGABlock& other) const {
        // RGA ordering:
        // 1. Successors follow their origin
        // 2. Siblings are ordered by ID descending (converts to total order)
        if (originLeft == other.originLeft) return id > other.id;
        return false; // Complex sort handled by RGA class
    }
};


} // namespace
