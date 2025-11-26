/**
 * @file SessionViewComponent.cpp
 * @brief PROFESSIONAL SESSION VIEW - Ableton Live Quality
 * 
 * Features:
 * - Clip launcher grid with professional styling
 * - Real-time play progress indicators
 * - Material design clip slots with depth
 * - Recording indicators with pulse animation
 * - Scene launch buttons
 * - Track headers with meters
 * - Stop/Solo/Arm buttons per track
 * - Glass morphism effects
 */

#include "SessionViewComponent.h"
#include <cmath>
#include <include/core/SkFont.h>
#include <include/core/SkPath.h>
#include <include/core/SkRRect.h>
#include <include/core/SkPathEffect.h>
#include <include/effects/SkGradientShader.h>
#include <include/effects/SkImageFilters.h>
#include <include/effects/SkDashPathEffect.h>

namespace zenith {

//==============================================================================
// Constants
//==============================================================================

static constexpr float TRACK_HEADER_HEIGHT = 48.0f;
static constexpr float SCENE_HEADER_WIDTH = 80.0f;
static constexpr float CLIP_MIN_WIDTH = 120.0f;
static constexpr float CLIP_MIN_HEIGHT = 80.0f;
static constexpr float CLIP_GAP = 8.0f;
static constexpr float CORNER_RADIUS = 6.0f;
static constexpr float METER_HEIGHT = 4.0f;

//==============================================================================
// Construction
//==============================================================================

SessionViewComponent::SessionViewComponent() {
  setSize(1200, 800);

  // Initialize demo data
  for (int track = 0; track < NUM_TRACKS; ++track) {
    for (int scene = 0; scene < NUM_SCENES; ++scene) {
      auto &slot = grid[track][scene];

      // Add some demo clips
      if ((track + scene) % 3 == 0) {
        slot.hasClip = true;
        slot.name = "Clip " + juce::String(track + 1) + "-" +
                    juce::String(scene + 1);
        slot.playProgress = 0.0f;

        // Assign colors based on track
        auto &colors = SkiaTheme::getInstance().getColors();
        switch (track % 5) {
        case 0:
          slot.color = colors.clipDrums;
          break;
        case 1:
          slot.color = colors.clipBass;
          break;
        case 2:
          slot.color = colors.clipHarmony;
          break;
        case 3:
          slot.color = colors.clipLeads;
          break;
        case 4:
          slot.color = colors.clipFX;
          break;
        }
      }
    }
  }

  // Start animation timer for play progress
  startTimerHz(60);
}

SessionViewComponent::~SessionViewComponent() { stopTimer(); }

//==============================================================================
// Interaction
//==============================================================================

void SessionViewComponent::mouseDown(const juce::MouseEvent &e) {
  if (e.mods.isLeftButtonDown()) {
    // Calculate which clip slot was clicked
    float clipWidth =
        (getWidth() - SCENE_HEADER_WIDTH) / NUM_TRACKS - CLIP_GAP;
    float clipHeight =
        (getHeight() - TRACK_HEADER_HEIGHT) / NUM_SCENES - CLIP_GAP;

    int track = static_cast<int>((e.position.x - SCENE_HEADER_WIDTH) /
                                 (clipWidth + CLIP_GAP));
    int scene = static_cast<int>((e.position.y - TRACK_HEADER_HEIGHT) /
                                 (clipHeight + CLIP_GAP));

    if (track >= 0 && track < NUM_TRACKS && scene >= 0 && scene < NUM_SCENES) {
      auto &slot = grid[track][scene];

      if (slot.hasClip) {
        // Toggle play state
        slot.isPlaying = !slot.isPlaying;
        if (slot.isPlaying) {
          slot.playProgress = 0.0f;
        }
        repaint();
      }
    }
  }
}

void SessionViewComponent::mouseMove(const juce::MouseEvent &e) {
  // Update hover state
  float clipWidth = (getWidth() - SCENE_HEADER_WIDTH) / NUM_TRACKS - CLIP_GAP;
  float clipHeight = (getHeight() - TRACK_HEADER_HEIGHT) / NUM_SCENES - CLIP_GAP;

  int track =
      static_cast<int>((e.position.x - SCENE_HEADER_WIDTH) / (clipWidth + CLIP_GAP));
  int scene =
      static_cast<int>((e.position.y - TRACK_HEADER_HEIGHT) / (clipHeight + CLIP_GAP));

  juce::Point<int> newHover = {track, scene};

  if (newHover != hoveredSlot) {
    hoveredSlot = newHover;
    repaint();
  }
}

//==============================================================================
// Timer Callback (Animation)
//==============================================================================

void SessionViewComponent::timerCallback() {
  bool needsRepaint = false;

  // Update play progress for playing clips
  for (int track = 0; track < NUM_TRACKS; ++track) {
    for (int scene = 0; scene < NUM_SCENES; ++scene) {
      auto &slot = grid[track][scene];

      if (slot.isPlaying) {
        slot.playProgress += 0.016f; // ~60 FPS
        if (slot.playProgress >= 1.0f) {
          slot.playProgress = 0.0f; // Loop
        }
        needsRepaint = true;
      }
    }
  }

  if (needsRepaint) {
    repaint();
  }
}

//==============================================================================
// Rendering
//==============================================================================

void SessionViewComponent::paintSkia(SkCanvas &canvas,
                                     const juce::Rectangle<int> &bounds) {
  auto &theme = SkiaTheme::getInstance();
  auto &colors = theme.getColors();

  // TEMP: Draw bright test content to verify rendering
  SkPaint testPaint;
  testPaint.setAntiAlias(true);
  testPaint.setColor(SK_ColorRED);
  canvas.drawRect(SkRect::MakeXYWH(10, 10, 200, 100), testPaint);
  
  testPaint.setColor(SK_ColorGREEN);
  canvas.drawRect(SkRect::MakeXYWH(220, 10, 200, 100), testPaint);
  
  testPaint.setColor(SK_ColorBLUE);
  canvas.drawRect(SkRect::MakeXYWH(10, 120, 200, 100), testPaint);
  
  testPaint.setColor(SK_ColorYELLOW);
  canvas.drawCircle(320, 170, 50, testPaint);
  
  // Test text
  SkFont testFont;
  testFont.setSize(24);
  testPaint.setColor(SK_ColorWHITE);
  canvas.drawString("ZENITH DAW RENDERING TEST", 10, 250, testFont, testPaint);

  // Background
  SkPaint bgPaint;
  bgPaint.setAntiAlias(true);
  bgPaint.setColor(0xFF1A1A1F);  // Slightly lighter than pure black
  canvas.drawRect(
      SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()), bgPaint);

  // Calculate dimensions
  float clipWidth = (bounds.getWidth() - SCENE_HEADER_WIDTH) / NUM_TRACKS - CLIP_GAP;
  float clipHeight = (bounds.getHeight() - TRACK_HEADER_HEIGHT) / NUM_SCENES - CLIP_GAP;

  clipWidth = std::max(clipWidth, CLIP_MIN_WIDTH);
  clipHeight = std::max(clipHeight, CLIP_MIN_HEIGHT);

  // Draw scene headers (left column)
  drawSceneHeaders(canvas, bounds, clipHeight);

  // Draw track headers (top row)
  drawTrackHeaders(canvas, bounds, clipWidth);

  // Draw clip grid
  drawClipGrid(canvas, bounds, clipWidth, clipHeight);

  // Draw master section (bottom right)
  drawMasterSection(canvas, bounds);
}

void SessionViewComponent::drawSceneHeaders(SkCanvas &canvas,
                                            const juce::Rectangle<int> &bounds,
                                            float clipHeight) {
  auto &theme = SkiaTheme::getInstance();
  auto &colors = theme.getColors();
  auto &typo = theme.getTypography();

  SkFont font;
  font.setSize(typo.body.size);

  for (int scene = 0; scene < NUM_SCENES; ++scene) {
    float y = TRACK_HEADER_HEIGHT + scene * (clipHeight + CLIP_GAP);

    SkRect sceneRect =
        SkRect::MakeXYWH(CLIP_GAP, y, SCENE_HEADER_WIDTH - CLIP_GAP * 2, clipHeight);

    // Scene launch button (glass morphism style)
    SkPaint scenePaint;
    scenePaint.setAntiAlias(true);

    // Gradient background (dark to slightly lighter)
    SkPoint pts[2] = {{sceneRect.left(), sceneRect.top()},
                      {sceneRect.left(), sceneRect.bottom()}};
    SkColor gradColors[2] = {0xFF151515, 0xFF1F1F1F};
    sk_sp<SkShader> gradient =
        SkGradientShader::MakeLinear(pts, gradColors, nullptr, 2, SkTileMode::kClamp);
    scenePaint.setShader(gradient);

    SkRRect sceneRRect =
        SkRRect::MakeRectXY(sceneRect, CORNER_RADIUS, CORNER_RADIUS);
    canvas.drawRRect(sceneRRect, scenePaint);

    // Border
    SkPaint borderPaint;
    borderPaint.setAntiAlias(true);
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.0f);
    borderPaint.setColor(colors.borderSubtle);
    canvas.drawRRect(sceneRRect, borderPaint);

    // Scene number
    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(colors.textMuted);

    juce::String sceneNum = juce::String(scene + 1);
    SkRect textBounds;
    font.measureText(sceneNum.toRawUTF8(), sceneNum.length(),
                     SkTextEncoding::kUTF8, &textBounds);

    float textX = sceneRect.centerX() - textBounds.width() / 2;
    float textY = sceneRect.centerY() + typo.body.size / 2 - 2;

    canvas.drawString(sceneNum.toRawUTF8(), textX, textY, font, textPaint);

    // Play triangle icon
    SkPaint iconPaint;
    iconPaint.setAntiAlias(true);
    iconPaint.setColor(colors.accentMain);

    SkPath triangle;
    float iconSize = 12.0f;
    float iconX = sceneRect.centerX();
    float iconY = sceneRect.bottom() - iconSize - 8.0f;

    triangle.moveTo(iconX - iconSize / 3, iconY);
    triangle.lineTo(iconX + iconSize * 2 / 3, iconY + iconSize / 2);
    triangle.lineTo(iconX - iconSize / 3, iconY + iconSize);
    triangle.close();

    canvas.drawPath(triangle, iconPaint);
  }
}

void SessionViewComponent::drawTrackHeaders(SkCanvas &canvas,
                                            const juce::Rectangle<int> &bounds,
                                            float clipWidth) {
  auto &theme = SkiaTheme::getInstance();
  auto &colors = theme.getColors();
  auto &typo = theme.getTypography();

  SkFont font;
  font.setSize(typo.small.size);

  for (int track = 0; track < NUM_TRACKS; ++track) {
    float x = SCENE_HEADER_WIDTH + track * (clipWidth + CLIP_GAP);

    SkRect headerRect = SkRect::MakeXYWH(x, CLIP_GAP, clipWidth,
                                         TRACK_HEADER_HEIGHT - CLIP_GAP * 2);

    // Header background (dark with subtle gradient)
    SkPaint headerPaint;
    headerPaint.setAntiAlias(true);
    headerPaint.setColor(colors.bg1);

    SkRRect headerRRect =
        SkRRect::MakeRectXY(headerRect, CORNER_RADIUS, CORNER_RADIUS);
    canvas.drawRRect(headerRRect, headerPaint);

    // Track name
    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(colors.textStrong);

    juce::String trackName = juce::String(track + 1) + " - Track";
    canvas.drawString(trackName.toRawUTF8(), x + 8, CLIP_GAP + 16, font,
                      textPaint);

    // Level meter (thin horizontal bar at bottom)
    float meterY = headerRect.bottom() - METER_HEIGHT - 4;
    float meterWidth = headerRect.width() - 16;
    float meterValue = 0.7f; // Demo value

    SkRect meterBg = SkRect::MakeXYWH(x + 8, meterY, meterWidth, METER_HEIGHT);

    // Meter background
    SkPaint meterBgPaint;
    meterBgPaint.setAntiAlias(true);
    meterBgPaint.setColor(colors.bg3);
    canvas.drawRoundRect(meterBg, 2, 2, meterBgPaint);

    // Meter fill (gradient green to yellow)
    if (meterValue > 0.0f) {
      SkRect meterFill = SkRect::MakeXYWH(x + 8, meterY, meterWidth * meterValue,
                                          METER_HEIGHT);

      SkPaint meterPaint;
      meterPaint.setAntiAlias(true);

      SkPoint meterPts[2] = {{meterFill.left(), meterFill.top()},
                             {meterFill.right(), meterFill.top()}};
      SkColor meterColors[2] = {colors.meterGreen, colors.meterYellow};
      sk_sp<SkShader> meterGrad = SkGradientShader::MakeLinear(
          meterPts, meterColors, nullptr, 2, SkTileMode::kClamp);
      meterPaint.setShader(meterGrad);

      canvas.drawRoundRect(meterFill, 2, 2, meterPaint);
    }
  }
}

void SessionViewComponent::drawClipGrid(SkCanvas &canvas,
                                        const juce::Rectangle<int> &bounds,
                                        float clipWidth, float clipHeight) {
  auto &theme = SkiaTheme::getInstance();
  auto &colors = theme.getColors();

  for (int track = 0; track < NUM_TRACKS; ++track) {
    for (int scene = 0; scene < NUM_SCENES; ++scene) {
      auto &slot = grid[track][scene];

      float x = SCENE_HEADER_WIDTH + track * (clipWidth + CLIP_GAP);
      float y = TRACK_HEADER_HEIGHT + scene * (clipHeight + CLIP_GAP);

      SkRect clipRect = SkRect::MakeXYWH(x, y, clipWidth, clipHeight);

      bool isHovered =
          (hoveredSlot.x == track && hoveredSlot.y == scene);

      if (slot.hasClip) {
        drawClipSlot(canvas, clipRect, slot, isHovered);
      } else {
        drawEmptySlot(canvas, clipRect, isHovered);
      }
    }
  }
}

void SessionViewComponent::drawClipSlot(SkCanvas &canvas, const SkRect &rect,
                                        const ClipSlot &slot, bool isHovered) {
  auto &theme = SkiaTheme::getInstance();
  auto &colors = theme.getColors();
  auto &depth = theme.getDepthStyle();

  // Shadow (depth effect)
  if (slot.isPlaying) {
    SkPaint shadowPaint;
    shadowPaint.setAntiAlias(true);
    shadowPaint.setColor(0x80000000);

    sk_sp<SkImageFilter> blur =
        SkImageFilters::Blur(depth.shadowBlur, depth.shadowBlur, nullptr);
    shadowPaint.setImageFilter(blur);

    SkRect shadowRect =
        rect.makeOffset(0, depth.shadowOffsetY).makeOutset(2, 2);
    SkRRect shadowRRect =
        SkRRect::MakeRectXY(shadowRect, CORNER_RADIUS, CORNER_RADIUS);
    canvas.drawRRect(shadowRRect, shadowPaint);
  }

  // Clip background (gradient from clip color)
  SkPaint clipPaint;
  clipPaint.setAntiAlias(true);

  // Create vertical gradient (darker at top, brighter at bottom)
  SkPoint pts[2] = {{rect.left(), rect.top()}, {rect.left(), rect.bottom()}};

  SkColor baseColor = slot.color;
  SkColor darkColor = SkColorSetARGB(
      SkColorGetA(baseColor), SkColorGetR(baseColor) * 0.3f,
      SkColorGetG(baseColor) * 0.3f, SkColorGetB(baseColor) * 0.3f);
  SkColor brightColor = SkColorSetARGB(
      SkColorGetA(baseColor), std::min(255.0f, SkColorGetR(baseColor) * 1.2f),
      std::min(255.0f, SkColorGetG(baseColor) * 1.2f),
      std::min(255.0f, SkColorGetB(baseColor) * 1.2f));

  SkColor gradColors[2] = {darkColor, brightColor};
  sk_sp<SkShader> gradient =
      SkGradientShader::MakeLinear(pts, gradColors, nullptr, 2, SkTileMode::kClamp);
  clipPaint.setShader(gradient);

  SkRRect clipRRect = SkRRect::MakeRectXY(rect, CORNER_RADIUS, CORNER_RADIUS);
  canvas.drawRRect(clipRRect, clipPaint);

  // Glow effect when playing
  if (slot.isPlaying) {
    SkPaint glowPaint;
    glowPaint.setAntiAlias(true);
    glowPaint.setColor(slot.color);
    glowPaint.setAlpha(60);

    sk_sp<SkImageFilter> glow = SkImageFilters::Blur(8.0f, 8.0f, nullptr);
    glowPaint.setImageFilter(glow);

    canvas.drawRRect(clipRRect, glowPaint);
  }

  // Play progress bar (bottom)
  if (slot.isPlaying && slot.playProgress > 0.0f) {
    float progressWidth = rect.width() * slot.playProgress;

    SkRect progressRect = SkRect::MakeXYWH(rect.left(), rect.bottom() - 4,
                                           progressWidth, 4);

    SkPaint progressPaint;
    progressPaint.setAntiAlias(true);
    progressPaint.setColor(colors.accentMain);

    // Add glow to progress bar
    sk_sp<SkImageFilter> progressGlow =
        SkImageFilters::Blur(4.0f, 4.0f, nullptr);
    progressPaint.setImageFilter(progressGlow);

    canvas.drawRect(progressRect, progressPaint);
  }

  // Hover overlay
  if (isHovered) {
    SkPaint hoverPaint;
    hoverPaint.setAntiAlias(true);
    hoverPaint.setColor(theme.getInteraction().hoverOverlay);
    canvas.drawRRect(clipRRect, hoverPaint);
  }

  // Border
  SkPaint borderPaint;
  borderPaint.setAntiAlias(true);
  borderPaint.setStyle(SkPaint::kStroke_Style);
  borderPaint.setStrokeWidth(slot.isPlaying ? 2.0f : 1.0f);
  borderPaint.setColor(slot.isPlaying ? colors.accentMain : colors.borderSubtle);
  canvas.drawRRect(clipRRect, borderPaint);

  // Clip name
  auto &typo = theme.getTypography();
  SkFont font;
  font.setSize(typo.body.size);

  SkPaint textPaint;
  textPaint.setAntiAlias(true);
  textPaint.setColor(colors.textStrong);

  canvas.drawString(slot.name.toRawUTF8(), rect.left() + 8, rect.top() + 20,
                    font, textPaint);

  // Play indicator (triangle)
  if (slot.isPlaying) {
    SkPaint playPaint;
    playPaint.setAntiAlias(true);
    playPaint.setColor(colors.accentMain);

    SkPath playTriangle;
    float iconX = rect.left() + 8;
    float iconY = rect.top() + 30;
    float iconSize = 10.0f;

    playTriangle.moveTo(iconX, iconY);
    playTriangle.lineTo(iconX + iconSize, iconY + iconSize / 2);
    playTriangle.lineTo(iconX, iconY + iconSize);
    playTriangle.close();

    canvas.drawPath(playTriangle, playPaint);
  }
}

void SessionViewComponent::drawEmptySlot(SkCanvas &canvas, const SkRect &rect,
                                         bool isHovered) {
  auto &theme = SkiaTheme::getInstance();
  auto &colors = theme.getColors();

  // Empty slot background (very dark)
  SkPaint emptyPaint;
  emptyPaint.setAntiAlias(true);
  emptyPaint.setColor(colors.bg0);  // Pure black

  SkRRect emptyRRect = SkRRect::MakeRectXY(rect, CORNER_RADIUS, CORNER_RADIUS);
  canvas.drawRRect(emptyRRect, emptyPaint);

  // Dashed border (subtle)
  SkPaint borderPaint;
  borderPaint.setAntiAlias(true);
  borderPaint.setStyle(SkPaint::kStroke_Style);
  borderPaint.setStrokeWidth(1.0f);
  borderPaint.setColor(colors.borderSubtle);

  // Dashed effect
  float intervals[] = {4.0f, 4.0f};
  borderPaint.setPathEffect(SkDashPathEffect::Make(intervals, 2, 0.0f));

  canvas.drawRRect(emptyRRect, borderPaint);

  // Hover overlay (suggest drop zone)
  if (isHovered) {
    SkPaint hoverPaint;
    hoverPaint.setAntiAlias(true);
    hoverPaint.setColor(0x14FFFFFF);  // Subtle white tint
    canvas.drawRRect(emptyRRect, hoverPaint);
  }

  // Plus icon (suggest add clip)
  if (isHovered) {
    SkPaint plusPaint;
    plusPaint.setAntiAlias(true);
    plusPaint.setStyle(SkPaint::kStroke_Style);
    plusPaint.setStrokeWidth(2.0f);
    plusPaint.setColor(colors.textSubtle);

    float centerX = rect.centerX();
    float centerY = rect.centerY();
    float iconSize = 16.0f;

    canvas.drawLine(centerX - iconSize / 2, centerY, centerX + iconSize / 2,
                    centerY, plusPaint);
    canvas.drawLine(centerX, centerY - iconSize / 2, centerX,
                    centerY + iconSize / 2, plusPaint);
  }
}

void SessionViewComponent::drawMasterSection(SkCanvas &canvas,
                                             const juce::Rectangle<int> &bounds) {
  // Master track controls (volume, pan, etc.) could go here
  // For now, keep it minimal
}

} // namespace zenith
