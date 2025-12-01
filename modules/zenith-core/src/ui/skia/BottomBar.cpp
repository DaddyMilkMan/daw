/*
  ==============================================================================

    BottomBar.cpp
    Created: 2025-11-28
    Author:  David Chen + Leo Rossi

  ==============================================================================
*/

#include "BottomBar.h"

#ifdef ZENITH_USE_SKIA
#include <include/core/SkCanvas.h>
#include <include/core/SkPaint.h>
#include <include/core/SkColor.h>
#include <include/core/SkPoint.h>
#include <include/effects/SkGradientShader.h>
#include <include/core/SkFont.h>
#include <include/core/SkRRect.h>
#include "../include/Engine.h"
#include "../include/Track.h"
#endif

namespace zenith {

#ifdef ZENITH_USE_SKIA

BottomBar::BottomBar(juce::MidiKeyboardState& state, Engine& engine)
    : midiState_(state), engine_(engine)
{
    // Create Piano Keyboard
    pianoKeyboard_ = std::make_unique<PianoKeyboardViewSkia>(midiState_,
                                                           juce::MidiKeyboardComponent::horizontalKeyboard);
    addChildComponent(pianoKeyboard_.get());

    // Default size
    setSize(800, 150);
}

BottomBar::~BottomBar() = default;

void BottomBar::drawSkia(SkCanvas* canvas) {
    SkPaint paint;
    paint.setAntiAlias(true);

    // Background
    paint.setColor(SkColorSetARGB(255, 30, 30, 30)); // Opaque dark grey
    canvas->drawRect(SkRect::MakeWH(getWidth(), getHeight()), paint);

    // Top border glow
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(1.0f);

    // Gradient for top border
    SkPoint points[2] = { SkPoint::Make(0, 0), SkPoint::Make(getWidth(), 0) };

    // Use uint32_t for colors array to avoid potential SkColor type issues
    uint32_t colors[3] = {
        0x0000AAFF, // Transparent Cyan
        0xFF00AAFF, // Opaque Cyan
        0x0000AAFF  // Transparent Cyan
    };

    paint.setShader(SkGradientShader::MakeLinear(points, (const SkColor*)colors, nullptr, 3, SkTileMode::kClamp));

    canvas->drawLine(0, 0, getWidth(), 0, paint);

    // If keyboard is hidden, show mixer strip
    if (!keyboardVisible_) {
        // Draw Mixer Strip
        int numTracks = engine_.getNumTracks();
        float trackWidth = 80.0f;
        float startX = 20.0f;
        
        SkFont font;
        font.setSize(12.0f);
        font.setEdging(SkFont::Edging::kAntiAlias);
        
        for (int i = 0; i < numTracks; ++i) {
            float x = startX + i * (trackWidth + 10.0f);
            float y = 10.0f;
            float h = getHeight() - 20.0f;
            
            SkRect stripRect = SkRect::MakeXYWH(x, y, trackWidth, h);
            
            // Strip Background
            paint.setColor(SkColorSetARGB(255, 40, 40, 45));
            paint.setStyle(SkPaint::kFill_Style);
            canvas->drawRoundRect(stripRect, 5, 5, paint);
            
            // Strip Border
            paint.setColor(SkColorSetARGB(255, 60, 60, 70));
            paint.setStyle(SkPaint::kStroke_Style);
            canvas->drawRoundRect(stripRect, 5, 5, paint);
            
            // Track Name
            auto& track = engine_.tracks()[i];
            paint.setColor(SK_ColorWHITE);
            paint.setStyle(SkPaint::kFill_Style);
            // Simple truncation
            std::string name = track->getName().toStdString();
            if (name.length() > 8) name = name.substr(0, 8) + "..";
            canvas->drawString(name.c_str(), x + 5, y + 20, font, paint);
            
            // Fader Track
            float faderX = x + trackWidth / 2.0f - 2.0f;
            float faderY = y + 40.0f;
            float faderH = h - 60.0f;
            
            paint.setColor(SkColorSetARGB(255, 20, 20, 20));
            canvas->drawRect(SkRect::MakeXYWH(faderX, faderY, 4.0f, faderH), paint);
            
            // Fader Handle
            float volume = track->getVolume(); // 0.0 to 1.0
            float handleY = faderY + faderH * (1.0f - volume);
            
            SkRect handleRect = SkRect::MakeXYWH(faderX - 10.0f, handleY - 5.0f, 24.0f, 10.0f);
            paint.setColor(SkColorSetRGB(0, 255, 255)); // Cyan handle
            canvas->drawRoundRect(handleRect, 2, 2, paint);
            
            // Mute Button
            bool isMuted = track->isMuted();
            SkRect muteRect = SkRect::MakeXYWH(x + 5, y + h - 25, 20, 20);
            paint.setColor(isMuted ? SK_ColorRED : SkColorSetRGB(60, 60, 60));
            canvas->drawRoundRect(muteRect, 3, 3, paint);
            
            paint.setColor(SK_ColorWHITE);
            canvas->drawString("M", x + 10, y + h - 10, font, paint);
            
            // Solo Button
            bool isSolo = track->isSolo();
            SkRect soloRect = SkRect::MakeXYWH(x + 30, y + h - 25, 20, 20);
            paint.setColor(isSolo ? SK_ColorYELLOW : SkColorSetRGB(60, 60, 60));
            canvas->drawRoundRect(soloRect, 3, 3, paint);
            
            paint.setColor(SK_ColorBLACK);
            canvas->drawString("S", x + 35, y + h - 10, font, paint);
        }
        
        if (numTracks == 0) {
            paint.setColor(SkColorSetARGB(100, 255, 255, 255));
            canvas->drawString("No Tracks Created", 20, 30, font, paint);
        }
    }
}

void BottomBar::resized() {
    auto area = getLocalBounds();

    if (pianoKeyboard_) {
        // Piano takes full height if visible
        pianoKeyboard_->setBounds(area);
    }
}

void BottomBar::setKeyboardVisible(bool visible) {
    keyboardVisible_ = visible;
    if (pianoKeyboard_) {
        pianoKeyboard_->setVisible(visible);
    }
    repaint();
}

#endif // ZENITH_USE_SKIA

} // namespace zenith
