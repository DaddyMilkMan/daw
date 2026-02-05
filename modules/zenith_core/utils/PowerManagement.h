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

    PowerManagement.h
    Created: 2026-01-13
    Author:  Zenith DAW

  ==============================================================================
*/


#pragma once

#include <juce_core/juce_core.h>

namespace zenith {

/**
    Handles system power management, such as preventing the system from sleeping
    while Zenith is active.
*/
class PowerManagement {
public:
    PowerManagement();
    ~PowerManagement();

    /**
        Disables or enables system sleep.
        When disabled, the system should not automatically enter sleep mode.
        The screen may still turn off depending on system settings.
    */
    void setSleepDisabled(bool disabled);

    /** Returns true if system sleep is currently disabled by this utility. */
    bool isSleepDisabled() const { return sleepDisabled; }

private:
    bool sleepDisabled = false;

    // Implementation details are platform-specific
    class Pimpl;
    std::unique_ptr<Pimpl> pimpl;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PowerManagement)
};

} // namespace zenith
