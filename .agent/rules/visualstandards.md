# Studio Black + Blue Visual Standards
1. **Primary Visual Language**: Use premium matte-black surfaces with restrained electric-blue accents. Violet may appear as a secondary accent, but avoid loud gamer-neon treatments.
2. **Surface Hierarchy**: Default to opaque matte panels for core work areas. Use glass, blur, or translucency only for menus, dialogs, popups, and other elevated surfaces where depth helps.
3. **Unified Rendering**: Use the Skia pipeline (`drawSkia`) for all complex UI. Avoid mixing `juce::Graphics` calls with Skia where possible.
4. **Typography & Spacing**: Use Inter or JetBrains Mono (for data). Follow a strict 4px/8px spacing grid with generous breathing room.
5. **Interaction Discipline**: Every interactive element must have clear default, hover, active, and focus states. Accent glow is reserved for focus, transport activity, Wingman, and other high-value states.
