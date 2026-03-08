/*
  ==============================================================================

    PlatformDisplayUtils.cpp
    Created: 2025-12-28
    Author:  Zenith DAW

    Implementation of platform-specific display utilities.

  ==============================================================================
*/

#include "PlatformDisplayUtils.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace zenith {

int PlatformDisplayUtils::getSystemRefreshRate()
{
    // Getting exact refresh rate from JUCE is not always direct in older versions or without VBlankListener.
    // For now, returning a safe default of 60Hz.
    // Ideally this would query the OS (EnumDisplaySettings on Windows, CGDisplayMode on Mac).
    
    // Future improvement: Implement OS-specific queries.
    return 60;
}

float PlatformDisplayUtils::getDisplayScaleFactor()
{
    // Get the primary display's scale factor
    auto& displays = juce::Desktop::getInstance().getDisplays();
    if (displays.displays.isEmpty()) {
        return 1.0f; // Fallback to standard scale
    }
    
    // Return the primary display's scale
    return static_cast<float>(displays.getPrimaryDisplay()->scale);
}

float PlatformDisplayUtils::getDisplayScaleFactor(juce::Component* component)
{
    if (component == nullptr) {
        return getDisplayScaleFactor();
    }
    
    // Get the display containing this component
    auto& displays = juce::Desktop::getInstance().getDisplays();
    
    // Get component's screen position
    auto componentBounds = component->getScreenBounds();
    
    if (componentBounds.isEmpty()) {
        return getDisplayScaleFactor();
    }
    
    // Find the display containing the component center
    auto* display = displays.getDisplayForPoint(componentBounds.getCentre());
    
    if (display != nullptr) {
        return static_cast<float>(display->scale);
    }
    
    // Fallback to primary display
    return getDisplayScaleFactor();
}

} // namespace zenith

