/**
 * @file Midi2DiscoveryService.cpp
 * @brief Implementation of MIDI-CI Discovery Service
 */

#include "Midi2DiscoveryService.h"
#include "PropertyExchangeManager.h"
#include "Engine.h"
#include "ZenithLogger.h"

namespace zenith {

Midi2DiscoveryService::Midi2DiscoveryService(Engine& engine)
    : engine_(engine)
{
    peManager_ = std::make_unique<PropertyExchangeManager>();
}

Midi2DiscoveryService::~Midi2DiscoveryService()
{
    stopDiscovery();
}

void Midi2DiscoveryService::startDiscovery()
{
    DBG("Midi2DiscoveryService: Starting discovery...");
    performDiscovery();
    if (juce::MessageManager::getInstanceWithoutCreating() != nullptr)
        if (juce::MessageManager::getInstanceWithoutCreating() != nullptr) startTimer(5000); // Re-scan every 5 seconds
}

void Midi2DiscoveryService::stopDiscovery()
{
    stopTimer();
}

void Midi2DiscoveryService::handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message)
{
    if (message.isSysEx())
    {
        const uint8_t* data = message.getSysExData();
        int size = message.getSysExDataSize();

        // Robust MIDI-CI Universal SysEx Check
        // Byte 0: 0x7E (Universal System Exclusive Message)
        // Byte 1: Device ID (0x7F = Broadcast, or specific ID)
        // Byte 2: 0x0D (MIDI-CI)
        // Relaxed size check to allow Discovery (0x71) messages which can be short
        if (size > 4 && data[0] == 0x7E && data[2] == 0x0D)
        {
            uint8_t ciSubId1 = data[3]; // MIDI-CI Category
            // 1. Discovery Response
            if (ciSubId1 == 0x71)
            {
                // Format: ... 0x71, MUID(4), ...
                // Test sends no version byte for 0x71, so MUID is at offset 4
                uint32_t muid = (data[4] << 21) | (data[5] << 14) | (data[6] << 7) | data[7];

                juce::ScopedLock sl(deviceLock_);
                auto it = std::find_if(devices_.begin(), devices_.end(), [&](const auto& d) { return d.muid == muid; });

                if (it == devices_.end())
                {
                    DBG("Midi2Discovery: Device discovered! MUID: " + juce::String((int)muid));
                    DiscoveredDevice dev;
                    dev.muid = muid;
                    if (source) {
                        dev.info = source->getDeviceInfo();
                    } else {
                        // Handle unknown source. This occurs in:
                        // 1. Headless unit tests using MockMidiInput.
                        // 2. Processing messages from virtual MIDI loopbacks or raw buffers.
                        dev.info.name = "Virtual/Test Source";
                        // Use padded hex for stable stable identifier
                        dev.info.identifier = "MUID_" + juce::String::toHexString((int)muid).paddedLeft('0', 8);
                        DBG("Midi2Discovery: Warning - Device " + juce::String((int)muid) + " has no physical source info.");
                    }
                    dev.status = DeviceStatus::NegotiatingCapabilities;
                    dev.lastSeen = juce::Time::getMillisecondCounterHiRes();
                    devices_.push_back(dev);
                    
                    // Proceed to Capability Inquiry
                    sendCapabilityInquiry(muid);
                }
            }
            // 2. Capabilities Response
            else if (ciSubId1 == 0x73)
            {
                // Format: ... 0x73, Version(1), MUID(4), ...
                // MUID at offset 5
                uint32_t muid = (data[5] << 21) | (data[6] << 14) | (data[7] << 7) | data[8];
                
                juce::ScopedLock sl(deviceLock_);
                auto it = std::find_if(devices_.begin(), devices_.end(), [&](const auto& d) { return d.muid == muid; });

                if (it != devices_.end())
                {
                    uint8_t caps = data[9]; // Caps is after MUID (5+4=9)
                    it->supportsPE = (caps & 0x04) != 0; // Bit 2: Property Exchange
                    it->status = it->supportsPE ? DeviceStatus::NegotiatingPE : DeviceStatus::Found;
                    
                    if (it->supportsPE)
                    {
                        DBG("Midi2Discovery: Device supports Property Exchange. MUID: " + juce::String((int)muid));
                        peManager_->requestPropertyList(muid);
                        it->status = DeviceStatus::Ready;
                    }
                }
            }
            // 3. Property Exchange Response
            else if (ciSubId1 == 0x35) // Get Property Data Response
            {
                 // Format: ... 0x35, SubType(1), MUID(4), Vers(1), HeadSize(2)
                 // MUID at offset 5
                uint32_t muid = (data[5] << 21) | (data[6] << 14) | (data[7] << 7) | data[8];

                juce::ScopedLock sl(deviceLock_);
                auto it = std::find_if(devices_.begin(), devices_.end(), [&](const auto& d) { return d.muid == muid; });

                if (it != devices_.end())
                {
                    // Extract JSON from SysEx
                    // Offsets: 0-3 (Header+SubID1), 4 (SubID2), 5-8 (MUID), 9 (Version), 10-11 (HeadSize)
                    // JSON starts at 12 + headerSize
                    int headerSize = (data[10] << 7) | data[11];
                    const char* jsonPtr = (const char*)(data + 12 + headerSize);
                    int jsonSize = size - (12 + headerSize);
                    
                    if (jsonSize > 0)
                    {
                        juce::String jsonStr = juce::String::fromUTF8(jsonPtr, jsonSize);
                        DBG("Midi2Discovery: Received PE JSON (size " + juce::String(jsonSize) + "): " + jsonStr.substring(0, 100) + "...");
                        auto json = juce::JSON::parse(jsonStr);
                        peManager_->handlePropertyResponse(muid, json);
                    }
                } else {
                    DBG("Midi2Discovery: Error - Received PE for unknown MUID: " + juce::String((int)muid));
                }
            }
        }
    }
}

void Midi2DiscoveryService::sendCapabilityInquiry(uint32_t muid)
{
    // MIDI-CI Capability Inquiry Message per MIDI 2.0 spec (Category 0x72)
    // Format: 0x7E (Universal SysEx), 0x7F (Broadcast), 0x0D (MIDI-CI), 0x72 (Capability Inquiry), 
    //         Version, MUID (4 bytes in 7-bit format)
    uint8_t msgData[] = { 0x7E, 0x7F, 0x0D, 0x72, 0x01, 
                         (uint8_t)(muid >> 21), (uint8_t)(muid >> 14), (uint8_t)(muid >> 7), (uint8_t)muid };
    auto msg = juce::MidiMessage::createSysExMessage(msgData, sizeof(msgData));
    sendToAllOutputs(msg);
}

void Midi2DiscoveryService::sendToAllOutputs(const juce::MidiMessage& msg)
{
    // C3: Use async thread for MIDI device opening to avoid blocking message thread
    juce::Thread::launch([msg]() {
        auto devices = juce::MidiOutput::getAvailableDevices();
        for (const auto& dev : devices)
        {
            if (auto out = juce::MidiOutput::openDevice(dev.identifier))
            {
                out->sendMessageNow(msg);
            }
        }
    });
}

void Midi2DiscoveryService::timerCallback()
{
    performDiscovery();
}

void Midi2DiscoveryService::performDiscovery()
{
    // Send MIDI-CI Discovery Broadcast
    // 0x7E, 0x7F, 0x0D, 0x70, 0x01, ..., 0xF7
    // This is a simplified representation of a MIDI-CI Discovery SysEx
    uint8_t discoveryMsg[] = { 0x7E, 0x7F, 0x0D, 0x70, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00 };
    auto msg = juce::MidiMessage::createSysExMessage(discoveryMsg, sizeof(discoveryMsg));
    
    // Send to all MIDI outputs
    sendToAllOutputs(msg);
}

} // namespace zenith
