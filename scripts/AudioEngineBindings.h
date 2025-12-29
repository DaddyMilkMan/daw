/**
 * AudioEngineBindings.h
 * C++ to Lua bindings for zenith::Engine functions
 */

#pragma once

struct lua_State;

namespace zenith {

class Engine; // Use the actual Engine class

/**
 * Register Engine API to Lua
 */
void registerAudioEngine(lua_State* L, Engine* engine);

} // namespace zenith