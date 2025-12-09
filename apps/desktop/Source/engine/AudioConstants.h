/*
  ==============================================================================

    AudioConstants.h
    Centralized audio processing constants for Zenith DAW

  ==============================================================================
*/

#pragma once

namespace zenith {

/**
 * @brief Centralized audio processing constants
 * 
 * Defines common audio processing parameters used throughout the DAW.
 * Using named constants improves readability and maintainability.
 */
namespace AudioConstants {

    // ==========================================================================
    // Tempo / Time
    // ==========================================================================
    
    /** Minimum allowed tempo in BPM */
    constexpr double kMinTempo = 20.0;
    
    /** Maximum allowed tempo in BPM */
    constexpr double kMaxTempo = 999.0;
    
    /** Default project tempo in BPM */
    constexpr double kDefaultTempo = 120.0;

    // ==========================================================================
    // MIDI
    // ==========================================================================
    
    /** Minimum MIDI note number (C-2) */
    constexpr int kMinMidiNote = 0;
    
    /** Maximum MIDI note number (G8) */
    constexpr int kMaxMidiNote = 127;
    
    /** Minimum MIDI velocity */
    constexpr int kMinVelocity = 0;
    
    /** Maximum MIDI velocity */
    constexpr int kMaxVelocity = 127;
    
    /** Default MIDI velocity */
    constexpr int kDefaultVelocity = 100;
    
    /** Minimum note length in beats */
    constexpr double kMinNoteLengthBeats = 0.01;

    // ==========================================================================
    // Buffers / PDC
    // ==========================================================================
    
    /** Maximum PDC compensation buffer length in seconds */
    constexpr double kMaxPdcBufferSeconds = 2.0;
    
    /** Number of snapshot history entries to keep for garbage collection */
    constexpr size_t kSnapshotHistorySize = 5;

    // ==========================================================================
    // Clip / Timeline
    // ==========================================================================
    
    /** Minimum clip length in beats */
    constexpr double kMinClipLengthBeats = 0.25;
    
    /** Default new clip length in beats (1 bar in 4/4) */
    constexpr double kDefaultClipLengthBeats = 4.0;

    // ==========================================================================
    // Sample Rate
    // ==========================================================================
    
    /** Default sample rate in Hz */
    constexpr double kDefaultSampleRate = 44100.0;

    // ==========================================================================
    // UI / Mixer
    // ==========================================================================
    
    /** Default mixer strip width in pixels */
    constexpr int kMixerStripWidth = 80;
    
    /** Tolerance for float comparison in UI updates */
    constexpr double kFloatComparisonTolerance = 0.001;

    // ==========================================================================
    // Time-Stretching / WSOLA
    // ==========================================================================
    
    /** WSOLA analysis window size in samples */
    constexpr int kWsolaWindowSize = 2048;
    
    /** WSOLA overlap factor (window/overlap = hop) */
    constexpr int kWsolaOverlapFactor = 4;

} // namespace AudioConstants

} // namespace zenith
