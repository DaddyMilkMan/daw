# Contributing to Zenith DAW

**Note:** Zenith DAW is **100% proprietary closed-source software**. This document provides development guidelines for internal development only. There are no open-source components at this time.

## Development Setup

1. Clone with submodules:
```bash
git clone --recursive https://github.com/zenith-daw/zenith.git
cd zenith/daw
```

2. Build:
```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON
cmake --build build -j$(nproc)
```

3. Run tests:
```bash
./build/ZenithDAWTests_artefacts/Release/ZenithDAWTests
```

## Code Style

- C++20 standard
- 4-space indentation
- `camelCase` for functions/variables
- `PascalCase` for classes
- Doxygen comments for public APIs

## Pull Request Process (Future Open Source Components)

**For Synth Engine (when open-sourced):**
1. Create a feature branch from `develop`
2. Write tests for new functionality
3. Ensure all tests pass
4. Update documentation if needed
5. Submit PR against `develop`

**For Internal Development:**
- Follow company branching and review processes
- Contact engineering leads for guidance

## Architecture Guidelines

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for details.

### Key Rules

1. **Audio Thread**: No allocations, no locks, no logging
2. **State**: All project state in `ProjectState` (ValueTree)
3. **UI**: Use `SkiaComponent` for new components
4. **Testing**: Add tests for new features

## Reporting Issues

Include:
- OS and version
- Build configuration
- Steps to reproduce
- Expected vs actual behavior
- Relevant logs
