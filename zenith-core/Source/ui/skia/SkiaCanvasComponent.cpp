#include "SkiaCanvasComponent.h"

namespace zenith {

SkiaCanvasComponent::SkiaCanvasComponent() {
  // Optimization: Tell JUCE not to bother with CPU painting
  // since we handle it in the OpenGL loop.
  setOpaque(false);
}

SkiaCanvasComponent::~SkiaCanvasComponent() {}

void SkiaCanvasComponent::resized() {
  // Layout logic usually handled by parent FlexBox
}

void SkiaCanvasComponent::drawSkia(SkCanvas *canvas) {
  // Convert float bounds to int rect for the painting method
  // This bridges the generic loop to your specific implementation
  if (canvas) {
    paintSkia(*canvas, getLocalBounds());
  }
}

} // namespace zenith
