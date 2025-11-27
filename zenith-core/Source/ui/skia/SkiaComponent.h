/*
  ==============================================================================
    SkiaComponent.h
    Inherit from this instead of juce::Component for your custom controls.
  ==============================================================================
*/
#pragma once

#include "../../rendering/SkiaContextManager.h"
#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#ifdef ZENITH_USE_SKIA
#include "include/core/SkCanvas.h"
#include "include/core/SkPaint.h"
#include "include/core/SkRRect.h"
#else
class SkCanvas;
#endif

namespace zenith {

class SkiaComponent : public juce::Component {
public:
  SkiaComponent() {
    // Skia handles the background, so JUCE shouldn't try to draw an opaque
    // background
    setOpaque(false);
  }

  virtual ~SkiaComponent() {
    // Notify manager to clean up the cached surface for this component
    if (zenith::SkiaContextManager::getInstance().isInitialized())
      zenith::SkiaContextManager::getInstance().componentDestroyed(*this);
  }

  // Abstract method: Implement this in your widgets
  virtual void drawSkia(SkCanvas *canvas) = 0;

  // Final overrides - Do not override these in your child classes
  void paint(juce::Graphics &g) final {
    auto &manager = zenith::SkiaContextManager::getInstance();

    if (manager.isInitialized()) {
      // Helper callback that forwards to your virtual drawSkia()
      manager.renderToComponent(g, *this,
                                [this](SkCanvas *c) { this->drawSkia(c); });
    } else {
      // Fallback if Skia crashed or didn't load
      paintFallback(g);
    }
  }

  // Optional: Override this to provide custom JUCE-based fallback rendering
  virtual void paintFallback(juce::Graphics &g) {
    g.fillAll(juce::Colours::red.withAlpha(0.5f));
    g.setColour(juce::Colours::white);
    g.drawText("Skia Error", getLocalBounds(), juce::Justification::centred);
  }

  void resized() override {
    // Notify manager that surface size needs to change
    if (zenith::SkiaContextManager::getInstance().isInitialized())
      zenith::SkiaContextManager::getInstance().componentResized(*this);

    // Call generic resized handler (optional hook)
    onResized();
  }

  // Optional: Override this if you need standard resize logic
  virtual void onResized() {}
};

} // namespace zenith
