---
description: Complete UI transformation to apply Neon Noir Glassmorphism design system
---

# UI Transformation Workflow

This workflow transforms the Zenith DAW from barebones developer UI to professional "Neon Noir Glassmorphism" styling.

## Prerequisites
- Read `.agent/tasks/UI_TRANSFORMATION_MASTER_PLAN.md` thoroughly
- Ensure project builds successfully
- Understand Skia rendering basics

## Phase 1: Foundation

// turbo
1. Build the project to verify current state:
```bash
cmake --build build --config Debug --target ZenithDAW -- /p:CL_MPCount=1
```

2. Create helper classes:
   - Create `apps/desktop/Source/ui/skia/GlassmorphicPanel.h`
   - Create `apps/desktop/Source/ui/skia/NeonGlow.h`
   - Add to CMakeLists.txt if needed

3. Update `MainLayoutComponent.cpp`:
   - Replace `SkColorSetRGB(30, 30, 30)` with `design::colors::BG_DARKEST`
   - Add gradient background rendering
   - Include `#include "skia/ZenithDesignSystem.h"`

// turbo
4. Build and test:
```bash
cmake --build build --config Debug --target ZenithDAW -- /p:CL_MPCount=1
```

5. Commit Phase 1:
```bash
git checkout -b feature/ui-neon-noir-phase1
git add .
git commit -m "feat(ui): Apply Neon Noir foundation - MainLayoutComponent gradient background"
git push -u origin feature/ui-neon-noir-phase1
```

## Phase 2: Core Components

6. Style Transport Bar (`TransportBar.cpp`):
   - Glassmorphic panel background
   - Glow effects on buttons
   - Digital time display

7. Style Track Headers (`TrackHeaderComponent.cpp`):
   - Color bars with glow
   - M/S/R button styling

8. Style Mixer Channels (`MixerChannelComponent.cpp`):
   - VU meters with gradients
   - Fader styling with glow

9. Style Clips (`ClipComponent.cpp`):
   - Waveform rendering
   - Selection glow

// turbo
10. Build and test:
```bash
cmake --build build --config Debug --target ZenithDAW -- /p:CL_MPCount=1
```

11. Commit Phase 2:
```bash
git checkout -b feature/ui-neon-noir-phase2
git add .
git commit -m "feat(ui): Style core components - Transport, Tracks, Mixer, Clips"
git push -u origin feature/ui-neon-noir-phase2
```

## Phase 3: Polish

12. Add animations:
    - VU meter smooth decay
    - Hover glow transitions
    - Button press effects

13. Add visual feedback:
    - Drag & drop previews
    - Selection highlights
    - Focus indicators

// turbo
14. Final build and test:
```bash
cmake --build build --config Debug --target ZenithDAW -- /p:CL_MPCount=1
```

15. Take screenshots of final result

16. Final commit:
```bash
git checkout -b feature/ui-neon-noir-complete
git add .
git commit -m "feat(ui): Complete Neon Noir Glassmorphism UI transformation"
git push -u origin feature/ui-neon-noir-complete
```

## Reference Files
- Design System: `apps/desktop/Source/ui/skia/ZenithDesignSystem.h`
- Master Plan: `.agent/tasks/UI_TRANSFORMATION_MASTER_PLAN.md`
- Example Components: `apps/desktop/Source/ui/skia/ZenithUIComponents.h`

## Success Criteria
- [ ] No hardcoded colors (all use design system)
- [ ] Gradient backgrounds on all panels
- [ ] Glassmorphic styling on overlays
- [ ] Neon glow on active elements
- [ ] 60fps render performance maintained
