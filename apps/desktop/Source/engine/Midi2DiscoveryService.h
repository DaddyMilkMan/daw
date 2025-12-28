/**
 * @file Midi2DiscoveryService.h
 * @brief MIDI-CI Discovery Service for MIDI 2.0 Hardware
 */

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_events/juce_events.h>
#include <vector>
#include <memory>

namespace zenith {

class Engine;
class PropertyExchangeManager;

/**
 * @class Midi2DiscoveryService
 * @brief Handles MIDI-CI Discovery and Capability Inquiry
 *
 * This service monitors MIDI inputs and performs MIDI-CI discovery
 * using a robust state machine (Discovery -> Capability -> Property Exchange).
 */
class Midi2DiscoveryService : public juce::MidiInputCallback,
                             public juce::Timer {
public:
    Midi2DiscoveryService(Engine& engine);
    ~Midi2DiscoveryService() override;

    enum class DeviceStatus {
        Unknown,
        Discovering,
        Found,
        NegotiatingCapabilities,
        NegotiatingPE,
        Ready,
        Failed
    };

    struct DiscoveredDevice {
        juce::MidiDeviceInfo info;
        uint32_t muid = 0;
        DeviceStatus status = DeviceStatus::Unknown;
        bool supportsPE = false;
        double lastSeen = 0;
    };

    /**
     * @brief Start discovery on all available MIDI inputs
     */
    void startDiscovery();

    /**
     * @brief Stop discovery and monitoring
     */
    void stopDiscovery();

    // juce::MidiInputCallback
    void handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message) override;

    // juce::Timer
    void timerCallback() override;

    /**
     * @brief Get the property exchange manager
     */
    PropertyExchangeManager& getPEManager() { return *peManager_; }

private:
    void performDiscovery();
    void sendCapabilityInquiry(uint32_t muid);
    void sendToAllOutputs(const juce::MidiMessage& msg);

    Engine& engine_;
    std::unique_ptr<PropertyExchangeManager> peManager_;
    
    std::vector<DiscoveredDevice> devices_;
    juce::CriticalSection deviceLock_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Midi2DiscoveryService)
};

} // namespace zenith
