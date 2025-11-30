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
    
    // If keyboard is hidden, show mixer placeholder
    if (!keyboardVisible_) {
        SkFont font;
        font.setSize(14.0f);
        paint.setShader(nullptr);
        paint.setStyle(SkPaint::kFill_Style);
        paint.setColor(SkColorSetARGB(100, 255, 255, 255));
        canvas->drawString("Mixer Strip (Coming Soon)", 20, 30, font, paint);
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
