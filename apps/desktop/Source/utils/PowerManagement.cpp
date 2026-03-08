/*
  ==============================================================================

    PowerManagement.cpp
    Created: 2026-01-13
    Author:  Zenith DAW

  ==============================================================================
*/

#include "PowerManagement.h"

namespace zenith {

#if !defined(JUCE_LINUX)
// Stub implementation for non-Linux platforms for now
class PowerManagement::Pimpl {
public:
    void setSleepDisabled(bool) {}
};

PowerManagement::PowerManagement() : pimpl(std::make_unique<Pimpl>()) {}
PowerManagement::~PowerManagement() = default;

void PowerManagement::setSleepDisabled(bool disabled) {
    sleepDisabled = disabled;
    pimpl->setSleepDisabled(disabled);
}
#endif

} // namespace zenith
