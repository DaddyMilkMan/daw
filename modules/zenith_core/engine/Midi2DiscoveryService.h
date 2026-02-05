/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/*
    ==============================================================================
    Original file header:
*/

 // File: Midi2DiscoveryService.h
// Brief: MIDI-CI Discovery Service for MIDI 2.0 Hardware



#include <vector>
#include <memory>

namespace zenith {

class Engine;
class PropertyExchangeManager;

/**
 * @class Midi2DiscoveryService
 // Brief: Handles MIDI-CI Discovery and Capability Inquiry
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
     // Brief: Start discovery on all available MIDI inputs
     */
    void startDiscovery();

    /**
     // Brief: Stop discovery and monitoring
     */
    void stopDiscovery();

    // juce::MidiInputCallback
    void handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message) override;

    // juce::Timer
    void timerCallback() override;

    /**
     // Brief: Get the property exchange manager
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
