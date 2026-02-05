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

    AudioConstants.h
    Created: 2025-12-14
    Author:  Zenith DAW

    Centralized audio engine constants for use across the audio processing
    subsystem. This header includes EngineConstants.h for the full set of
    engine constants and provides additional aliases for convenience.


    Thread Safety:
    - All values are constexpr and compile-time constant
    - Safe to use from any thread without synchronization

  ==============================================================================
*/

#pragma once

// Include the main engine constants header - this provides the canonical
// definitions for all audio engine constants
#include "EngineConstants.h"

namespace zenith {
namespace constants {

//==============================================================================
// Audio Constants
//==============================================================================

// Currently effectively a forward to EngineConstants.h
// Future audio-specific constants can be added here.

} // namespace constants
} // namespace zenith
