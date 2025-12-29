/*
  ==============================================================================

    PianoKeyboardViewSkia.cpp
    Created: 2025-11-28
    Author:  Leo Rossi
    Refactored: 2025-11-30 for robustness

  ==============================================================================
*/

#include "PianoKeyboardViewSkia.h"

#ifdef ZENITH_USE_SKIA
#include "ZenithSkia.h"
#include <core/SkPath.h>
#include <core/SkPoint.h>
#include <core/SkTileMode.h>
#include <effects/SkGradientShader.h>
#endif

namespace zenith {

#ifdef ZENITH_USE_SKIA

PianoKeyboardViewSkia::PianoKeyboardViewSkia(juce::MidiKeyboardState& state, 
                                             juce::MidiKeyboardComponent::Orientation orientation)
    : state_(state), orientation_(orientation)
{
    // Register as a listener to the keyboard state to receive MIDI events
    state_.addListener(this);
}

PianoKeyboardViewSkia::~PianoKeyboardViewSkia()
{
    state_.removeListener(this);
}

void PianoKeyboardViewSkia::drawSkia(SkCanvas* canvas)
{
    // --- Setup Paints ---

    SkPaint whiteKeyPaint;
    whiteKeyPaint.setAntiAlias(true);
    // Gradient for white keys (top to bottom)
    SkPoint pts[2] = { SkPoint::Make(0, 0), SkPoint::Make(0, (float)getHeight()) };
    SkColor colors[2] = { 0xFFEEEEEE, 0xFFCCCCCC }; // White to Light Grey
    whiteKeyPaint.setShader(SkGradientShader::MakeLinear(pts, colors, nullptr, 2, SkTileMode::kClamp));

    SkPaint blackKeyPaint;
    blackKeyPaint.setAntiAlias(true);
    // Gradient for black keys
    SkColor blackColors[2] = { 0xFF333333, 0xFF000000 }; // Dark Grey to Black
    blackKeyPaint.setShader(SkGradientShader::MakeLinear(pts, blackColors, nullptr, 2, SkTileMode::kClamp));
    
    SkPaint activeKeyPaint;
    activeKeyPaint.setAntiAlias(true);
    SkColor activeColors[2] = { 0xFF00FFFF, 0xFF0088AA }; // Cyan Gradient for active keys (Neon Noir)
    activeKeyPaint.setShader(SkGradientShader::MakeLinear(pts, activeColors, nullptr, 2, SkTileMode::kClamp));
    
    float w = (float)getWidth();
    float h = (float)getHeight();
    
    // --- Geometry Calculations ---

    // Calculate key width based on visible range
    int numWhiteKeys = 0;
    for (int i = rangeStart_; i <= rangeEnd_; ++i) {
        if (!juce::MidiMessage::isMidiNoteBlack(i)) numWhiteKeys++;
    }
    
    // Avoid division by zero
    if (numWhiteKeys == 0) numWhiteKeys = 1;

    float whiteKeyWidth = w / (float)numWhiteKeys;
    float blackKeyWidth = whiteKeyWidth * 0.6f;
    float blackKeyHeight = h * 0.6f;
    
    // --- Draw White Keys ---
    
    float currentX = 0;
    for (int i = rangeStart_; i <= rangeEnd_; ++i) {
        if (juce::MidiMessage::isMidiNoteBlack(i)) continue;
        
        SkRect keyRect = SkRect::MakeXYWH(currentX, 0, whiteKeyWidth - 1, h);
        
        if (state_.isNoteOn(1, i)) {
            // Glow effect for active key
            canvas->drawRect(keyRect, activeKeyPaint);
        } else {
            canvas->drawRect(keyRect, whiteKeyPaint);
        }
        
        currentX += whiteKeyWidth;
    }
    
    // --- Draw Black Keys (Overlay) ---
    
    currentX = 0; // Reset X walker
    for (int i = rangeStart_; i <= rangeEnd_; ++i) {
        if (!juce::MidiMessage::isMidiNoteBlack(i)) {
            currentX += whiteKeyWidth;
            continue;
        }
        
        // Black key is centered on the line between white keys
        // But we need to account for the previous white key position
        float blackKeyX = currentX - (blackKeyWidth * 0.5f);
        SkRect keyRect = SkRect::MakeXYWH(blackKeyX, 0, blackKeyWidth, blackKeyHeight);
        
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
    // Simple monophonic drag: if note changed, turn off old (not tracked here for simplicity), turn on new.
    // In a real implementation, we would track the 'last played note' to turn it off.
    // For this prototype, we just trigger the new note.
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
    // Trigger a repaint to update the visual state
    // repaint() is available because SkiaComponent inherits from juce::Component
    repaint();
}

void PianoKeyboardViewSkia::handleNoteOff(juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity)
{
    repaint();
}

int PianoKeyboardViewSkia::getNoteAtPosition(juce::Point<float> pos)
{
    float w = (float)getWidth();
    int numWhiteKeys = 0;
    for (int i = rangeStart_; i <= rangeEnd_; ++i) {
        if (!juce::MidiMessage::isMidiNoteBlack(i)) numWhiteKeys++;
    }
    if (numWhiteKeys == 0) return -1;

    float whiteKeyWidth = w / (float)numWhiteKeys;
    
    // Check black keys first (z-order top)
    float blackKeyWidth = whiteKeyWidth * 0.6f;
    float blackKeyHeight = (float)getHeight() * 0.6f;
    
    float currentX = 0;
    for (int i = rangeStart_; i <= rangeEnd_; ++i) {
        if (!juce::MidiMessage::isMidiNoteBlack(i)) {
            currentX += whiteKeyWidth;
            continue;
        }
        
        float blackKeyX = currentX - (blackKeyWidth * 0.5f);
        if (pos.x >= blackKeyX && pos.x <= blackKeyX + blackKeyWidth && pos.y <= blackKeyHeight) {
            return i;
        }
    }
    
    // Check white keys
    int whiteKeyIndex = (int)(pos.x / whiteKeyWidth);
    
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
