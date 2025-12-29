/**
 * @file PropertyExchangeManager.h
 * @brief Handles MIDI-CI Property Exchange (JSON-based)
 */

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <vector>
#include <functional>

namespace zenith {

/**
 * @class PropertyExchangeManager
 * @brief Manages JSON property exchange with MIDI 2.0 devices
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
     * @brief Trigger a property list request for a device
     */
    void requestPropertyList(uint32_t deviceMuid);

    /**
     * @brief Handle an incoming PE response (JSON)
     */
    void handlePropertyResponse(uint32_t deviceMuid, const juce::var& json);

    /**
     * @brief Set a property value on the hardware
     */
    void setProperty(uint32_t deviceMuid, const juce::String& propertyId, float value);

    /**
     * @brief Get currently discovered properties for a device
     */
    std::vector<Property> getProperties(uint32_t deviceMuid) const;

    /**
     * @brief Callback for when properties are updated
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
