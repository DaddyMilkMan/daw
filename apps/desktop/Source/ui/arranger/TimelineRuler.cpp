/**
 * @file TimelineRuler.cpp
 * @brief Timeline ruler implementation with flat Skia design
 */

// POLISH: spacing normalized to 8px grid (labels at Typography.small)
// POLISH: typography now uses ZenithDesignSystem
// POLISH: flattened background (bg2, no gradients)

#include "TimelineRuler.h"
#include "../design-system/ZenithDesignSystem.h"
#include "../design-system/ColorBridge.h"
#include "../Theme.h"
#include <core/SkCanvas.h>
#include <core/SkFont.h>
#include <core/SkPaint.h>

namespace zenith {

TimelineRuler::TimelineRuler() {
  setSize(800, 30);

  // Start 60Hz animation timer for smooth hover effects
  if (juce::MessageManager::getInstanceWithoutCreating() != nullptr) startTimerHz(60);

  // Enable mouse events
  setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void TimelineRuler::resized() {
}

void TimelineRuler::timerCallback() {
  repaint();
}

void TimelineRuler::setVisibleRange(double start, double length) {
  viewStartBeat = start;
  viewLengthBeats = length;
  repaint();
}

int TimelineRuler::beatsToPixels(double beats) const {
  return static_cast<int>((beats - viewStartBeat) * pixelsPerBeat);
}

double TimelineRuler::pixelsToBeats(int pixels) const {
  return viewStartBeat + (pixels / pixelsPerBeat);
}

void TimelineRuler::setLoopRange(double startBeat, double endBeat, bool enabled) {
  if (loopStartBeat != startBeat || loopEndBeat != endBeat ||
      loopEnabled != enabled) {
    loopStartBeat = startBeat;
    loopEndBeat = endBeat;
    loopEnabled = enabled;
    repaint();
  }
}

void TimelineRuler::mouseMove(const juce::MouseEvent &event) {
  mousePosition = event.getPosition();

  // Calculate which measure is being hovered
  double beatAtMouse = pixelsToBeats(mousePosition.x);
  hoveredMeasure = static_cast<int>(beatAtMouse / 4.0) + 1;

  // Check hover state for loop handles
  if (loopEnabled) {
      int loopStartX = beatsToPixels(loopStartBeat);
      int loopEndX = beatsToPixels(loopEndBeat);
      int tolerance = 8;
      
      bool overStart = std::abs(event.x - loopStartX) < tolerance;
      bool overEnd = std::abs(event.x - loopEndX) < tolerance;
      bool overRegion = event.x > loopStartX && event.x < loopEndX && event.y < 12;
      
      if (overStart || overEnd)
          setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
      else if (overRegion)
          setMouseCursor(juce::MouseCursor::DraggingHandCursor);
      else
          setMouseCursor(juce::MouseCursor::PointingHandCursor);
  } else {
      setMouseCursor(juce::MouseCursor::PointingHandCursor);
  }

  repaint();
}

void TimelineRuler::mouseEnter(const juce::MouseEvent &event) {
  isHovered = true;
  mousePosition = event.getPosition();
}

void TimelineRuler::mouseExit(const juce::MouseEvent &event) {
  isHovered = false;
  hoveredMeasure = -1;
  repaint();
}

void TimelineRuler::mouseDown(const juce::MouseEvent &event) {
  double beatAtClick = pixelsToBeats(event.x);
  
  if (loopEnabled) {
      int loopStartX = beatsToPixels(loopStartBeat);
      int loopEndX = beatsToPixels(loopEndBeat);
      int tolerance = 8;
      
      if (std::abs(event.x - loopStartX) < tolerance) {
          currentDragMode = DragMode::MoveLoopStart;
          dragStartBeat = loopStartBeat;
          return;
      }
      
      if (std::abs(event.x - loopEndX) < tolerance) {
          currentDragMode = DragMode::MoveLoopEnd;
          dragStartBeat = loopEndBeat;
          return;
      }
      
      if (event.x > loopStartX && event.x < loopEndX && event.y < 12) {
          currentDragMode = DragMode::MoveLoopRegion;
          dragStartBeat = beatAtClick;
          initialLoopStart = loopStartBeat;
          initialLoopEnd = loopEndBeat;
          return;
      }
  }

  // Click-to-seek functionality
  currentDragMode = DragMode::Seek;
  
  // Snap to nearest beat for seek
  double snapBeat = std::round(beatAtClick);

  // Call seek callback if set
  if (onSeek)
    onSeek(snapBeat);
}

void TimelineRuler::mouseDrag(const juce::MouseEvent &event) {
    if (currentDragMode == DragMode::None) return;
    
    double beatAtMouse = pixelsToBeats(event.x);
    
    if (currentDragMode == DragMode::Seek) {
        if (onSeek)
            onSeek(std::max(0.0, beatAtMouse));
        return;
    }
    
    double newStart = loopStartBeat;
    double newEnd = loopEndBeat;
    
    if (currentDragMode == DragMode::MoveLoopStart) {
        newStart = beatAtMouse;
        // Apply snap
        if (!event.mods.isShiftDown()) newStart = std::round(newStart * 4.0) / 4.0;
        
        // Constraint
        if (newStart >= newEnd) newStart = newEnd - 0.25;
        
    } else if (currentDragMode == DragMode::MoveLoopEnd) {
        newEnd = beatAtMouse;
        // Apply snap
        if (!event.mods.isShiftDown()) newEnd = std::round(newEnd * 4.0) / 4.0;
        
        // Constraint
        if (newEnd <= newStart) newEnd = newStart + 0.25;
        
    } else if (currentDragMode == DragMode::MoveLoopRegion) {
        double delta = beatAtMouse - dragStartBeat;
        // Snap delta
        if (!event.mods.isShiftDown()) delta = std::round(delta * 4.0) / 4.0;
        
        double length = initialLoopEnd - initialLoopStart;
        newStart = initialLoopStart + delta;
        newEnd = newStart + length;
    }
    
    // Notify change
    if (onLoopChanged) {
        onLoopChanged(std::max(0.0, newStart), std::max(0.0, newEnd));
    }
}

void TimelineRuler::mouseUp(const juce::MouseEvent &) {
    currentDragMode = DragMode::None;
}

void TimelineRuler::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds();
  using namespace zenith::design;

  // POLISH: Flat background using BG_DARKER (no gradients)
  SkPaint bgPaint;
  bgPaint.setAntiAlias(true);
  bgPaint.setColor(design::unified::bg_01());
  canvas->drawRect(SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()),
                   bgPaint);

  // Top border for separation
  SkPaint borderPaint;
  borderPaint.setAntiAlias(true);
  borderPaint.setColor(colors::BORDER_SUBTLE);
  canvas->drawLine(0, 0, bounds.getWidth(), 0, borderPaint);

  // Draw beat markers
  int startBeat = static_cast<int>(std::floor(viewStartBeat));
  int endBeat = static_cast<int>(std::ceil(viewStartBeat + viewLengthBeats));

  SkFont font = typography::getSkFont(typography::FONT_SM);
  font.setEdging(SkFont::Edging::kAntiAlias);

  for (int beat = startBeat; beat <= endBeat; ++beat) {
    int x = beatsToPixels(beat);

    if (x < 0 || x > bounds.getWidth())
      continue;

    // Downbeats (measure starts) - every 4 beats
    if (beat % 4 == 0) {
      int measure = beat / 4 + 1;
      bool isHoveredMeasure = (measure == hoveredMeasure);

      // POLISH: Subtle highlight for hovered measure
      if (isHoveredMeasure && hoverAnimation > 0.01f) {
        SkPaint hoverBgPaint;
        hoverBgPaint.setAntiAlias(true);
        hoverBgPaint.setColor(SkColorSetARGB(30, 255, 255, 255));
        hoverBgPaint.setAlpha(static_cast<uint8_t>(30 * hoverAnimation));

        int nextX = beatsToPixels(beat + 4);
        if (nextX > bounds.getWidth())
          nextX = bounds.getWidth();
        canvas->drawRect(SkRect::MakeXYWH(x, 0, nextX - x, bounds.getHeight()),
                         hoverBgPaint);
      }

      // Measure line using borderStrong
      SkPaint linePaint;
      linePaint.setAntiAlias(true);
      linePaint.setColor(isHoveredMeasure ? colors::CYAN
                                          : colors::BORDER_STRONG);
      if (isHoveredMeasure)
        linePaint.setAlpha(
            static_cast<uint8_t>(255 * hoverAnimation * 0.6f + 255 * 0.4f));
      linePaint.setStrokeWidth(1.0f);
      canvas->drawLine(x, 0, x, bounds.getHeight(), linePaint);

      // POLISH: Measure number using Typography.small
      juce::String text = juce::String(measure);
      SkPaint textPaint;
      textPaint.setAntiAlias(true);
      textPaint.setColor(isHoveredMeasure ? colors::ACCENT_PRIMARY
                                           : colors::TEXT_SECONDARY);
      if (isHoveredMeasure)
        textPaint.setAlphaf(hoverAnimation * 0.5f + 0.5f);

      canvas->drawString(
          text.toRawUTF8(), x + 8,
          bounds.getHeight() / 2 + typography::FONT_SM / 2, font, textPaint);
    } else {
      // Minor beat ticks using borderSubtle
      SkPaint tickPaint;
      tickPaint.setAntiAlias(true);
      tickPaint.setColor(colors::BORDER_SUBTLE);
      tickPaint.setStrokeWidth(0.5f);
      canvas->drawLine(x, bounds.getHeight() - 8, x, bounds.getHeight(),
                       tickPaint);
    }
  }

  // Draw Loop Region
  if (loopEnabled) {
      int loopStartX = beatsToPixels(loopStartBeat);
      int loopEndX = beatsToPixels(loopEndBeat);
      
      // Only draw if visible
      if (loopEndX >= 0 && loopStartX <= bounds.getWidth()) {
          SkPaint bracketPaint;
          bracketPaint.setAntiAlias(true);
          bracketPaint.setColor(colors::NEON_GREEN);
          bracketPaint.setStyle(SkPaint::kStroke_Style);
          bracketPaint.setStrokeWidth(2.0f);
          
          float rulerTop = 2.0f;
          float rulerBottom = 15.0f; // Limit brackets to top half
          float bracketWidth = 8.0f;
          
          // Left bracket: ⌐
          SkPath leftBracket;
          leftBracket.moveTo(loopStartX + bracketWidth, rulerTop);
          leftBracket.lineTo(loopStartX, rulerTop);
          leftBracket.lineTo(loopStartX, rulerBottom);
          canvas->drawPath(leftBracket, bracketPaint);
          
          // Right bracket: ¬
          SkPath rightBracket;
          rightBracket.moveTo(loopEndX - bracketWidth, rulerTop);
          rightBracket.lineTo(loopEndX, rulerTop);
          rightBracket.lineTo(loopEndX, rulerBottom);
          canvas->drawPath(rightBracket, bracketPaint);
          
          // Shaded region in ruler (top strip)
          SkPaint regionPaint;
          regionPaint.setColor(withAlpha(colors::NEON_GREEN, 0.15f));
          canvas->drawRect(SkRect::MakeLTRB(loopStartX, 0, loopEndX, bounds.getHeight()), regionPaint);
          
          // Loop Label
          if (loopEndX - loopStartX > 60) {
              SkFont labelFont = typography::getSkFont(typography::FONT_XS, FontWeight::Bold);
              SkPaint textPaint;
              textPaint.setColor(colors::NEON_GREEN);
              textPaint.setAntiAlias(true);
              
              // Add a subtle glow to the text
              textPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 2.0f));
              canvas->drawString("LOOP", loopStartX + 5, 12, labelFont, textPaint);
              textPaint.setMaskFilter(nullptr);
              canvas->drawString("LOOP", loopStartX + 5, 12, labelFont, textPaint);
          }
      }
  }

  // Tooltip (preserved from original)
  if (isHovered && hoverAnimation > 0.5f) {
    double beatAtMouse = pixelsToBeats(mousePosition.x);
    juce::String timeText = formatTimePosition(beatAtMouse);

    SkFont tooltipFont = typography::getSkFont(typography::FONT_SM);
    tooltipFont.setEdging(SkFont::Edging::kAntiAlias);

    SkRect textBounds;
    tooltipFont.measureText(timeText.toRawUTF8(), timeText.length(),
                            SkTextEncoding::kUTF8, &textBounds);
    int tooltipWidth = static_cast<int>(textBounds.width()) + 16;
    int tooltipHeight = 24;

    int tooltipX = mousePosition.x - tooltipWidth / 2;
    int tooltipY = bounds.getHeight() + 4;
    tooltipX = juce::jlimit(2, bounds.getWidth() - tooltipWidth - 2, tooltipX);

    SkRect tooltipRect =
        SkRect::MakeXYWH(tooltipX, tooltipY, tooltipWidth, tooltipHeight);

    // POLISH: Simplified tooltip (no heavy shadows)
    SkPaint tooltipBgPaint;
    tooltipBgPaint.setAntiAlias(true);
    tooltipBgPaint.setColor(colors::BG_DARK);
    tooltipBgPaint.setAlpha(static_cast<uint8_t>(255 * hoverAnimation));
    canvas->drawRoundRect(tooltipRect, 4.0f, 4.0f, tooltipBgPaint);

    SkPaint tooltipBorderPaint;
    tooltipBorderPaint.setAntiAlias(true);
    tooltipBorderPaint.setColor(colors::BORDER_SUBTLE);
    tooltipBorderPaint.setStyle(SkPaint::kStroke_Style);
    tooltipBorderPaint.setStrokeWidth(1.0f);
    tooltipBorderPaint.setAlpha(static_cast<uint8_t>(255 * hoverAnimation));
    canvas->drawRoundRect(tooltipRect, 4.0f, 4.0f, tooltipBorderPaint);

    SkPaint tooltipTextPaint;
    tooltipTextPaint.setAntiAlias(true);
    tooltipTextPaint.setColor(colors::TEXT_PRIMARY);
    tooltipTextPaint.setAlpha(static_cast<uint8_t>(255 * hoverAnimation));

    float textX = tooltipRect.centerX() - textBounds.width() / 2;
    float textY = tooltipRect.centerY() + typography::FONT_SM / 2;
    canvas->drawSimpleText(timeText.toRawUTF8(), timeText.length(),
                           SkTextEncoding::kUTF8, textX, textY, tooltipFont,
                           tooltipTextPaint);
  }
}

juce::String TimelineRuler::formatTimePosition(double beat) const {
  // Format: Measure.Beat (e.g., "3.2" for 3rd measure, 2nd beat)
  int measure = static_cast<int>(beat / 4.0) + 1;
  int beatInMeasure = static_cast<int>(beat) % 4 + 1;
  double fraction = beat - std::floor(beat);

  if (fraction < 0.01) // On the beat
    return juce::String::formatted("%d.%d", measure, beatInMeasure);
  else // Between beats
    return juce::String::formatted("%d.%d.%02d", measure, beatInMeasure,
                                   static_cast<int>(fraction * 100));
}

} // namespace zenith
