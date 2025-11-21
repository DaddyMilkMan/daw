/**
 * @file ProjectSettingsComponent.h
 * @brief Project settings editor with Apple-like design
 *
 * Features:
 * - Edit project name
 * - Edit tempo (BPM) with instant feedback
 * - Edit time signature (numerator/denominator)
 * - Display sample rate (read-only)
 * - Smooth animations and transitions
 * - Real-time validation
 * - Apple-inspired flat design
 *
 * @phase Phase 0: Foundation
 */

#pragma once

#include <JuceHeader.h>

class ProjectState;

namespace zenith {

/**
 * @class ProjectSettingsComponent
 * @brief Project metadata editor with Apple-inspired modern UI design
 *
 * Features gradient backgrounds, validation feedback, stepper buttons,
 * tap tempo, and smooth 60Hz animations.
 */
class ProjectSettingsComponent : public juce::Component,
                                 public juce::Timer,
                                 public juce::TextEditor::Listener
{
public:
    //==========================================================================
    explicit ProjectSettingsComponent(ProjectState& state);
    ~ProjectSettingsComponent() override;

    //==========================================================================
    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    //==========================================================================
    // TextEditor::Listener
    //==========================================================================
    void textEditorTextChanged(juce::TextEditor& editor) override;
    void textEditorReturnKeyPressed(juce::TextEditor& editor) override;
    void textEditorEscapeKeyPressed(juce::TextEditor& editor) override;
    void textEditorFocusLost(juce::TextEditor& editor) override;

    //==========================================================================
    // Methods
    //==========================================================================

    /**
     * @brief Refresh UI to show current project state
     */
    void refresh();

private:
    //==========================================================================
    // Helpers
    //==========================================================================

    /**
     * @brief Create a styled text input field
     */
    void setupTextEditor(juce::TextEditor& editor, int maxLength = -1);

    /**
     * @brief Paint a settings row (label + value)
     */
    void paintSettingRow(juce::Graphics& g, const juce::String& label,
                        const juce::Rectangle<int>& bounds);

    /**
     * @brief Update tempo from text editor
     */
    void updateTempoFromText();

    /**
     * @brief Update time signature from text editors
     */
    void updateTimeSignatureFromText();

    /**
     * @brief Validate tempo value
     */
    float validateTempo(float value) const;

    /**
     * @brief Validate time signature values
     */
    void validateTimeSignature(int& num, int& den) const;

    /**
     * @brief Adjust tempo by delta
     */
    void adjustTempo(float delta) [[maybe_unused]];

    /**
     * @brief Adjust time signature numerator by delta
     */
    void adjustTimeSignatureNum(int delta) [[maybe_unused]];

    /**
     * @brief Adjust time signature denominator by delta
     */
    void adjustTimeSignatureDen(int delta) [[maybe_unused]];

    /**
     * @brief Handle tap tempo button click
     */
    void onTapTempo();

    /**
     * @brief Apply all changes
     */
    void applyChanges();

    /**
     * @brief Cancel all changes
     */
    void cancelChanges();

    //==========================================================================
    // Members
    //==========================================================================

    ProjectState& projectState_;

    // Project name editor
    juce::TextEditor projectNameEditor_;

    // Tempo editor
    juce::TextEditor tempoEditor_;

    // Time signature editors
    juce::TextEditor timeSignatureNumEditor_;
    juce::TextEditor timeSignatureDenEditor_;

    // Sample rate display (read-only)
    juce::Label sampleRateLabel_;
    juce::Label sampleRateValue_;

    // Stepper buttons
    juce::TextButton tempoUpButton_;
    juce::TextButton tempoDownButton_;
    juce::TextButton timeSignatureNumUpButton_;
    juce::TextButton timeSignatureNumDownButton_;
    juce::TextButton timeSignatureDenUpButton_;
    juce::TextButton timeSignatureDenDownButton_;

    // Tap tempo button
    juce::TextButton tapTempoButton_;
    std::vector<juce::int64> tapTimes_;
    juce::int64 lastTapTime_ = 0;

    // Action buttons
    juce::TextButton applyButton_;
    juce::TextButton cancelButton_;

    // Current tempo for validation
    double currentTempo_ = 120.0;
    int timeSignatureNum_ = 4;
    int timeSignatureDen_ = 4;

    // Validation state
    bool projectNameValid_ = true;
    bool tempoValid_ = true;
    bool timeSignatureValid_ = true;

    // Animation state
    float nameEditorFocusAnim_ = 0.0f;
    float tempoEditorFocusAnim_ = 0.0f;
    float timeSignatureNumFocusAnim_ = 0.0f;
    float timeSignatureDenFocusAnim_ = 0.0f;

    // Layout constants
    static constexpr int ROW_HEIGHT = 40;
    static constexpr int LABEL_WIDTH = 140;
    static constexpr int SPACING = 12;
    static constexpr int PADDING = 16;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProjectSettingsComponent)
};

}  // namespace zenith

