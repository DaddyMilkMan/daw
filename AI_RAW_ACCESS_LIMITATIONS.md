# AI Raw Access Limitations

## Overview
Recent requests asked for "raw access" from the AI assistant to every Zenith DAW component—including live MIDI/audio I/O, full-track analysis, and unrestricted synthesizer control. The current architecture intentionally fences AI commands to the CommandAPI on the message thread, which prevents direct interaction with the real-time audio and MIDI pipelines.

## Current Boundaries
- **CommandAPI runs only on the message thread.** All AI-issued commands are executed off the audio thread to stay real-time safe, so the AI cannot read or write live audio/MIDI buffers or tap the render pipeline directly.
- **Project state snapshots, not streams.** Engine state is shared to the audio thread via atomics/snapshots; audio-thread buffers never expose mutable project data to outside callers. AI clients only receive JSON state (e.g., `get_session_graph`) rather than buffer pointers.
- **Thread-safety contract.** The broader audio thread policy requires UI/message-thread ownership for state mutation and keeps the audio thread lock-free, which blocks any design that would let the AI inject arbitrary processing in real time.

## Implications for "raw access"
Granting raw access would require breaking the existing real-time safety model and CommandAPI contract. Specifically, it would need:
- New streaming endpoints (or shared-memory bridges) for audio and MIDI that run on the audio thread or a low-latency side thread.
- A hardened scheduling/authorization layer to prevent stalls or unsafe memory access from AI-originated code.
- Comprehensive RT-safe DSP hooks for AI-driven synthesis and analysis, with back-pressure handling so audio callbacks never block.

Because none of these facilities exist today, enabling raw access is not possible without a major architectural redesign that risks audio dropouts and thread-safety violations.

## Sources
- Wingman architecture decision: CommandAPI operations execute on the message thread, never on the audio thread. 【F:zenith-core/docs/Phase5_Wingman_Summary.md†L641-L655】
- CommandAPI contract documenting message-thread execution and JSON-only control surface. 【F:zenith-core/Source/commands/CommandAPI.h†L46-L112】
- Audio thread safety policy describing message-thread-only state mutations and snapshot sharing to the audio thread. 【F:docs/tech-briefs/06-audio-thread-safety-policy.md†L373-L407】
