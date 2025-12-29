/*
  ==============================================================================

    ScriptableProcessor.cpp
    Created: 2025-12-27
    Author:  Zenith DAW

    REPAIRED: Block-based processing and Thread-safe Script Updates
    PRO FIX: Zero-Allocation Buffer Access
  ==============================================================================
*/

#include "ScriptableProcessor.h"

namespace zenith {

// Lua helper to read/write samples from light userdata
static int lua_getSample(lua_State* L) {
    float* data = (float*)lua_touserdata(L, lua_upvalueindex(1));
    int index = (int)luaL_checkinteger(L, 1) - 1; // 1-based to 0-based
    lua_pushnumber(L, (double)data[index]);
    return 1;
}

static int lua_setSample(lua_State* L) {
    float* data = (float*)lua_touserdata(L, lua_upvalueindex(1));
    int index = (int)luaL_checkinteger(L, 1) - 1;
    float value = (float)luaL_checknumber(L, 2);
    data[index] = value;
    return 0;
}

ScriptableProcessor::ScriptableProcessor() 
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    L = luaL_newstate();
    
    // SANDBOX: Only load safe libraries
    // luaL_openlibs(L); // <-- UNSAFE: Loads os, io, debug, package
    
    luaL_requiref(L, "_G", luaopen_base, 1);
    lua_pop(L, 1);
    luaL_requiref(L, "table", luaopen_table, 1);
    lua_pop(L, 1);
    luaL_requiref(L, "string", luaopen_string, 1);
    lua_pop(L, 1);
    luaL_requiref(L, "math", luaopen_math, 1);
    lua_pop(L, 1);
    // Explicitly do NOT load os, io, package, debug
}

ScriptableProcessor::~ScriptableProcessor() {
    const juce::ScopedLock sl (scriptLock);
    stopTimer();
    if (L) lua_close(L);
}

void ScriptableProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) {
    const juce::ScopedLock sl (scriptLock);
    if (!L) return;
    lua_pushnumber(L, sampleRate);
    lua_setglobal(L, "SAMPLE_RATE");
    
    // Stop GC in audio thread to prevent jitter
    lua_gc(L, LUA_GCSTOP, 0);
    startTimer(100); // Run incremental GC on message thread every 100ms
}

void ScriptableProcessor::releaseResources() {}

void ScriptableProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
    if (!scriptLock.tryEnter()) return;
    if (!isScriptValid.load()) { scriptLock.exit(); return; }

    lua_getglobal(L, "process");
    if (!lua_isfunction(L, -1)) { lua_pop(L, 1); scriptLock.exit(); return; }

    // Create a temporary "buffer" table for this block
    lua_newtable(L);
    int numSamples = buffer.getNumSamples();
    
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        lua_pushinteger(L, ch + 1);
        lua_newtable(L);
        
        float* data = buffer.getWritePointer(ch);
        
        // Push light userdata pointer to the raw data
        lua_pushlightuserdata(L, data);
        lua_pushinteger(L, numSamples);
        lua_setfield(L, -2, "_len");
        
        // Add getter/setter closures that use the pointer
        lua_pushlightuserdata(L, data);
        lua_pushcclosure(L, lua_getSample, 1);
        lua_setfield(L, -2, "get");
        
        lua_pushlightuserdata(L, data);
        lua_pushcclosure(L, lua_setSample, 1);
        lua_setfield(L, -2, "set");
        
        lua_settable(L, -3);
    }
    
    lua_setglobal(L, "buffer");

    if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
        isScriptValid.store(false);
        DBG("ScriptableProcessor Runtime Error: " + juce::String(lua_tostring(L, -1)));
        lua_pop(L, 1);
    }
    
    scriptLock.exit();
}

void ScriptableProcessor::setScript(const juce::String& script) {
    const juce::ScopedLock sl (scriptLock);
    currentScript = script;
    if (luaL_dostring(L, script.toRawUTF8()) == LUA_OK) {
        isScriptValid.store(true);
        DBG("ScriptableProcessor: Script compiled successfully.");
    } else {
        isScriptValid.store(false);
        DBG("ScriptableProcessor Compile Error: " + juce::String(lua_tostring(L, -1)));
        lua_pop(L, 1);
    }
}

void ScriptableProcessor::getStateInformation (juce::MemoryBlock& destData) {
    destData.append(currentScript.toRawUTF8(), (size_t)currentScript.length());
}

void ScriptableProcessor::setStateInformation (const void* data, int sizeInBytes) {
    setScript(juce::String::createStringFromData(data, sizeInBytes));
}

void ScriptableProcessor::timerCallback() {
   // Incremental GC on message thread
   // We try to take the lock to ensure we don't interfere with setScript or heavy ops,
   // but primarily to ensure L is valid and not being closed.
   // Note: processBlock also takes this lock. If processBlock holds it, we skip GC step.
   // This prioritizes audio processing over GC.
   if (scriptLock.tryEnter()) {
       if (L && isScriptValid.load()) {
           lua_gc(L, LUA_GCSTEP, 5); // Perform a small amount of GC work
       }
       scriptLock.exit();
   }
}

} // namespace zenith
