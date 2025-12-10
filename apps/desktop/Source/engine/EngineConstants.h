/*
  ==============================================================================

    EngineConstants.h
    Created: 2025-12-09
    Author:  Zenith DAW

    Centralized constants for the audio engine to eliminate magic numbers
    and improve code readability and maintainability.

    Thread Safety:
    - All values are constexpr and compile-time constant
    - Safe to use from any thread

  ==============================================================================
*/

#pragma once

#include <cstdint>

namespace zenith {
namespace constants {

//==============================================================================
// Audio Processing Constants
//==============================================================================

/// Maximum number of audio channels supported (stereo to 32-channel surround)
constexpr int kMaxAudioChannels = 32;

/// Default sample rate in Hz
constexpr double kDefaultSampleRate = 44100.0;

/// Default buffer size in samples
constexpr int kDefaultBufferSize = 512;

/// Maximum buffer size in samples (for allocation)
constexpr int kMaxBufferSize = 8192;

//==============================================================================
// Metering Constants
//==============================================================================

/// Smoothing factor for level meters (0.0 = instant, 1.0 = frozen)
/// Lower values = faster response, higher values = smoother display
constexpr float kMeterSmoothingFactor = 0.3f;

/// Decay rate for peak meters (per block) - approximately 20dB/second at 44.1kHz
constexpr float kPeakMeterDecay = 0.995f;

/// Meter update rate in Hz (for UI refresh)
constexpr int kMeterUpdateRateHz = 60;

//==============================================================================
// Recording Constants
//==============================================================================

/// Size of the audio writer FIFO buffer in bytes
constexpr int kAudioWriterFifoSize = 32768;

/// Size of the MIDI recording FIFO in events
constexpr int kMidiRecordFifoSize = 4096;

/// Size of the command/event queue buffer
constexpr int kCommandBufferSize = 1024;

/// Recording file bit depth
constexpr int kRecordingBitDepth = 24;

//==============================================================================
// DSP Processing Constants
//==============================================================================

/// Default test tone frequency in Hz (A4)
constexpr double kTestToneFrequency = 440.0;

/// Default test tone amplitude in linear scale (-12 dB)
constexpr double kTestToneAmplitude = 0.25;

/// Constant power pan coefficient (π/4)
constexpr float kConstantPowerPanCoeff = 0.7853981633974483f; // π/4

/// dB to linear conversion threshold (below this is considered silence)
constexpr float kSilenceThresholdDb = -96.0f;

/// Linear silence threshold
constexpr float kSilenceThresholdLinear = 0.0000158f; // ~10^(-96/20)

//==============================================================================
// Compressor/Dynamics Constants
//==============================================================================

/// Default compressor threshold in dB
constexpr float kDefaultCompThresholdDb = -10.0f;

/// Default compressor ratio
constexpr float kDefaultCompRatio = 4.0f;

/// Default compressor attack time in ms
constexpr float kDefaultCompAttackMs = 10.0f;

/// Default compressor release time in ms
constexpr float kDefaultCompReleaseMs = 100.0f;

/// Minimum compressor attack time in ms
constexpr float kMinCompAttackMs = 0.1f;

/// Maximum compressor attack time in ms
constexpr float kMaxCompAttackMs = 100.0f;

/// Minimum compressor release time in ms
constexpr float kMinCompReleaseMs = 10.0f;

/// Maximum compressor release time in ms
constexpr float kMaxCompReleaseMs = 1000.0f;

/// Compressor lookahead buffer size in ms
constexpr float kCompLookaheadMs = 5.0f;

/// RMS window size in ms for compressor
constexpr float kCompRmsWindowMs = 10.0f;

//==============================================================================
// Limiter Constants
//==============================================================================

/// Default limiter ceiling in dB
constexpr float kDefaultLimiterCeilingDb = -0.1f;

/// Limiter attack time in ms (fast for brickwall limiting)
constexpr float kLimiterAttackMs = 0.1f;

/// Limiter release time in ms
constexpr float kLimiterReleaseMs = 50.0f;

/// Limiter lookahead time in ms
constexpr float kLimiterLookaheadMs = 1.0f;

//==============================================================================
// EQ Constants
//==============================================================================

/// Number of EQ bands per channel strip
constexpr int kNumEQBands = 4;

/// Default EQ Q factor (Butterworth)
constexpr float kDefaultEQQ = 0.707f;

/// Minimum EQ frequency in Hz
constexpr float kMinEQFrequency = 20.0f;

/// Maximum EQ frequency in Hz
constexpr float kMaxEQFrequency = 20000.0f;

/// Minimum EQ gain in dB
constexpr float kMinEQGainDb = -24.0f;

/// Maximum EQ gain in dB
constexpr float kMaxEQGainDb = 24.0f;

//==============================================================================
// High-Pass Filter Constants
//==============================================================================

/// Minimum HPF frequency in Hz
constexpr float kMinHPFFrequency = 20.0f;

/// Maximum HPF frequency in Hz
constexpr float kMaxHPFFrequency = 500.0f;

//==============================================================================
// Send/Aux Constants
//==============================================================================

/// Maximum number of sends per channel
constexpr int kNumSends = 4;

//==============================================================================
// Input/Output Constants
//==============================================================================

/// Minimum input gain in dB
constexpr float kMinInputGainDb = -60.0f;

/// Maximum input gain in dB
constexpr float kMaxInputGainDb = 24.0f;

//==============================================================================
// Snapshot/RCU Constants
//==============================================================================

/// Maximum number of snapshots to keep in trash before garbage collection
constexpr size_t kMaxSnapshotTrashSize = 5;

//==============================================================================
// Export Constants
//==============================================================================

/// Default export block size for offline rendering
constexpr int kExportBlockSize = 4096;

/// Tail time added to export duration for reverb/delay (in seconds)
constexpr double kExportTailSeconds = 2.0;

/// Default export duration if no clips (in seconds)
constexpr double kDefaultExportDuration = 10.0;

//==============================================================================
// Plugin Delay Compensation (PDC) Constants
//==============================================================================

/// Maximum plugin latency to compensate (in samples at 44.1kHz)
constexpr int kMaxPDCLatencySamples = 44100; // ~1 second

//==============================================================================
// Oversampling Constants
//==============================================================================

/// Oversampling factor for nonlinear DSP (saturation, limiting)
constexpr int kOversamplingFactor = 2;

/// Maximum oversampling factor supported
constexpr int kMaxOversamplingFactor = 4;

//==============================================================================
// Timing Constants
//==============================================================================

/// Automation update rate in Hz
constexpr int kAutomationUpdateRateHz = 60;

/// Session debugger monitoring interval in ms
constexpr int kSessionDebuggerIntervalMs = 500;

/// Thread stop timeout in ms
constexpr int kThreadStopTimeoutMs = 1000;

//==============================================================================
// MIDI Constants
//==============================================================================

/// Default quantize grid (1/16 note)
constexpr double kDefaultQuantizeGrid = 0.25;

/// Minimum MIDI velocity
constexpr int kMinMidiVelocity = 0;

/// Maximum MIDI velocity
constexpr int kMaxMidiVelocity = 127;

/// Minimum MIDI note number
constexpr int kMinMidiNote = 0;

/// Maximum MIDI note number
constexpr int kMaxMidiNote = 127;

//==============================================================================
// Freeze Constants
//==============================================================================

/// Freeze render block size
constexpr int kFreezeBlockSize = 4096;

/// Freeze file bit depth
constexpr int kFreezeBitDepth = 32;

//==============================================================================
// Utility Functions (constexpr)
//==============================================================================

/// Convert decibels to linear gain
constexpr float dbToGain(float db) noexcept {
    // Use approximation for constexpr: 10^(db/20)
    // For runtime, use std::pow or juce::Decibels
    return (db <= kSilenceThresholdDb) ? 0.0f : 
           (db == 0.0f) ? 1.0f :
           // Cannot use std::pow in constexpr before C++26
           // This is a placeholder - actual implementation uses runtime function
           1.0f; // Placeholder - use zenith::dbToGain() at runtime
}

/// Convert samples to milliseconds
constexpr double samplesToMs(int64_t samples, double sampleRate) noexcept {
    return (sampleRate > 0.0) ? (static_cast<double>(samples) / sampleRate * 1000.0) : 0.0;
}

/// Convert milliseconds to samples
constexpr int64_t msToSamples(double ms, double sampleRate) noexcept {
    return static_cast<int64_t>(ms * sampleRate / 1000.0);
}

} // namespace constants
} // namespace zenith
