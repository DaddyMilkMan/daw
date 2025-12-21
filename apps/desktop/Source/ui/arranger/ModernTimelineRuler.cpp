/**
 * @file ModernTimelineRuler.cpp
 * @brief Professional timeline ruler implementation
 */

#include "ModernTimelineRuler.h"
#include "ZenithDesignSystem.h"
#include <cmath>

#include <core/SkCanvas.h>
#include <core/SkFont.h>
#include <core/SkPaint.h>
#include <core/SkPath.h>
#include <core/SkRect.h>
#include <core/SkTextBlob.h>

namespace zenith {

ModernTimelineRuler::ModernTimelineRuler() {
    setSize(800, 48);
}

void ModernTimelineRuler::drawSkia(SkCanvas* canvas) {
    auto bounds = getLocalBounds();
    float width = (float)bounds.getWidth();
    float height = (float)bounds.getHeight();
    
    using namespace design;
    
    // Background
    canvas->clear(colors::BG_DARK);
    
    // Bottom border
    SkPaint borderPaint;
    borderPaint.setColor(colors::BORDER_DEFAULT);
    canvas->drawRect(SkRect::MakeXYWH(0, height - 1.0f, width, 1.0f), borderPaint);
    
    // Loop Region
    if (loopEnabled_) {
        float loopStartX = (float)beatToPixel(loopStartBeat_);
        float loopEndX = (float)beatToPixel(loopEndBeat_);
        
        if (loopEndX >= 0 && loopStartX <= width) {
            SkPaint loopBgPaint;
            loopBgPaint.setColor(colors::CYAN);
            loopBgPaint.setAlphaf(0.1f);
            canvas->drawRect(SkRect::MakeXYWH(loopStartX, 0, loopEndX - loopStartX, height), loopBgPaint);
            
            SkPaint loopMarkerPaint;
            loopMarkerPaint.setColor(colors::CYAN);
            canvas->drawRect(SkRect::MakeXYWH(loopStartX, 0, 2.0f, height), loopMarkerPaint);
            canvas->drawRect(SkRect::MakeXYWH(loopEndX - 2.0f, 0, 2.0f, height), loopMarkerPaint);
        }
    }
    
    // Grid Lines
    double startBeat = viewportStartBeat_;
    double endBeat = startBeat + (width / pixelsPerBeat_);
    int beatsPerBar = timeSignatureNumerator_;
    
    SkPaint linePaint;
    linePaint.setAntiAlias(true);
    
    for (int beat = (int)std::floor(startBeat); beat <= (int)std::ceil(endBeat); ++beat) {
        float x = (float)beatToPixel(beat);
        if (x < 0 || x > width) continue;
        
        bool isBarLine = (beat % beatsPerBar) == 0;
        if (isBarLine) {
            linePaint.setColor(colors::BORDER_STRONG);
            canvas->drawRect(SkRect::MakeXYWH(x, 0, 2.0f, height), linePaint);
            
            // Bar numbers
            SkPaint textPaint;
            textPaint.setColor(colors::TEXT_SECONDARY);
            textPaint.setAntiAlias(true);
            SkFont font = typography::getSkFont(typography::FONT_SM);
            juce::String label = formatTimeDisplay(beat);
            canvas->drawString(label.toRawUTF8(), x + 4, 16.0f, font, textPaint);
        } else {
            linePaint.setColor(colors::BORDER_DEFAULT);
            canvas->drawRect(SkRect::MakeXYWH(x, 0, 1.0f, height), linePaint);
        }
    }
    
    // Subdivision lines
    if (pixelsPerBeat_ > 20.0) {
        linePaint.setColor(colors::BORDER_SUBTLE);
        double subdivisionsPerBeat = 4.0;
        for (double subdivision = std::floor(startBeat * subdivisionsPerBeat); 
             subdivision <= std::ceil(endBeat * subdivisionsPerBeat); 
             ++subdivision) {
            double beat = subdivision / subdivisionsPerBeat;
            float x = (float)beatToPixel(beat);
            if (x < 0 || x > width) continue;
            if (std::fmod(subdivision, subdivisionsPerBeat) == 0.0) continue;
            canvas->drawRect(SkRect::MakeXYWH(x, height - 8.0f, 1.0f, 8.0f), linePaint);
        }
    }
    
    // Playhead
    float playheadX = (float)beatToPixel(playheadBeat_);
    if (playheadX >= 0 && playheadX <= width) {
        SkPaint phGlowPaint;
        phGlowPaint.setColor(colors::NEON_RED);
        phGlowPaint.setAlphaf(0.5f);
        canvas->drawRect(SkRect::MakeXYWH(playheadX - 1.0f, 0, 3.0f, height), phGlowPaint);
        
        SkPaint phPaint;
        phPaint.setColor(colors::NEON_RED);
        canvas->drawRect(SkRect::MakeXYWH(playheadX, 0, 1.0f, height), phPaint);
        
        SkPath triangle;
        triangle.moveTo(playheadX - 6.0f, 0.0f);
        triangle.lineTo(playheadX + 6.0f, 0.0f);
        triangle.lineTo(playheadX, 8.0f);
        triangle.close();
        canvas->drawPath(triangle, phPaint);
    }
}



void ModernTimelineRuler::mouseDown(const juce::MouseEvent& e) {
    double beat = pixelToBeat(e.x);

    // Check if clicking near playhead (within 8px)
    int playheadX = beatToPixel(playheadBeat_);
    if (std::abs(e.x - playheadX) < 8) {
        isDraggingPlayhead_ = true;
    } else {
        // Jump playhead to click position
        setPlayheadPosition(beat);
        if (onPlayheadMoved) onPlayheadMoved(beat);
    }
}

void ModernTimelineRuler::mouseDrag(const juce::MouseEvent& e) {
    if (isDraggingPlayhead_) {
        double beat = pixelToBeat(e.x);
        setPlayheadPosition(beat);
        if (onPlayheadMoved) onPlayheadMoved(beat);
    }
}

void ModernTimelineRuler::mouseUp(const juce::MouseEvent& e) {
    juce::ignoreUnused(e);
    isDraggingPlayhead_ = false;
}

//==============================================================================
// Public API
//==============================================================================

void ModernTimelineRuler::setPixelsPerBeat(double ppb) {
    pixelsPerBeat_ = juce::jlimit(10.0, 200.0, ppb);
    repaint();
}

void ModernTimelineRuler::setViewportStartBeat(double beat) {
    viewportStartBeat_ = beat;
    repaint();
}

void ModernTimelineRuler::setPlayheadPosition(double beat) {
    playheadBeat_ = beat;
    repaint();
}

void ModernTimelineRuler::setTimeSignature(int numerator, int denominator) {
    timeSignatureNumerator_ = numerator;
    timeSignatureDenominator_ = denominator;
    repaint();
}

void ModernTimelineRuler::setTempo(double bpm) {
    tempo_ = bpm;
}

void ModernTimelineRuler::setLoopRegion(bool enabled, double startBeat, double endBeat) {
    loopEnabled_ = enabled;
    loopStartBeat_ = startBeat;
    loopEndBeat_ = endBeat;
    repaint();
}

void ModernTimelineRuler::setTimeFormat(TimeFormat format) {
    timeFormat_ = format;
    repaint();
}

//==============================================================================
// Private Helpers
//==============================================================================

int ModernTimelineRuler::beatToPixel(double beat) const {
    return static_cast<int>((beat - viewportStartBeat_) * pixelsPerBeat_);
}

double ModernTimelineRuler::pixelToBeat(int pixel) const {
    return viewportStartBeat_ + (pixel / pixelsPerBeat_);
}

juce::String ModernTimelineRuler::formatTimeDisplay(double beat) const {
    int beatsPerBar = timeSignatureNumerator_;
    
    switch (timeFormat_) {
    case TimeFormat::Bars: {
        int barNumber = static_cast<int>(beat / beatsPerBar) + 1;
        int beatInBar = static_cast<int>(beat) % beatsPerBar + 1;
        return juce::String(barNumber) + "." + juce::String(beatInBar);
    }
    
    case TimeFormat::Time: {
        double seconds = beat * 60.0 / tempo_;
        int minutes = static_cast<int>(seconds / 60.0);
        int secs = static_cast<int>(seconds) % 60;
        int ms = static_cast<int>((seconds - std::floor(seconds)) * 1000.0);
        return juce::String::formatted("%d:%02d.%03d", minutes, secs, ms);
    }
    
    case TimeFormat::Samples: {
        int sampleRate = 44100; // Default, should be configurable
        juce::int64 samples = static_cast<juce::int64>(beat * 60.0 / tempo_ * sampleRate);
        return juce::String(samples);
    }
    
    case TimeFormat::Frames: {
        double seconds = beat * 60.0 / tempo_;
        int frames = static_cast<int>(seconds * 30.0); // 30 fps default
        return juce::String(frames) + "f";
    }
    
    default:
        return juce::String(static_cast<int>(beat));
    }
}

} // namespace zenith
