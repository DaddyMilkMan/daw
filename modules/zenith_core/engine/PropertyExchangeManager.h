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

 // File: PropertyExchangeManager.h
 // Brief: Handles MIDI-CI Property Exchange (JSON-based)
 */


#include <functional>

namespace zenith {

/**
 * @class PropertyExchangeManager
 // Brief: Manages JSON property exchange with MIDI 2.0 devices
 */
class PropertyExchangeManager {
public:
    PropertyExchangeManager();
    ~PropertyExchangeManager();

    struct Property {
        juce::String id;
        juce::String name;
        float value = 0.0f;
        float min = 0.0f;
        float max = 1.0f;
        juce::String type; // "float", "int", "choice"
    };

    /**
     // Brief: Trigger a property list request for a device
     */
    void requestPropertyList(uint32_t deviceMuid);

    /**
     // Brief: Handle an incoming PE response (JSON)
     */
    void handlePropertyResponse(uint32_t deviceMuid, const juce::var& json);

    /**
     // Brief: Set a property value on the hardware
     */
    void setProperty(uint32_t deviceMuid, const juce::String& propertyId, float value);

    /**
     // Brief: Get currently discovered properties for a device
     */
    std::vector<Property> getProperties(uint32_t deviceMuid) const;

    /**
     // Brief: Callback for when properties are updated
     */
    std::function<void(uint32_t)> onPropertiesChanged;

private:
    struct DeviceState {
        uint32_t muid;
        std::vector<Property> properties;
    };

    std::vector<DeviceState> deviceStates_;
    mutable juce::CriticalSection stateLock_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PropertyExchangeManager)
};

} // namespace zenith
