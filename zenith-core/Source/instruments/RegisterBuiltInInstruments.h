/*
  ==============================================================================

    RegisterBuiltInInstruments.h
    Created: 2025-11-18
    Author:  Zenith DAW

    Registers all built-in instruments with the InstrumentRegistry.
    Called during application initialization.

  ==============================================================================
*/

#pragma once

namespace zenith {

/**
 * @brief Register all built-in instruments
 *
 * This function registers:
 * - ZenithPolySynth
 * - ZenithSampler
 *
 * Must be called once during application startup before CommandAPI is used.
 */
void registerBuiltInInstruments();

} // namespace zenith
