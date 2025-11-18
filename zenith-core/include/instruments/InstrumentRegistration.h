/**
 * @file InstrumentRegistration.h
 * @brief Registration functions for built-in instruments
 */

#pragma once

namespace zenith {

/**
 * @brief Register all built-in instruments with the InstrumentRegistry
 *
 * Call this once during application startup before creating any instrument instances.
 *
 * @note Thread-safe - can be called from any thread, but should only be called once
 */
void registerAllInstruments();

/**
 * @brief Create factory presets for all instruments
 *
 * This creates and saves factory presets to disk.
 * Call this during first-time setup or when updating presets.
 *
 * @note This should only be called during development or on first run
 */
void createFactoryPresets();

} // namespace zenith
