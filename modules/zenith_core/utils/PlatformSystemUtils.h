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

    PlatformSystemUtils.h
    Created: 2025-12-22

    Interface for platform-specific system utility functions.

  ==============================================================================
*/



#pragma once

#include <juce_core/juce_core.h>

namespace zenith {

class PlatformSystemUtils {
public:
    /**
     * Performs many platform-specific system logging and initialization.
     */
    static void logSystemInfo();
    
    /**
     * Gets the name of the system storage location for app data.
     */
    static juce::String getSystemInfoString();
};

} // namespace zenith
