/**
 * @file SkiaSimpleTest.cpp
 * @brief Simple Skia linkage test
 */

#include <iostream>

#ifdef ZENITH_USE_SKIA
#include "include/core/SkCanvas.h"
#include "include/core/SkImageInfo.h"
#include "include/core/SkPaint.h"
#include "include/core/SkSurface.h"

#endif

int main() {
  std::cout << "========================================\n";
  std::cout << "Skia Simple Integration Test\n";
  std::cout << "========================================\n\n";

#ifdef ZENITH_USE_SKIA
  std::cout << "✓ ZENITH_USE_SKIA is defined\n";
  std::cout << "Skia library linked successfully\n\n";

  // Test 1: Create a simple surface
  std::cout << "Test 1: Creating raster surface...\n";
  SkImageInfo info = SkImageInfo::MakeN32Premul(100, 100);
  sk_sp<SkSurface> surface = SkSurfaces::Raster(info);

  if (surface) {
    std::cout << "✓ Surface created successfully\n";
    std::cout << "  Width: " << info.width() << "\n";
    std::cout << "  Height: " << info.height() << "\n";

    // Test 2: Get canvas and draw
    std::cout << "\nTest 2: Drawing on canvas...\n";
    SkCanvas *canvas = surface->getCanvas();
    if (canvas) {
      canvas->clear(SK_ColorWHITE);
      std::cout << "✓ Canvas cleared\n";

      SkPaint paint;
      paint.setColor(SK_ColorBLUE);
      canvas->drawRect(SkRect::MakeXYWH(10, 10, 50, 50), paint);
      std::cout << "✓ Rectangle drawn\n";

      std::cout << "\n========================================\n";
      std::cout << "✓✓✓ ALL TESTS PASSED ✓✓✓\n";
      std::cout << "Skia is properly linked and working!\n";
      std::cout << "========================================\n";
      return 0;
    } else {
      std::cout << "✗ Failed to get canvas\n";
      return 1;
    }
  } else {
    std::cout << "✗ Failed to create surface\n";
    return 1;
  }
#else
  std::cout << "✗ ZENITH_USE_SKIA is NOT defined\n";
  std::cout << "Skia is not enabled in this build\n";
  return 2;
#endif
}
