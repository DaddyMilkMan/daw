/**
 * ScriptEngine.h
 * Lua scripting engine for DAW automation
 *
 * Allows users to write Lua scripts to automate:
 * - Track creation/deletion
 * - Volume/pan automation
 * - MIDI generation
 * - Plugin parameter control
 * - Batch processing
 */

#pragma once

#include <memory>
#include <string>
#include <functional>

// Forward declare Lua state
struct lua_State;

namespace zenith {

class Engine;

/**
 * Lua scripting engine
 */
class ScriptEngine
{
public:
    explicit ScriptEngine(Engine* engine);
    ~ScriptEngine();

    // ========================================================================
    // Script Execution
    // ========================================================================

    /**
     * Execute a Lua script from string
     * @return true if successful, false if error
     */
    bool executeScript(const std::string& script, std::string& errorMessage);

    /**
     * Execute a Lua script from file
     * @return true if successful, false if error
     */
    bool executeScriptFile(const std::string& filepath, std::string& errorMessage);

    /**
     * Call a Lua function by name
     */
    bool callFunction(const std::string& functionName, std::string& errorMessage);

    // ========================================================================
    // Environment
    // ========================================================================

    /**
     * Register global variable accessible to scripts
     */
    void setGlobalNumber(const std::string& name, double value);
    void setGlobalString(const std::string& name, const std::string& value);
    void setGlobalBool(const std::string& name, bool value);

    /**
     * Get Lua state for advanced usage
     */
    lua_State* getLuaState() { return m_luaState; }

    // ========================================================================
    // Built-in Helper Functions
    // ========================================================================

    /**
     * Register standard library of helper functions
     */
    void registerStandardLibrary();

private:
    lua_State* m_luaState = nullptr;
    AudioEngine* m_audioEngine = nullptr;

    void initializeLua();
    void registerAudioEngineBindings();
    void handleLuaError(const std::string& context);

    // Prevent copying
    ScriptEngine(const ScriptEngine&) = delete;
    ScriptEngine& operator=(const ScriptEngine&) = delete;
};

} // namespace zenith
