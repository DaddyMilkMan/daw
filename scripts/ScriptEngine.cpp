/**
 * ScriptEngine.cpp
 * Implementation of Lua scripting engine
 */

#include "ScriptEngine.h"
#include "AudioEngineBindings.h"
#include "../apps/desktop/Source/engine/Engine.h"

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

#include <iostream>

namespace zenith {

ScriptEngine::ScriptEngine(Engine* engine)
    : m_audioEngine(engine)
{
    initializeLua();
}

ScriptEngine::~ScriptEngine()
{
    if (m_luaState) {
        lua_close(m_luaState);
    }
}

void ScriptEngine::initializeLua()
{
    // Create new Lua state
    m_luaState = luaL_newstate();

    if (!m_luaState) {
        std::cerr << "Failed to create Lua state" << std::endl;
        return;
    }

    // Load standard Lua libraries
    luaL_openlibs(m_luaState);

    // Register our custom bindings
    registerAudioEngineBindings();
    registerStandardLibrary();

    std::cout << "Lua scripting engine initialized" << std::endl;
}

void ScriptEngine::registerAudioEngineBindings()
{
    if (!m_luaState || !m_audioEngine) return;

    // Register audio engine bindings (implemented in AudioEngineBindings.cpp)
    zenith::registerAudioEngine(m_luaState, m_audioEngine);
}

void ScriptEngine::registerStandardLibrary()
{
    if (!m_luaState) return;

    // Register helper print function
    const char* helperFunctions = R"(
        -- Helper: Print with timestamp
        function log(message)
            print("[LUA] " .. tostring(message))
        end

        -- Helper: Sleep (for non-real-time scripts)
        function sleep(seconds)
            local start = os.clock()
            while os.clock() - start < seconds do end
        end

        -- Helper: Map value from one range to another
        function map(value, inMin, inMax, outMin, outMax)
            return (value - inMin) * (outMax - outMin) / (inMax - inMin) + outMin
        end

        -- Helper: Clamp value
        function clamp(value, min, max)
            if value < min then return min end
            if value > max then return max end
            return value
        end

        -- MUSIC THEORY LIBRARY (Complaint #4 Fix: Algorithmic Creativity)
        -- ===============================================================
        
        music = {
            scales = {
                major = {0, 2, 4, 5, 7, 9, 11},
                minor = {0, 2, 3, 5, 7, 8, 10},
                pentatonic_major = {0, 2, 4, 7, 9},
                pentatonic_minor = {0, 3, 5, 7, 10}
            },
            
            -- Map a 0-based degree to a MIDI note in a key
            get_note = function(root, scale, degree)
                local octave = math.floor(degree / #scale)
                local note_idx = (degree % #scale) + 1
                return root + (octave * 12) + scale[note_idx]
            end,
            
            -- Euclidean rhythm generator
            euclidean = function(pulses, steps)
                local pattern = {}
                local bucket = 0
                for i = 1, steps do
                    bucket = bucket + pulses
                    if bucket >= steps then
                        bucket = bucket - steps
                        table.insert(pattern, true)
                    else
                        table.insert(pattern, false)
                    end
                end
                return pattern
            end
        }

        log("Zenith DAW Lua environment loaded with Music Theory Lib")
    )";

    std::string errorMsg;
    executeScript(helperFunctions, errorMsg);
}

bool ScriptEngine::executeScript(const std::string& script, std::string& errorMessage)
{
    if (!m_luaState) {
        errorMessage = "Lua state not initialized";
        return false;
    }

    // Load script
    int loadResult = luaL_loadstring(m_luaState, script.c_str());

    if (loadResult != LUA_OK) {
        errorMessage = lua_tostring(m_luaState, -1);
        lua_pop(m_luaState, 1);
        return false;
    }

    // Execute script
    int execResult = lua_pcall(m_luaState, 0, 0, 0);

    if (execResult != LUA_OK) {
        errorMessage = lua_tostring(m_luaState, -1);
        lua_pop(m_luaState, 1);
        return false;
    }

    return true;
}

bool ScriptEngine::executeScriptFile(const std::string& filepath, std::string& errorMessage)
{
    if (!m_luaState) {
        errorMessage = "Lua state not initialized";
        return false;
    }

    // Load file
    int loadResult = luaL_loadfile(m_luaState, filepath.c_str());

    if (loadResult != LUA_OK) {
        errorMessage = std::string("Failed to load file: ") + lua_tostring(m_luaState, -1);
        lua_pop(m_luaState, 1);
        return false;
    }

    // Execute
    int execResult = lua_pcall(m_luaState, 0, 0, 0);

    if (execResult != LUA_OK) {
        errorMessage = lua_tostring(m_luaState, -1);
        lua_pop(m_luaState, 1);
        return false;
    }

    return true;
}

bool ScriptEngine::callFunction(const std::string& functionName, std::string& errorMessage)
{
    if (!m_luaState) {
        errorMessage = "Lua state not initialized";
        return false;
    }

    lua_getglobal(m_luaState, functionName.c_str());

    if (!lua_isfunction(m_luaState, -1)) {
        errorMessage = "Function not found: " + functionName;
        lua_pop(m_luaState, 1);
        return false;
    }

    int result = lua_pcall(m_luaState, 0, 0, 0);

    if (result != LUA_OK) {
        errorMessage = lua_tostring(m_luaState, -1);
        lua_pop(m_luaState, 1);
        return false;
    }

    return true;
}

void ScriptEngine::setGlobalNumber(const std::string& name, double value)
{
    if (m_luaState) {
        lua_pushnumber(m_luaState, value);
        lua_setglobal(m_luaState, name.c_str());
    }
}

void ScriptEngine::setGlobalString(const std::string& name, const std::string& value)
{
    if (m_luaState) {
        lua_pushstring(m_luaState, value.c_str());
        lua_setglobal(m_luaState, name.c_str());
    }
}

void ScriptEngine::setGlobalBool(const std::string& name, bool value)
{
    if (m_luaState) {
        lua_pushboolean(m_luaState, value ? 1 : 0);
        lua_setglobal(m_luaState, name.c_str());
    }
}

} // namespace zenith
