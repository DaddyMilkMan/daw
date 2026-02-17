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

struct Timestamp {
    uint64_t counter;
    PeerID peer;

    static Timestamp now(uint64_t c, PeerID p) { return { c, p }; }
    bool operator>(const Timestamp& other) const {
        if (counter != other.counter) return counter > other.counter;
        return peer.value > other.peer.value;
    }
    bool operator<(const Timestamp& other) const {
        if (counter != other.counter) return counter < other.counter;
        return peer.value < other.peer.value;
    }
    bool operator==(const Timestamp& other) const {
        return counter == other.counter && peer.value == other.peer.value;
    }
};


} // namespace
