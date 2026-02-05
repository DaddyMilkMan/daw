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

