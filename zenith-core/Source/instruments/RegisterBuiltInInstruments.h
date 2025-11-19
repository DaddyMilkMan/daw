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
 * - ZenithPolySynth (subtractive synthesizer)
 * - ZenithSampler (base sampler instrument)
 * - Canonical sampler instruments:
 *   - 808 Essentials (drums)
 *   - LoFi Keys (piano/keys)
 *   - Trap Pluck (synth)
 *   - Orchestral Strings (orchestral)
 *   - FX & Impacts (sound effects)
 *
 * Must be called once during application startup before CommandAPI is used.
 */
void registerBuiltInInstruments();

} // namespace zenith
