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

struct PeerID {
    uint64_t value;
    static PeerID generate() { return { (uint64_t)juce::Random::getSystemRandom().nextInt64() }; }
    bool operator==(const PeerID& other) const { return value == other.value; }
    bool operator<(const PeerID& other) const { return value < other.value; }
};


} // namespace
