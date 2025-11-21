/**
 * @file LoopEditorComponent.h
 * @brief Loop region editor with visual drag handles
 *
 * Features:
 * - Visual loop markers on timeline
 * - Click-drag handles to adjust loop start/end
 * - Time input fields for precise loop times
 * - Loop enable/disable toggle
 * - Loop length display
 * - Snap-to-grid toggle with grid size selector
 * - Visual feedback during dragging
 * - Animated loop region highlight
 * - Beat/bar display for musical alignment
 * - Auto-extend loop option
 * - Keyboard shortcuts for loop adjust
 * - Apple-inspired design with smooth animations
 *
 * @phase Phase 3: Loop Control
 */

#pragma once

#include <JuceHeader.h>

class Engine;
class ProjectState;

namespace zenith {

/**
 * @class LoopEditorComponent
 * @brief Visual loop region editor with animations
 */
class LoopEditorComponent : public juce::Component,
                           public juce::Slider::Listener,
                           public juce::Button::Listener,
                           public juce::Timer
{
public:
    //==========================================================================
    LoopEditorComponent(Engine& eng, ProjectState& state);
    ~LoopEditorComponent() override;

    //==========================================================================
    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    //==========================================================================
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

    //==========================================================================
    // Listeners
    //==========================================================================
    void sliderValueChanged(juce::Slider* slider) override;
    void buttonClicked(juce::Button* button) override;

    //==========================================================================
    // Methods
    //==========================================================================

    /**
     * @brief Set loop region in samples
     */
    void setLoopRegion(juce::int64 startSamples, juce::int64 endSamples) [[maybe_unused]];

    /**
     * @brief Get loop start position in samples
     */
    juce::int64 getLoopStart() const;

    /**
     * @brief Get loop end position in samples
     */
    juce::int64 getLoopEnd() const;

private:
    //==========================================================================
    // Helpers
    //==========================================================================

    /**
     * @brief Paint loop region indicator
     */
    void paintLoopRegion(juce::Graphics& g, const juce::Rectangle<int>& bounds);

    /**
     * @brief Paint drag handles
     */
    void paintDragHandles(juce::Graphics& g, const juce::Rectangle<int>& bounds);

    /**
     * @brief Convert samples to pixel position
     */
    int samplesToPixels(juce::int64 samples) const;

    /**
     * @brief Convert pixel position to samples
     */
    juce::int64 pixelsToSamples(int pixels) const;

    /**
     * @brief Get loop start handle bounds
     */
    juce::Rectangle<int> getStartHandleBounds() const;

    /**
     * @brief Get loop end handle bounds
     */
    juce::Rectangle<int> getEndHandleBounds() const;

    /**
     * @brief Update time display fields
     */
    void updateTimeFields();

    //==========================================================================
    // Members
    //==========================================================================

    Engine& engine_;
    ProjectState& projectState_;

    // Loop region in samples
    juce::int64 loopStartSamples_ = 0;
    juce::int64 loopEndSamples_ = 44100 * 4;  // 4 seconds default

    // Loop enabled state
    bool loopEnabled_ = false;

    // Loop enable/disable button
    juce::ToggleButton loopButton_;

    // Time input fields
    juce::TextEditor loopStartEditor_;
    juce::TextEditor loopEndEditor_;
    juce::Label lengthLabel_;

    // Snap to grid
    juce::ToggleButton snapToGridButton_;
    juce::ComboBox gridSizeSelector_;

    // Dragging state
    enum DragMode { None, DraggingStart, DraggingEnd, DraggingRegion };
    DragMode dragMode_ = None;
    juce::int64 dragStartSamples_ = 0;

    // Animation
    float loopAlpha_ = 0.3f;
    float handleScale_ = 1.0f;

    // Layout
    static constexpr int HANDLE_WIDTH = 12;
    static constexpr int HANDLE_HEIGHT = 40;
    static constexpr int SPACING = 8;
    static constexpr int PADDING = 12;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LoopEditorComponent)
};

}  // namespace zenith

