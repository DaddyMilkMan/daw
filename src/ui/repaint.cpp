#include "ui/repaint.h"
#include <atomic>

namespace ui {

static std::atomic<int> g_repaintRequested{0};

void requestSkiaRepaint() {
    g_repaintRequested.store(1, std::memory_order_release);
}

bool consumeSkiaRepaintRequest() {
    return g_repaintRequested.exchange(0, std::memory_order_acq_rel) == 1;
}

} // namespace ui
