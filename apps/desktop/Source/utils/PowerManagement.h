/*
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
