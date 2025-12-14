---
description: Implement REAL glassmorphism with backdrop blur - no more fake panels
---

# Context
The current "GlassmorphicPanel" is fake—it just uses a transparent background. We need **True Glassmorphism** where the background content is actively blurred behind the panel.

# Objectives
1.  **Rendering Engine Upgrade**:
    *   Create `RealGlassPanel` class inheriting from `SkiaComponent`.
    *   Use `SkCanvas::saveLayer` with an `SkImageFilter::MakeBlur` to create the backdrop effect.
    *   *Performance Note*: This is expensive. Implement a "Low Quality" mode that uses a pre-cached blurred background image if realtime blur is too slow.

2.  **Visual Stack**:
    *   **Layer 1 (Backdrop)**: The blurred content behind the panel.
    *   **Layer 2 (Tint)**: A noise texture overlay (opacity 0.05) to reduce banding.
    *   **Layer 3 (Surface)**: Linear gradient (White 10% -> White 2%) for the "glass" body.
    *   **Layer 4 (border)**: 1px Inner stroke (White 20%) + 1px Outer drop shadow.

# Execution Steps
1.  Update `ZenithDesignSystem.h` with `SkImageFilter` includes.
2.  Implement the `drawGlass` method using `Skia` advanced layers.
3.  Replace the `mainCardBounds` rendering in `ZenithHubComponent.cpp` with this new system.
