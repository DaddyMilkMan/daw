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

    PlatformDisplayUtils.h
    Created: 2025-12-22

    Platform-specific display utilities (refresh rate, DPI scaling, etc).

  ==============================================================================
*/



#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace zenith {

/**
 * @class PlatformDisplayUtils
 * @brief Platform-specific display utilities for refresh rate and HiDPI support
 */
class PlatformDisplayUtils
{
public:
    //==========================================================================
    // Refresh Rate
    //==========================================================================
    
    /**
     * @brief Get the system's primary display refresh rate
     * @return Refresh rate in Hz (typically 60, 120, 144, etc.)
     */
    static int getSystemRefreshRate();
    
    //==========================================================================
    // High-DPI / Retina Support
    //==========================================================================
    
    /**
     * @brief Get display scale factor for HiDPI support
     * @return Scale factor (1.0 = standard, 2.0 = Retina/HiDPI, etc.)
     * 
     * This returns the scale factor for the primary display.
     * For per-monitor DPI awareness, use the component-specific overload.
     */
    static float getDisplayScaleFactor();
    
    /**
     * @brief Get display scale factor for a specific component's display
     * @param component The component to get scale factor for
     * @return Scale factor for the display containing this component
     */
    static float getDisplayScaleFactor(juce::Component* component);
    
    //==========================================================================
    // Pixel Conversion Utilities
    //==========================================================================
    
    /**
     * @brief Convert logical pixels to physical pixels
     * @param logicalPixels Size in logical (design) pixels
     * @param scaleFactor Scale factor (0 = use primary display scale)
     * @return Size in physical pixels
     */
    static int toPhysicalPixels(int logicalPixels, float scaleFactor = 0.0f) {
        if (scaleFactor <= 0.0f) {
            scaleFactor = getDisplayScaleFactor();
        }
        return static_cast<int>(logicalPixels * scaleFactor);
    }
    
    static float toPhysicalPixels(float logicalPixels, float scaleFactor = 0.0f) {
        if (scaleFactor <= 0.0f) {
            scaleFactor = getDisplayScaleFactor();
        }
        return logicalPixels * scaleFactor;
    }
    
    /**
     * @brief Convert physical pixels to logical pixels
     * @param physicalPixels Size in physical pixels
     * @param scaleFactor Scale factor (0 = use primary display scale)
     * @return Size in logical (design) pixels
     */
    static int toLogicalPixels(int physicalPixels, float scaleFactor = 0.0f) {
        if (scaleFactor <= 0.0f) {
            scaleFactor = getDisplayScaleFactor();
        }
        return static_cast<int>(physicalPixels / scaleFactor);
    }
    
    static float toLogicalPixels(float physicalPixels, float scaleFactor = 0.0f) {
        if (scaleFactor <= 0.0f) {
            scaleFactor = getDisplayScaleFactor();
        }
        return physicalPixels / scaleFactor;
    }
    
    /**
     * @brief Check if running on a HiDPI display
     * @return true if scale factor > 1.0
     */
    static bool isHiDPI() {
        return getDisplayScaleFactor() > 1.0f;
    }
};

} // namespace zenith
