#pragma once

#ifdef ZENITH_USE_SKIA
#include <memory>
#include <skia/include/core/SkCanvas.h>
#include <skia/include/core/SkSurface.h>
#include <skia/include/gpu/GrDirectContext.h>
#endif

namespace ui {

class SkiaPainter {
public:
    // Initialize shared GPU context (no-op if software fallback)
    static void initialize();
    static void shutdown();

    // Paint entry point for a window or component. Width/height are in pixels.
    // This is a minimal shim: real widgets should draw using Skia primitives.
    static void paint(void* platformNativeWindowHandle, int width, int height);
};

} // namespace ui
