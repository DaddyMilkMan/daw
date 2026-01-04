# Copilot Guidance for micahcooley/daw

**You are assisting in a Digital Audio Workstation (DAW) codebase. Please follow these best practices for all code in this repository:**

---

## General Project Guidelines

- Write clean, modular, and well-documented code appropriate for DAW applications.
- Prioritize real-time safety, especially for audio-thread code (no memory allocations or heavy computation in RT threads).
- Follow PEP8 (Python) and C++ Core Guidelines.
- Avoid unsafe, blocking, or non-deterministic operations in audio-critical sections.
- Prefer pure functions and immutable data for DSP operations.
- Annotate all types (type hints for Python, explicit types for C++).

---

## Python (67.8%)
- Always use type hints and meaningful variable/function names.
- Prefer numpy/scipy for DSP, and validate array bounds.
- Handle exceptions gracefully, especially for audio I/O and threading.
- Write docstrings for all functions and classes.

## C++ (30.7%)
- Use RAII and smart pointers for resource management.
- Minimize heap allocation in audio processing paths.
- Use const-correctness, and mark functions noexcept where possible.
- Write descriptive Doxygen-style comments.

## Build Scripts (Makefile, CMake, PowerShell)
- Write portable, reproducible scripts for all supported platforms.
- Comment any non-obvious build logic and ensure clear dependency management.

---

## Testing and Quality
- Prefer TDD: include unit tests for all new audio or DSP components.
- Validate edge cases (denormals, buffer wraparound, multi-threading).
- All code must be thoroughly tested and free of common C/C++/DSP bugs.

---

## Documentation
- Ensure all new public APIs are documented.
- Add clear comments explaining DSP flows, audio routing, and threading models.
- Use standard terminology for audio, MIDI, and DAW concepts.

---

**You are helping with serious, production-quality, cross-platform DAW code. All code must be secure, performant, clear, and auditable. Never generate or suggest hardcoded secrets, credentials, or unsafe operations.**

---

_Copilot: Always follow these instructions for any code in this repository!_
