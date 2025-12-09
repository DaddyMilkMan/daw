# Contributing to Zenith

Thank you for your interest in contributing to Zenith! We aim to build a professional-grade DAW, and your help is welcome.

## Code Style & Standards

We use **C++20**. Please ensure your compiler supports it.

### General Guidelines
- **Formatting**: We use `clang-format`. Please run it before committing.
- **Naming**:
    - Classes/Structs: `PascalCase` (e.g., `AudioEngine`)
    - Functions/Methods: `camelCase` (e.g., `processAudio`)
    - Variables: `camelCase` (e.g., `sampleRate`)
    - Members: `camelCase` (no prefix, or `m_` prefix if preferred by team convention - *check existing code*).
    - Constants: `kPascalCase` or `UPPER_CASE`.
- **Modern C++**:
    - Use `auto` where the type is obvious.
    - Use `std::unique_ptr` and `std::shared_ptr` for ownership. **Avoid raw pointers** unless non-owning.
    - Use `[[nodiscard]]` for functions with return values that shouldn't be ignored.
    - Use `std::span` for array views (replaces pointer+size pairs).
    - Use **Concepts** (`template <typename T> requires ...`) to constrain template parameters, especially for DSP code.

### Real-Time Safety (CRITICAL)

Code running in the `Engine`'s audio callback (e.g., `processBlock`) must be **Real-Time Safe**.

| Context | Allowed | Forbidden |
| :--- | :--- | :--- |
| **Audio Thread** | Atomics, Lock-free queues, Stack allocation | `malloc`/`free`, `new`/`delete`, Mutexes/Locks, File I/O, `printf`/`std::cout`, Exceptions |
| **Message/UI Thread** | Locks, File I/O, Allocations | Touching audio graph directly (use CommandAPI) |

**DO:**
- Use pre-allocated buffers.
- Use lock-free data structures (e.g., `juce::AbstractFifo`, `moodycamel::ConcurrentQueue`) for communicating with the UI.
- Use atomic variables for parameters.

## JUCE & Skia Patterns

- **Components**: Always use `JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClassName)` in your component headers.
- **Layout**: Use `juce::FlexBox` or `juce::Grid` for complex layouts in `resized()`. Avoid magic numbers; use constants.
- **Skia**:
    - If drawing with Skia, ensure you are drawing to the offscreen surface provided by `SkiaCanvasComponent`.
    - Do not make direct OpenGL calls; use the Skia abstraction.

## Git Workflow

1.  **Fork & Clone**: Fork the repo and clone it locally.
2.  **Branch**: Create a feature branch: `git checkout -b feat/my-new-feature`.
3.  **Commit**: Use [Conventional Commits](https://www.conventionalcommits.org/):
    - `feat: add piano roll velocity lane`
    - `fix: resolve crash in audio engine`
    - `docs: update architecture diagrams`
    - `refactor: simplify track header layout`
4.  **Test**: Run unit tests (`ctest`) to ensure no regressions.
5.  **Pull Request**: Push to your fork and open a PR against `master`.

## Documentation

- Update `docs/` if you change architecture or add major features.
- Add Doxygen-style comments (`///`) to public headers.

## Directory Structure

- Put **Logic** in `zenith-core/src` and `zenith-core/include`.
- Put **UI** in `zenith-core/Source/ui`.
- Put **Tests** in `zenith-core/tests`.

Happy Coding!
