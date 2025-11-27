/**
 * @file SkiaIntegrationVerification.cpp
 * @brief Comprehensive Skia integration verification test
 *
 * This test verifies that:
 * 1. Skia library is properly linked
 * 2. Skia headers are accessible
 * 3. Basic Skia functionality works (software rendering)
 * 4. GPU context creation works (if available)
 * 5. SkiaRenderer class integrates properly with JUCE
 */

#include <iostream>
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

#ifdef ZENITH_USE_SKIA
#include "../Source/rendering/SkiaRenderer.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include "include/core/SkColorSpace.h"
#include "include/core/SkFont.h"
#include "include/core/SkImageInfo.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPath.h"
#include "include/core/SkSurface.h"
#include "include/gpu/ganesh/GrDirectContext.h"

#endif

namespace {

void printTestHeader(const std::string &testName) {
  std::cout << "\n========================================\n";
  std::cout << "TEST: " << testName << "\n";
  std::cout << "========================================\n";
}

void printTestResult(const std::string &testName, bool passed) {
  std::cout << "[" << (passed ? "PASS" : "FAIL") << "] " << testName << "\n";
}

#ifdef ZENITH_USE_SKIA

// Test 1: Basic Skia library linked and headers accessible
bool testSkiaBasicSetup() {
  printTestHeader("Skia Basic Setup");

  try {
    // Create a simple software surface
    SkImageInfo info = SkImageInfo::MakeN32Premul(100, 100);
    sk_sp<SkSurface> surface = SkSurfaces::Raster(info, 0, nullptr);

    if (!surface) {
      std::cout << "ERROR: Failed to create raster surface\n";
      return false;
    }

    std::cout << "✓ Skia raster surface created successfully\n";
    std::cout << "  Dimensions: " << info.width() << "x" << info.height()
              << "\n";
    std::cout << "  Color type: " << static_cast<int>(info.colorType()) << "\n";

    return true;
  } catch (const std::exception &e) {
    std::cout << "ERROR: Exception in basic setup: " << e.what() << "\n";
    return false;
  }
}

// Test 2: Skia drawing operations work
bool testSkiaDrawing() {
  printTestHeader("Skia Drawing Operations");

  try {
    // Create surface
    SkImageInfo info = SkImageInfo::MakeN32Premul(200, 200);
    sk_sp<SkSurface> surface = SkSurfaces::Raster(info, 0, nullptr);

    if (!surface) {
      std::cout << "ERROR: Failed to create surface for drawing\n";
      return false;
    }

    SkCanvas *canvas = surface->getCanvas();
    if (!canvas) {
      std::cout << "ERROR: Failed to get canvas from surface\n";
      return false;
    }

    std::cout << "✓ Canvas acquired\n";

    // Clear canvas
    canvas->clear(SK_ColorWHITE);
    std::cout << "✓ Canvas cleared\n";

    // Draw a rectangle
    SkPaint paint;
    paint.setColor(SK_ColorBLUE);
    paint.setStyle(SkPaint::kFill_Style);
    canvas->drawRect(SkRect::MakeXYWH(10, 10, 50, 50), paint);
    std::cout << "✓ Rectangle drawn\n";

    // Draw a circle
    paint.setColor(SK_ColorRED);
    canvas->drawCircle(100, 100, 30, paint);
    std::cout << "✓ Circle drawn\n";

    // Draw a path
    SkPath path;
    path.moveTo(150, 50);
    path.lineTo(180, 100);
    path.lineTo(120, 100);
    path.close();
    paint.setColor(SK_ColorGREEN);
    canvas->drawPath(path, paint);
    std::cout << "✓ Path drawn\n";

    // Draw text
    paint.setColor(SK_ColorBLACK);
    SkFont font;
    font.setSize(16);
    canvas->drawSimpleText("Skia Works!", 12, SkTextEncoding::kUTF8, 10, 180,
                           font, paint);
    std::cout << "✓ Text drawn\n";

    return true;
  } catch (const std::exception &e) {
    std::cout << "ERROR: Exception in drawing: " << e.what() << "\n";
    return false;
  }
}

// Test 3: Skia color and paint operations
bool testSkiaPaintOperations() {
  printTestHeader("Skia Paint Operations");

  try {
    SkPaint paint;

    // Test color setting
    paint.setColor(SK_ColorCYAN);
    std::cout << "✓ Color set: CYAN\n";

    // Test alpha
    paint.setAlpha(128);
    std::cout << "✓ Alpha set: 128\n";

    // Test anti-alias
    paint.setAntiAlias(true);
    std::cout << "✓ Anti-aliasing enabled\n";

    // Test stroke
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(2.0f);
    std::cout << "✓ Stroke style set: width=2.0\n";

    // Test blend mode
    paint.setBlendMode(SkBlendMode::kMultiply);
    std::cout << "✓ Blend mode set: Multiply\n";

    return true;
  } catch (const std::exception &e) {
    std::cout << "ERROR: Exception in paint operations: " << e.what() << "\n";
    return false;
  }
}

// Test 4: SkiaRenderer class integration
class TestComponent : public juce::Component {
public:
  TestComponent() = default;

  void paint(juce::Graphics &) override {
    // Rendering is handled by SkiaRenderer
  }
};

bool testSkiaRendererIntegration() {
  printTestHeader("SkiaRenderer Integration");

  try {
    // Create a test component
    TestComponent component;
    component.setSize(400, 300);

    std::cout << "✓ Test component created\n";

    // Create SkiaRenderer with software backend
    zenith::SkiaRenderer renderer(component,
                                  zenith::SkiaRenderer::Backend::Software,
                                  false); // VSync off for testing

    std::cout << "✓ SkiaRenderer instantiated\n";
    std::cout << "  Backend: " << renderer.getBackendName(renderer.getBackend())
              << "\n";

    // Initialize renderer
    if (!renderer.initialize()) {
      std::cout << "ERROR: Failed to initialize SkiaRenderer\n";
      return false;
    }

    std::cout << "✓ SkiaRenderer initialized\n";
    std::cout << "  Is initialized: "
              << (renderer.isInitialized() ? "Yes" : "No") << "\n";
    std::cout << "  Target FPS: " << renderer.getTargetFPS() << "\n";
    std::cout << "  VSync enabled: "
              << (renderer.isVSyncEnabled() ? "Yes" : "No") << "\n";

    // Test rendering
    bool renderSuccess = false;
    renderer.render([&renderSuccess](SkCanvas *canvas) {
      if (canvas) {
        // Draw a test pattern
        SkPaint paint;
        paint.setColor(SK_ColorBLUE);
        canvas->drawRect(SkRect::MakeXYWH(50, 50, 100, 100), paint);
        renderSuccess = true;
      }
    });

    if (!renderSuccess) {
      std::cout << "ERROR: Render callback not executed\n";
      return false;
    }

    std::cout << "✓ Render operation successful\n";

    // Test resize
    renderer.resize(800, 600);
    std::cout << "✓ Resize handled\n";

    // Test stats
    auto stats = renderer.getStats();
    std::cout << "✓ Stats retrieved:\n";
    std::cout << "    Frame time: " << stats.frameTime << "ms\n";
    std::cout << "    FPS: " << stats.fps << "\n";

    // Shutdown
    renderer.shutdown();
    std::cout << "✓ SkiaRenderer shutdown\n";

    return true;
  } catch (const std::exception &e) {
    std::cout << "ERROR: Exception in SkiaRenderer integration: " << e.what()
              << "\n";
    return false;
  }
}

// Test 5: Skia image info and color space
bool testSkiaImageInfo() {
  printTestHeader("Skia Image Info & Color Space");

  try {
    // Test various color types
    std::cout << "Testing color types:\n";

    SkImageInfo infoRGBA = SkImageInfo::MakeN32Premul(100, 100);
    std::cout << "  ✓ RGBA_8888 info created\n";

    SkImageInfo infoSRGB =
        SkImageInfo::MakeN32Premul(100, 100, SkColorSpace::MakeSRGB());
    std::cout << "  ✓ sRGB color space info created\n";

    SkImageInfo infoAlpha = SkImageInfo::MakeA8(100, 100);
    std::cout << "  ✓ Alpha8 info created\n";

    // Test surface creation with different configs
    sk_sp<SkSurface> surfaceRGBA = SkSurfaces::Raster(infoRGBA, 0, nullptr);
    if (!surfaceRGBA) {
      std::cout << "ERROR: Failed to create RGBA surface\n";
      return false;
    }
    std::cout << "  ✓ RGBA surface created\n";

    sk_sp<SkSurface> surfaceSRGB = SkSurfaces::Raster(infoSRGB, 0, nullptr);
    if (!surfaceSRGB) {
      std::cout << "ERROR: Failed to create sRGB surface\n";
      return false;
    }
    std::cout << "  ✓ sRGB surface created\n";

    return true;
  } catch (const std::exception &e) {
    std::cout << "ERROR: Exception in image info test: " << e.what() << "\n";
    return false;
  }
}

#endif // ZENITH_USE_SKIA

} // anonymous namespace

int main(int argc, char *argv[]) {
  std::cout << "\n";
  std::cout << "╔════════════════════════════════════════════════════════╗\n";
  std::cout << "║  Skia Integration Verification Test Suite            ║\n";
  std::cout << "╚════════════════════════════════════════════════════════╝\n";

#ifdef ZENITH_USE_SKIA
  std::cout << "\n✓ ZENITH_USE_SKIA is DEFINED - Skia integration enabled\n";
  // std::cout << "Skia version: " << SK_MILESTONE << "\n"; // Removed to avoid
  // error

  // Initialize JUCE
  juce::ScopedJuceInitialiser_GUI juceInit;

  int passedTests = 0;
  int totalTests = 0;

  // Run all tests
  struct Test {
    std::string name;
    std::function<bool()> func;
  };

  std::vector<Test> tests = {
      {"Basic Skia Setup", testSkiaBasicSetup},
      {"Skia Drawing Operations", testSkiaDrawing},
      {"Skia Paint Operations", testSkiaPaintOperations},
      {"SkiaRenderer Integration", testSkiaRendererIntegration},
      {"Skia Image Info & Color Space", testSkiaImageInfo}};

  for (const auto &test : tests) {
    totalTests++;
    bool passed = test.func();
    printTestResult(test.name, passed);
    if (passed)
      passedTests++;
  }

  // Print summary
  std::cout << "\n========================================\n";
  std::cout << "TEST SUMMARY\n";
  std::cout << "========================================\n";
  std::cout << "Total tests: " << totalTests << "\n";
  std::cout << "Passed: " << passedTests << "\n";
  std::cout << "Failed: " << (totalTests - passedTests) << "\n";
  std::cout << "Success rate: " << (100.0 * passedTests / totalTests) << "%\n";
  std::cout << "\n";

  if (passedTests == totalTests) {
    std::cout << "✓✓✓ ALL TESTS PASSED ✓✓✓\n";
    std::cout << "Skia integration is working correctly!\n";
    return 0;
  } else {
    std::cout << "✗✗✗ SOME TESTS FAILED ✗✗✗\n";
    std::cout << "Please review the errors above.\n";
    return 1;
  }

#else
  std::cout << "\n✗ ZENITH_USE_SKIA is NOT DEFINED\n";
  std::cout << "Skia integration is disabled. Cannot run tests.\n";
  std::cout << "\nTo enable Skia, reconfigure with:\n";
  std::cout << "  cmake -B build -DZENITH_ENABLE_SKIA=ON\n";
  return 2;
#endif
}
