/**
 * @file ZenithKnob.h
 * @brief Stub for ZenithKnob - custom rotary control
 *
 * Temporary stub - wraps standard JUCE Slider
 */

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace zenith {

class ZenithKnob : public juce::Slider
{
public:
    ZenithKnob() : juce::Slider()
    {
        setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    }

    ZenithKnob(juce::Slider::SliderStyle style, juce::Slider::TextEntryBoxPosition textBoxPos)
        : juce::Slider(style, textBoxPos)
    {
    }

    void setLabel(const juce::String& label)
    {
        labelText_ = label;
        setName(label);
    }

    juce::String getLabel() const { return labelText_; }

    std::function<void(float)> onChange;

protected:
    void valueChanged() override
    {
        if (onChange)
            onChange(static_cast<float>(getValue()));
    }

private:
    juce::String labelText_;
};

} // namespace zenith
