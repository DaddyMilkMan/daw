Rendering guidelines for the DAW project.

Goal: Use Skia exclusively for all UI rendering. No JUCE rendering APIs must be used for drawing.

High-level rules:
- All components that perform drawing must use the Skia APIs. Do not call `juce::Graphics` or `Component::paint`.
- Platform-specific windowing code should provide a GPU-backed `SkSurface`.
- Avoid allocations and locks in render-critical paths.
- Use resource caching for `SkImage`, `SkShader`, and fonts. Use `Skia::GrDirectContext` per-process.
- Implement dirty-region tracking and partial repaints to minimize GPU work.

See `tools/scan_juce_rendering.py` for a CI helper that fails if JUCE rendering APIs are found.
