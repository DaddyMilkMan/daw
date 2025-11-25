#include "SessionViewComponent.h"
#include <include/core/SkCanvas.h>
#include <include/core/SkFont.h>
#include <include/core/SkPaint.h>
#include <include/core/SkPath.h>


namespace zenith {

SessionViewComponent::SessionViewComponent() {
  // Initialize dummy data for visualization
  auto &theme = SkiaTheme::getInstance();
  auto colors = theme.getColors();

  for (int t = 0; t < NUM_TRACKS; ++t) {
    for (int s = 0; s < NUM_SCENES; ++s) {
      // Randomly populate some clips
      if ((t + s) % 3 == 0) {
        grid[t][s].hasClip = true;
        grid[t][s].name = "Clip " + juce::String(t) + "-" + juce::String(s);
        // Alternate colors based on track type logic
        if (t < 2)
          grid[t][s].color = colors.clipDrums; // Drums
        else if (t < 4)
          grid[t][s].color = colors.clipBass; // Bass
        else
          grid[t][s].color = colors.clipHarmony; // Synths
      }
    }
  }
}

SessionViewComponent::~SessionViewComponent() {}

void SessionViewComponent::paintSkia(SkCanvas &canvas,
                                     const juce::Rectangle<int> &bounds) {
  auto &theme = SkiaTheme::getInstance();
  auto colors = theme.getColors();
  auto &typo = theme.getTypography();

  canvas.clear(colors.bg0);

  float width = (float)bounds.getWidth();
  float height = (float)bounds.getHeight();

  // Calculate dynamic cell sizes
  float availableW = width - sceneHeaderWidth;
  float availableH = height - trackHeaderHeight;
  float cellWidth = availableW / NUM_TRACKS;
  float cellHeight = availableH / NUM_SCENES; // Or fixed height like 80px

  SkPaint paint;
  paint.setAntiAlias(true);

  // --- 1. Draw Grid ---
  for (int t = 0; t < NUM_TRACKS; ++t) {
    for (int s = 0; s < NUM_SCENES; ++s) {

      float x = sceneHeaderWidth + (t * cellWidth);
      float y = trackHeaderHeight + (s * cellHeight);

      SkRect cellRect =
          SkRect::MakeXYWH(x + clipGap / 2, y + clipGap / 2,
                           cellWidth - clipGap, cellHeight - clipGap);

      const auto &slot = grid[t][s];
      bool isHovered = (hoveredSlot.x == t && hoveredSlot.y == s);

      if (slot.hasClip) {
        // Draw Clip Content
        paint.setColor(slot.color);
        if (isHovered) {
          // Brighten on hover
          paint.setColor(SkColorSetA(slot.color, 255));
        }

        // Main Body
        canvas.drawRoundRect(cellRect, 4.0f, 4.0f, paint);

        // Play Button Icon
        SkPath playIcon;
        float cx = cellRect.left() + 15;
        float cy = cellRect.centerY();
        playIcon.moveTo(cx - 4, cy - 6);
        playIcon.lineTo(cx + 6, cy);
        playIcon.lineTo(cx - 4, cy + 6);
        playIcon.close();

        SkPaint iconPaint;
        iconPaint.setColor(SK_ColorWHITE);
        iconPaint.setStyle(SkPaint::kFill_Style);
        canvas.drawPath(playIcon, iconPaint);

        // Text Label
        SkFont font;
        font.setSize(typo.body.size);
        SkPaint textPaint;
        textPaint.setColor(colors.textStrong);

        canvas.drawString(slot.name.toRawUTF8(), cellRect.left() + 30,
                          cellRect.centerY() + 5, font, textPaint);

      } else {
        // Empty Slot
        paint.setColor(colors.bg2);
        paint.setStyle(SkPaint::kFill_Style);

        // Draw "Stop" button square if hovered
        if (isHovered) {
          paint.setColor(colors.bg3);
        }

        canvas.drawRoundRect(cellRect, 4.0f, 4.0f, paint);
      }
    }
  }

  // --- 2. Draw Headers (Scene Triggers) ---
  for (int s = 0; s < NUM_SCENES; ++s) {
    float y = trackHeaderHeight + (s * cellHeight);
    SkRect sceneRect = SkRect::MakeXYWH(
        0, y + clipGap / 2, sceneHeaderWidth - clipGap, cellHeight - clipGap);

    paint.setColor(colors.bg3);
    canvas.drawRoundRect(sceneRect, 4.0f, 4.0f, paint);

    // Scene Label (1, 2, 3...)
    SkFont font;
    font.setSize(typo.small.size);
    SkPaint textPaint;
    textPaint.setColor(colors.textMuted);
    juce::String label = juce::String(s + 1);
    canvas.drawString(label.toRawUTF8(), sceneRect.centerX() - 4,
                      sceneRect.centerY() + 4, font, textPaint);
  }
}

void SessionViewComponent::mouseMove(const juce::MouseEvent &e) {
  // Simple hit testing for hover effects
  float width = (float)getWidth();
  float height = (float)getHeight();
  float availableW = width - sceneHeaderWidth;
  float availableH = height - trackHeaderHeight;
  float cellWidth = availableW / NUM_TRACKS;
  float cellHeight = availableH / NUM_SCENES;

  int t = (int)((e.x - sceneHeaderWidth) / cellWidth);
  int s = (int)((e.y - trackHeaderHeight) / cellHeight);

  if (t >= 0 && t < NUM_TRACKS && s >= 0 && s < NUM_SCENES) {
    if (hoveredSlot.x != t || hoveredSlot.y != s) {
      hoveredSlot = {t, s};
      repaint(); // Trigger Skia repaint
    }
  } else {
    if (hoveredSlot.x != -1) {
      hoveredSlot = {-1, -1};
      repaint();
    }
  }
}

void SessionViewComponent::mouseDown(const juce::MouseEvent &e) {
  // Handle clip launching logic here
  if (hoveredSlot.x != -1) {
    DBG("Triggered Clip: " << hoveredSlot.x << ", " << hoveredSlot.y);
    // TODO: Send command to AudioEngine
  }
}

} // namespace zenith
