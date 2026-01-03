/*
  ==============================================================================

    SkiaSliderComponent.h
    Created: 2025-11-28
    Author:  Zenith DAW Team

    Skia-rendered slider component.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include "SkiaComponent.h"

#include "ZenithSkia.h"
#endif

namespace zenith {


class SkiaSliderComponent : public SkiaComponent {
public:
    enum class SliderStyle { LinearVertical, LinearHorizontal, Rotary };
    enum class TextBoxPos { NoTextBox, TextBoxBelow, TextBoxAbove, TextBoxLeft, TextBoxRight };

    SkiaSliderComponent() {
        setRepaintsOnMouseActivity(true);
    }
    
    ~SkiaSliderComponent() override = default;

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
        
        // Track
        paint.setColor(SkColorSetARGB(50, 255, 255, 255));
        SkRect trackRect;
        
        if (style_ == SliderStyle::LinearVertical) {
            trackRect = SkRect::MakeXYWH(w/2 - 2, 10, 4, h - 20);
        } else {
            trackRect = SkRect::MakeXYWH(10, h/2 - 2, w - 20, 4);
        }
        
        canvas->drawRoundRect(trackRect, 2, 2, paint);
        
        // Thumb
        paint.setColor(0xFF00FFFF); // Cyan
        float normalized = (float)((value_ - rangeStart_) / (rangeEnd_ - rangeStart_));
        
        SkRect thumbRect;
        if (style_ == SliderStyle::LinearVertical) {
            float thumbY = (h - 20) * (1.0f - normalized) + 10;
            thumbRect = SkRect::MakeXYWH(w/2 - 8, thumbY - 4, 16, 8);
        } else {
            float thumbX = (w - 20) * normalized + 10;
            thumbRect = SkRect::MakeXYWH(thumbX - 4, h/2 - 8, 8, 16);
        }
        
        canvas->drawRoundRect(thumbRect, 2, 2, paint);
    }
    
    void mouseDown(const juce::MouseEvent& e) override {
        mouseDrag(e);
    }
    
    void mouseDrag(const juce::MouseEvent& e) override {
        float normalized = 0.0f;
        auto bounds = getLocalBounds().toFloat();
        
        if (style_ == SliderStyle::LinearVertical) {
            normalized = 1.0f - (e.position.y - 10) / (bounds.getHeight() - 20);
        } else {
            normalized = (e.position.x - 10) / (bounds.getWidth() - 20);
        }
        
        normalized = juce::jlimit(0.0f, 1.0f, normalized);
        setValue(rangeStart_ + normalized * (rangeEnd_ - rangeStart_));
    }

private:
    double value_ = 0.0;
    double rangeStart_ = 0.0;
    double rangeEnd_ = 1.0;
    double interval_ = 0.0;
    SliderStyle style_ = SliderStyle::LinearVertical;
    TextBoxPos textBoxPos_ = TextBoxPos::NoTextBox;
};


} // namespace zenith
