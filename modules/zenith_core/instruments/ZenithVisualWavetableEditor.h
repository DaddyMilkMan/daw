/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

*/

#pragma once

#include "WavetableData.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <functional>

namespace zenith {

//==============================================================================
// VISUAL WAVETABLE EDITOR
//==============================================================================
/**
 * Professional wavetable editor matching Serum 2:
 * - Real-time waveform visualization
 * - Drawing tools (line, freehand, smooth, morph)
 * - Processors (normalize, phase align, fade, symmetry)
 * - Harmonics editor (additive synthesis)
 * - Spectrum analyzer (FFT)
 * - Multiple tables with frame interpolation
 */
class ZenithVisualWavetableEditor : public juce::Component {
public:
    //==========================================================================
    // Listeners
    //==========================================================================

    class Listener {
    public:
        virtual ~Listener() = default;
        virtual void wavetableChanged() = 0;
        virtual void frameChanged(int frameIndex) = 0;
    };

    void addListener(Listener* l) { listeners_.add(l); }
    void removeListener(Listener* l) { listeners_.remove(l); }

    //==========================================================================
    // Configuration
    //==========================================================================

    void setWavetableData(const WavetableData* wt) { wavetableData_ = wt; }
    void setCurrentFrame(int index) { currentFrame_ = index; }

    //==========================================================================
    // JUCE Component
    //==========================================================================

    void paint(juce::Graphics& g) override;
    void resized() override;

    //==========================================================================
    // Mouse Events
    //==========================================================================

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

private:
    //==========================================================================
    // Drawing State
    //==========================================================================

    enum class DrawTool {
        Line,           ///< Straight line
        Freehand,       ///< Freehand drawing
        Smooth,         ///< Smooth/blur
        Symmetry,       ///< Mirror symmetry
        Morph           ///< Morph between frames
    };

    struct DrawState {
        bool drawing = false;
        float startX = 0.0f;
        bool symmetryMode = false;
        int symmetryAxis = 0;  // 0 = vertical, 1 = horizontal
    } drawState_;

    //==========================================================================
    // Visual Settings
    //==========================================================================

    struct VisualSettings {
        juce::Colour waveformColour = juce::Colours::green;
        juce::Colour gridColour = juce::Colours::white.withAlpha(0.3f);
        juce::Colour axisColour = juce::Colours::white.withAlpha(0.5f);
        juce::Colour zeroLineColour = juce::Colours::white.withAlpha(0.7f);
        float lineWidth = 2.0f;
        bool showGrid = true;
        bool showAxis = true;
    } visual_;

    //==========================================================================
    // Data
    //==========================================================================

    const WavetableData* wavetableData_ = nullptr;
    int currentFrame_ = 0;
    int hoveredFrame_ = -1;

    //==========================================================================
    // GUI Elements
    //==========================================================================

    juce::ListenerList<Listener*> listeners_;

    //==========================================================================
    // Layout
    //==========================================================================

    juce::Rectangle<int> getWaveformArea() const;
    juce::Rectangle<int> getFrameSelectorArea() const;
    juce::Rectangle<int> getToolbarArea() const;

    //==========================================================================
    // Internal Helpers
    //==========================================================================

    void drawWaveform(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void drawGrid(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void drawAxis(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void drawFrameSelector(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void drawToolbar(juce::Graphics& g, const juce::Rectangle<int>& bounds);

    float getWaveformValueAtPhase(double phase) const;
    void setWaveformValueAtPhase(double phase, float value);

    void notifyListeners() {
        for (auto* l : listeners_) {
            l->wavetableChanged();
        }
    }
};

} // namespace zenith
