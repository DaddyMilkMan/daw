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
class InstrumentRegistry; // Forward declaration
void registerBuiltInInstruments(InstrumentRegistry& registry);

} // namespace zenith

