/*
  ==============================================================================

    RegisterBuiltInInstruments.cpp
    Created: 2025-11-18
    Author:  Zenith DAW

    Implementation of built-in instrument registration.

  ==============================================================================
*/

#include "RegisterBuiltInInstruments.h"
#include "InstrumentRegistry.h"
#include "ZenithPolySynth.h"
#include "ZenithSampler.h"

namespace zenith {

void registerBuiltInInstruments()
{
    auto& registry = InstrumentRegistry::getInstance();

    // Register ZenithPolySynth
    registry.registerInstrument(
        "zenith.poly_synth",
        ZenithPolySynth::createMetadata(),
        []() { return std::make_unique<ZenithPolySynth>(); }
    );

    // Register ZenithSampler
    registry.registerInstrument(
        "zenith_sampler",
        ZenithSampler::createMetadata(),
        []() { return std::make_unique<ZenithSampler>(); }
    );

    DBG("Built-in instruments registered successfully");
}

} // namespace zenith
