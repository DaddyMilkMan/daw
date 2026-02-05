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

    ScriptableProcessor.h
    Created: 2025-12-27
    Author:  Zenith DAW

    A Lua-powered Audio Processor that runs custom DSP code in the audio thread.
    Allows Grok to "Write an effect" and have it run instantly.


  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <lua.hpp>
#include <atomic>
#include <string>

namespace zenith {

class ScriptableProcessor : public juce::AudioProcessor {
public:
    ScriptableProcessor();
    ~ScriptableProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    const juce::String getName() const override { return "Scriptable Effect"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==============================================================================
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    /**
     * @brief Update the DSP script. 
     * @param script Lua code containing a process(sample, channel) or processBlock(buffer) function.
     */
    void setScript(const juce::String& script);

private:
    lua_State* L = nullptr;
    juce::CriticalSection scriptLock; // Prevents Message Thread from changing script while Audio Thread is running
    std::atomic<bool> isScriptValid{false};
    juce::String currentScript;
    
    // Internal helper to wrap JUCE buffer for Lua
    void pushBufferToLua(juce::AudioBuffer<float>& buffer);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScriptableProcessor)
};

} // namespace zenith
