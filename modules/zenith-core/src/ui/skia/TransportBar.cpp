/*
  ==============================================================================

    TransportBar.cpp
    Created: 2025-11-28
    Author:  Leo "Lil Bit" Rossi

    Implementation of transport controls with Neon Noir styling.

  ==============================================================================
*/

#include "TransportBar.h"

#ifdef ZENITH_USE_SKIA

namespace zenith {

TransportBar::TransportBar() {
    setSize(800, 60);
}

void TransportBar::resized() {
    auto area = getLocalBounds();
    int buttonWidth = 50;
    int spacing = 10;
    
    auto leftSection = area.removeFromLeft(250);
    playButtonBounds_ = leftSection.removeFromLeft(buttonWidth).reduced(spacing);
    leftSection.removeFromLeft(spacing);
    stopButtonBounds_ = leftSection.removeFromLeft(buttonWidth).reduced(spacing);
    leftSection.removeFromLeft(spacing);
    recordButtonBounds_ = leftSection.removeFromLeft(buttonWidth).reduced(spacing);
    
    // View Toggle Button (Right side)
    viewToggleButtonBounds_ = area.removeFromRight(60).reduced(10);
}

void TransportBar::drawSkia(SkCanvas* canvas) {
    auto bounds = getLocalBounds().toFloat();
    
    SkPaint paint;
    paint.setAntiAlias(true);
    
    // Background with glassmorphism
    paint.setStyle(SkPaint::kFill_Style);
    paint.setColor(SkColorSetARGB(180, 15, 15, 20));
    canvas->drawRect(SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()), paint);
    
    // Border glow
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(1.0f);
    paint.setColor(SkColorSetARGB(100, 0, 255, 255));
    canvas->drawLine(0, bounds.getHeight(), bounds.getWidth(), bounds.getHeight(), paint);
    
    // Draw buttons
    drawButton(canvas, playButtonBounds_, "▶", isPlaying_, 0xFF00FF64); // Green
    drawButton(canvas, stopButtonBounds_, "■", !isPlaying_, 0xFF6464FF); // Blue
    drawButton(canvas, recordButtonBounds_, "●", isRecording_, 0xFFFF3232); // Red
    
    // View Toggle
    drawButton(canvas, viewToggleButtonBounds_, "↹", false, 0xFFFFFFFF);
    
    // Draw tempo
    SkFont font;
    font.setSize(16.0f);
    paint.setStyle(SkPaint::kFill_Style);
    paint.setColor(SK_ColorWHITE);
    
    juce::String tempoStr = juce::String(tempo_, 1) + " BPM";
    canvas->drawSimpleText(tempoStr.toStdString().c_str(), tempoStr.length(), SkTextEncoding::kUTF8, 250, 35, font, paint);
    
    // Draw Project Name
    font.setSize(14.0f);
    paint.setColor(SkColorSetARGB(150, 255, 255, 255));
    canvas->drawSimpleText(projectName_.toStdString().c_str(), projectName_.length(), SkTextEncoding::kUTF8, 350, 35, font, paint);
    
    // Draw CPU meter
    juce::Rectangle<int> cpuBounds(bounds.getWidth() - 250, 20, 100, 20);
    drawMeter(canvas, cpuBounds, cpuUsage_ / 100.0f, "CPU");
}

void TransportBar::drawButton(SkCanvas* canvas, const juce::Rectangle<int>& bounds,
                               const char* label, bool isActive, uint32_t color) {
    SkPaint paint;
    paint.setAntiAlias(true);
    
    SkRect rect = SkRect::MakeXYWH(bounds.getX(), bounds.getY(), 
                                    bounds.getWidth(), bounds.getHeight());
    SkRRect rrect = SkRRect::MakeRectXY(rect, 4.0f, 4.0f);
    
    // Background
    if (isActive) {
        paint.setColor(color & 0x64FFFFFF); // ~40% alpha
    } else {
        paint.setColor(SkColorSetARGB(30, 255, 255, 255));
    }
    canvas->drawRRect(rrect, paint);
    
    // Border
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(isActive ? 2.0f : 1.0f);
    paint.setColor(isActive ? color : SkColorSetARGB(100, 255, 255, 255));
    canvas->drawRRect(rrect, paint);
    
    // Label
    SkFont font;
    font.setSize(20.0f);
    paint.setStyle(SkPaint::kFill_Style);
    paint.setColor(isActive ? color : SK_ColorWHITE);
    
    float textWidth = font.measureText(label, strlen(label), SkTextEncoding::kUTF8);
    canvas->drawSimpleText(label, strlen(label), SkTextEncoding::kUTF8, bounds.getCentreX() - textWidth / 2, bounds.getCentreY() + 7, font, paint);
}

void TransportBar::drawMeter(SkCanvas* canvas, const juce::Rectangle<int>& bounds,
                              float value, const char* label) {
    SkPaint paint;
    paint.setAntiAlias(true);
    
    // Background
    paint.setColor(SkColorSetARGB(50, 0, 0, 0));
    canvas->drawRect(SkRect::MakeXYWH(bounds.getX(), bounds.getY(), 
                                      bounds.getWidth(), bounds.getHeight()), paint);
    
    // Fill
    float fillWidth = bounds.getWidth() * juce::jlimit(0.0f, 1.0f, value);
    uint32_t fillColor = value > 0.8f ? 0xFFFF3232 : // Red
                        value > 0.6f ? 0xFFFFC800 : // Amber
                                       0xFF00FF64;  // Green
    paint.setColor(fillColor);
    canvas->drawRect(SkRect::MakeXYWH(bounds.getX(), bounds.getY(), 
                                      fillWidth, bounds.getHeight()), paint);
    
    // Label
    SkFont font;
    font.setSize(12.0f);
    paint.setColor(SK_ColorWHITE);
    canvas->drawSimpleText(label, strlen(label), SkTextEncoding::kUTF8, bounds.getX() + 5, bounds.getY() - 5, font, paint);
}

void TransportBar::mouseDown(const juce::MouseEvent& e) {
    if (playButtonBounds_.contains(e.getPosition())) {
        if (onPlayClicked) onPlayClicked();
    } else if (stopButtonBounds_.contains(e.getPosition())) {
        if (onStopClicked) onStopClicked();
    } else if (recordButtonBounds_.contains(e.getPosition())) {
        if (onRecordClicked) onRecordClicked();
    } else if (viewToggleButtonBounds_.contains(e.getPosition())) {
        if (onViewToggleClicked) onViewToggleClicked();
    }
}

} // namespace zenith

#endif // ZENITH_USE_SKIA
