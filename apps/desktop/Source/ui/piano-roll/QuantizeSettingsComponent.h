/*
  ==============================================================================
    QuantizeSettingsComponent.h
    Created: 7 Dec 2025
    Description: Pure Skia-rendered UI for configuring quantization settings.
  ==============================================================================
*/

#pragma once
#include "../framework/SkiaComponent.h"
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
