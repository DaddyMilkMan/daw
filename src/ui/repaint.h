#pragma once

#include <atomic>

namespace ui {

// Simple request-repaint mechanism used during migration from JUCE's repaint()
void requestSkiaRepaint();

// Query whether a repaint is requested (for main loop)
bool consumeSkiaRepaintRequest();

} // namespace ui
