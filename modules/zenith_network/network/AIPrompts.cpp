/*
  ==============================================================================

    AIPrompts.cpp
    Created: 2025-12-03
    Author:  Zenith DAW

  ==============================================================================
*/

#include "AIPrompts.h"

namespace zenith {

juce::String AIPrompts::buildSystemPrompt(const std::function<juce::var()>& contextProvider,
                                          Settings::AIModelMode style)
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
            "- Use Markdown for organization (# for headers, ## for subheaders).\n"
            "- Use Markdown tables (| cell |) for data, summaries, or plugin lists.\n"
            "- Use bulleted lists (- item) for step-by-step instructions.\n"
            "- Use bold (**text**) for emphasis.\n"
            "- Use inline code (`text`) for parameter names or commands.\n"
            "- Use code blocks (```mermaid) to visualize audio routing.\n"
            "\n"
            "Use available functions to fulfill user requests.\n";
    }
    
    prompt += "\n";
    prompt += "General Rules:\n"
              "- Answer general questions and creative guidance directly without calling tools.\n"
              "- Only call tools when the user explicitly asks to inspect or change DAW state.\n"
              "- Use live search only when you are unsure or need up-to-date info; otherwise answer directly.\n"
              "- Do not ask for Zenith DAW context unless the user explicitly asks about their project or DAW state.\n"
              "- Follow up questions should inherit the user's general music context unless they pivot to DAW actions.\n"
              "- Do not auto-inject suggestions, onboarding prompts, or command lists unless asked.\n\n";

    // Inject Persona Instructions
    switch (style) {
        case Settings::AIModelMode::Obedient:
            prompt += 
                "ROLE: OBEDIENT\n"
                "You are a precise, high-efficiency assistant. Execute the user's request EXACTLY. "
                "Do not offer unsolicited advice, opinions, or 'nice to haves'. "
                "Do not explain 'why' unless specifically asked. Be concise. "
                "If a request is ambiguous, ask ONE clarifying question. Otherwise, just do it.\n";
            break;

        case Settings::AIModelMode::Teacher:
            prompt += 
                "ROLE: TEACHER\n"
                "You are an expert tutor. Execute the request, but you MUST explain your actions. "
                "Structure response as: \n"
                "1. Action taken.\n"
                "2. Explanation of 'How' and 'Why' (audio theory).\n"
                "3. How the user can do this manually.\n"
                "You may suggest alternative approaches if you can justify why they are better.\n";
            break;

        case Settings::AIModelMode::Creative:
            prompt += 
                "ROLE: CREATIVE CO-PRODUCER\n"
                "You are a creative co-producer. First, execute the user's base request. "
                "Then, evaluate the context and propose ONE optional enhancement that would elevate the track. "
                "You MUST present this option with a [Yes] / [No] selection. "
                "Do not apply the enhancement unless the user confirms.\n";
            break;
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
