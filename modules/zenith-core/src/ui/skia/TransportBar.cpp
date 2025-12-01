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
#include <include/effects/SkGradientShader.h>
#include <include/core/SkPath.h>
#include <include/core/SkRRect.h>

namespace zenith {

TransportBar::TransportBar() {
    setSize(800, 60);
    
    // Play Button
    playBtn_ = std::make_unique<SkiaButton>("Play");
    playBtn_->setStyle(SkiaButton::Style::Primary);
    playBtn_->setToggleable(true);
    playBtn_->onClick = [this]() {
        if (onPlayClicked) onPlayClicked();
    };
    addChildComponent(playBtn_.get());
    
    // Stop Button
    stopBtn_ = std::make_unique<SkiaButton>("Stop");
    stopBtn_->setStyle(SkiaButton::Style::Secondary);
    stopBtn_->onClick = [this]() {
        if (onStopClicked) onStopClicked();
    };
    addChildComponent(stopBtn_.get());
    
    // Record Button
    recordBtn_ = std::make_unique<SkiaButton>("Record");
    recordBtn_->setStyle(SkiaButton::Style::Danger);
    recordBtn_->setToggleable(true);
    recordBtn_->onClick = [this]() {
        if (onRecordClicked) onRecordClicked();
    };
    addChildComponent(recordBtn_.get());
    
    // View Toggle
    viewToggleBtn_ = std::make_unique<SkiaButton>("Mixer");
    viewToggleBtn_->setStyle(SkiaButton::Style::Ghost);
    viewToggleBtn_->setToggleable(true);
    viewToggleBtn_->onClick = [this]() {
        if (onViewToggleClicked) onViewToggleClicked();
    };
    addChildComponent(viewToggleBtn_.get());
    
    // Tempo Knob
    tempoKnob_ = std::make_unique<SkiaKnob>("Tempo");
    tempoKnob_->setStyle(SkiaKnob::Style::ArcAndDot);
    tempoKnob_->setDisplayRange(60.0f, 200.0f);
    tempoKnob_->setValue(0.5f); // 130 BPM approx
    tempoKnob_->setValueColoring(true);
    addChildComponent(tempoKnob_.get());
    
    // Master Volume
    masterVolSlider_ = std::make_unique<SkiaSlider>("Master");
    masterVolSlider_->setStyle(SkiaSlider::Style::Fader);
    masterVolSlider_->setOrientation(SkiaSlider::Orientation::Horizontal);
    masterVolSlider_->setValue(0.8f);
    addChildComponent(masterVolSlider_.get());
}

TransportBar::~TransportBar() {
    playBtn_ = nullptr;
    stopBtn_ = nullptr;
    recordBtn_ = nullptr;
    viewToggleBtn_ = nullptr;
    tempoKnob_ = nullptr;
    masterVolSlider_ = nullptr;
}

void TransportBar::resized() {
    auto area = getLocalBounds().reduced(10);
    
    // Left: Transport Controls
    auto transportArea = area.removeFromLeft(200);
    int btnSize = 40;
    int gap = 10;
    
    playBtn_->setBounds(transportArea.getX(), transportArea.getCentreY() - btnSize/2, btnSize, btnSize);
    stopBtn_->setBounds(playBtn_->getRight() + gap, transportArea.getCentreY() - btnSize/2, btnSize, btnSize);
    recordBtn_->setBounds(stopBtn_->getRight() + gap, transportArea.getCentreY() - btnSize/2, btnSize, btnSize);
    
    // Center: Info Display (CPU, Time) - Drawn manually in drawSkia
    auto centerArea = area.removeFromLeft(300);
    
    // Right: Tempo & Volume
    auto rightArea = area;
    
    tempoKnob_->setBounds(rightArea.getX(), rightArea.getY(), 50, 50);
    
    masterVolSlider_->setBounds(tempoKnob_->getRight() + 20, rightArea.getCentreY() - 15, 150, 30);
    
    viewToggleBtn_->setBounds(area.getRight() - 60, area.getCentreY() - 15, 60, 30);
}

void TransportBar::setPlaying(bool playing) {
    isPlaying_ = playing;
    if (playBtn_) playBtn_->setToggleState(playing, false);
}

void TransportBar::setRecording(bool recording) {
    isRecording_ = recording;
    if (recordBtn_) playBtn_->setToggleState(recording, false);
}

void TransportBar::setTempo(double bpm) {
    tempo_ = bpm;
    // Map BPM to 0-1 range (60-200)
    if (tempoKnob_) {
        float norm = (bpm - 60.0) / (200.0 - 60.0);
        tempoKnob_->setValue(juce::jlimit(0.0f, 1.0f, norm));
    }
}

void TransportBar::drawSkia(SkCanvas* canvas) {
    auto bounds = getLocalBounds().toFloat();
    
    // Draw Panel Background
    SkPaint bgPaint;
    bgPaint.setColor(design::colors::BG_DARKER);
    canvas->drawRect(SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()), bgPaint);
    
    // Draw Glass Overlay
    SkPaint glassPaint;
    glassPaint.setColor(design::colors::GLASS_10);
    canvas->drawRect(SkRect::MakeWH(bounds.getWidth(), bounds.getHeight() / 2), glassPaint);
    
    // Draw Bottom Border
    SkPaint borderPaint;
    borderPaint.setColor(design::colors::BORDER_DEFAULT);
    borderPaint.setStrokeWidth(1.0f);
    canvas->drawLine(0, bounds.getHeight(), bounds.getWidth(), bounds.getHeight(), borderPaint);
    
    // Draw Info Text (CPU, Time)
    SkFont font;
    font.setSize(14.0f);
    SkPaint textPaint;
    textPaint.setColor(design::colors::TEXT_SECONDARY);
    
    juce::String cpuText = "CPU: " + juce::String(cpuUsage_ * 100.0f, 1) + "%";
    canvas->drawString(cpuText.toRawUTF8(), 300, 35, font, textPaint);
    
    juce::String timeText = juce::String(timeSigNum_) + "/" + juce::String(timeSigDen_);
    canvas->drawString(timeText.toRawUTF8(), 400, 35, font, textPaint);
}

} // namespace zenith
#endif
