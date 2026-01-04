# 🧠 AI MIDI GENERATION - INTEGRATION GUIDE
**Date**: 2025-11-30 19:10 PST
**Status**: CORE ENGINE READY

---

## 🏗️ ARCHITECTURE OVERVIEW

We have built a "Neural Bridge" that allows Zenith DAW to communicate directly with Large Language Models (LLMs) like Grok or GPT-4 to generate MIDI data.

### Core Components
1.  **`AiDataStructures.h`**: Defines the protocol (`AiProjectContext`, `AiMidiNote`).
2.  **`AiBridge` Service**: Handles the asynchronous HTTP communication.
3.  **`toMidiMessageSequence`**: Converts AI output to JUCE MIDI.

---

## 🔌 HOW TO USE (For UI Team)

To add a "Generate Melody" button to a track, follow this pattern:

```cpp
// 1. Create the Bridge (Member variable or Singleton)
zenith::ai::AiBridge aiBridge;
aiBridge.setApiKey("your-api-key-here"); // Optional: Leave empty for Simulation Mode

// 2. Prepare Context
zenith::ai::AiProjectContext context;
context.bpm = 128.0;
context.keyRoot = "F#";
context.keyScale = "Minor";
context.genre = "Cyberpunk";
context.userPrompt = "Create a driving, arpeggiated bassline.";

// 3. Call Generate (Non-blocking)
aiBridge.generateMidi(context, [this](zenith::ai::AiGenerationResult result) {
    if (result.success) {
        // 4. Convert to MIDI
        auto midiSequence = result.toMidiMessageSequence();
        
        // 5. Add to Track (Pseudo-code)
        myTrack->addMidiClip(midiSequence, 0.0); // Add at start
        
        DBG("Generated " + juce::String(result.notes.size()) + " notes!");
    } else {
        DBG("Error: " + result.errorMessage);
    }
});
```

---

## 🤖 SIMULATION MODE
If no API Key is provided, the `AiBridge` automatically enters **Simulation Mode**.
It will generate a procedural 8th-note bassline pattern. This allows you to test the UI and data flow without needing a paid API subscription.

---

## 🔮 FUTURE EXPANSION
- **Harmonization**: Pass existing track notes in `AiProjectContext` so the AI can harmonize.
- **Automation**: Add `AiAutomationCurve` to the data structures.
- **Lyrics**: Add a `generateLyrics` method to `AiBridge`.
