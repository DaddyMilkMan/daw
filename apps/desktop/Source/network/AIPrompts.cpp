/*
  ==============================================================================

    AIPrompts.cpp
    Created: 2025-12-03
    Author:  Zenith DAW

  ==============================================================================
*/

#include "AIPrompts.h"

namespace zenith {

juce::String AIPrompts::buildSystemPrompt(const std::function<juce::var()>& contextProvider)
{
    juce::String prompt;
    
    // Try to load from file (Complaint #7 Fix)
    // Priority 1: Relative to executable (Deployment)
    juce::File promptFile = juce::File::getSpecialLocation(juce::File::currentApplicationFile)
                               .getSiblingFile("Content")
                               .getChildFile("Prompts")
                               .getChildFile("system_prompt.txt");
                               
    if (!promptFile.existsAsFile())
    {
        // Priority 2: Relative to CWD (Development)
        // Assumes running from project root or build dir
        promptFile = juce::File::getCurrentWorkingDirectory()
                        .getChildFile("apps")
                        .getChildFile("desktop")
                        .getChildFile("Content")
                        .getChildFile("Prompts")
                        .getChildFile("system_prompt.txt");
    }

    if (promptFile.existsAsFile())
    {
        prompt = promptFile.loadFileAsString();
    }
    else
    {
        // Fallback default
        prompt = 
            "You are an AI assistant for Zenith DAW. "
            "You can control the DAW via function calling. "
            "\n\n"
            "Capabilities:\n"
            "- Manage tracks and clips\n"
            "- Control plugins and automation\n"
            "- Load/save presets\n"
            "- Generate synthesizer presets\n"
            "- Edit MIDI\n"
            "- Control transport and project settings\n"
            "- Analyze audio tracks\n"
            "- Search for plugins and inspect signal routing\n"
            "\n"
            "Formatting:\n"
            "- Use Mermaid diagrams (```mermaid) to visualize audio routing.\n"
            "- Use Markdown tables for plugin search results.\n"
            "\n"
            "Use available functions to fulfill user requests. "
            "Confirm actions concisely.\n";
    }
    
    prompt += "\n";
    
    // Add current DAW context if available
    if (contextProvider)
    {
        auto context = contextProvider();
        if (context.isObject())
        {
            prompt += "Current DAW State:\n";
            
            if (context.hasProperty("trackCount"))
                prompt += "- Tracks: " + context["trackCount"].toString() + "\n";
            
            if (context.hasProperty("selectedTrack"))
                prompt += "- Selected Track: " + context["selectedTrack"].toString() + "\n";
            
            if (context.hasProperty("tempo"))
                prompt += "- Tempo: " + context["tempo"].toString() + " BPM\n";
            
            if (context.hasProperty("isPlaying"))
            {
                bool playing = context["isPlaying"];
                prompt += "- Playback: " + juce::String(playing ? "Playing" : "Stopped") + "\n";
            }
            
            prompt += "\n";
        }
    }
    
    return prompt;
}
} // namespace zenith
