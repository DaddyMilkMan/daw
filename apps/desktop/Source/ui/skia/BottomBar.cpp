/*
  ==============================================================================

    BottomBar.cpp
    Created: 2025-11-28
    Author:  David Chen + Leo Rossi

  ==============================================================================
*/

#include "BottomBar.h"

#define ZENITH_USE_SKIA 1 // FORCE DEFINITION FOR DEBUGGING

#ifdef ZENITH_USE_SKIA
#include <core/SkCanvas.h>
#include <core/SkPaint.h>
#include <core/SkColor.h>
#include <core/SkPoint.h>
#include <effects/SkGradientShader.h>
#include <core/SkFont.h>
#endif

namespace zenith {

#ifdef ZENITH_USE_SKIA

BottomBar::BottomBar(juce::MidiKeyboardState& state)
    : midiState_(state)
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
    auto bounds = getLocalBounds().toFloat();
    SkRect skBounds = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());

    // Lazy update of cached resources on the Render Thread
    if (skBounds != cachedBounds_) {
        updateCachedPaints(skBounds);
        cachedBounds_ = skBounds;
    }
    
    // Background
    canvas->drawRect(skBounds, bgPaint_);
    
    // Top border glow
    canvas->drawLine(0.0f, 0.0f, skBounds.width(), 0.0f, borderPaint_);
    
    // If keyboard is hidden, show mixer strip
    if (!keyboardVisible_) {
        // Draw 8 channel strips
        int numChannels = 8;
        float stripWidth = skBounds.width() / numChannels;
        
        for (int i = 0; i < numChannels; ++i) {
            float x = i * stripWidth;
            
            // Channel background
            SkRect channelRect = SkRect::MakeXYWH(x + 4, 10, stripWidth - 8, skBounds.height() - 20);
            canvas->drawRoundRect(channelRect, 4.0f, 4.0f, channelBgPaint_);
            
            // Volume meter (placeholder - would connect to actual channels)
            float meterHeight = channelRect.height() - 40;
            float meterLevel = 0.3f + (i * 0.05f); // Demo levels
            
            // Meter track
            SkRect meterTrack = SkRect::MakeXYWH(channelRect.centerX() - 8, channelRect.y() + 25, 16, meterHeight);
            canvas->drawRoundRect(meterTrack, 2.0f, 2.0f, meterTrackPaint_);
            
            // Meter fill (green to red gradient)
            float fillHeight = meterHeight * meterLevel;
            SkRect meterFill = SkRect::MakeXYWH(meterTrack.left(), meterTrack.bottom() - fillHeight, 16, fillHeight);
            
            SkColor meterColor = meterLevel > 0.8f ? 0xFFFF3232 : (meterLevel > 0.6f ? 0xFFFFC800 : 0xFF00FF64);
            meterFillPaint_.setColor(meterColor);
            canvas->drawRoundRect(meterFill, 2.0f, 2.0f, meterFillPaint_);
            
            // Channel label
            juce::String label = juce::String(i + 1);
            canvas->drawString(label.toStdString().c_str(), channelRect.centerX() - 4, channelRect.y() + 15, font_, textPaint_);
        }
    }
}

void BottomBar::updateCachedPaints(const SkRect& bounds) {
    // 1. Background Paint
    bgPaint_.setAntiAlias(true);
    bgPaint_.setColor(SkColorSetARGB(255, 20, 20, 20)); // Opaque dark grey
    bgPaint_.setStyle(SkPaint::kFill_Style);

    // 2. Border Paint (Gradient)
    borderPaint_.setAntiAlias(true);
    borderPaint_.setStyle(SkPaint::kStroke_Style);
    borderPaint_.setStrokeWidth(1.0f);
    
    SkPoint points[2] = { SkPoint::Make(0.0f, 0.0f), SkPoint::Make(bounds.width(), 0.0f) };
    SkColor colors[3] = { 0x0000AAFF, 0xFF00AAFF, 0x0000AAFF };
    borderPaint_.setShader(SkGradientShader::MakeLinear(points, colors, nullptr, 3, SkTileMode::kClamp));

    // 3. Channel Background
    channelBgPaint_.setAntiAlias(true);
    channelBgPaint_.setColor(SkColorSetARGB(30, 255, 255, 255));
    channelBgPaint_.setStyle(SkPaint::kFill_Style);

    // 4. Meter Track
    meterTrackPaint_.setAntiAlias(true);
    meterTrackPaint_.setColor(SkColorSetARGB(50, 0, 0, 0));
    meterTrackPaint_.setStyle(SkPaint::kFill_Style);

    // 5. Meter Fill (Base)
    meterFillPaint_.setAntiAlias(true);
    meterFillPaint_.setStyle(SkPaint::kFill_Style);

    // 6. Text Paint
    textPaint_.setAntiAlias(true);
    textPaint_.setColor(SkColorSetARGB(150, 255, 255, 255));
    textPaint_.setStyle(SkPaint::kFill_Style);

    // 7. Font
    font_.setSize(10.0f);
    font_.setSubpixel(true);
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
