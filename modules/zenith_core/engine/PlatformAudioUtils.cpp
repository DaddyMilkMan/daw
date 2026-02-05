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

    PlatformAudioUtils.cpp
    Created: 2025-12-28
    Author:  Zenith DAW

    Implementation of platform-specific audio device initialization.

  ==============================================================================

*/

#include "PlatformAudioUtils.h"

namespace zenith {

#ifndef __linux__
void PlatformAudioUtils::initializeAudioDeviceSetup(juce::AudioDeviceManager& deviceManager)
{
    // Basic initialization - can be expanded for WASAPI/ASIO specific logic on Windows
    // or CoreAudio on macOS.
    
    // For now, we rely on JUCE's default behavior which is usually sufficient 
    // for standard desktop apps unless specific routing is needed.
    // Specifying 2 inputs and 2 outputs as a safe default.
    deviceManager.initialise(2, 2, nullptr, true);
}
#endif

} // namespace zenith
