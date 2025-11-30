/*
  ==============================================================================

    PianoKeyboardViewSkia.cpp
    Created: 2025-11-28
    Author:  Leo Rossi

  ==============================================================================
*/

#include "PianoKeyboardViewSkia.h"

#define ZENITH_USE_SKIA 1 // FORCE DEFINITION FOR DEBUGGING

#ifdef ZENITH_USE_SKIA
#include <skia/include/core/SkCanvas.h>
#include <skia/include/core/SkPaint.h>
#include <skia/include/core/SkRect.h>
#include <skia/include/core/SkColor.h>
#endif

namespace zenith {

#ifdef ZENITH_USE_SKIA

PianoKeyboardViewSkia::PianoKeyboardViewSkia(juce::MidiKeyboardState& state, 
                                             juce::MidiKeyboardComponent::Orientation orientation)
    : state_(state), orientation_(orientation)
{
    state_.addListener(this);
}

PianoKeyboardViewSkia::~PianoKeyboardViewSkia()
{
    state_.removeListener(this);
}

#include <skia/include/effects/SkGradientShader.h>

void PianoKeyboardViewSkia::drawSkia(SkCanvas* canvas)
{
    SkPaint whiteKeyPaint;
    whiteKeyPaint.setAntiAlias(true);
    // Gradient for white keys (top to bottom)
    SkPoint pts[2] = { SkPoint::Make(0, 0), SkPoint::Make(0, getHeight()) };
    SkColor colors[2] = { 0xFFEEEEEE, 0xFFCCCCCC }; // White to Light Grey
    whiteKeyPaint.setShader(SkGradientShader::MakeLinear(pts, colors, nullptr, 2, SkTileMode::kClamp));

    SkPaint blackKeyPaint;
    blackKeyPaint.setAntiAlias(true);
    // Gradient for black keys
    SkColor blackColors[2] = { 0xFF333333, 0xFF000000 }; // Dark Grey to Black
    blackKeyPaint.setShader(SkGradientShader::MakeLinear(pts, blackColors, nullptr, 2, SkTileMode::kClamp));
    
    SkPaint activeKeyPaint;
    activeKeyPaint.setAntiAlias(true);
    SkColor activeColors[2] = { 0xFF00FFFF, 0xFF0088AA }; // Cyan Gradient
    activeKeyPaint.setShader(SkGradientShader::MakeLinear(pts, activeColors, nullptr, 2, SkTileMode::kClamp));
    
    float x = 0;
    float w = getWidth();
    float h = getHeight();
    
    // Calculate key width based on visible range
    int numWhiteKeys = 0;
    for (int i = rangeStart_; i <= rangeEnd_; ++i) {
        if (!juce::MidiMessage::isMidiNoteBlack(i)) numWhiteKeys++;
    }
    
    float whiteKeyWidth = w / (float)numWhiteKeys;
    float blackKeyWidth = whiteKeyWidth * 0.6f;
    float blackKeyHeight = h * 0.6f;
    
    // Draw White Keys first
    float currentX = 0;
    for (int i = rangeStart_; i <= rangeEnd_; ++i) {
        if (juce::MidiMessage::isMidiNoteBlack(i)) continue;
        
        ::SkRect keyRect = ::SkRect::MakeXYWH((float)currentX, 0.0f, (float)(whiteKeyWidth - 1), (float)h);
        
        if (state_.isNoteOn(1, i)) {
            // Glow effect
            canvas->drawRect(keyRect, activeKeyPaint);
        } else {
            canvas->drawRect(keyRect, whiteKeyPaint);
        }
        
        currentX += whiteKeyWidth;
    }
    
    // Draw Black Keys on top
    currentX = 0;
    for (int i = rangeStart_; i <= rangeEnd_; ++i) {
        if (!juce::MidiMessage::isMidiNoteBlack(i)) {
            currentX += whiteKeyWidth;
            continue;
        }
        
        // Black key is centered on the line between white keys
        // But we need to account for the previous white key
        float blackKeyX = currentX - (blackKeyWidth * 0.5f);
        ::SkRect keyRect = ::SkRect::MakeXYWH((float)blackKeyX, 0.0f, (float)blackKeyWidth, (float)blackKeyHeight);
        
        if (state_.isNoteOn(1, i)) {
            canvas->drawRect(keyRect, activeKeyPaint);
        } else {
            canvas->drawRect(keyRect, blackKeyPaint);
        }
    }
}

void PianoKeyboardViewSkia::mouseDown(const juce::MouseEvent& e)
{
    int note = getNoteAtPosition(e.position);
    if (note >= 0) {
        state_.noteOn(1, note, 1.0f);
    }
}

void PianoKeyboardViewSkia::mouseDrag(const juce::MouseEvent& e)
{
    int note = getNoteAtPosition(e.position);
    // Simple logic: if note changed, turn off old, turn on new
    // For now, just trigger new notes (monophonic drag style for simplicity)
    if (note >= 0 && !state_.isNoteOn(1, note)) {
        state_.noteOn(1, note, 1.0f);
    }
}

void PianoKeyboardViewSkia::mouseUp(const juce::MouseEvent& e)
{
    state_.allNotesOff(1);
}

void PianoKeyboardViewSkia::handleNoteOn(juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity)
{
    repaint();
}

void PianoKeyboardViewSkia::handleNoteOff(juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity)
{
    repaint();
}

int PianoKeyboardViewSkia::getNoteAtPosition(juce::Point<float> pos)
{
    // Simplified hit testing
    float w = getWidth();
    int numWhiteKeys = 0;
    for (int i = rangeStart_; i <= rangeEnd_; ++i) {
        if (!juce::MidiMessage::isMidiNoteBlack(i)) numWhiteKeys++;
    }
    float whiteKeyWidth = w / (float)numWhiteKeys;
    
    // Check black keys first (they are on top)
    // ... (omitted for brevity in this rapid implementation, implementing simple white key mapping)
    
    int whiteKeyIndex = (int)(pos.x / whiteKeyWidth);
    
    // Map index back to note
    int currentWhite = 0;
    for (int i = rangeStart_; i <= rangeEnd_; ++i) {
        if (!juce::MidiMessage::isMidiNoteBlack(i)) {
            if (currentWhite == whiteKeyIndex) return i;
            currentWhite++;
        }
    }
    
    return -1;
}

#endif // ZENITH_USE_SKIA

} // namespace zenith
