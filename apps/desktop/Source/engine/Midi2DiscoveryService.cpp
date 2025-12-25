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
    startTimer(5000); // Re-scan every 5 seconds
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
        if (size >= 10 && data[0] == 0x7E && data[2] == 0x0D)
        {
            uint8_t ciSubId1 = data[3]; // MIDI-CI Category
            uint32_t muid = (data[4] << 21) | (data[5] << 14) | (data[6] << 7) | data[7];
            
            juce::ScopedLock sl(deviceLock_);
            auto it = std::find_if(devices_.begin(), devices_.end(), [&](const auto& d) { return d.muid == muid; });
            
            // 1. Discovery Response
            if (ciSubId1 == 0x71)
            {
                if (it == devices_.end())
                {
                    DBG("Midi2Discovery: Device discovered! MUID: " + juce::String((int)muid));
                    DiscoveredDevice dev;
                    dev.muid = muid;
                    dev.info = source->getDeviceInfo();
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
                if (it != devices_.end())
                {
                    uint8_t caps = data[9];
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
                if (it != devices_.end())
                {
                    // Extract JSON from SysEx
                    // 0x7E, DeviceID, 0x0D, 0x35, 0x02, MUID(4), PE_Version, HeaderSize(2), Header, Body...
                    int headerSize = (data[10] << 7) | data[11];
                    const char* jsonPtr = (const char*)(data + 12 + headerSize);
                    int jsonSize = size - (12 + headerSize);
                    
                    if (jsonSize > 0)
                    {
                        auto json = juce::JSON::parse(juce::String::fromUTF8(jsonPtr, jsonSize));
                        peManager_->handlePropertyResponse(muid, json);
                    }
                }
            }
        }
    }
}

void Midi2DiscoveryService::sendCapabilityInquiry(uint32_t muid)
{
    // Mocking Capability Inquiry Message construction
    uint8_t msgData[] = { 0x7E, 0x7F, 0x0D, 0x72, 0x01, 
                         (uint8_t)(muid >> 21), (uint8_t)(muid >> 14), (uint8_t)(muid >> 7), (uint8_t)muid };
    auto msg = juce::MidiMessage::createSysExMessage(msgData, sizeof(msgData));
    sendToAllOutputs(msg);
}

void Midi2DiscoveryService::sendToAllOutputs(const juce::MidiMessage& msg)
{
    auto devices = juce::MidiOutput::getAvailableDevices();
    for (const auto& dev : devices)
    {
        if (auto out = juce::MidiOutput::openDevice(dev.identifier))
        {
            out->sendMessageNow(msg);
        }
    }
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
    auto devices = juce::MidiOutput::getAvailableDevices();
    for (const auto& dev : devices)
    {
        if (auto out = juce::MidiOutput::openDevice(dev.identifier))
        {
            out->sendMessageNow(msg);
        }
    }
}

} // namespace zenith
