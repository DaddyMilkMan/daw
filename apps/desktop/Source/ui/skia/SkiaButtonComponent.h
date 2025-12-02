/*
  ==============================================================================

    SkiaButtonComponent.h
    Created: 2025-11-28
    Author:  Zenith DAW Team

    Skia-rendered button component.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "SkiaComponent.h"

#ifdef ZENITH_USE_SKIA
#include <core/SkCanvas.h>
#include <core/SkPaint.h>
#include <core/SkRRect.h>
#include <core/SkFont.h>
#include <core/SkColor.h>
#endif

namespace zenith {

#ifdef ZENITH_USE_SKIA

class SkiaButtonComponent : public SkiaComponent {
public:
    SkiaButtonComponent() {
        setRepaintsOnMouseActivity(true);
    }
    
    ~SkiaButtonComponent() override = default;

    void setButtonText(const juce::String& text) {
        text_ = text;
        repaint();
    }
    
    void setToggleState(bool toggled, juce::NotificationType notification = juce::sendNotificationAsync) {
        isToggled_ = toggled;
        if (notification != juce::dontSendNotification && onClick) {
            onClick();
        }
        repaint();
    }
    
    bool getToggleState() const { return isToggled_; }
    
    void setClickingTogglesState(bool shouldToggle) {
        clickingTogglesState_ = shouldToggle;
    }
    
    void setColour(int id, juce::Colour color) {
        // Simple mapping for now
        if (id == juce::TextButton::buttonOnColourId) {
            onColor_ = color.getARGB();
        }
    }
    
    std::function<void()> onClick;

    void drawSkia(SkCanvas* canvas) override {
        SkPaint paint;
        paint.setAntiAlias(true);
        
        auto bounds = getLocalBounds().toFloat();
        SkRect rect = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());
        SkRRect rrect = SkRRect::MakeRectXY(rect, 4.0f, 4.0f);
        
        // Background
        if (isToggled_ || isDown_) {
            paint.setColor(onColor_); 
        } else {
            paint.setColor(SkColorSetARGB(50, 100, 100, 100));
        }
        canvas->drawRRect(rrect, paint);
        
        // Border
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setStrokeWidth(1.0f);
        paint.setColor(SK_ColorWHITE);
        canvas->drawRRect(rrect, paint);
        
        // Text
        SkFont font;
        font.setSize(14.0f);
        paint.setStyle(SkPaint::kFill_Style);
        paint.setColor(SK_ColorWHITE);
        
        std::string str = text_.toStdString();
        float width = font.measureText(str.c_str(), str.length(), SkTextEncoding::kUTF8);
        
        canvas->drawString(str.c_str(), 
                          bounds.getCentreX() - width/2, 
                          bounds.getCentreY() + 5, 
                          font, paint);
    }
    
    void mouseDown(const juce::MouseEvent& e) override {
        isDown_ = true;
        repaint();
    }
    
    void mouseUp(const juce::MouseEvent& e) override {
        isDown_ = false;
        if (contains(e.getPosition())) {
            if (clickingTogglesState_) {
                setToggleState(!isToggled_);
            } else if (onClick) {
                onClick();
            }
        }
        repaint();
    }

private:
    juce::String text_;
    bool isToggled_ = false;
    bool isDown_ = false;
    bool clickingTogglesState_ = false;
    uint32_t onColor_ = 0xFF00FF00; // Green default
};

#endif // ZENITH_USE_SKIA

} // namespace zenith
