/*
  ==============================================================================

    SkiaKnobComponent.h
    Created: 2025-11-28
    Author:  Zenith DAW Team

    Skia-rendered knob component.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include "SkiaComponent.h"

#ifdef ZENITH_USE_SKIA
#include "ZenithSkia.h"
#include <core/SkPath.h>
#endif

namespace zenith {

#ifdef ZENITH_USE_SKIA

class SkiaKnobComponent : public SkiaComponent {
public:
    enum class SliderStyle { Rotary, RotaryHorizontalVerticalDrag };
    enum class TextBoxPos { NoTextBox, TextBoxBelow, TextBoxAbove, TextBoxLeft, TextBoxRight };

    SkiaKnobComponent() {
        setRepaintsOnMouseActivity(true);
    }
    
    ~SkiaKnobComponent() override = default;

    void setSliderStyle(SliderStyle style) { style_ = style; repaint(); }
    void setTextBoxStyle(TextBoxPos pos, bool isReadOnly, int width, int height) { 
        textBoxPos_ = pos; 
        repaint(); 
    }
    
    void setRange(double min, double max, double interval = 0.0) {
        rangeStart_ = min;
        rangeEnd_ = max;
        interval_ = interval;
        repaint();
    }
    
    void setValue(double value, juce::NotificationType notification = juce::sendNotificationAsync) {
        value_ = juce::jlimit(rangeStart_, rangeEnd_, value);
        if (notification != juce::dontSendNotification && onValueChange) {
            onValueChange();
        }
        repaint();
    }
    
    double getValue() const { return value_; }
    
    std::function<void()> onValueChange;

    void drawSkia(SkCanvas* canvas) override {
        SkPaint paint;
        paint.setAntiAlias(true);
        
        auto bounds = getLocalBounds().toFloat();
        float w = bounds.getWidth();
        float h = bounds.getHeight();
        float radius = std::min(w, h) * 0.4f;
        float cx = w * 0.5f;
        float cy = h * 0.5f;
        
        // Background
        paint.setColor(SkColorSetARGB(50, 255, 255, 255));
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setStrokeWidth(4.0f);
        canvas->drawCircle(cx, cy, radius, paint);
        
        // Value Arc
        float normalized = (float)((value_ - rangeStart_) / (rangeEnd_ - rangeStart_));
        float startAngle = 135.0f;
        float sweepAngle = 270.0f * normalized;
        
        paint.setColor(0xFFFF0096); // Pink
        SkPath path;
        path.addArc(SkRect::MakeXYWH(cx - radius, cy - radius, radius * 2, radius * 2), startAngle, sweepAngle);
        canvas->drawPath(path, paint);
        
        // Indicator
        paint.setStyle(SkPaint::kFill_Style);
        paint.setColor(SK_ColorWHITE);
        float angleRad = (startAngle + sweepAngle) * (3.14159f / 180.0f);
        float ix = cx + cos(angleRad) * radius * 0.8f;
        float iy = cy + sin(angleRad) * radius * 0.8f;
        canvas->drawCircle(ix, iy, 3.0f, paint);
    }
    
    void mouseDown(const juce::MouseEvent& e) override {
        lastMouseY_ = e.position.y;
    }
    
    void mouseDrag(const juce::MouseEvent& e) override {
        float delta = (lastMouseY_ - e.position.y) / 100.0f;
        lastMouseY_ = e.position.y;
        
        double range = rangeEnd_ - rangeStart_;
        setValue(value_ + delta * range);
    }

private:
    double value_ = 0.0;
    double rangeStart_ = 0.0;
    double rangeEnd_ = 1.0;
    double interval_ = 0.0;
    float lastMouseY_ = 0.0f;
    SliderStyle style_ = SliderStyle::Rotary;
    TextBoxPos textBoxPos_ = TextBoxPos::NoTextBox;
};

#endif // ZENITH_USE_SKIA

} // namespace zenith
