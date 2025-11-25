#include "SkiaCanvasComponent.h"

#ifdef ZENITH_USE_SKIA
#include <include/core/SkColorSpace.h>
#include <include/core/SkImageInfo.h>

#endif

namespace zenith {

SkiaCanvasComponent::SkiaCanvasComponent() {
  setOpaque(true); // We draw the whole background usually
}

SkiaCanvasComponent::~SkiaCanvasComponent() {}

void SkiaCanvasComponent::resized() { recreateSurface(); }

void SkiaCanvasComponent::recreateSurface() {
  int w = std::max(1, getWidth());
  int h = std::max(1, getHeight());

  // Recreate the backing image
  backingImage = juce::Image(juce::Image::ARGB, w, h, true);

#ifdef ZENITH_USE_SKIA
  // We don't create the SkSurface here because we need to lock the pixels
  // which is best done in paint() to ensure thread safety and validity.
  // However, we could potentially keep a persistent surface if we kept the
  // BitmapData locked, but JUCE doesn't encourage that.
  // So we'll wrap in paint().
  skSurface.reset();
#endif
}

void SkiaCanvasComponent::paintToSkia(SkCanvas *canvas, SkRect bounds) {
#ifdef ZENITH_USE_SKIA
  // Save state
  canvas->save();

  // Clip to bounds to prevent drawing outside
  canvas->clipRect(bounds);

  // Translate to component position so local (0,0) works as expected
  canvas->translate(bounds.left(), bounds.top());

  // Create local bounds for the component (0, 0, w, h)
  juce::Rectangle<int> localBounds(0, 0, (int)bounds.width(),
                                   (int)bounds.height());

  // Call the specific component's Skia painting implementation
  paintSkia(*canvas, localBounds);

  // Restore state
  canvas->restore();
#endif
}

void SkiaCanvasComponent::paint(juce::Graphics &g) {
  if (backingImage.isNull() ||
      backingImage.getWidth() != std::max(1, getWidth()) ||
      backingImage.getHeight() != std::max(1, getHeight())) {
    recreateSurface();
  }

#ifdef ZENITH_USE_SKIA
  {
    // Lock the image data
    juce::Image::BitmapData bitmapData(backingImage,
                                       juce::Image::BitmapData::readWrite);

    // Create Skia info matching JUCE's ARGB format
    // JUCE ARGB is typically BGRA on Windows/Intel Mac, RGBA on ARM Mac?
    // Skia's kN32_SkColorType is usually BGRA on Windows.
    // We'll trust MakeN32Premul to match the platform native 32-bit format
    // which JUCE usually uses for ARGB.
    SkImageInfo info = SkImageInfo::MakeN32Premul(backingImage.getWidth(),
                                                  backingImage.getHeight());

    // Wrap the pixels
    skSurface = SkSurfaces::WrapPixels(info, bitmapData.getLinePointer(0),
                                       bitmapData.lineStride);

    if (skSurface) {
      SkCanvas *canvas = skSurface->getCanvas();

      // Clear to transparent (or background color if we wanted)
      // We'll let paintSkia handle background filling if needed,
      // but clearing to transparent is safe.
      canvas->clear(SkColors::kTransparent);

      paintSkia(*canvas, getLocalBounds());

      // Flush to ensure pixels are written back to memory
      // (WrapPixels usually doesn't need flush but good practice)
      // skSurface->flush();
    }
  } // BitmapData destructor unlocks pixels here
#else
  // Fallback if Skia is disabled: just clear
  juce::Graphics g2(backingImage);
  g2.fillAll(juce::Colours::black);
  g2.setColour(juce::Colours::white);
  g2.drawText("Skia Disabled", getLocalBounds(), juce::Justification::centred,
              true);
#endif

  // Draw the result to the JUCE context
  g.drawImageAt(backingImage, 0, 0);
}

} // namespace zenith
