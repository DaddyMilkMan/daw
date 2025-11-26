#include "SkiaCanvasComponent.h"

namespace zenith {

SkiaCanvasComponent::SkiaCanvasComponent() {
  // Optimization: Tell JUCE not to bother with CPU painting
  // since we handle it in the OpenGL loop.
  setOpaque(false);
}

SkiaCanvasComponent::~SkiaCanvasComponent() {}

void SkiaCanvasComponent::paint(juce::Graphics &g) {
  // EMPTY!
  // We do NOT want JUCE to paint anything here.
  // The drawing happens in paintToSkia called by renderOpenGL.
}

void SkiaCanvasComponent::resized() {
  // Layout logic usually handled by parent FlexBox
}

void SkiaCanvasComponent::paintToSkia(SkCanvas *canvas, SkRect bounds) {
  // Convert float bounds to int rect for the painting method
  // This bridges the generic loop to your specific implementation
  if (canvas) {
    paintSkia(*canvas,
              juce::Rectangle<int>(0, 0, bounds.width(), bounds.height()));
  }
}

} // namespace zenith
