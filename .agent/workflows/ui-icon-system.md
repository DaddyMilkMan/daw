---
description: Replace Unicode symbol embarrassment with proper SVG/vector icon system
---

# Context
The Zenith Hub currently uses colored circles (`canvas->drawCircle`) and emojis ("🎹") as placeholders. This is unacceptable for a "Premium" DAW.

# Objectives
1.  **Icon System Implementation**:
    *   Create `ZenithIcons.h/cpp` (if not fully populated).
    *   Define icons as `SkPath` objects.
    *   Categories: `Transport`, `FileTypes`, `Instruments`, `GeneralUI`.

2.  **Asset Migration**:
    *   Convert standard icons (Play, Stop, Record, Folder, File, Synth, Guitar, Mic) to Skia Paths.
    *   *Source*: Adapt standard SVG paths (e.g. from Material Design or Phosphor) to C++ `SkPath` commands.

3.  **UI Integration**:
    *   Update `ZenithHubComponent` to use `ZenithIcons::drawIcon(canvas, rect, iconID, color)`.
    *   Ensure icons align pixel-perfectly (pixel snapping).

# Execution Steps
1.  Populate `ZenithIcons.h` with `getIconPath(IconID)` methods.
2.  Refactor `ZenithHubComponent::drawTemplates` to use these icons instead of emojis.
3.  Refactor `ZenithHubComponent::drawRecentProjects` to show file-type icons.
