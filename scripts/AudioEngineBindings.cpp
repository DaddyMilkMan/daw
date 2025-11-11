/**
 * AudioEngineBindings.cpp
 * Expose AudioEngine functions to Lua
 */

#include "AudioEngineBindings.h"
#include "../src/audio/AudioEngine.h"

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

namespace vexel {

// Global pointer to engine (accessed by Lua C functions)
static AudioEngine* g_engine = nullptr;

// ============================================================================
// Transport Control Bindings
// ============================================================================

static int lua_play(lua_State* L) {
    if (g_engine) {
        g_engine->play();
    }
    return 0;
}

static int lua_stop(lua_State* L) {
    if (g_engine) {
        g_engine->stop();
    }
    return 0;
}

static int lua_setTempo(lua_State* L) {
    double bpm = luaL_checknumber(L, 1);
    if (g_engine) {
        g_engine->setTempo(bpm);
    }
    return 0;
}

static int lua_getTempo(lua_State* L) {
    if (g_engine) {
        lua_pushnumber(L, g_engine->getTempo());
        return 1;
    }
    return 0;
}

// ============================================================================
// Track Management Bindings
// ============================================================================

static int lua_addAudioTrack(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    if (g_engine) {
        Track* track = g_engine->addAudioTrack(name);
        if (track) {
            lua_pushinteger(L, track->getId());
            return 1;
        }
    }
    return 0;
}

static int lua_addMidiTrack(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    if (g_engine) {
        Track* track = g_engine->addMidiTrack(name);
        if (track) {
            lua_pushinteger(L, track->getId());
            return 1;
        }
    }
    return 0;
}

static int lua_removeTrack(lua_State* L) {
    int trackId = static_cast<int>(luaL_checkinteger(L, 1));
    if (g_engine) {
        g_engine->removeTrack(trackId);
    }
    return 0;
}

static int lua_setTrackVolume(lua_State* L) {
    int trackId = static_cast<int>(luaL_checkinteger(L, 1));
    float volume = static_cast<float>(luaL_checknumber(L, 2));

    if (g_engine) {
        if (Track* track = g_engine->getTrack(trackId)) {
            track->setVolume(volume);
        }
    }
    return 0;
}

static int lua_setTrackPan(lua_State* L) {
    int trackId = static_cast<int>(luaL_checkinteger(L, 1));
    float pan = static_cast<float>(luaL_checknumber(L, 2));

    if (g_engine) {
        if (Track* track = g_engine->getTrack(trackId)) {
            track->setPan(pan);
        }
    }
    return 0;
}

static int lua_setTrackMute(lua_State* L) {
    int trackId = static_cast<int>(luaL_checkinteger(L, 1));
    bool mute = lua_toboolean(L, 2);

    if (g_engine) {
        if (Track* track = g_engine->getTrack(trackId)) {
            track->setMute(mute);
        }
    }
    return 0;
}

static int lua_getNumTracks(lua_State* L) {
    if (g_engine) {
        lua_pushinteger(L, g_engine->getNumTracks());
        return 1;
    }
    return 0;
}

// ============================================================================
// Master Output Bindings
// ============================================================================

static int lua_setMasterVolume(lua_State* L) {
    float volumeDb = static_cast<float>(luaL_checknumber(L, 1));
    if (g_engine) {
        g_engine->setMasterVolume(volumeDb);
    }
    return 0;
}

static int lua_getMasterVolume(lua_State* L) {
    if (g_engine) {
        lua_pushnumber(L, g_engine->getMasterVolume());
        return 1;
    }
    return 0;
}

// ============================================================================
// Status Queries
// ============================================================================

static int lua_isPlaying(lua_State* L) {
    if (g_engine) {
        lua_pushboolean(L, g_engine->isPlaying() ? 1 : 0);
        return 1;
    }
    return 0;
}

static int lua_getCpuLoad(lua_State* L) {
    if (g_engine) {
        lua_pushnumber(L, g_engine->getCpuLoad());
        return 1;
    }
    return 0;
}

// ============================================================================
// Registration Function
// ============================================================================

void registerAudioEngine(lua_State* L, AudioEngine* engine)
{
    g_engine = engine;

    // Create "daw" table in Lua global scope
    lua_newtable(L);

    // Transport control
    lua_pushcfunction(L, lua_play);
    lua_setfield(L, -2, "play");

    lua_pushcfunction(L, lua_stop);
    lua_setfield(L, -2, "stop");

    lua_pushcfunction(L, lua_setTempo);
    lua_setfield(L, -2, "setTempo");

    lua_pushcfunction(L, lua_getTempo);
    lua_setfield(L, -2, "getTempo");

    // Track management
    lua_pushcfunction(L, lua_addAudioTrack);
    lua_setfield(L, -2, "addAudioTrack");

    lua_pushcfunction(L, lua_addMidiTrack);
    lua_setfield(L, -2, "addMidiTrack");

    lua_pushcfunction(L, lua_removeTrack);
    lua_setfield(L, -2, "removeTrack");

    lua_pushcfunction(L, lua_setTrackVolume);
    lua_setfield(L, -2, "setTrackVolume");

    lua_pushcfunction(L, lua_setTrackPan);
    lua_setfield(L, -2, "setTrackPan");

    lua_pushcfunction(L, lua_setTrackMute);
    lua_setfield(L, -2, "setTrackMute");

    lua_pushcfunction(L, lua_getNumTracks);
    lua_setfield(L, -2, "getNumTracks");

    // Master output
    lua_pushcfunction(L, lua_setMasterVolume);
    lua_setfield(L, -2, "setMasterVolume");

    lua_pushcfunction(L, lua_getMasterVolume);
    lua_setfield(L, -2, "getMasterVolume");

    // Status
    lua_pushcfunction(L, lua_isPlaying);
    lua_setfield(L, -2, "isPlaying");

    lua_pushcfunction(L, lua_getCpuLoad);
    lua_setfield(L, -2, "getCpuLoad");

    // Set as global "daw"
    lua_setglobal(L, "daw");
}

} // namespace vexel
