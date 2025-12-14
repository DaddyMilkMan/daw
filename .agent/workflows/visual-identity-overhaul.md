---
description: Complete Visual Identity Overhaul - The "Neon Noir" Standard
---

# Context
The current UI feels "functional but flat". It lacks the "Wow" factor. The goal is to eradicate any trace of default JUCE look-and-feel and establish a premium, cohesive "Neon Noir" aesthetic.

# Objectives
1.  **Color Palette Refinement**:
    *   Audit `ZenithDesignSystem.h`. The current "Neon Cyan" (0xFF00FFFF) is too raw. Shift to "Electric Blue" (0xFF00F0FF) and "Deep Violet" (0xFF7000FF).
    *   Backgrounds must not be black. Use "obsidian" (0xFF0D0D11) and "charcoal" (0xFF1C1C24).
    *   Text must never be pure white. Use "starlight" (0xFFF2F2F7).

2.  **Typography**:
    *   Enforce **Inter** for UI and **JetBrains Mono** for values.
    *   Delete all usages of `juce::Font`. Use `design::getSkFont()` exclusively.
    *   Kerning and Line Height: Increase breathing room. `LINE_HEIGHT_RELAXED` (1.5).

3.  **Global Polish**:
    *   Corner Radius: Standardize to 8px (Small) and 16px (Large).
    *   Borders: 1px subtle gradients, not solid colors.
    *   Shadows: `SkShadowUtils::DrawShadow` for depth, not just `DropShadowEffect`.

# Execution Steps
1.  Modify `ZenithDesignSystem.h` to refine the color palette.
2.  Scan codebase for `juce::Colours::...` and replace with `zenith::design::colors::...`.
3.  Scan codebase for `drawRect` and replace with `drawRRect` (Rounded Rects) everywhere.
4.  Implement a `GlowEffect` helper in `SkiaComponent` that adds an outer glow to any path.
