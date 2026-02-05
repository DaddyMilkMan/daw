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

    PlatformLogUtils.cpp
    Created: 2025-12-28
    Author:  Zenith DAW

    Implementation of platform-agnostic logging utilities.

  ==============================================================================

*/

#include "PlatformLogUtils.h"
#include <juce_core/juce_core.h>

#if JUCE_WINDOWS
#include <windows.h>
#include <iostream>
#endif

namespace zenith {

void PlatformLogUtils::showDebugConsole()
{
#if JUCE_WINDOWS
    if (AllocConsole())
    {
        FILE* fp;
        freopen_s(&fp, "CONOUT$", "w", stdout);
        freopen_s(&fp, "CONOUT$", "w", stderr);
        freopen_s(&fp, "CONIN$", "r", stdin);
        std::cout << "Debug Console Attached" << std::endl;
    }
#else
    // On macOS/Linux, output usually goes to stdout/terminal by default if launched from there
    // No explicit console allocation needed usually.
#endif
}

void PlatformLogUtils::freeDebugConsole()
{
#if JUCE_WINDOWS
    FreeConsole();
#endif
}

} // namespace zenith
