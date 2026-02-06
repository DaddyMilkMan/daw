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

#include "../design-system/ZenithDesignSystem.h"
#include "PianoRollComponent.h"
#include <JuceHeader.h>

namespace zenith {

class QuantizeSettingsComponent : public SkiaComponent {
public:
    QuantizeSettingsComponent(PianoRollComponent &owner,
                            PianoRollComponent::QuantizeOptions &options);

    void drawSkia(SkCanvas *canvas) override;

    void mouseMove(const juce::MouseEvent &e) override;
    void mouseDown(const juce::MouseEvent &e) override;
    void mouseDrag(const juce::MouseEvent &e) override;
    void mouseUp(const juce::MouseEvent &e) override;
    void mouseExit(const juce::MouseEvent &e) override;

private:
    PianoRollComponent &owner_;
    PianoRollComponent::QuantizeOptions &options_;

    // Control bounds
    juce::Rectangle<float> gridDropdownBounds;
    juce::Rectangle<float> tripletsBounds;
    juce::Rectangle<float> swingSliderBounds;
    juce::Rectangle<float> strengthSliderBounds;
    juce::Rectangle<float> startToggleBounds;
    juce::Rectangle<float> endToggleBounds;
    juce::Rectangle<float> applyButtonBounds;

    // State
    std::vector<juce::String> gridOptions;
    std::vector<double> gridValues;
    int selectedGridIndex = 0;
    bool gridDropdownOpen = false;
    bool gridDropdownHovered = false;
    bool tripletsHovered = false;
    bool swingHovered = false;
    bool swingDragging = false;
    bool strengthHovered = false;
    bool strengthDragging = false;
    bool startHovered = false;
    bool endHovered = false;
    bool applyHovered = false;
    int hoveredDropdownIndex = -1;

    // Drawing helpers
    void drawLabel(SkCanvas *canvas, const char *text, float x, float y);
    void drawSlider(SkCanvas *canvas, const juce::Rectangle<float> &bounds,
                    float value, bool dragging, bool hovered);
    void drawToggleButton(SkCanvas *canvas, const juce::Rectangle<float> &bounds,
                        const char *label, bool active, bool hovered);
    void drawDropdown(SkCanvas *canvas, const juce::Rectangle<float> &bounds,
                    const juce::String &text, bool open, bool hovered);
    void drawDropdownMenu(SkCanvas *canvas,
                        const juce::Rectangle<float> &bounds);
    void drawButton(SkCanvas *canvas, const juce::Rectangle<float> &bounds,
                    const char *label, bool hovered);
    void updateSliderValue(const juce::Rectangle<float> &bounds, float mouseX,
                         float &value);
};

} // namespace zenith
