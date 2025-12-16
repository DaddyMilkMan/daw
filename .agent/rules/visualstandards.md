# Neon Noir Visual Standards
1. **Strict Color Palette**: ALWAYS use ZenithColours namespace. Never use raw hex codes or standard JUCE colors (e.g., juce::Colours::black).
2. **Glassmorphism Mandatory**: All panels, overlays, and popups must use GlassmorphicPanel or drawGlassPanel. No opaque flat backgrounds.
3. **Unified Rendering**: Use the Skia pipeline (drawSkia) for all complex UI. Avoid mixing juce::Graphics calls with Skia where possible.
4. **Typography & Spacing**: Use Inter or JetBrains Mono (for data). Follow a strict 4px/8px spacing grid.
5. **Alive Interface**: Every interactive element must have a hover state, active state, and subtle micro-animation (e.g., glow intensity change).
