# 🚀 ZENITH DAW UI TRANSFORMATION - AGENT PROMPT

Copy and paste this entire prompt to an AI coding agent to execute the UI transformation.

---

## PROMPT START

You are tasked with transforming the Zenith DAW's user interface from its current barebones state to a professional "Neon Noir Glassmorphism" design.

### YOUR MISSION

Transform the Zenith DAW UI to look like a professional DAW (Ableton, FL Studio, Bitwig quality). The design system is already defined but NOT being applied.

### CRITICAL CONTEXT

1. **Read the master plan first:**
   ```
   View file: c:\zenith\daw\.agent\tasks\UI_TRANSFORMATION_MASTER_PLAN.md
   ```

2. **The design system exists but is UNUSED:**
   - File: `apps/desktop/Source/ui/skia/ZenithDesignSystem.h`
   - Colors, effects, typography all defined
   - Currently, code uses hardcoded values like `SkColorSetRGB(30, 30, 30)` instead of `design::colors::BG_DARKEST`

3. **Project builds with:**
   ```bash
   cd c:\zenith\daw
   cmake --build build --config Debug --target ZenithDAW
   ```

### PHASE 1: FOUNDATION (Do This First)

1. **Create helper classes:**
   - `apps/desktop/Source/ui/skia/GlassmorphicPanel.h` - Reusable glassmorphic panel renderer
   - `apps/desktop/Source/ui/skia/NeonGlow.h` - Reusable glow effect renderer

2. **Update MainLayoutComponent.cpp** (line ~93):
   - Replace `canvas->clear(SkColorSetRGB(30, 30, 30));`
   - With gradient background using `design::colors::BG_DARKEST` and `design::colors::BG_DARKER`

3. **Build and verify** the background gradient renders correctly

### PHASE 2: CORE COMPONENTS

Style these files in order:

1. **TransportBar.cpp** - Play/Stop/Record buttons with glow, time display
2. **TrackHeaderComponent.cpp** - M/S/R buttons, track colors, faders
3. **MixerChannelComponent.cpp** - VU meters with gradient, faders with glow
4. **ClipComponent.cpp** - Waveform rendering, selection glow

### PHASE 3: POLISH

1. Add hover effects (glow intensifies)
2. Add selection states (cyan border glow)
3. Ensure 60fps performance

### KEY DESIGN TOKENS TO USE

```cpp
// Colors
design::colors::CYAN          // 0xFF00F0FF - Primary accent
design::colors::MAGENTA       // 0xFFFF00D4 - Secondary accent
design::colors::BG_DARKEST    // 0xFF0D0D11 - Window background
design::colors::BG_DARKER     // 0xFF141419 - Panel background

// Effects
design::effects::GLOW_MEDIUM  // 4.0f - Active glow radius
design::effects::BLUR_GLASS   // 20.0f - Glassmorphism blur

// For glow: SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, radius)
// For gradients: SkGradientShader::MakeLinear(pts, colors, nullptr, 2, SkTileMode::kClamp)
```

### GIT WORKFLOW

Create feature branches for each phase:
```bash
git checkout -b feature/ui-neon-noir-phase1
# ... make changes ...
git add .
git commit -m "feat(ui): Apply Neon Noir foundation"
git push -u origin feature/ui-neon-noir-phase1
```

### SUCCESS CRITERIA

- [ ] No hardcoded colors remain
- [ ] Background has subtle gradient
- [ ] Panels use glassmorphic styling
- [ ] Active controls have neon glow
- [ ] VU meters animate smoothly with gradients
- [ ] 60fps render performance

### START HERE

1. Read `.agent/tasks/UI_TRANSFORMATION_MASTER_PLAN.md`
2. View `apps/desktop/Source/ui/skia/ZenithDesignSystem.h` 
3. Build project to verify it works
4. Start with MainLayoutComponent.cpp gradient background
5. Take screenshots before/after each change

Go make this UI GLOW! 🔥

---

## PROMPT END
