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
    luaL_openlibs(L);
}

ScriptableProcessor::~ScriptableProcessor() {
    const juce::ScopedLock sl (scriptLock);
    if (L) lua_close(L);
}

void ScriptableProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) {
    const juce::ScopedLock sl (scriptLock);
    if (!L) return;
    lua_pushnumber(L, sampleRate);
    lua_setglobal(L, "SAMPLE_RATE");
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

} // namespace zenith
