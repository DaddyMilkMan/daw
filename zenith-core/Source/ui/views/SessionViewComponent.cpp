/**
 * @file SessionViewComponent.cpp
 * @brief Professional Session/Clip Launcher View - Clean Empty DAW Design
 * 
 * GPU-accelerated Skia rendering for smooth 60fps animations.
 * Starts empty like Ableton Live - user creates their own content.
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
// Design Constants (8px grid)
//==============================================================================

static constexpr float TRACK_HEADER_HEIGHT = 80.0f;
static constexpr float SCENE_HEADER_WIDTH = 48.0f;
static constexpr float CLIP_GAP = 2.0f;
static constexpr float CORNER_RADIUS = 4.0f;
static constexpr float SMALL_RADIUS = 2.0f;
static constexpr float METER_WIDTH = 4.0f;
static constexpr float TRACK_WIDTH = 100.0f;
static constexpr float CLIP_HEIGHT = 48.0f;

//==============================================================================
// Color Palette - Professional Dark Theme
//==============================================================================

static constexpr SkColor BG_DARKEST = 0xFF0D0D0D;
static constexpr SkColor BG_DARK = 0xFF141414;
static constexpr SkColor BG_MID = 0xFF1A1A1A;
static constexpr SkColor BG_LIGHT = 0xFF242424;

static constexpr SkColor SURFACE = 0xFF1E1E1E;
static constexpr SkColor SURFACE_ELEVATED = 0xFF2A2A2A;

static constexpr SkColor ACCENT_PRIMARY = 0xFF00B4D8;  // Cyan accent
static constexpr SkColor ACCENT_WARM = 0xFFFF9500;     // Orange for record/arm

static constexpr SkColor TEXT_PRIMARY = 0xFFE0E0E0;
static constexpr SkColor TEXT_SECONDARY = 0xFF909090;
static constexpr SkColor TEXT_MUTED = 0xFF505050;

static constexpr SkColor BORDER_SUBTLE = 0xFF2A2A2A;
static constexpr SkColor BORDER_LIGHT = 0xFF3A3A3A;

static constexpr SkColor METER_GREEN = 0xFF4ADE80;
static constexpr SkColor METER_YELLOW = 0xFFFACC15;
static constexpr SkColor METER_RED = 0xFFEF4444;

// Track arm/solo/mute button colors
static constexpr SkColor BTN_ARM = 0xFFEF4444;
static constexpr SkColor BTN_SOLO = 0xFFFACC15;
static constexpr SkColor BTN_MUTE = 0xFF3B82F6;

//==============================================================================
// Construction
//==============================================================================

SessionViewComponent::SessionViewComponent() {
  setSize(1200, 800);
  
  // Initialize empty - no demo data, like a real DAW
  for (int i = 0; i < NUM_TRACKS; ++i) {
    trackNames[i] = juce::String(i + 1) + "-Audio";
    trackMeterValues[i] = 0.0f;
  }

  // Clear all slots - start empty
  for (int track = 0; track < NUM_TRACKS; ++track) {
    for (int scene = 0; scene < NUM_SCENES; ++scene) {
      grid[track][scene] = ClipSlot(); // Empty by default
    }
  }
  
  startTimerHz(60);
}

SessionViewComponent::~SessionViewComponent() { 
  stopTimer(); 
}

void SessionViewComponent::initializeDemoData() {
  // Intentionally empty - DAW starts clean
}

//==============================================================================
// Interaction
//==============================================================================

void SessionViewComponent::mouseDown(const juce::MouseEvent &e) {
  auto [track, scene] = getSlotAtPosition(e.position);
  
  if (track >= 0 && track < NUM_TRACKS && scene >= 0 && scene < NUM_SCENES) {
    auto &slot = grid[track][scene];
    
    if (slot.hasClip) {
      slot.isPlaying = !slot.isPlaying;
      if (slot.isPlaying) {
        slot.playProgress = 0.0f;
      }
      repaint();
    }
  }
}

void SessionViewComponent::mouseMove(const juce::MouseEvent &e) {
  auto [track, scene] = getSlotAtPosition(e.position);
  juce::Point<int> newHover = {track, scene};
  
  if (newHover != hoveredSlot) {
    hoveredSlot = newHover;
    repaint();
  }
}

void SessionViewComponent::mouseExit(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  hoveredSlot = {-1, -1};
  repaint();
}

std::pair<int, int> SessionViewComponent::getSlotAtPosition(juce::Point<float> pos) const {
  if (pos.x < SCENE_HEADER_WIDTH || pos.y < TRACK_HEADER_HEIGHT) {
    return {-1, -1};
  }
  
  int track = static_cast<int>((pos.x - SCENE_HEADER_WIDTH) / (TRACK_WIDTH + CLIP_GAP));
  int scene = static_cast<int>((pos.y - TRACK_HEADER_HEIGHT) / (CLIP_HEIGHT + CLIP_GAP));
  
  if (track < 0 || track >= NUM_TRACKS || scene < 0 || scene >= NUM_SCENES) {
    return {-1, -1};
  }
  
  return {track, scene};
}

//==============================================================================
// Timer Callback
//==============================================================================

void SessionViewComponent::timerCallback() {
  bool needsRepaint = false;
  
  for (int track = 0; track < NUM_TRACKS; ++track) {
    for (int scene = 0; scene < NUM_SCENES; ++scene) {
      auto &slot = grid[track][scene];
      
      if (slot.isPlaying) {
        slot.playProgress += 0.008f;
        if (slot.playProgress >= 1.0f) {
          slot.playProgress = 0.0f;
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
// Main Rendering
//==============================================================================

void SessionViewComponent::paintSkia(SkCanvas &canvas, const juce::Rectangle<int> &bounds) {
  // Dark background
  canvas.clear(BG_DARKEST);
  
  // Draw track headers (top)
  drawTrackHeaders(canvas, bounds, TRACK_WIDTH);
  
  // Draw scene headers (left)
  drawSceneHeaders(canvas, bounds, CLIP_HEIGHT);
  
  // Draw clip grid
  drawClipGrid(canvas, bounds, TRACK_WIDTH, CLIP_HEIGHT);
  
  // Draw grid lines
  drawGridLines(canvas, bounds);
}

void SessionViewComponent::drawBackground(SkCanvas &canvas, const juce::Rectangle<int> &bounds) {
  canvas.clear(BG_DARKEST);
}

void SessionViewComponent::drawGridPanel(SkCanvas &canvas, const juce::Rectangle<int> &bounds,
                                         float clipWidth, float clipHeight) {
  juce::ignoreUnused(canvas, bounds, clipWidth, clipHeight);
}

//==============================================================================
// Grid Lines
//==============================================================================

void SessionViewComponent::drawGridLines(SkCanvas &canvas, const juce::Rectangle<int> &bounds) {
  SkPaint linePaint;
  linePaint.setColor(BORDER_SUBTLE);
  linePaint.setStrokeWidth(1.0f);
  linePaint.setAntiAlias(false); // Crisp lines
  
  // Vertical lines between tracks
  for (int track = 0; track <= NUM_TRACKS; ++track) {
    float x = SCENE_HEADER_WIDTH + track * (TRACK_WIDTH + CLIP_GAP);
    canvas.drawLine(x, 0, x, static_cast<float>(bounds.getHeight()), linePaint);
  }
  
  // Horizontal lines between scenes
  for (int scene = 0; scene <= NUM_SCENES; ++scene) {
    float y = TRACK_HEADER_HEIGHT + scene * (CLIP_HEIGHT + CLIP_GAP);
    canvas.drawLine(SCENE_HEADER_WIDTH, y, static_cast<float>(bounds.getWidth()), y, linePaint);
  }
}

//==============================================================================
// Scene Headers (Left Column)
//==============================================================================

void SessionViewComponent::drawSceneHeaders(SkCanvas &canvas, const juce::Rectangle<int> &bounds,
                                            float clipHeight) {
  juce::ignoreUnused(bounds);
  
  SkFont font;
  font.setSize(11.0f);
  
  for (int scene = 0; scene < NUM_SCENES; ++scene) {
    float y = TRACK_HEADER_HEIGHT + scene * (clipHeight + CLIP_GAP);
    
    SkRect sceneRect = SkRect::MakeXYWH(0, y, SCENE_HEADER_WIDTH, clipHeight);
    
    // Scene background
    SkPaint scenePaint;
    scenePaint.setColor(BG_MID);
    canvas.drawRect(sceneRect, scenePaint);
    
    // Scene number
    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(TEXT_MUTED);
    
    juce::String sceneLabel = juce::String(scene + 1);
    canvas.drawString(sceneLabel.toRawUTF8(), 
                      sceneRect.centerX() - 4, 
                      sceneRect.centerY() + 4, 
                      font, textPaint);
  }
  
  // Master/Scene launch header
  SkRect masterRect = SkRect::MakeXYWH(0, 0, SCENE_HEADER_WIDTH, TRACK_HEADER_HEIGHT);
  SkPaint masterPaint;
  masterPaint.setColor(BG_DARK);
  canvas.drawRect(masterRect, masterPaint);
}

//==============================================================================
// Track Headers (Top Row) - Like Ableton
//==============================================================================

void SessionViewComponent::drawTrackHeaders(SkCanvas &canvas, const juce::Rectangle<int> &bounds,
                                            float clipWidth) {
  juce::ignoreUnused(bounds);
  
  SkFont font;
  font.setSize(10.0f);
  
  SkFont boldFont;
  boldFont.setSize(11.0f);
  boldFont.setEmbolden(true);
  
  for (int track = 0; track < NUM_TRACKS; ++track) {
    float x = SCENE_HEADER_WIDTH + track * (clipWidth + CLIP_GAP);
    
    SkRect headerRect = SkRect::MakeXYWH(x, 0, clipWidth, TRACK_HEADER_HEIGHT);
    
    // Header background
    SkPaint headerPaint;
    headerPaint.setColor(BG_MID);
    canvas.drawRect(headerRect, headerPaint);
    
    // Track number and name
    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(TEXT_SECONDARY);
    
    juce::String trackLabel = juce::String(track + 1) + " Audio";
    canvas.drawString(trackLabel.toRawUTF8(), x + 8, 18, boldFont, textPaint);
    
    // Arm button (circle)
    float btnY = 28;
    float btnSize = 14.0f;
    float btnSpacing = 20.0f;
    
    // Record arm button
    SkPaint armPaint;
    armPaint.setAntiAlias(true);
    armPaint.setStyle(SkPaint::kStroke_Style);
    armPaint.setStrokeWidth(1.5f);
    armPaint.setColor(TEXT_MUTED);
    canvas.drawCircle(x + 14, btnY + btnSize/2, btnSize/2 - 1, armPaint);
    
    // Solo button "S"
    SkPaint soloPaint;
    soloPaint.setAntiAlias(true);
    soloPaint.setColor(TEXT_MUTED);
    canvas.drawString("S", x + 14 + btnSpacing, btnY + 11, font, soloPaint);
    
    // Mute button "M"  
    canvas.drawString("M", x + 14 + btnSpacing * 2, btnY + 11, font, soloPaint);
    
    // Level meter (vertical)
    float meterX = x + clipWidth - METER_WIDTH - 6;
    float meterTop = 8;
    float meterHeight = TRACK_HEADER_HEIGHT - 16;
    
    // Meter background
    SkRect meterBg = SkRect::MakeXYWH(meterX, meterTop, METER_WIDTH, meterHeight);
    SkPaint meterBgPaint;
    meterBgPaint.setColor(BG_DARKEST);
    canvas.drawRect(meterBg, meterBgPaint);
    
    // Meter fill (from bottom up)
    float level = trackMeterValues[track];
    if (level > 0.0f) {
      float fillHeight = meterHeight * level;
      SkRect meterFill = SkRect::MakeXYWH(meterX, meterTop + meterHeight - fillHeight, 
                                           METER_WIDTH, fillHeight);
      SkPaint meterPaint;
      meterPaint.setColor(METER_GREEN);
      canvas.drawRect(meterFill, meterPaint);
    }
    
    // Volume fader area (simplified)
    float faderY = 50;
    SkPaint faderPaint;
    faderPaint.setColor(TEXT_MUTED);
    faderPaint.setAntiAlias(true);
    canvas.drawString("0.0", x + 8, faderY + 12, font, faderPaint);
    canvas.drawString("dB", x + 28, faderY + 12, font, faderPaint);
    
    // Pan knob area
    canvas.drawString("C", x + 60, faderY + 12, font, faderPaint);
  }
}

//==============================================================================
// Clip Grid - Empty slots ready for content
//==============================================================================

void SessionViewComponent::drawClipGrid(SkCanvas &canvas, const juce::Rectangle<int> &bounds,
                                        float clipWidth, float clipHeight) {
  juce::ignoreUnused(bounds);
  
  for (int track = 0; track < NUM_TRACKS; ++track) {
    for (int scene = 0; scene < NUM_SCENES; ++scene) {
      auto &slot = grid[track][scene];
      
      float x = SCENE_HEADER_WIDTH + track * (clipWidth + CLIP_GAP);
      float y = TRACK_HEADER_HEIGHT + scene * (clipHeight + CLIP_GAP);
      
      SkRect clipRect = SkRect::MakeXYWH(x, y, clipWidth, clipHeight);
      
      bool isHovered = (hoveredSlot.x == track && hoveredSlot.y == scene);
      
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
  // Clip background
  SkPaint clipPaint;
  clipPaint.setAntiAlias(true);
  clipPaint.setColor(slot.color);
  
  SkRRect clipRRect = SkRRect::MakeRectXY(rect, SMALL_RADIUS, SMALL_RADIUS);
  canvas.drawRRect(clipRRect, clipPaint);
  
  // Clip name
  SkFont font;
  font.setSize(10.0f);
  
  SkPaint textPaint;
  textPaint.setAntiAlias(true);
  textPaint.setColor(0xFF000000); // Black text on colored clip
  
  canvas.drawString(slot.name.toRawUTF8(), rect.left() + 6, rect.top() + 14, font, textPaint);
  
  // Playing indicator
  if (slot.isPlaying) {
    SkPaint borderPaint;
    borderPaint.setAntiAlias(true);
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(2.0f);
    borderPaint.setColor(0xFFFFFFFF);
    canvas.drawRRect(clipRRect, borderPaint);
  }
  
  // Hover overlay
  if (isHovered && !slot.isPlaying) {
    SkPaint hoverPaint;
    hoverPaint.setColor(0x20FFFFFF);
    canvas.drawRRect(clipRRect, hoverPaint);
  }
}

void SessionViewComponent::drawWaveform(SkCanvas &canvas, const SkRect &rect, const ClipSlot &slot) {
  juce::ignoreUnused(canvas, rect, slot);
}

void SessionViewComponent::drawEmptySlot(SkCanvas &canvas, const SkRect &rect, bool isHovered) {
  // Empty slot - subtle dark background
  SkPaint emptyPaint;
  emptyPaint.setColor(isHovered ? BG_LIGHT : BG_DARK);
  canvas.drawRect(rect, emptyPaint);
  
  // Stop button in center (small square) - appears on hover
  if (isHovered) {
    float stopSize = 8.0f;
    SkRect stopRect = SkRect::MakeXYWH(
      rect.centerX() - stopSize/2,
      rect.centerY() - stopSize/2,
      stopSize, stopSize
    );
    
    SkPaint stopPaint;
    stopPaint.setAntiAlias(true);
    stopPaint.setStyle(SkPaint::kStroke_Style);
    stopPaint.setStrokeWidth(1.0f);
    stopPaint.setColor(TEXT_MUTED);
    canvas.drawRect(stopRect, stopPaint);
  }
}

void SessionViewComponent::drawMasterSection(SkCanvas &canvas, const juce::Rectangle<int> &bounds) {
  juce::ignoreUnused(canvas, bounds);
}

} // namespace zenith
