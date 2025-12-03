/*
  ==============================================================================

    MockAIProvider.cpp
    Created: 2025-12-02
    Author:  Zenith DAW

  ==============================================================================
*/

#include "MockAIProvider.h"
#include <random>

namespace zenith {

juce::String MockAIProvider::processRequest(const juce::String& jsonRequest) {
    juce::var requestVar;
    auto result = juce::JSON::parse(jsonRequest, requestVar);

    if (result.failed()) {
        return JSON::toString(juce::DynamicObject::Ptr(new juce::DynamicObject())); 
    }

    juce::String requestId = requestVar.getProperty("requestId", "").toString();
    juce::String text = requestVar.getProperty("text", "").toString().toLowerCase();

    juce::Array<juce::var> commands;
    juce::String thought;

    if (text.contains("drum") || text.contains("beat")) {
        thought = "Generating a Euclidean drum beat (4/4, Trap style).";
        commands.addArray(generateDrums(text));
    } else if (text.contains("bass")) {
        thought = "Generating a pentatonic bassline.";
        commands.addArray(generateBass(text));
    } else if (text.contains("melody") || text.contains("lead")) {
        thought = "Composing a melody in C Minor.";
        commands.addArray(generateMelody(text));
    } else if (text.contains("chord")) {
        thought = "Generating a chord progression.";
        commands.addArray(generateChords(text));
    } else {
        thought = "I'll make a simple melody.";
        commands.addArray(generateMelody(text));
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

// Helper to create a command object
juce::var createCommand(const juce::String& name, juce::DynamicObject* params) {
    juce::DynamicObject::Ptr cmd = new juce::DynamicObject();
    cmd->setProperty("command", name);
    cmd->setProperty("params", params);
    return juce::var(cmd);
}

juce::Array<juce::var> MockAIProvider::generateMelody(const juce::String& description) {
    juce::Array<juce::var> batch;
    juce::String trackId = "track_0"; 
    juce::String clipId = "clip_ai_melody";

    // 1. Create Track (if not exists - simulated)
    // batch.add(createCommand("create_track", ...));

    // 2. Create Clip
    juce::DynamicObject::Ptr clipParams = new juce::DynamicObject();
    clipParams->setProperty("trackId", trackId);
    clipParams->setProperty("type", "midi");
    clipParams->setProperty("start", 0);
    clipParams->setProperty("length", 176400); // 4s
    clipParams->setProperty("name", "AI Melody");
    batch.add(createCommand("create_clip", clipParams));

    // 3. Generate Notes
    juce::var notesVar;
    auto* notesArray = notesVar.getArray();

    int scale[] = { 0, 2, 3, 5, 7, 8, 10 }; // C Natural Minor
    int root = 72; // C5
    int currentNoteIndex = 0;
    
    // Seed
    thread_local static std::mt19937 rng(12345); 
    std::uniform_int_distribution<int> stepDist(-2, 2); 
    std::uniform_int_distribution<int> rhythmDist(0, 2); 

    double currentBeat = 0.0;
    for (int i = 0; i < 8; ++i) {
        double durations[] = { 0.5, 0.5, 1.0 };
        double duration = durations[rhythmDist(rng)];
        
        int step = stepDist(rng);
        currentNoteIndex += step;
        if (currentNoteIndex < -7) currentNoteIndex = -7;
        if (currentNoteIndex > 7) currentNoteIndex = 7;
        
        int octave = currentNoteIndex / 7;
        int degree = currentNoteIndex % 7;
        if (degree < 0) { degree += 7; octave--; }
        
        int pitch = root + (octave * 12) + scale[degree];
        
        juce::DynamicObject::Ptr note = new juce::DynamicObject();
        note->setProperty("pitch", pitch);
        note->setProperty("startBeats", currentBeat);
        note->setProperty("lengthBeats", duration * 0.9);
        note->setProperty("velocity", 90 + (i % 2 == 0 ? 10 : -10));
        
        notesArray->add(juce::var(note));
        currentBeat += duration;
    }

    juce::DynamicObject::Ptr setNotesParams = new juce::DynamicObject();
    setNotesParams->setProperty("trackId", trackId);
    setNotesParams->setProperty("clipId", clipId); // Assumed ID from create_clip (requires API update to return ID or use temp ID)
    // Since CommandAPI generateUniqueId, we can't predict. 
    // NOTE: This Mock implementation assumes the Controller will execute this intelligently 
    // or that we are just demonstrating the algos.
    setNotesParams->setProperty("notes", notesVar);
    
    batch.add(createCommand("set_clip_notes", setNotesParams));
    return batch;
}

juce::Array<juce::var> MockAIProvider::generateDrums(const juce::String& description) {
    juce::Array<juce::var> batch;
    juce::String trackId = "track_1"; 
    juce::String clipId = "clip_ai_drums";

    juce::DynamicObject::Ptr clipParams = new juce::DynamicObject();
    clipParams->setProperty("trackId", trackId);
    clipParams->setProperty("type", "midi");
    clipParams->setProperty("start", 0);
    clipParams->setProperty("length", 176400);
    clipParams->setProperty("name", "AI Drums");
    batch.add(createCommand("create_clip", clipParams));

    juce::var notesVar;
    auto* notesArray = notesVar.getArray();

    // Euclidean Rhythm: 4 kicks in 16 steps, 8 hihats in 16 steps
    auto addEuclidean = [&](int pitch, int steps, int pulses) {
        int bucket = 0;
        for (int i = 0; i < steps; ++i) {
            bucket += pulses;
            if (bucket >= steps) {
                bucket -= steps;
                juce::DynamicObject::Ptr note = new juce::DynamicObject();
                note->setProperty("pitch", pitch);
                note->setProperty("startBeats", i * 0.25);
                note->setProperty("lengthBeats", 0.25);
                note->setProperty("velocity", 100);
                notesArray->add(juce::var(note));
            }
        }
    };

    addEuclidean(36, 16, 5); // Kick
    addEuclidean(42, 16, 12); // Hihat
    addEuclidean(38, 16, 4); // Snare (offset? simple euclidean puts on 1)
    
    juce::DynamicObject::Ptr setNotesParams = new juce::DynamicObject();
    setNotesParams->setProperty("trackId", trackId);
    setNotesParams->setProperty("clipId", clipId);
    setNotesParams->setProperty("notes", notesVar);
    
    batch.add(createCommand("set_clip_notes", setNotesParams));
    return batch;
}

juce::Array<juce::var> MockAIProvider::generateBass(const juce::String& description) {
    return generateMelody(description); // Reuse for now
}

juce::Array<juce::var> MockAIProvider::generateChords(const juce::String& description) {
    return generateMelody(description); // Reuse
}
