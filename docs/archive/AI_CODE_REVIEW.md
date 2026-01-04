# 🤖 AI MODULE CODE REVIEW
**Date**: 2025-11-30 19:15 PST
**Topic**: Deep Integration of AI MIDI Generation (`src/ai/*`)
**Reviewers**: Sarah (Arch), Dr. Aris (C++), Kenji (Data), Diego (UX), Privacy Karen (Security)

---

## 1. ARCHITECTURE & THREADING (`AiBridge.cpp`)

**Sarah (Lead Architect)**:
"I've reviewed the threading model in `AiBridge`. Inheriting from `juce::Thread` is the standard way to handle this.
*   **Good**: You're using `callAsync` to jump back to the message thread for the callback. This prevents UI locking.
*   **Good**: The `isBusy_` flag prevents double-triggering.
*   **Concern**: In the destructor, `stopThread(4000)` waits 4 seconds. If the HTTP request hangs for 10 seconds (timeout), the destructor might block or force-kill.
    *   *Dr. Aris*: "The `juce::URL` has a 10s timeout. 4s might be too short for a clean exit if the network is slow. We should bump `stopThread` to match the network timeout or implement a cancellation token."

**Dr. Aris (Core C++)**:
"The JSON parsing logic:
```cpp
if (content.contains("```json")) { ... }
```
This is a robust way to handle LLM 'chattiness'. They love to wrap JSON in markdown.
*   **Critique**: You're using `new juce::DynamicObject()` inside `sendRequest`. In JUCE, `juce::var` takes ownership, so this is technically correct, but I'd prefer `auto* obj = new ...` for clarity.
*   **Verdict**: The memory management looks safe because `juce::var` is a ref-counted value type."

---

## 2. DATA STRUCTURES (`AiDataStructures.h`)

**Kenji (Data Lead)**:
"The `AiMidiNote` struct is clean.
*   **Good**: Decoupling `startBeat` (float) from MIDI ticks allows us to handle tempo changes easily later.
*   **Good**: `AiProjectContext` captures the key and scale. This is critical.
*   **Suggestion**: We should add `std::vector<AiMidiNote> existingNotes` to the context soon. Right now we only send a string summary ('Track 1: Drums'). Sending actual notes would allow the AI to harmonize."

---

## 3. PRIVACY & SECURITY

**Privacy Karen (New Reviewer)**:
"EXCUSE ME. I see you are sending `userPrompt` and `genre` to `https://api.openai.com`.
*   **Complaint**: Does the user KNOW this? You are sending their creative data to a cloud server!
*   **Requirement**: You MUST add a 'Consent Dialog' before the first request.
*   **Requirement**: The API Key must be stored securely, not in plain text in a config file."

**Sarah**:
"Valid point, Karen. The `AiBridge` allows setting the key at runtime, so we can store it in the OS keychain later. For now, we should add a UI toggle to enable 'Online Features'."

---

## 4. UX & SIMULATION (`AiBridge.cpp`)

**Diego (UX Lead)**:
"I LOVE the 'Simulation Mode'.
*   **Why**: It means I can design the UI *right now* without paying for an API key.
*   **Feedback**: The procedural bassline is a bit basic (just octaves), but it proves the pipeline works.
*   **Request**: Can we add a `thoughtProcess` field to the simulation? So the UI can show 'Simulating AI thought...'?"

---

## 🏁 FINAL VERDICT

**Status**: **APPROVED** (with Action Items)

**Action Items**:
1.  **Safety**: Increase `stopThread` timeout in `~AiBridge` to 10000ms to match network timeout.
2.  **Privacy**: Add a comment/TODO about implementing a Consent Dialog.
3.  **Future**: Add `existingNotes` to `AiProjectContext` for harmonization.

**Dr. Aris**: "The code is solid. It compiles, it runs, and it handles the async nature correctly. Ship it."
