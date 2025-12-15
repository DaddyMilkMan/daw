/*
  ==============================================================================
    RemoteCursorOverlay.h
    AnyDesk-style Remote Cursor Overlay with Name Bubbles
    Uses Skia for smooth interpolation and vector rendering.
  ==============================================================================
*/
#pragma once
#include "../network/CollaborationManager.h"
#include "skia/SkiaComponent.h"
#include "skia/ZenithDesignSystem.h"
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <core/SkCanvas.h>
#include <core/SkFont.h>
#include <core/SkMaskFilter.h>
#include <core/SkPaint.h>
#include <core/SkPath.h>
#include <core/SkPoint.h>
#include <core/SkRect.h>
#include <map>
#include <string>

namespace zenith {

class RemoteCursorOverlay : public SkiaComponent, public juce::ChangeListener {
public:
  RemoteCursorOverlay() {
    setInterceptsMouseClicks(false, false); // Pass clicks through
    CollaborationManager::getInstance().addChangeListener(this);
    setWantsKeyboardFocus(false);
    startTimerHz(60); // 60 FPS for smooth interpolation
  }

  ~RemoteCursorOverlay() override {
    CollaborationManager::getInstance().removeChangeListener(this);
  }

  void changeListenerCallback(juce::ChangeBroadcaster *) override {
    // Network updates happen here, but we pull data in timer/paint
  }

  void timerCallback() override {
    auto &mgr = CollaborationManager::getInstance();

    // 1. Send MY position
    auto mouse =
        juce::Desktop::getInstance().getMainMouseSource().getScreenPosition();
    auto relative = getLocalPoint(nullptr, mouse);
    // Use getX() / getY() accessors for safety
    float normX = (float)relative.getX() / (float)getWidth();
    float normY = (float)relative.getY() / (float)getHeight();
    mgr.updateLocalCursor(normX, normY);

    // 2. Interpolate Remote Cursors
    if (mgr.getState() == CollaborationManager::ConnectionState::Connected ||
        mgr.getState() == CollaborationManager::ConnectionState::Hosting) {

      for (const auto &user : mgr.getRemoteUsers()) {
        if (!user.isOnline)
          continue;

        // RemoteUser uses juce::Point<float> which may have public x,y but
        // .getX() is safer
        float targetX = user.mousePosition.getX() * getWidth();
        float targetY = user.mousePosition.getY() * getHeight();

        // Initialize if new
        if (smoothPositions.find(user.id) == smoothPositions.end()) {
          smoothPositions[user.id] = ::SkPoint::Make(targetX, targetY);
        }

        // Lerp
        auto &current = smoothPositions[user.id];
        float lerpFactor = 0.2f; // Fast but smooth
        current.fX += (targetX - current.fX) * lerpFactor;
        current.fY += (targetY - current.fY) * lerpFactor;
      }
      markDirty();
    }
  }

  void drawSkia(SkCanvas *canvas) override {
    auto &mgr = CollaborationManager::getInstance();
    if (mgr.getState() != CollaborationManager::ConnectionState::Connected &&
        mgr.getState() != CollaborationManager::ConnectionState::Hosting)
      return;

    ::SkPaint paint;
    paint.setAntiAlias(true);

    for (const auto &user : mgr.getRemoteUsers()) {
      if (!user.isOnline)
        continue;

      if (smoothPositions.find(user.id) == smoothPositions.end())
        continue;
      auto pos = smoothPositions[user.id];

      // Safe color conversion
      ::SkColor userColor =
          (user.color.getARGB() != 0)
              ? SkColorSetARGB(255, user.color.getRed(), user.color.getGreen(),
                               user.color.getBlue())
              : SkColorSetRGB(255, 0, 100); // Fallback Red

      // --- 1. Cursor Arrow ---
      ::SkPath cursorPath;
      cursorPath.moveTo(pos.fX, pos.fY);
      cursorPath.lineTo(pos.fX + 8, pos.fY + 24);
      cursorPath.lineTo(pos.fX + 12, pos.fY + 14); // Notch
      cursorPath.lineTo(pos.fX + 22, pos.fY + 22); // Tail
      cursorPath.lineTo(pos.fX + 24, pos.fY + 18); // Tail width
      cursorPath.lineTo(pos.fX + 14, pos.fY + 11); // Notch back
      cursorPath.lineTo(pos.fX + 24, pos.fY + 8);  // Right point
      cursorPath.close();

      // Shadow
      paint.setColor(SkColorSetARGB(100, 0, 0, 0));
      paint.setMaskFilter(
          ::SkMaskFilter::MakeBlur(::kNormal_SkBlurStyle, 3.0f));
      canvas->drawPath(cursorPath, paint);
      paint.setMaskFilter(nullptr);

      // Fill
      paint.setColor(userColor);
      paint.setStyle(::SkPaint::kFill_Style);
      canvas->drawPath(cursorPath, paint);

      // Outline
      paint.setColor(SK_ColorWHITE);
      paint.setStyle(::SkPaint::kStroke_Style);
      paint.setStrokeWidth(2.0f);
      canvas->drawPath(cursorPath, paint);

      // --- 2. Name Bubble ---
      ::SkFont font =
          zenith::design::getSkFont(12.0f, zenith::design::FontWeight::Bold);
      std::string nameStr = user.name.toStdString();
      float textWidth = font.measureText(nameStr.c_str(), nameStr.length(),
                                         SkTextEncoding::kUTF8);

      float bubbleX = pos.fX + 20.0f;
      float bubbleY = pos.fY + 20.0f;
      ::SkRect bubbleRect =
          ::SkRect::MakeXYWH(bubbleX, bubbleY, textWidth + 16.0f, 24.0f);

      // Bubble Shadow
      paint.setStyle(::SkPaint::kFill_Style);
      paint.setColor(SkColorSetARGB(80, 0, 0, 0));
      paint.setMaskFilter(
          ::SkMaskFilter::MakeBlur(::kNormal_SkBlurStyle, 4.0f));
      canvas->drawRoundRect(bubbleRect, 12.0f, 12.0f, paint);
      paint.setMaskFilter(nullptr);

      // Bubble Fill
      paint.setColor(userColor);
      canvas->drawRoundRect(bubbleRect, 12.0f, 12.0f, paint);

      // Text
      paint.setColor(SK_ColorWHITE);
      canvas->drawString(nameStr.c_str(), bubbleX + 8.0f, bubbleY + 16.0f, font,
                         paint);
    }
  }

private:
  // Using std::map instead of unordered_map to avoid hash compilation issues
  // with juce::String
  std::map<juce::String, ::SkPoint> smoothPositions;
};

} // namespace zenith
