## 2025-10-27 - Transport Bar Focus Visibility
**Learning:** JUCE Components painted via custom Skia renderers often lose their native focus ring visualization. Inherited `juce::Button` behavior for focus is invisible if `paintButton` is empty.
**Action:** Always check `hasKeyboardFocus` in the Skia drawing loop and use `InteractionHelper::drawFocusRing` to restore accessibility.
