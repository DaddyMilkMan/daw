/**
 * @file PropertyExchangeManager.cpp
 * @brief Implementation of Property Exchange Manager
 */

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>

#include "PropertyExchangeManager.h"
#include "ZenithLogger.h"

namespace zenith {

PropertyExchangeManager::PropertyExchangeManager() {}
PropertyExchangeManager::~PropertyExchangeManager() {}

void PropertyExchangeManager::requestPropertyList(uint32_t deviceMuid)
{
    DBG("PropertyExchangeManager: Requesting property list from MUID: " + juce::String((int)deviceMuid));
    
    // Construct MIDI-CI Property Exchange: Get Property Data (Inquiry)
    // 0x7E, DeviceID, 0x0D, 0x34, 0x01, MUID(4), ...
    juce::uint8 header[] = { 
        0x7E, 0x7F, 0x0D, 0x34, 0x01,
        (juce::uint8)(deviceMuid >> 21), (juce::uint8)(deviceMuid >> 14), 
        (juce::uint8)(deviceMuid >> 7), (juce::uint8)deviceMuid,
        0x01, // PE Version
        0x00, 0x00 // Request Header (JSON, empty for list request)
    };
    
    auto msg = juce::MidiMessage::createSysExMessage(header, sizeof(header));
    
    // Dispatch to ALL outputs (In a real setup, we'd target the specific device)
    auto devices = juce::MidiOutput::getAvailableDevices();
    for (const auto& dev : devices) {
        if (auto out = juce::MidiOutput::openDevice(dev.identifier)) {
            out->sendMessageNow(msg);
        }
    }
}

void PropertyExchangeManager::handlePropertyResponse(uint32_t deviceMuid, const juce::var& json)
{
    juce::ScopedLock sl(stateLock_);
    
    DeviceState* state = nullptr;
    for (auto& s : deviceStates_)
    {
        if (s.muid == deviceMuid) { state = &s; break; }
    }
    
    if (!state)
    {
        deviceStates_.push_back({ deviceMuid, {} });
        state = &deviceStates_.back();
    }
    
    state->properties.clear();
    
    if (auto* props = json["properties"].getArray())
    {
        for (const auto& p : *props)
        {
            Property prop;
            prop.id = p["id"].toString();
            prop.name = p["name"].toString();
            prop.min = (float)p["min"];
            prop.max = (float)p["max"];
            prop.value = (float)p["value"];
            state->properties.push_back(prop);
        }
    }
    
    DBG("PropertyExchangeManager: Updated " + juce::String((int)state->properties.size()) + " properties for " + juce::String((int)deviceMuid));
    
    if (onPropertiesChanged)
        onPropertiesChanged(deviceMuid);
}

void PropertyExchangeManager::setProperty(uint32_t deviceMuid, const juce::String& propertyId, float value)
{
    DBG("PropertyExchangeManager: Setting " + propertyId + " to " + juce::String(value) + " on MUID " + juce::String((int)deviceMuid));
    
    // Construct MIDI-CI Set Property Data
    // We send a JSON snippet: {"id": "propId", "value": val}
    juce::DynamicObject::Ptr payload = new juce::DynamicObject();
    payload->setProperty("id", propertyId);
    payload->setProperty("value", value);
    juce::String json = juce::JSON::toString(juce::var(payload.get()), true);
    
    juce::MemoryBlock sysex;
    juce::uint8 header[] = { 
        0x7E, 0x7F, 0x0D, 0x34, 0x02, // Set Property Data
        (juce::uint8)(deviceMuid >> 21), (juce::uint8)(deviceMuid >> 14), 
        (juce::uint8)(deviceMuid >> 7), (juce::uint8)deviceMuid,
        0x01 // PE Version
    };
    sysex.append(header, sizeof(header));
    sysex.append(json.toRawUTF8(), json.length());
    
    auto msg = juce::MidiMessage::createSysExMessage(sysex.getData(), sysex.getSize());
    
    auto devices = juce::MidiOutput::getAvailableDevices();
    for (const auto& dev : devices) {
        if (auto out = juce::MidiOutput::openDevice(dev.identifier)) {
            out->sendMessageNow(msg);
        }
    }

    // Update local cache
    juce::ScopedLock sl(stateLock_);
    for (auto& s : deviceStates_)
    {
        if (s.muid == deviceMuid)
        {
            for (auto& p : s.properties)
            {
                if (p.id == propertyId)
                {
                    p.value = value;
                    break;
                }
            }
            break;
        }
    }
}

std::vector<PropertyExchangeManager::Property> PropertyExchangeManager::getProperties(uint32_t deviceMuid) const
{
    juce::ScopedLock sl(stateLock_);
    for (const auto& s : deviceStates_)
    {
        if (s.muid == deviceMuid) return s.properties;
    }
    return {};
}

} // namespace zenith
