/**
 * MidiRouter.h
 * MIDI event routing and management system
 *
 * Handles MIDI input from multiple devices, routes to tracks, and
 * manages MIDI output to external hardware/virtual MIDI ports.
 */

#pragma once

#include <JuceHeader.h>
#include <vector>
#include <unordered_map>
#include "MidiEvent.h"

namespace vexel {

/**
 * MIDI routing destination
 */
struct MidiRoute {
    int sourceDeviceId;
    int destinationTrackId;
    int midiChannel; // 0 = all channels, 1-16 = specific channel
    bool enabled = true;
};

/**
 * MIDI Router manages all MIDI I/O and routing
 */
class MidiRouter
{
public:
    MidiRouter();
    ~MidiRouter();

    // ========================================================================
    // MIDI Input
    // ========================================================================

    /**
     * Add incoming MIDI event (called from audio callback or MIDI thread)
     */
    void addMidiEvent(const juce::MidiMessage& message, const juce::String& sourceName);

    /**
     * Get MIDI events for a specific track in current buffer
     */
    juce::MidiBuffer getMidiForTrack(int trackId, int numSamples);

    /**
     * Clear event buffer (call at end of audio processing)
     */
    void clearBuffer();

    // ========================================================================
    // MIDI Routing
    // ========================================================================

    /**
     * Create a MIDI route from input device to track
     */
    void addRoute(int sourceDeviceId, int destinationTrackId, int channel = 0);

    /**
     * Remove a MIDI route
     */
    void removeRoute(int sourceDeviceId, int destinationTrackId);

    /**
     * Enable/disable a route
     */
    void setRouteEnabled(int sourceDeviceId, int destinationTrackId, bool enabled);

    /**
     * Get all routes
     */
    const std::vector<MidiRoute>& getRoutes() const { return m_routes; }

    // ========================================================================
    // MIDI Devices
    // ========================================================================

    /**
     * Get list of available MIDI input devices
     */
    static juce::StringArray getAvailableMidiInputs();

    /**
     * Get list of available MIDI output devices
     */
    static juce::StringArray getAvailableMidiOutputs();

    /**
     * Open MIDI output device
     */
    bool openMidiOutput(const juce::String& deviceName);

    /**
     * Close MIDI output
     */
    void closeMidiOutput();

    /**
     * Send MIDI message to output
     */
    void sendMidiOutput(const juce::MidiMessage& message);

    // ========================================================================
    // MIDI Learn
    // ========================================================================

    /**
     * Enable MIDI learn mode for parameter automation
     */
    void setMidiLearnEnabled(bool enabled);

    bool isMidiLearnEnabled() const { return m_midiLearnEnabled; }

    /**
     * Get last MIDI CC message for learning
     */
    bool getLastMidiCC(int& controller, int& value);

private:
    // Lock-free MIDI buffer for real-time thread
    juce::MidiBuffer m_incomingMidi;
    juce::CriticalSection m_midiBufferLock;

    // Routing table
    std::vector<MidiRoute> m_routes;
    juce::CriticalSection m_routeLock;

    // MIDI output
    std::unique_ptr<juce::MidiOutput> m_midiOutput;

    // MIDI learn
    std::atomic<bool> m_midiLearnEnabled { false };
    std::atomic<int> m_lastMidiCC { -1 };
    std::atomic<int> m_lastMidiCCValue { 0 };

    // Device name -> ID mapping
    std::unordered_map<juce::String, int> m_deviceNameToId;
    int m_nextDeviceId = 1;

    int getDeviceId(const juce::String& deviceName);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiRouter)
};

} // namespace vexel
