---
description: Complete Visual Identity Overhaul - Studio Black + Blue Standard
---

# Context
The current UI feels functional but visually fragmented. The goal is to establish a premium, cohesive matte-black visual language with restrained electric-blue accents and calm studio-grade polish.

# Objectives
1.  **Color Palette Refinement**:
    *   Audit `ZenithDesignSystem.h`. Keep the primary accent in the professional blue range and use violet only as a restrained secondary accent.
    *   Backgrounds should read as obsidian, charcoal, and elevated graphite rather than flat black or loud gradients.
    *   Text must stay soft and readable, never stark pure white on every surface.

2.  **Typography**:
    *   Enforce **Inter** for UI and **JetBrains Mono** for values.
    *   Delete all usages of `juce::Font`. Use `design::getSkFont()` exclusively.
    *   Keep kerning and line height relaxed enough to feel premium and unhurried.

3.  **Global Polish**:
    *   Standardize corner radii and spacing.
    *   Use subtle borders and depth cues instead of heavy chrome or gamer neon.
    *   Reserve glow for active, focused, or AI-related moments.

# Execution Steps
1.  Modify `ZenithDesignSystem.h` to keep the palette aligned with the matte-black / blue-accent direction.
2.  Scan the codebase for direct `juce::Colours::...` usage and replace it with `zenith::design::colors::...` where appropriate.
3.  Round and soften surfaces with intent instead of applying the same effect everywhere.
4.  Keep overlays, menus, and dialogs visually elevated while core work surfaces remain matte and stable.
