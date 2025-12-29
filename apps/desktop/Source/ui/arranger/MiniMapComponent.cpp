/**
 * @file MiniMapComponent.cpp
 * @brief Implementation of the arrangement mini-map navigator.
 */

#include "MiniMapComponent.h"
#include "ZenithDesignSystem.h"

// Skia includes (available via SkiaComponent inheritance)
#include <core/SkBitmap.h> // For fallback rendering
#include "ZenithSkia.h"
#include <core/SkImageInfo.h>
#include <core/SkSurface.h>


namespace zenith {

//==============================================================================
// Construction / Destruction
//==============================================================================

MiniMapComponent::MiniMapComponent() {
  // Enable mouse interaction for click-jump navigation
  setInterceptsMouseClicks(true, true);
}

MiniMapComponent::~MiniMapComponent() = default;

//==============================================================================
// Public API
//==============================================================================

void MiniMapComponent::setArrangementData(
    double totalDurationBeats, int totalTracks,
    const std::vector<MiniMapClip> &clips) {
  totalDurationBeats_ = std::max(1.0, totalDurationBeats);
  totalTracks_ = std::max(1, totalTracks);
  clips_ = clips;

  needsCacheUpdate_ = true;
  repaint();
}

void MiniMapComponent::setVisibleRange(double startBeat, double durationBeats,
                                       int firstTrackIndex, int visibleTracks) {
  visibleStart_ = startBeat;
  visibleDuration_ = durationBeats;
  firstTrackIndex_ = firstTrackIndex;
  visibleTrackCount_ = visibleTracks;

  // Repaint to update viewport box (cache remains valid)
  repaint();
}

//==============================================================================
// Mouse Interaction - Click-Jump Navigation
//==============================================================================

void MiniMapComponent::mouseDown(const juce::MouseEvent &e) {
  if (onNavigate) {
    // Center the main view on the clicked beat position
    double clickedBeat = xToBeats(e.position.x);
    // Offset by half the visible duration to center
    double targetBeat = clickedBeat - (visibleDuration_ / 2.0);
    onNavigate(std::max(0.0, targetBeat));
  }
}

void MiniMapComponent::mouseDrag(const juce::MouseEvent &e) {
  // Scrubbing - same behavior as click
  mouseDown(e);
}

void MiniMapComponent::mouseUp(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
}

void MiniMapComponent::resized() { needsCacheUpdate_ = true; }

//==============================================================================
// Cached Rendering
//==============================================================================

void MiniMapComponent::updateCachedImage() {
  auto bounds = getLocalBounds();
  if (bounds.isEmpty())
    return;

  // Create an offscreen bitmap for cached rendering
  // (Using SkBitmap approach which is more compatible across Skia versions)
  SkBitmap bitmap;
  SkImageInfo imageInfo =
      SkImageInfo::MakeN32Premul(bounds.getWidth(), bounds.getHeight());

  if (!bitmap.tryAllocPixels(imageInfo)) {
    return; // Failed to allocate
  }

  // Create a canvas that draws to the bitmap
  SkCanvas canvas(bitmap);

  // Clear with dark background
  canvas.clear(design::colors::BG_DARK);

  SkPaint clipPaint;
  clipPaint.setAntiAlias(false); // Fast rendering for LOD blocks

  // Draw each clip as a simple colored rectangle
  for (const auto &clip : clips_) {
    float x = beatsToX(clip.startBeats);
    float w = static_cast<float>(
        clip.lengthBeats *
        (static_cast<double>(bounds.getWidth()) / totalDurationBeats_));
    float y = trackToY(clip.trackIndex);
    float h = static_cast<float>(bounds.getHeight()) /
              static_cast<float>(totalTracks_);

    // Ensure minimal visibility
    w = std::max(1.0f, w);
    h = std::max(1.0f, h - 1.0f); // 1px gap for track separation

    // Color based on type and selection state
    if (clip.isSelected) {
      clipPaint.setColor(design::colors::CYAN); // Use CYAN for selected
    } else if (clip.isMidi) {
      // Teal for MIDI clips
      clipPaint.setColor(SkColorSetARGB(200, 80, 200, 180));
    } else {
      // Orange/amber for Audio clips
      clipPaint.setColor(SkColorSetARGB(200, 200, 150, 60));
    }

    canvas.drawRect(SkRect::MakeXYWH(x, y, w, h), clipPaint);
  }

  // Convert bitmap to image for caching
  bitmap.setImmutable();
  cachedImage_ = bitmap.asImage();
}

//==============================================================================
// Skia Rendering
//==============================================================================

void MiniMapComponent::drawSkia(SkCanvas *canvas) {
  // 1. Rebuild cache if needed
  if (needsCacheUpdate_ || !cachedImage_) {
    updateCachedImage();
    needsCacheUpdate_ = false;
  }

  // 2. Draw cached arrangement overview
  if (cachedImage_) {
    canvas->drawImage(cachedImage_, 0, 0);
  }

  // 3. Draw Viewport Overlay (the "glass" rectangle showing visible area)
  float viewportX = beatsToX(visibleStart_);
  float viewportW =
      static_cast<float>(visibleDuration_ * (static_cast<double>(getWidth()) /
                                             totalDurationBeats_));

  // Clamp to bounds
  if (viewportX < 0) {
    viewportW += viewportX;
    viewportX = 0;
  }
  if (viewportX + viewportW > getWidth()) {
    viewportW = static_cast<float>(getWidth()) - viewportX;
  }

  // Calculate vertical viewport (if we're showing vertical scrolling)
  float viewportY = 0.0f;
  float viewportH = static_cast<float>(getHeight());

  // If not all tracks are visible, show the vertical slice
  if (totalTracks_ > 0 && visibleTrackCount_ < totalTracks_) {
    viewportY = trackToY(firstTrackIndex_);
    viewportH =
        static_cast<float>(visibleTrackCount_) *
        (static_cast<float>(getHeight()) / static_cast<float>(totalTracks_));
  }

  SkRect viewportRect =
      SkRect::MakeXYWH(viewportX, viewportY, viewportW, viewportH);

  // Semi-transparent fill
  SkPaint viewportPaint;
  viewportPaint.setColor(SkColorSetARGB(50, 255, 255, 255));
  viewportPaint.setStyle(SkPaint::kFill_Style);
  canvas->drawRect(viewportRect, viewportPaint);

  // Bright border - use NEON_GREEN for accent
  viewportPaint.setStyle(SkPaint::kStroke_Style);
  viewportPaint.setStrokeWidth(1.5f);
  viewportPaint.setColor(design::colors::NEON_GREEN);
  canvas->drawRect(viewportRect, viewportPaint);

  // Optional: Draw thin border around the entire mini-map
  SkPaint borderPaint;
  borderPaint.setStyle(SkPaint::kStroke_Style);
  borderPaint.setStrokeWidth(1.0f);
  borderPaint.setColor(design::colors::BORDER_SUBTLE);
  canvas->drawRect(SkRect::MakeWH(static_cast<float>(getWidth()),
                                  static_cast<float>(getHeight())),
                   borderPaint);
}

//==============================================================================
// Coordinate Conversion
//==============================================================================

float MiniMapComponent::beatsToX(double beats) const {
  if (totalDurationBeats_ <= 0.001)
    return 0.0f;
  return static_cast<float>((beats / totalDurationBeats_) *
                            static_cast<double>(getWidth()));
}

double MiniMapComponent::xToBeats(float x) const {
  if (getWidth() <= 0)
    return 0.0;
  return (static_cast<double>(x) / static_cast<double>(getWidth())) *
         totalDurationBeats_;
}

float MiniMapComponent::trackToY(int trackIndex) const {
  if (totalTracks_ <= 0)
    return 0.0f;
  return static_cast<float>(trackIndex) *
         (static_cast<float>(getHeight()) / static_cast<float>(totalTracks_));
}

} // namespace zenith
