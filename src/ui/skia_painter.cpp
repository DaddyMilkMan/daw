#include "ui/skia_painter.h"
#ifdef ZENITH_USE_SKIA
#include "platform/skia_context.h"
#include <skia/include/core/SkCanvas.h>
#include <skia/include/core/SkSurface.h>
#include <skia/include/core/SkPaint.h>
#endif

#include <iostream>

namespace ui {

void SkiaPainter::initialize() {
#ifdef ZENITH_USE_SKIA
    (void)platform::SkiaContext::instance().grContext();
#endif
}

void SkiaPainter::shutdown() {
#ifdef ZENITH_USE_SKIA
    // Let the context destructor clean up
#endif
}

void SkiaPainter::paint(void* platformNativeWindowHandle, int width, int height) {
#ifdef ZENITH_USE_SKIA
    auto surface = platform::SkiaContext::instance().makeWindowSurface(platformNativeWindowHandle, width, height);
    if (!surface) {
        std::cerr << "Skia surface creation failed\n";
        return;
    }
    SkCanvas* canvas = surface->getCanvas();
    SkPaint paint;
    paint.setColor(SK_ColorWHITE);
    canvas->clear(SK_ColorWHITE);
    paint.setColor(SK_ColorBLUE);
    paint.setAntiAlias(true);
    canvas->drawRect(SkRect::MakeXYWH(10, 10, width - 20, height - 20), paint);
    surface->flushAndSubmit();
#else
    (void)platformNativeWindowHandle; (void)width; (void)height;
    std::cerr << "Skia is disabled, no painting performed\n";
#endif
}

} // namespace ui
