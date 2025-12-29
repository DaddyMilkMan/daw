# Zenith DAW Coding Conventions

## Naming Conventions (Bug 57 Fix)

Consistent naming is critical for codebase maintainability.

### 1. Variables
- **Local Variables**: `camelCase` (e.g., `numSamples`, `trackIndex`)
- **Member Variables**: `camelCase_` with trailing underscore (e.g., `sampleRate_`, `trackName_`)
- **Global/Static Constants**: `kPascalCase` or `SCREAMING_SNAKE_CASE` (e.g., `kMaxBufferSize`, `MAX_CHANNELS`)

### 2. Functions
- **Methods**: `camelCase` (e.g., `processBlock`, `getTrackLevel`)
- **Events/Callbacks**: `onEventName` (e.g., `onButtonDown`, `handleTimer`)

### 3. Types
- **Classes/Structs**: `PascalCase` (e.g., `AudioRenderer`, `TrackState`)
- **Enums**: `PascalCase` with `EnumClass` (e.g., `enum class TrackType { Audio, Midi }`)
- **Templates**: `T` or `TPascalCase` (e.g., `template <typename TValue>`)

### 4. Files
- **Source/Header**: `PascalCase` matching class name (e.g., `AudioRenderer.cpp`)
- **Documentation**: `SCREAMING_SNAKE_CASE` or `PascalCase` (e.g., `CODING_CONVENTIONS.md`)

## Best Practices

### Safety
- Always use `std::unique_ptr` or `std::shared_ptr` for ownership.
- Avoid raw pointers unless non-owning (observing).
- Use `jassert` to document invariants.

### Structure
- Initialize member variables in class definitions or constructor initializer lists.
- Place `public`, `protected`, `private` in that order.

### Comments
- Use `//` for implementation comments.
- Use `///` or `/** */` for Doxygen API documentation.
- TODOs should include a ticket number or context (e.g., `// TODO: Fix Bug 42`).
