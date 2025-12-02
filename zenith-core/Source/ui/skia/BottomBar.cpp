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
#include <skia/include/core/SkCanvas.h>
#include <skia/include/core/SkPaint.h>
#include <skia/include/core/SkColor.h>
#include <skia/include/core/SkPoint.h>
#include <skia/include/effects/SkGradientShader.h>
#include <skia/include/core/SkFont.h>
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
    SkPaint paint;
    paint.setAntiAlias(true);
    
    // Background
    paint.setColor(SkColorSetARGB(255, 20, 20, 20)); // Opaque dark grey
    canvas->drawRect(SkRect::MakeWH((float)getWidth(), (float)getHeight()), paint);
    
    // Top border glow
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(1.0f);
    
    // Gradient for top border
    SkPoint points[2] = { SkPoint::Make(0.0f, 0.0f), SkPoint::Make((float)getWidth(), 0.0f) };
    
    SkColor colors[3] = { 
        0x0000AAFF, // Transparent Cyan
        0xFF00AAFF, // Opaque Cyan
        0x0000AAFF  // Transparent Cyan
    };
    
    paint.setShader(SkGradientShader::MakeLinear(points, colors, nullptr, 3, SkTileMode::kClamp));
    
    canvas->drawLine(0.0f, 0.0f, (float)getWidth(), 0.0f, paint);
    
    // If keyboard is hidden, show mixer strip
    if (!keyboardVisible_) {
        // Simple horizontal mixer strip with volume meters
        SkFont font;
        font.setSize(10.0f);
        paint.setShader(nullptr);
        paint.setStyle(SkPaint::kFill_Style);
        
        // Draw 8 channel strips
        int numChannels = 8;
        float stripWidth = (float)getWidth() / numChannels;
        
        for (int i = 0; i < numChannels; ++i) {
            float x = i * stripWidth;
            
            // Channel background
            paint.setColor(SkColorSetARGB(30, 255, 255, 255));
            SkRect channelRect = SkRect::MakeXYWH(x + 4, 10, stripWidth - 8, getHeight() - 20);
            canvas->drawRoundRect(channelRect, 4.0f, 4.0f, paint);
            
            // Volume meter (placeholder - would connect to actual channels)
            float meterHeight = channelRect.height() - 40;
            float meterLevel = 0.3f + (i * 0.05f); // Demo levels
            
            // Meter track
            paint.setColor(SkColorSetARGB(50, 0, 0, 0));
            SkRect meterTrack = SkRect::MakeXYWH(channelRect.centerX() - 8, channelRect.y() + 25, 16, meterHeight);
            canvas->drawRoundRect(meterTrack, 2.0f, 2.0f, paint);
            
            // Meter fill (green to red gradient)
            float fillHeight = meterHeight * meterLevel;
            SkRect meterFill = SkRect::MakeXYWH(meterTrack.left(), meterTrack.bottom() - fillHeight, 16, fillHeight);
            
            SkColor meterColor = meterLevel > 0.8f ? 0xFFFF3232 : (meterLevel > 0.6f ? 0xFFFFC800 : 0xFF00FF64);
            paint.setColor(meterColor);
            canvas->drawRoundRect(meterFill, 2.0f, 2.0f, paint);
            
            // Channel label
            paint.setColor(SkColorSetARGB(150, 255, 255, 255));
            juce::String label = juce::String(i + 1);
            canvas->drawString(label.toStdString().c_str(), channelRect.centerX() - 4, channelRect.y() + 15, font, paint);
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
