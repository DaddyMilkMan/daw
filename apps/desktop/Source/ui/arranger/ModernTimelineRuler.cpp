/**
 * @file ModernTimelineRuler.cpp
 * @brief Professional timeline ruler implementation
 */

#include "ModernTimelineRuler.h"
#include "ZenithDesignSystem.h"
#include <cmath>

namespace zenith {

// Helper to convert SkColor to juce::Colour
static juce::Colour skToJuce(SkColor sk) {
  return juce::Colour::fromRGBA(SkColorGetR(sk), SkColorGetG(sk),
                                SkColorGetB(sk), SkColorGetA(sk));
}

ModernTimelineRuler::ModernTimelineRuler() {
    setSize(800, 48);
}

void ModernTimelineRuler::paint(juce::Graphics& g) {
    drawRulerBackground(g);
    drawLoopRegion(g);
    drawGridLines(g);
    drawTimeMarkers(g);
    drawPlayhead(g);
}

void ModernTimelineRuler::drawRulerBackground(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat();
    
    // Background
    g.setColour(skToJuce(design::colors::BG_DARK));
    g.fillRect(bounds);
    
    // Bottom border
    g.setColour(skToJuce(design::colors::BORDER_DEFAULT));
    g.fillRect(bounds.removeFromBottom(1.0f));
}

void ModernTimelineRuler::drawGridLines(juce::Graphics& g) {
    auto bounds = getLocalBounds();
    int width = bounds.getWidth();
    int height = bounds.getHeight();
    
    // Calculate visible beat range
    double startBeat = viewportStartBeat_;
    double endBeat = startBeat + (width / pixelsPerBeat_);
    
    int beatsPerBar = timeSignatureNumerator_;
    
    // Draw beat lines
    for (int beat = std::floor(startBeat); beat <= std::ceil(endBeat); ++beat) {
        int x = beatToPixel(beat);
        
        if (x < 0 || x > width) continue;
        
        bool isBarLine = (beat % beatsPerBar) == 0;
        
        if (isBarLine) {
            // Bar line (strong)
            g.setColour(skToJuce(design::colors::BORDER_STRONG));
            g.fillRect(x, 0, 2, height);
        } else {
            // Beat line (default)
            g.setColour(skToJuce(design::colors::BORDER_DEFAULT));
            g.fillRect(x, 0, 1, height);
        }
    }
    
    // Draw subdivision lines (16th notes) if zoomed in enough
    if (pixelsPerBeat_ > 20.0) {
        double subdivisionsPerBeat = 4.0;  // 16th notes
        
        for (double subdivision = std::floor(startBeat * subdivisionsPerBeat); 
             subdivision <= std::ceil(endBeat * subdivisionsPerBeat); 
             ++subdivision) {
            
            double beat = subdivision / subdivisionsPerBeat;
            int x = beatToPixel(beat);
            
            if (x < 0 || x > width) continue;
            
            // Skip if this is on a beat line
            if (std::fmod(subdivision, subdivisionsPerBeat) == 0.0) continue;
            
            g.setColour(skToJuce(design::colors::BORDER_SUBTLE));
            g.fillRect(x, height - 8, 1, 8);
        }
    }
}

void ModernTimelineRuler::drawTimeMarkers(juce::Graphics& g) {
    auto bounds = getLocalBounds();
    int width = bounds.getWidth();
    
    double startBeat = viewportStartBeat_;
    double endBeat = startBeat + (width / pixelsPerBeat_);
    
    int beatsPerBar = timeSignatureNumerator_;
    
    g.setColour(skToJuce(design::colors::TEXT_SECONDARY));
    g.setFont(12.0f);
    
    // Draw bar numbers
    for (int beat = std::floor(startBeat); beat <= std::ceil(endBeat); ++beat) {
        if ((beat % beatsPerBar) != 0) continue;
        
        int x = beatToPixel(beat);
        if (x < 0 || x > width) continue;
        
        int barNumber = (beat / beatsPerBar) + 1;
        juce::String label = formatTimeDisplay(beat);
        
        juce::Rectangle<int> textBounds(x + 4, 4, 100, 20);
        g.drawText(label, textBounds, juce::Justification::left, false);
    }
}

void ModernTimelineRuler::drawPlayhead(juce::Graphics& g) {
    int x = beatToPixel(playheadBeat_);
    
    if (x < 0 || x > getWidth()) return;
    
    auto bounds = getLocalBounds();
    
    // Playhead line with glow
    g.setColour(skToJuce(design::colors::NEON_RED).withAlpha(0.5f));
    g.fillRect(x - 1, 0, 3, bounds.getHeight());
    
    // Core line
    g.setColour(skToJuce(design::colors::NEON_RED));
    g.fillRect(x, 0, 1, bounds.getHeight());
    
    // Triangle indicator at top
    juce::Path triangle;
    triangle.addTriangle(x - 6.0f, 0.0f, x + 6.0f, 0.0f, x, 8.0f);
    g.setColour(skToJuce(design::colors::NEON_RED));
    g.fillPath(triangle);
}

void ModernTimelineRuler::drawLoopRegion(juce::Graphics& g) {
    if (!loopEnabled_) return;
    
    int loopStartX = beatToPixel(loopStartBeat_);
    int loopEndX = beatToPixel(loopEndBeat_);
    
    if (loopEndX < 0 || loopStartX > getWidth()) return;
    
    auto bounds = getLocalBounds();
    juce::Rectangle<int> loopBounds(loopStartX, 0, loopEndX - loopStartX, bounds.getHeight());
    
    // Filled region
    g.setColour(skToJuce(design::colors::CYAN).withAlpha(0.1f));
    g.fillRect(loopBounds);
    
    // Loop markers
    g.setColour(skToJuce(design::colors::CYAN));
    g.fillRect(loopStartX, 0, 2, bounds.getHeight());
    g.fillRect(loopEndX - 2, 0, 2, bounds.getHeight());
}

void ModernTimelineRuler::mouseDown(const juce::MouseEvent& e) {
    double clickedBeat = pixelToBeat(e.x);
    
    if (onPlayheadMoved) {
        onPlayheadMoved(clickedBeat);
    }
}

void ModernTimelineRuler::mouseDrag(const juce::MouseEvent& e) {
    double draggedBeat = pixelToBeat(e.x);
    
    if (onPlayheadMoved) {
        onPlayheadMoved(draggedBeat);
    }
}

//==============================================================================
// Public API
//==============================================================================

void ModernTimelineRuler::setPixelsPerBeat(double ppb) {
    pixelsPerBeat_ = ppb;
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
