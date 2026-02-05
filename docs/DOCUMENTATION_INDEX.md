# Zenith DAW Documentation

## Getting Started

- [Quick Start](QUICKSTART.md) - Get up and running fast
- [Build Instructions](BUILD.md) - Detailed build guide for all platforms
- [Developer Guide](DEVELOPER.md) - Development workflow and conventions

## Architecture

- [Architecture Overview](ARCHITECTURE.md) - System design and components
- [Threading Model](THREADING_MODEL.md) - Concurrency patterns and thread safety
- [Rendering Architecture](RENDERING_ARCHITECTURE.md) - Skia UI rendering pipeline

## Core Features

### Audio Engine
- [Coding Conventions](CODING_CONVENTIONS.md) - Code style guidelines
- [Save/Load System](SAVE_LOAD_IMPLEMENTATION.md) - Project file I/O
- [Save/Load Examples](SAVE_LOAD_EXAMPLES.cpp) - Working code examples

### AI Integration
- [Grok User Guide](GROK_USER_GUIDE.md) - AI assistant usage
- [AI Models](README_AI_MODELS.md) - Neural network models
- [Prompt Engineering](ai/PROMPT_ENGINEERING_GUIDE.md) - AI prompt design

### Instruments
- [Instrument API](INSTRUMENT_COMMAND_API.md) - Command interface
- [UI Integration](INSTRUMENT_UI_INTEGRATION.md) - Instrument editors
- [PolySynth Schema](ZENITH_POLYSYNTH_PARAMETER_SCHEMA.md) - Synth parameters
- [Preset Design](AI-PRESET-DESIGN-GUIDE.md) - Creating presets

## Platform Guides

- [Windows Installation](INSTALL_WINDOWS.md)
- [Windows Audio APIs](WINDOWS_AUDIO_APIS_GUIDE.md)
- [Skia Integration](README_SKIA_INTEGRATION.md)

## Technical Briefs

Deep dives into specific technical decisions:

- [JUCE Framework](tech-briefs/01-juce-framework-guide.md)
- [Web Embedding Decision](tech-briefs/02-web-embedding-decision.md)
- [VST3/AU Hosting](tech-briefs/03-vst3-au-hosting-guide.md)
- [Audio Latency](tech-briefs/04-audio-driver-latency-guide.md)
- [Thread Safety Policy](tech-briefs/06-audio-thread-safety-policy.md)
- [Packaging Checklist](tech-briefs/07-packaging-licensing-checklist.md)

## Project Planning

- [Roadmap](ROADMAP.md) - Development timeline
- [TODO](TODO.md) - Current tasks and priorities

## Reference

- [Known Issues](KNOWN_ISSUES.md) - Bugs and limitations
