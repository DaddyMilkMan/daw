/*
  ==============================================================================

    MidiControllerConflictResolver.h
    Created: 2026-02-18
    Author:  Zenith DAW - Month 6: MIDI Safety (Gap #8)

    Resolves conflicting MIDI controller assignments.

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <vector>
#include <map>

namespace zenith {

//==============================================================================
/**
 * @brief Conflict resolution strategy
 */
enum class ConflictResolution {
    ReplaceExisting,             // Remove old, keep new
    KeepExisting,                // Keep old, ignore new
    Merge,                       // Merge both (layer)
    SplitZone,                  // Split into zones
    UserDecision,                // Ask user to decide
    AutoBlock                    // Automatically block conflicts
};

//==============================================================================
/**
 * @brief Controller conflict
 */
struct ControllerConflict {
    juce::String conflictId;       // Unique conflict ID
    juce::String parameterId1;     // First parameter involved
    juce::String parameterId2;     // Second parameter involved
    int channel = 0;               // MIDI channel
    int controllerNumber = 0;       // CC/note number
    bool isNote = false;            // Note vs CC
    juce::Time detectedAt;         // When conflict was detected
    juce::String description;

    juce::String toString() const {
        return (isNote ? "Note" : "CC") +
               juce::String(controllerNumber) +
               "[ch" + juce::String(channel) + "]: " +
               parameterId1 + " <-> " + parameterId2;
    }
};

//==============================================================================
/**
 * @brief Conflict resolution result
 */
struct ConflictResolutionResult {
    bool success = true;
    ConflictResolution strategyUsed;
    std::vector<juce::String> resolvedConflicts;
    std::vector<juce::String> blockedParameters;
    juce::String description;

    juce::String toString() const {
        juce::String strategyStr;
        switch (strategyUsed) {
            case ConflictResolution::ReplaceExisting: strategyStr = "Replace"; break;
            case ConflictResolution::KeepExisting: strategyStr = "Keep"; break;
            case ConflictResolution::Merge: strategyStr = "Merge"; break;
            case ConflictResolution::SplitZone: strategyStr = "Split"; break;
            case ConflictResolution::UserDecision: strategyStr = "User"; break;
            case ConflictResolution::AutoBlock: strategyStr = "Block"; break;
        }
        return "Resolution: " + strategyStr + " (" +
               juce::String(resolvedConflicts.size()) + " resolved)";
    }
};

//==============================================================================
/**
 * @brief Resolves MIDI controller assignment conflicts
 *
 * Features:
 * - Conflict detection
 * - Multiple resolution strategies
 * - Visual conflict indication
 * - Manual resolution UI
 * - Zone splitting
 */
class MidiControllerConflictResolver {
public:
    //==========================================================================
    MidiControllerConflictResolver();
    ~MidiControllerConflictResolver();

    //==========================================================================
    /**
     * @brief Detect conflicts between assignments
     * @param assignments All assignments
     * @return List of conflicts found
     */
    std::vector<ControllerConflict> detectConflicts(
        const std::vector<MidiControllerAssignment>& assignments) const;

    //==========================================================================
    /**
     * @brief Resolve conflict using strategy
     * @param conflict Conflict to resolve
     * @param strategy Resolution strategy
     * @param assignments Assignments list (will be modified)
     * @return Resolution result
     */
    ConflictResolutionResult resolveConflict(
        const ControllerConflict& conflict,
        ConflictResolution strategy,
        std::vector<MidiControllerAssignment>& assignments) const;

    //==========================================================================
    /**
     * @brief Resolve all conflicts
     * @param assignments Assignments list
     * @param strategy Default strategy to use
     * @return Resolution result
     */
    ConflictResolutionResult resolveAllConflicts(
        std::vector<MidiControllerAssignment>& assignments,
        ConflictResolution strategy = ConflictResolution::UserDecision) const;

    //==========================================================================
    /**
     * @brief Set default resolution strategy
     * @param strategy Default strategy
     */
    void setDefaultStrategy(ConflictResolution strategy) {
        defaultStrategy_ = strategy;
    }

    //==========================================================================
    /**
     * @brief Enable/disable auto-resolution
     * @param enable true to automatically resolve conflicts
     */
    void setAutoResolutionEnabled(bool enable) {
        autoResolve_ = enable;
    }

    //==========================================================================
    /**
     * @brief Get conflict severity score
     * @param conflict Conflict to score
     * @return Severity 0-10
     */
    double calculateConflictSeverity(const ControllerConflict& conflict) const;

    //==========================================================================
    /**
     * @brief Get visual representation of conflict (for UI)
     * @param conflict Conflict to describe
     * @return Human-readable description
     */
    juce::String getConflictDescription(const ControllerConflict& conflict) const;

private:
    //==========================================================================
    bool canMerge(const ControllerConflict& conflict) const;
    bool canSplitZone(const ControllerConflict& conflict) const;
    std::vector<int> calculateZones(int controllerNumber) const;

    //==========================================================================
    // Default strategy
    ConflictResolution defaultStrategy_ = ConflictResolution::UserDecision;

    // Auto-resolution
    bool autoResolve_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiControllerConflictResolver)
};

//==============================================================================
/**
 * @brief Singleton accessor for MIDI controller conflict resolver
 */
class MidiControllerConflictResolverHolder {
public:
    static MidiControllerConflictResolver& getInstance() {
        static MidiControllerConflictResolver instance;
        return instance;
    }

    MidiControllerConflictResolverHolder(const MidiControllerConflictResolverHolder&) = delete;
    MidiControllerConflictResolverHolder& operator=(const MidiControllerConflictResolverHolder&) = delete;

private:
    MidiControllerConflictResolverHolder() = default;
    ~MidiControllerConflictResolverHolder() = default;
};

} // namespace zenith
