/*
  ==============================================================================

    MidiControllerConflictResolver.cpp
    Implementation of MIDI controller conflict resolution

  ==============================================================================
*/

#include "MidiControllerConflictResolver.h"
#include <iostream>
#include <algorithm>

namespace zenith {

//==============================================================================
// MidiControllerConflictResolver Implementation
//==============================================================================

MidiControllerConflictResolver::MidiControllerConflictResolver() {
    std::cout << "MidiControllerConflictResolver: Initialized" << std::endl;
}

MidiControllerConflictResolver::~MidiControllerConflictResolver() {
    std::cout << "MidiControllerConflictResolver: Shut down" << std::endl;
}

//==============================================================================
std::vector<ControllerConflict> MidiControllerConflictResolver::detectConflicts(
    const std::vector<MidiControllerAssignment>& assignments) const {

    std::vector<ControllerConflict> conflicts;

    // Check each pair of assignments
    for (size_t i = 0; i < assignments.size(); ++i) {
        for (size_t j = i + 1; j < assignments.size(); ++j) {
            const auto& a1 = assignments[i];
            const auto& a2 = assignments[j];

            // Same controller and channel = conflict
            if (a1.channel == a2.channel &&
                a1.controllerNumber == a2.controllerNumber &&
                a1.isNote == a2.isNote) {

                // Same parameter = not a conflict (already checked)
                if (a1.parameterId != a2.parameterId) {
                    ControllerConflict conflict;
                    conflict.conflictId = a1.id + "_vs_" + a2.id;
                    conflict.parameterId1 = a1.parameterId;
                    conflict.parameterId2 = a2.parameterId;
                    conflict.channel = a1.channel;
                    conflict.controllerNumber = a1.controllerNumber;
                    conflict.isNote = a1.isNote;
                    conflict.detectedAt = juce::Time::getCurrentTime();
                    conflict.description = "Controller conflict between " +
                                        a1.parameterId + " and " + a2.parameterId;

                    conflicts.push_back(conflict);
                }
            }
        }
    }

    std::cout << "MidiControllerConflictResolver: Detected "
              << conflicts.size() << " conflicts" << std::endl;

    return conflicts;
}

//==============================================================================
ConflictResolutionResult MidiControllerConflictResolver::resolveConflict(
    const ControllerConflict& conflict,
    ConflictResolution strategy,
    std::vector<MidiControllerAssignment>& assignments) const {

    ConflictResolutionResult result;
    result.success = true;
    result.strategyUsed = strategy;

    switch (strategy) {
        case ConflictResolution::ReplaceExisting:
            // Remove first assignment, keep second
            assignments.erase(
                std::remove_if(assignments.begin(), assignments.end(),
                              [&conflict](const MidiControllerAssignment& a) {
                                  return a.parameterId == conflict.parameterId1;
                              }),
                assignments.end());
            result.resolvedConflicts.push_back(conflict.conflictId);
            result.description = "Replaced " + conflict.parameterId1 +
                                 " with " + conflict.parameterId2;
            break;

        case ConflictResolution::KeepExisting:
            // Keep first, block second
            result.blockedParameters.push_back(conflict.parameterId2);
            result.resolvedConflicts.push_back(conflict.conflictId);
            result.description = "Kept " + conflict.parameterId1 +
                                 ", blocked " + conflict.parameterId2;
            break;

        case ConflictResolution::Merge:
            // Both parameters can coexist (layered)
            if (canMerge(conflict)) {
                result.resolvedConflicts.push_back(conflict.conflictId);
                result.description = "Merged " + conflict.parameterId1 +
                                     " and " + conflict.parameterId2;
            } else {
                result.success = false;
                result.description = "Cannot merge these parameters";
            }
            break;

        case ConflictResolution::SplitZone:
            // Split controller into zones
            if (canSplitZone(conflict)) {
                // For simplicity, just mark as resolved
                // Real implementation would create zone assignments
                result.resolvedConflicts.push_back(conflict.conflictId);
                result.description = "Split controller into zones";
            } else {
                result.success = false;
                result.description = "Cannot split this controller";
            }
            break;

        case ConflictResolution::UserDecision:
            // Don't auto-resolve - user must decide
            result.success = false;
            result.description = "User decision required";
            break;

        case ConflictResolution::AutoBlock:
            // Block both assignments
            result.blockedParameters.push_back(conflict.parameterId1);
            result.blockedParameters.push_back(conflict.parameterId2);
            result.resolvedConflicts.push_back(conflict.conflictId);
            result.description = "Blocked both " + conflict.parameterId1 +
                                 " and " + conflict.parameterId2;
            break;
    }

    return result;
}

//==============================================================================
ConflictResolutionResult MidiControllerConflictResolver::resolveAllConflicts(
    std::vector<MidiControllerAssignment>& assignments,
    ConflictResolution strategy) const {

    ConflictResolutionResult result;
    result.success = true;
    result.strategyUsed = strategy;

    auto conflicts = detectConflicts(assignments);

    if (conflicts.empty()) {
        result.description = "No conflicts to resolve";
        return result;
    }

    for (const auto& conflict : conflicts) {
        auto conflictResult = resolveConflict(conflict, strategy, assignments);
        if (!conflictResult.success) {
            result.success = false;
        }
        result.resolvedConflicts.insert(result.resolvedConflicts.end(),
                                        conflictResult.resolvedConflicts.begin(),
                                        conflictResult.resolvedConflicts.end());
        result.blockedParameters.insert(result.blockedParameters.end(),
                                        conflictResult.blockedParameters.begin(),
                                        conflictResult.blockedParameters.end());
    }

    std::cout << "MidiControllerConflictResolver: Resolved "
              << result.resolvedConflicts.size() << " conflicts" << std::endl;

    return result;
}

//==============================================================================
double MidiControllerConflictResolver::calculateConflictSeverity(
    const ControllerConflict& conflict) const {

    // Base severity on whether it's notes (more critical) or CC
    double baseSeverity = conflict.isNote ? 7.0 : 5.0;

    return baseSeverity;
}

juce::String MidiControllerConflictResolver::getConflictDescription(
    const ControllerConflict& conflict) const {

    juce::String desc = conflict.description;

    desc += "\n  Controller: " +
             (conflict.isNote ? "Note" : "CC") +
             juce::String(conflict.controllerNumber);

    desc += "\n  Channel: " + juce::String(conflict.channel);

    desc += "\n  Severity: " + juce::String(calculateConflictSeverity(conflict), 1);

    return desc;
}

//==============================================================================
// Private Methods
//==============================================================================
bool MidiControllerConflictResolver::canMerge(const ControllerConflict& conflict) const {
    // Can merge if parameters are compatible (e.g., volume levels)
    // For simplicity, assume all parameters can be merged
    // In real implementation, would check parameter types
    return true;
}

bool MidiControllerConflictResolver::canSplitZone(const ControllerConflict& conflict) const {
    // Can split if controller has sufficient resolution (e.g., pitch bend > 14-bit)
    // For CC (7-bit), splitting reduces resolution
    // For notes, splitting doesn't make sense
    return !conflict.isNote;
}

std::vector<int> MidiControllerConflictResolver::calculateZones(
    int controllerNumber) const {

    std::vector<int> zones;

    // For simplicity, split into 2 zones
    // Zone 1: 0-63, Zone 2: 64-127
    zones.push_back(63);
    zones.push_back(64);

    return zones;
}

} // namespace zenith
