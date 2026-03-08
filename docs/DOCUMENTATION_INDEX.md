# Zenith DAW Documentation Index

**Last Updated:** 2026-02-20

---

## Quick Start

- [Status](STATUS.md) - **START HERE** - Current project status and capabilities
- [Build Instructions](BUILD.md) - How to build from source
- [Developer Guide](DEVELOPER.md) - Development workflow and conventions
- [Known Issues](KNOWN_ISSUES.md) - Bugs and limitations

---

## System Documentation

### Architecture
- [Architecture Overview](ARCHITECTURE.md) - System design and component structure
- [Threading Model](THREADING_MODEL.md) - Concurrency patterns and thread safety
- [Rendering Architecture](RENDERING_ARCHITECTURE.md) - Skia UI rendering pipeline

### Core Systems
- **Safety Systems** - Comprehensive safety documentation
  - [Safety Systems Overview](SAFETY.md) - All 12 safety components
  - [Real-Time Safety](RT_SAFETY.md) - Audio thread safety guidelines
  - [RT Safety Quick Reference](RT_SAFETY_QUICK_REF.md) - Common pitfalls and solutions

- **Collaboration**
  - [Collaboration System](COLLABORATION.md) - Full ICE/STUN/TURN, Internet-ready (needs TURN server)
  - [Thread Safety Audit](THREAD_SAFETY_AUDIT.md) - Thread safety analysis

---

## Feature Documentation

### Audio Engine
- [Save/Load System](SAVE_LOAD_IMPLEMENTATION.md) - Project file I/O
- [Coding Conventions](CODING_CONVENTIONS.md) - Code style guidelines

### AI Integration
- [Grok User Guide](GROK_USER_GUIDE.md) - AI assistant usage
- [AI Models](README_AI_MODELS.md) - Neural network models
- [AI Synth Guide](AI_SYNTH_GUIDE.md) - AI-powered synthesis
- [AI Preset Design Guide](AI-PRESET-DESIGN-GUIDE.md) - Creating AI presets

### Instruments
- [Instrument Command API](INSTRUMENT_COMMAND_API.md) - Command interface
- [Instrument UI Integration](INSTRUMENT_UI_INTEGRATION.md) - Instrument editors
- [PolySynth Parameter Schema](ZENITH_POLYSYNTH_PARAMETER_SCHEMA.md) - Synth parameters

### Platform Guides
- [Windows Installation](INSTALL_WINDOWS.md)
- [Windows Audio APIs Guide](WINDOWS_AUDIO_APIS_GUIDE.md)
- [Skia Integration](README_SKIA_INTEGRATION.md)

---

## Planning & Status

### Roadmap
- [Roadmap](ROADMAP.md) - Development timeline and priorities
- [12-Month Roadmap (Complete)](12_MONTH_ROADMAP_COMPLETE.md) - Historical roadmap

### TODO
- [TODO](TODO.md) - Current tasks and priorities

---

## Technical Briefs

Deep dives into specific technical decisions:

- [JUCE Framework Guide](tech-briefs/01-juce-framework-guide.md)
- [Web Embedding Decision](tech-briefs/02-web-embedding-decision.md)
- [VST3/AU Hosting Guide](tech-briefs/03-vst3-au-hosting-guide.md)
- [Audio Driver Latency Guide](tech-briefs/04-audio-driver-latency-guide.md)
- [Audio Thread Safety Policy](tech-briefs/06-audio-thread-safety-policy.md)
- [Packaging Checklist](tech-briefs/07-packaging-licensing-checklist.md)

---

## Additional Documentation

- [MCP Server](MCP_SERVER.md) - Model Context Protocol server
- [MCP Server README](MCP_SERVER_README.md) - MCP documentation
- [Developer Workflow](DEVELOPER_WORKFLOW.md) - Day-to-day development guide
- [Quick Start](QUICKSTART.md) - Fast path to running Zenith

---

## Archive

Historical documentation is preserved in `archive/` for reference.

---

## Documentation Standards

### Formatting
- Markdown (.md) format
- Code blocks with syntax highlighting
- Tables for structured data
- Emoji sparingly (⚠️ for warnings, ✅ for confirmed)

### Dating
All documents should include:
```markdown
**Last Updated:** YYYY-MM-DD
```

### Structure
1. Overview/Purpose
2. Current Status
3. Usage/API
4. Examples
5. Known Issues
6. References

---

## Contributing to Documentation

When adding new documentation:
1. Use descriptive filenames (UPPER_CASE for primary docs)
2. Add to this index
3. Include "Last Updated" date
4. Cross-reference related docs
5. Update README.md if user-facing

---

**For the latest status, always check [STATUS.md](STATUS.md) first.**
