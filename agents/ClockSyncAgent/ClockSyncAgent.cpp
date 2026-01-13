/**
 * @file ClockSyncAgent.cpp
 * @brief Implementation of ClockSyncAgent
 */

#include "ClockSyncAgent.h"
#include <sstream>
#include <iomanip>
#include <cmath>

namespace zenith {

ClockSyncAgent::ClockSyncAgent() {
}

ClockSyncAgent::~ClockSyncAgent() {
}

void ClockSyncAgent::initialize(double sampleRate) {
    sampleRate_ = sampleRate;
    
    // Reset sync state
    driftSamples_.store(0);
    syncStatus_.store(SyncStatus::Unlocked);
    lastExternalTimestamp_.store(0);
    lastLocalSample_.store(0);
}

void ClockSyncAgent::setSyncSource(SyncSource source) {
    // MESSAGE THREAD ONLY
    syncSource_ = source;
    
    // Reset sync state when changing source
    syncStatus_.store(SyncStatus::Unlocked);
    driftSamples_.store(0);
}

void ClockSyncAgent::updateSync(int64_t externalTimestamp, int64_t localSamplePosition) {
    // AUDIO THREAD SAFE - RT-safe operations only
    
    if (syncSource_ == SyncSource::Internal) {
        syncStatus_.store(SyncStatus::Locked);
        driftSamples_.store(0);
        return;
    }
    
    // Calculate drift
    int64_t lastExternal = lastExternalTimestamp_.load();
    int64_t lastLocal = lastLocalSample_.load();
    
    if (lastExternal != 0 && lastLocal != 0) {
        int64_t externalDelta = externalTimestamp - lastExternal;
        int64_t localDelta = localSamplePosition - lastLocal;
        int64_t drift = localDelta - externalDelta;
        
        driftSamples_.store(drift);
        updateSyncStatus();
    }
    
    // Update last known positions
    lastExternalTimestamp_.store(externalTimestamp);
    lastLocalSample_.store(localSamplePosition);
}

int64_t ClockSyncAgent::getDriftSamples() const {
    return driftSamples_.load();
}

SyncStatus ClockSyncAgent::getSyncStatus() const {
    return syncStatus_.load();
}

double ClockSyncAgent::getDriftMs() const {
    int64_t driftSamples = driftSamples_.load();
    return (static_cast<double>(driftSamples) / sampleRate_) * 1000.0;
}

std::string ClockSyncAgent::generateStatusReport() {
    // MESSAGE THREAD ONLY - can allocate and format strings
    
    std::ostringstream report;
    report << std::fixed << std::setprecision(3);
    
    report << "=== Clock Synchronization Status ===\n";
    report << "Sample Rate: " << sampleRate_ << " Hz\n";
    
    // Sync source
    report << "Sync Source: ";
    switch (syncSource_) {
        case SyncSource::Internal:    report << "Internal\n"; break;
        case SyncSource::MidiClock:   report << "MIDI Clock\n"; break;
        case SyncSource::MTC:         report << "MIDI Time Code\n"; break;
        case SyncSource::WordClock:   report << "Word Clock\n"; break;
        case SyncSource::NetworkPTP:  report << "Network PTP\n"; break;
        case SyncSource::AbletonLink: report << "Ableton Link\n"; break;
    }
    
    // Sync status
    report << "Status: ";
    switch (syncStatus_.load()) {
        case SyncStatus::Unlocked: report << "❌ Unlocked\n"; break;
        case SyncStatus::Locking:  report << "🔄 Locking...\n"; break;
        case SyncStatus::Locked:   report << "✅ Locked\n"; break;
        case SyncStatus::Drifting: report << "⚠️  Drifting\n"; break;
        case SyncStatus::Lost:     report << "❌ Lost\n"; break;
    }
    
    // Drift information
    int64_t drift = driftSamples_.load();
    double driftMs = getDriftMs();
    report << "Drift: " << drift << " samples (" << driftMs << " ms)\n";
    
    if (std::abs(drift) > kDriftToleranceSamples) {
        report << "\n⚠️  WARNING: Drift exceeds tolerance\n";
        report << "Consider resyncing or checking clock source\n";
    }
    
    return report.str();
}

void ClockSyncAgent::updateSyncStatus() {
    // Called from audio thread - RT-safe
    int64_t drift = std::abs(driftSamples_.load());
    
    if (drift == 0) {
        syncStatus_.store(SyncStatus::Locked);
    } else if (drift < kDriftToleranceSamples) {
        syncStatus_.store(SyncStatus::Locked);
    } else if (drift < kDriftToleranceSamples * 2) {
        syncStatus_.store(SyncStatus::Drifting);
    } else {
        syncStatus_.store(SyncStatus::Lost);
    }
}

} // namespace zenith
