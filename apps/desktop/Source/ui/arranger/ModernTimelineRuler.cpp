/**
 * @file ModernTimelineRuler.cpp
 * @brief Professional timeline ruler implementation
 * @author Fixed by Claude - December 2025
 */

#include "ModernTimelineRuler.h"
#include <cmath>

namespace zenith {

ModernTimelineRuler::ModernTimelineRuler() {
    setSize(800, 48);  // Height from spacing system
}

void ModernTimelineRuler::paint(juce::Graphics& g) {
    drawRulerBackground(g);
    drawLoopRegion(g);
    drawGridLines(g);
    drawTimeMarkers(g);
    drawPlayhead(g);
}

void ModernTimelineRuler::drawRulerBackground(juce::Graphics& g) {
    using namespace ZenithTheme;
    
    auto bounds = getLocalBounds().toFloat();
    
    // Background
    g.setColour(Colors::bg_02);
    g.fillRect(bounds);
    
    // Bottom border
    g.setColour(Colors::border_default);
    g.fillRect(bounds.removeFromBottom(1.0f));
}

void ModernTimelineRuler::drawGridLines(juce::Graphics& g) {
    using namespace ZenithTheme;
    
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
            g.setColour(Colors::border_strong);
            g.fillRect(x, 0, 2, height);
        } else {
            // Beat line (default)
            g.setColour(Colors::border_default);
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
            
            // Skip if this is a beat or bar line
            if (std::fmod(subdivision, subdivisionsPerBeat) < 0.01) continue;
            
            // Subtle subdivision line
            g.setColour(Colors::border_subtle);
            g.fillRect(x, height - 8, 1, 8);
        }
    }
}

void ModernTimelineRuler::drawTimeMarkers(juce::Graphics& g) {
    using namespace ZenithTheme;
    
    auto bounds = getLocalBounds();
    int width = bounds.getWidth();
    
    // Calculate visible beat range
    double startBeat = viewportStartBeat_;
    double endBeat = startBeat + (width / pixelsPerBeat_);
    
    int beatsPerBar = timeSignatureNumerator_;
    
    // Draw bar numbers
    g.setColour(Colors::text_secondary);
    g.setFont(Typography::getSmallFont(Typography::Weight::Medium));
    
    for (int beat = std::floor(startBeat); beat <= std::ceil(endBeat); ++beat) {
        // Only draw markers on bar lines
        if ((beat % beatsPerBar) != 0) continue;
        
        int x = beatToPixel(beat);
        
        if (x < 0 || x > width) continue;
        
        int barNumber = (beat / beatsPerBar) + 1;
        juce::String markerText = formatTime(beat);
        
        auto textBounds = juce::Rectangle<int>(x + 4, 4, 100, 20);
        g.drawText(markerText, textBounds, juce::Justification::left, false);
    }
}

void ModernTimelineRuler::drawPlayhead(juce::Graphics& g) {
    using namespace ZenithTheme;
    
    int x = beatToPixel(playheadBeat_);
    auto bounds = getLocalBounds();
    
    if (x < 0 || x > bounds.getWidth()) return;
    
    // Playhead line
    g.setColour(Colors::playhead);
    g.fillRect(x - 1, 0, 3, bounds.getHeight());
    
    // Playhead handle (triangle at top)
    juce::Path handle;
    handle.addTriangle(x - 6.0f, 0.0f, x + 6.0f, 0.0f, x, 8.0f);
    
    g.setColour(Colors::playhead);
    g.fillPath(handle);
    
    // Glow effect
    g.setColour(Colors::playhead.withAlpha(0.3f));
    g.fillRect(x - 2, 0, 5, bounds.getHeight());
}

void ModernTimelineRuler::drawLoopRegion(juce::Graphics& g) {
    if (!loopEnabled_) return;
    
    using namespace ZenithTheme;
    
    int startX = beatToPixel(loopStartBeat_);
    int endX = beatToPixel(loopEndBeat_);
    auto bounds = getLocalBounds();
    
    // Clamp to visible area
    startX = std::max(0, std::min(startX, bounds.getWidth()));
    endX = std::max(0, std::min(endX, bounds.getWidth()));
    
    if (startX >= endX) return;
    
    // Loop region highlight
    auto loopBounds = juce::Rectangle<float>(startX, 0, endX - startX, bounds.getHeight());
    
    g.setColour(Colors::accent_subtle);
    g.fillRect(loopBounds);
    
    // Loop boundaries
    g.setColour(Colors::accent_primary);
    g.fillRect(startX - 1, 0, 2, bounds.getHeight());
    g.fillRect(endX - 1, 0, 2, bounds.getHeight());
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

//==============================================================================
// Timeline Control
//==============================================================================

void ModernTimelineRuler::setPixelsPerBeat(double ppb) {
    pixelsPerBeat_ = juce::jlimit(10.0, 200.0, ppb);
    repaint();
}

void ModernTimelineRuler::setViewportStartBeat(double beat) {
    viewportStartBeat_ = std::max(0.0, beat);
    repaint();
}

void ModernTimelineRuler::setTimeSignature(int numerator, int denominator) {
    timeSignatureNumerator_ = numerator;
    timeSignatureDenominator_ = denominator;
    repaint();
}

void ModernTimelineRuler::setTempo(double bpm) {
    tempo_ = bpm;
    repaint();
}

void ModernTimelineRuler::setSampleRate(double sampleRate) {
    if (sampleRate > 0.0) {
        sampleRate_ = sampleRate;
        repaint();
    }
}

void ModernTimelineRuler::setFrameRate(double fps) {
    if (fps > 0.0) {
        fps_ = fps;
        repaint();
    }
}

void ModernTimelineRuler::setPlayheadPosition(double beat) {
    playheadBeat_ = std::max(0.0, beat);
    repaint();
}

void ModernTimelineRuler::setLoopRegion(double startBeat, double endBeat) {
    loopEnabled_ = true;
    loopStartBeat_ = startBeat;
    loopEndBeat_ = endBeat;
    repaint();
}

void ModernTimelineRuler::clearLoopRegion() {
    loopEnabled_ = false;
    repaint();
}

void ModernTimelineRuler::setTimeFormat(TimeFormat format) {
    timeFormat_ = format;
    repaint();
}

//==============================================================================
// Helper Functions
//==============================================================================

juce::String ModernTimelineRuler::formatTime(double beat) {
    int beatsPerBar = timeSignatureNumerator_;
    
    switch (timeFormat_) {
        case TimeFormat::Bars: {
            int bar = static_cast<int>(beat / beatsPerBar) + 1;
            int beatInBar = static_cast<int>(std::fmod(beat, beatsPerBar)) + 1;
            int tick = static_cast<int>((beat - std::floor(beat)) * 480.0);  // 480 ticks per beat
            return juce::String(bar) + "." + juce::String(beatInBar) + "." + juce::String(tick);
        }
        
        case TimeFormat::Time: {
            double seconds = (beat / tempo_) * 60.0;
            int minutes = static_cast<int>(seconds / 60.0);
            int secs = static_cast<int>(std::fmod(seconds, 60.0));
            int millis = static_cast<int>((seconds - std::floor(seconds)) * 1000.0);
            
            juce::String timeStr;
            timeStr << juce::String(minutes).paddedLeft('0', 2) << ":"
                   << juce::String(secs).paddedLeft('0', 2) << ":"
                   << juce::String(millis).paddedLeft('0', 3);
            return timeStr;
        }
        
        case TimeFormat::Samples: {
            int samples = static_cast<int>(beat * (60.0 / tempo_) * sampleRate_);
            return juce::String(samples);
        }
        
        case TimeFormat::Frames: {
            int frames = static_cast<int>(beat * (60.0 / tempo_) * fps_);
            return juce::String(frames);
        }
        
        default:
            return juce::String(static_cast<int>(beat / beatsPerBar) + 1);
    }
}

int ModernTimelineRuler::beatToPixel(double beat) {
    return static_cast<int>((beat - viewportStartBeat_) * pixelsPerBeat_);
}

double ModernTimelineRuler::pixelToBeat(int pixel) {
    return viewportStartBeat_ + (pixel / pixelsPerBeat_);
}

} // namespace zenith

