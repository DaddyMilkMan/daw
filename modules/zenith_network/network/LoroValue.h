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

struct LoroValue {
    enum class Type { Null, Bool, Int, Float, String, Binary };
    Type type = Type::Null;

    bool bVal = false;
    int64_t iVal = 0;
    double fVal = 0.0;
    juce::String sVal;
    juce::MemoryBlock binVal;

    LoroValue() = default;
    LoroValue(bool b) : type(Type::Bool), bVal(b) {}
    LoroValue(juce::int64 i) : type(Type::Int), iVal(i) {}
    LoroValue(double f) : type(Type::Float), fVal(f) {}
    LoroValue(const juce::String& s) : type(Type::String), sVal(s) {}

    juce::var toVar() const {
        switch (type) {
            case Type::Bool: return juce::var(bVal);
            case Type::Int: return juce::var((juce::int64)iVal);
            case Type::Float: return juce::var(fVal);
            case Type::String: return juce::var(sVal);
            case Type::Binary: return juce::var(binVal);
            default: return {};
        }
    }

    static LoroValue fromVar(const juce::var& v) {
        LoroValue lv;
        if (v.isBool()) {
            lv.type = Type::Bool;
            lv.bVal = v.operator bool();
        } else if (v.isInt() || v.isInt64()) {
            lv.type = Type::Int;
            lv.iVal = (int64_t)v.operator juce::int64();
        } else if (v.isDouble()) {
            lv.type = Type::Float;
            lv.fVal = v.operator double();
        } else if (v.isBinaryData()) {
            lv.type = Type::Binary;
            lv.binVal = *v.getBinaryData();
        } else {
            lv.type = Type::String;
            lv.sVal = v.toString();
        }
        return lv;
    }

};

/**
 * @struct RGABlock
 * @brief A block in the Replicated Growable Array for lists.
 */

} // namespace
