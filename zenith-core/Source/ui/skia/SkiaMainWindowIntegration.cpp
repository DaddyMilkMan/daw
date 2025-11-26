/**
 * @file SkiaMainWindowIntegration.cpp
 * @brief Implementation of GPU-accelerated Skia DAW rendering
 */

#include "SkiaMainWindowIntegration.h"

#ifdef ZENITH_USE_SKIA

// Define colors for the DAW UI
#define SK_COLOR_BG_DARK SkColorSetRGB(18, 18, 20)
#define SK_COLOR_PANEL SkColorSetRGB(30, 30, 35)
#define SK_COLOR_ACCENT SkColorSetRGB(0, 160, 255)
#define SK_COLOR_TEXT_MAIN SkColorSetRGB(220, 220, 220)
#define SK_COLOR_TEXT_DIM SkColorSetRGB(150, 150, 150)
#define SK_COLOR_GRID_LINE SkColorSetRGB(50, 50, 55)

namespace zenith {

//==============================================================================
// Constructor / Destructor
//==============================================================================

SkiaMainWindowIntegration::SkiaMainWindowIntegration()
    : isPlaying_(true), frameCounter_(0) {
  // 1. Configure and attach OpenGL context
  // CRITICAL: setComponentPaintingEnabled(true) allows JUCE components to paint
  // on top of OpenGL
  openGLContext.setRenderer(this);
  openGLContext.setContinuousRepainting(true); // VSYNC enabled (60 FPS)
  openGLContext.setComponentPaintingEnabled(
      true); // KEY FIX: Enable component painting
  openGLContext.setMultisamplingEnabled(true);
  openGLContext.attachTo(*this);

  // 2. Start animation timer at 60 Hz
  startTimerHz(60);

  DBG("SkiaMainWindowIntegration: Initialized with OpenGL + Component "
      "Painting");
}

SkiaMainWindowIntegration::~SkiaMainWindowIntegration() {
  openGLContext.detach();
  shutdownSkiaRendering();
}

//==============================================================================
// OpenGL Lifecycle
//==============================================================================

void SkiaMainWindowIntegration::newOpenGLContextCreated() {
  DBG("newOpenGLContextCreated - Initializing Skia on render thread");

  try {
    renderer_ = std::make_unique<SkiaRenderer>(
        *this, SkiaRenderer::Backend::OpenGL, true);
    if (renderer_->initialize()) {
      skiaInitialized_ = true;
      DBG("✓ Skia initialized successfully");
    } else {
      DBG("✗ SkiaRenderer initialization failed");
    }
  } catch (const std::exception &e) {
    DBG("✗ Exception: " << e.what());
  }
}

void SkiaMainWindowIntegration::openGLContextClosing() {
  DBG("openGLContextClosing - Cleaning up Skia");
  renderer_.reset();
  skiaInitialized_ = false;
}

void SkiaMainWindowIntegration::renderOpenGL() {
  // CRITICAL FIX: Do NOT draw UI manually here.
  // Just clear the screen to dark background.
  // Because setComponentPaintingEnabled(true) is on, JUCE will
  // automatically call paint() on all visible components AFTER this returns.

  juce::OpenGLHelpers::clear(juce::Colour(0xff121214));

  // Note: All the actual UI is now rendered by the component tree
  // (TransportBar, BrowserPanel, SessionView, Arranger, etc.)
  // via their paintSkia() methods which are called by JUCE's paint system.
}

//==============================================================================
// Standard Component Methods
//==============================================================================

void SkiaMainWindowIntegration::paint(juce::Graphics &g) {
  // Usually empty - OpenGL handles rendering
  // But show loading message if Skia not ready
  if (!skiaInitialized_) {
    g.fillAll(juce::Colours::black);
    g.setColour(juce::Colours::white);
    g.drawText("Initializing GPU Engine...", getLocalBounds(),
               juce::Justification::centred, true);
  }
}

void SkiaMainWindowIntegration::resized() {
  // No-op - child components handle their own layout via
  // MainComponent::resized()
}

void SkiaMainWindowIntegration::timerCallback() {
  // Trigger render updates at 60 Hz
  openGLContext.triggerRepaint();
}

//==============================================================================
// Control Methods
//==============================================================================

bool SkiaMainWindowIntegration::initializeSkiaRendering() {
  // Initialization moved to newOpenGLContextCreated
  return true;
}

void SkiaMainWindowIntegration::shutdownSkiaRendering() {
  renderer_.reset();
  skiaInitialized_ = false;
}

} // namespace zenith

#endif // ZENITH_USE_SKIA
