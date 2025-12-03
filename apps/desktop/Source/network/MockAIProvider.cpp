/*
  ==============================================================================

    MockAIProvider.cpp
    Created: 2025-12-02
    Author:  Zenith DAW

  ==============================================================================
*/

#include "MockAIProvider.h"

namespace zenith {

juce::String MockAIProvider::processRequest(const juce::String& jsonRequest) {
    juce::var requestVar;
    auto result = juce::JSON::parse(jsonRequest, requestVar);

    if (result.failed()) {
        return JSON::toString(juce::DynamicObject::Ptr(new juce::DynamicObject())); // Empty error
    }

    juce::String requestId = requestVar.getProperty("requestId", "").toString();
    juce::String text = requestVar.getProperty("text", "").toString().toLowerCase();

    juce::Array<juce::var> commands;
    juce::String thought;

    // Simple keyword matching for mock generation
    if (text.contains("drum") || text.contains("beat")) {
        thought = "I'll create a drum beat for you.";
        commands.add(generateDrums(text));
    } else if (text.contains("bass")) {
        thought = "Generating a bassline.";
        commands.add(generateBass(text));
    } else if (text.contains("melody") || text.contains("lead")) {
        thought = "Composing a melody.";
        commands.add(generateMelody(text));
    } else if (text.contains("chord")) {
        thought = "Generating a chord progression.";
        commands.add(generateChords(text));
    } else {
        thought = "I'm not sure what you mean, so I'll make a simple melody.";
        commands.add(generateMelody(text));
    }

    return createResponse(requestId, thought, commands);
}

juce::String MockAIProvider::createResponse(const juce::String& requestId, 
                                          const juce::String& thought, 
                                          const juce::Array<juce::var>& commands) {
    juce::DynamicObject::Ptr response = new juce::DynamicObject();
    response->setProperty("type", "wingman_nl_response");
    response->setProperty("requestId", requestId);
    response->setProperty("status", "ok");
    response->setProperty("thought", thought);
    
    juce::var commandsArray(commands);
    response->setProperty("commands", commandsArray);

    return juce::JSON::toString(response);
}

juce::var MockAIProvider::generateDrums(const juce::String& description) {
    juce::DynamicObject::Ptr cmd = new juce::DynamicObject();
    cmd->setProperty("command", "create_clip");
    
    juce::DynamicObject::Ptr params = new juce::DynamicObject();
    params->setProperty("trackType", "midi");
    params->setProperty("instrument", "ZenithSampler");
    params->setProperty("preset", "Trap Kit 1");
    params->setProperty("notes", "36:0:0.25, 38:1:0.25, 42:0:0.125"); // Simple mock format
    
    cmd->setProperty("params", params);
    return cmd;
}

juce::var MockAIProvider::generateBass(const juce::String& description) {
    juce::DynamicObject::Ptr cmd = new juce::DynamicObject();
    cmd->setProperty("command", "create_clip");
    
    juce::DynamicObject::Ptr params = new juce::DynamicObject();
    params->setProperty("trackType", "midi");
    params->setProperty("instrument", "ZenithPolySynth");
    params->setProperty("preset", "Deep Bass");
    params->setProperty("notes", "36:0:1.0, 36:2:1.0");
    
    cmd->setProperty("params", params);
    return cmd;
}

juce::var MockAIProvider::generateMelody(const juce::String& description) {
    juce::DynamicObject::Ptr cmd = new juce::DynamicObject();
    cmd->setProperty("command", "create_clip");
    
    juce::DynamicObject::Ptr params = new juce::DynamicObject();
    params->setProperty("trackType", "midi");
    params->setProperty("instrument", "ZenithPolySynth");
    params->setProperty("preset", "Pluck Lead");
    params->setProperty("notes", "60:0:0.25, 62:0.25:0.25, 64:0.5:0.25, 67:0.75:0.25");
    
    cmd->setProperty("params", params);
    return cmd;
}

juce::var MockAIProvider::generateChords(const juce::String& description) {
    juce::DynamicObject::Ptr cmd = new juce::DynamicObject();
    cmd->setProperty("command", "create_clip");
    
    juce::DynamicObject::Ptr params = new juce::DynamicObject();
    params->setProperty("trackType", "midi");
    params->setProperty("instrument", "ZenithPolySynth");
    params->setProperty("preset", "Warm Pad");
    params->setProperty("notes", "60:0:4.0, 64:0:4.0, 67:0:4.0");
    
    cmd->setProperty("params", params);
    return cmd;
}

} // namespace zenith
