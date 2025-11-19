/**
 * AudioEngineBindings.h
 * C++ to Lua bindings for AudioEngine functions
 */

#pragma once

struct lua_State;

namespace zenith {

class AudioEngine;

/**
 * Register AudioEngine API to Lua
 * Makes engine functions callable from Lua scripts
 */
void registerAudioEngine(lua_State* L, AudioEngine* engine);

} // namespace zenith
