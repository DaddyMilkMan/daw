/**
 * @file ExportDialogComponent.h
 * @brief Professional export dialog with format and quality selection
 *
 * Features:
 * - Format selector (WAV, FLAC, MP3, AAC, OGG)
 * - Quality settings (bitrate, sample rate, bit depth)
 * - Export region selector (entire project or loop region)
 * - File browser and filename input
 * - Real-time duration preview
 * - File size estimation
 * - Progress bar during export
 * - Export presets (podcast, streaming, CD quality, etc.)
 * - Normalize audio option
 * - Metadata editor (title, artist, album)
 * - Beautiful modal dialog with smooth animations
 * - Cancel/Export buttons with state feedback
 *
 * @phase Phase 1: Export
 */

#pragma once

#include <JuceHeader.h>

class Engine;
class ProjectState;

namespace zenith {

/**
 * @class ExportDialogComponent
 * @brief Professional export dialog
 */
class ExportDialogComponent : public juce::Component,
                             public juce::Button::Listener,
                             public juce::ComboBox::Listener,
                             public juce::Timer
{
public:
    //==========================================================================
    ExportDialogComponent(Engine& eng, ProjectState& state);
    ~ExportDialogComponent() override;

    //==========================================================================
    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    //==========================================================================
    // Listeners
    //==========================================================================
    void buttonClicked(juce::Button* button) override;
    void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override;

    //==========================================================================
    // Methods
    //==========================================================================

    /**
     * @brief Show export dialog as modal
     */
    void showModal();

    /**
     * @brief Close export dialog
     */
    void closeDialog();

    /**
     * @brief Get export in progress state
     */
    bool isExporting() const { return exporting_; }

private:
    //==========================================================================
    // Helpers
    //==========================================================================

    /**
     * @brief Paint format option card
     */
    void paintFormatCard(juce::Graphics& g, const juce::String& format,
                        const juce::Rectangle<int>& bounds, bool selected);

    /**
     * @brief Handle export button click
     */
    void handleExport();

    /**
     * @brief Handle format selection
     */
    void handleFormatSelected(const juce::String& format);

    /**
     * @brief Update file size estimation
     */
    void updateFileSizeEstimation();

    /**
     * @brief Update quality settings for selected preset
     */
    void updateQualityForPreset(const juce::String& preset);

    //==========================================================================
    // Members
    //==========================================================================

    Engine& engine_;
    ProjectState& projectState_;

    // Format selector (WAV, FLAC, MP3, AAC, OGG)
    juce::ComboBox formatSelector_;
    juce::String selectedFormat_ = "WAV";

    // Preset selector
    juce::ComboBox presetSelector_;

    // Quality settings
    juce::ComboBox bitDepthSelector_;  // 16, 24, 32 bit
    juce::ComboBox sampleRateSelector_;  // 44.1, 48, 96, 192 kHz
    juce::ComboBox bitrateSelector_;  // For MP3, AAC, OGG

    // Export region
    juce::ComboBox regionSelector_;  // Entire project, Loop region

    // File chooser
    juce::TextEditor filePathEditor_;
    juce::TextButton browseButton_;

    // Metadata
    juce::TextEditor titleEditor_;
    juce::TextEditor artistEditor_;
    juce::TextEditor albumEditor_;

    // Options
    juce::ToggleButton normalizeButton_;
    juce::ToggleButton dithersButton_;

    // Progress
    juce::ProgressBar progressBar_;
    juce::Label progressLabel_;

    // Info labels
    juce::Label fileSizeLabel_;
    juce::Label durationLabel_;

    // Buttons
    juce::TextButton exportButton_;
    juce::TextButton cancelButton_;

    // State
    bool exporting_ = false;
    float exportProgress_ = 0.0f;
    juce::String exportStatus_ = "";

    // Layout
    static constexpr int CARD_HEIGHT = 80;
    static constexpr int SPACING = 16;
    static constexpr int PADDING = 20;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ExportDialogComponent)
};

}  // namespace zenith

