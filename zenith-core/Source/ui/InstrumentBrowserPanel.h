/**
 * @file InstrumentBrowserPanel.h
 * @brief Instrument & Preset Browser UI component
 *
 * Provides:
 * - List of all registered instruments from InstrumentRegistry
 * - Preset browser for selected instrument (via ZenithPresetManager)
 * - Search box for filtering by name/tag
 * - Quick tag chips for common categories
 * - Assign to track functionality
 */

#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_events/juce_events.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_data_structures/juce_data_structures.h>


#include "../../include/ProjectState.h"
#include "../engine/Track.h"
#include "../instruments/InstrumentPreset.h"
#include "../instruments/InstrumentRegistry.h"

#ifdef ZENITH_USE_SKIA
    #include "skia/SkiaLabel.h"
    #include "skia/SkiaButtonNative.h"
    #include "skia/SkiaTextInput.h"
    #include "skia/SkiaListBox.h"
#endif

// Forward declarations
class Engine;

namespace zenith {

//==============================================================================
/**
 * @brief Instrument & Preset Browser Panel
 *
 * Sidebar component that allows users to:
 * 1. Browse all registered instruments
 * 2. View presets for selected instrument
 * 3. Search and filter presets by name/tag
 * 4. Assign presets to instrument tracks
 *
 * Features Apple-inspired design with gradients, rounded corners, and smooth
 * animations.
 */
class InstrumentBrowserPanel : public juce::Component,
                               public juce::Timer
#ifndef ZENITH_USE_SKIA
                               , private juce::TextEditor::Listener
#endif
{
public:
  //==========================================================================
  /**
   * @brief Construct instrument browser panel
   * @param engine Engine reference for accessing tracks
   * @param projectState Project state for accessing tracks
   */
  InstrumentBrowserPanel(Engine &engine, ProjectState &projectState);
  ~InstrumentBrowserPanel() override;

  //==========================================================================
  // Component interface
  //==========================================================================

  void paint(juce::Graphics &g) override;
  void resized() override;
  void timerCallback() override;

  //==========================================================================
  // Visibility control
  //==========================================================================

  /**
   * @brief Toggle panel visibility
   */
  void toggleVisibility();

  /**
   * @brief Set panel visibility
   */
  void setPanelVisible(bool shouldBeVisible);

private:
  //==========================================================================
  // TextEditor::Listener interface (for search box)
  //==========================================================================

#ifndef ZENITH_USE_SKIA
  void textEditorTextChanged(juce::TextEditor &editor) override;
#endif

  //==========================================================================
  // Internal data structures
  //==========================================================================

  /**
   * @brief Preset item with all metadata for display
   */
  struct PresetItem {
    ZenithInstrumentPreset preset;
    bool matchesSearch = true;
    bool matchesTag = true;

    bool isVisible() const { return matchesSearch && matchesTag; }
  };

  //==========================================================================
  // UI Components
  //==========================================================================

#ifdef ZENITH_USE_SKIA
  // Skia components (GPU-accelerated)
  // Header
  zenith::SkiaLabel titleLabel;
  zenith::SkiaButtonNative toggleButton;

  // Search
  zenith::SkiaLabel searchLabel;
  zenith::SkiaTextInput searchBox;

  // Tag filter chips
  zenith::SkiaLabel tagsLabel;
  juce::Component tagChipsContainer;

  std::vector<std::unique_ptr<zenith::SkiaButtonNative>> tagChips;

  juce::String activeTag; // Empty = show all

  // Instrument list
  zenith::SkiaLabel instrumentsLabel;
  zenith::SkiaListBox instrumentList;

  // Preset list
  zenith::SkiaLabel presetsLabel;
  zenith::SkiaListBox presetList;

  zenith::SkiaButtonNative loadPresetButton;
  zenith::SkiaButtonNative searchClearButton;

  // Status/toast message
  zenith::SkiaLabel statusLabel;
#else
  // JUCE fallback
  juce::Label titleLabel;
  juce::TextButton toggleButton;

  juce::Label searchLabel;
  juce::TextEditor searchBox;

  juce::Label tagsLabel;
  juce::Component tagChipsContainer;

  std::vector<std::unique_ptr<juce::TextButton>> tagChips;

  juce::String activeTag;

  juce::Label instrumentsLabel;
  juce::ListBox instrumentList;

  juce::Label presetsLabel;
  juce::ListBox presetList;

  juce::TextButton loadPresetButton;
  juce::TextButton searchClearButton;

  juce::Label statusLabel;
#endif

  int statusLabelAlpha = 0; // For fade-out animation
  int statusHoldTicks = 0;  // Hold time before fading

  // Search bar enhancements
  float searchFocusAnimation = 0.0f;
  bool searchHasFocus = false;

  //==========================================================================
  // Data
  //==========================================================================

  Engine &engine_;
  ProjectState &projectState_;
  InstrumentRegistry &instrumentRegistry_;
  ZenithPresetManager presetManager_;

  juce::StringArray instrumentIds_;
  juce::String selectedInstrumentId_;
  std::vector<PresetItem> presetItems_;
  juce::StringArray
      visiblePresetIndices_; // Indices of visible presets after filtering

  //==========================================================================
  // ListBox models (JUCE only)
  //==========================================================================

#ifndef ZENITH_USE_SKIA
  /**
   * @brief ListBox model for instrument list
   */
  class InstrumentListBoxModel : public juce::ListBoxModel {
  public:
    InstrumentListBoxModel(InstrumentBrowserPanel &owner);

    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics &g, int width,
                          int height, bool rowIsSelected) override;
    void listBoxItemClicked(int row, const juce::MouseEvent &e) override;

  private:
    InstrumentBrowserPanel &owner_;
  };

  /**
   * @brief ListBox model for preset list
   */
  class PresetListBoxModel : public juce::ListBoxModel {
  public:
    PresetListBoxModel(InstrumentBrowserPanel &owner);

    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics &g, int width,
                          int height, bool rowIsSelected) override;
    void listBoxItemDoubleClicked(int row, const juce::MouseEvent &e) override;

  private:
    InstrumentBrowserPanel &owner_;
  };

  std::unique_ptr<InstrumentListBoxModel> instrumentListModel_;
  std::unique_ptr<PresetListBoxModel> presetListModel_;
#endif

  //==========================================================================
  // Internal methods
  //==========================================================================

  /**
   * @brief Initialize tag chips
   */
  void initializeTagChips();

  /**
   * @brief Handle instrument selection
   */
  void onInstrumentSelected(int instrumentIndex);

  /**
   * @brief Handle preset selection
   */
  void onPresetDoubleClicked(int presetIndex);

  /**
   * @brief Load selected preset to selected track
   */
  void loadPresetToSelectedTrack();

  /**
   * @brief Update search filter
   */
  void updateSearchFilter();

  /**
   * @brief Update tag filter
   */
  void updateTagFilter(const juce::String &tag);

  /**
   * @brief Apply filters and update visible preset list
   */
  void applyFilters();

  /**
   * @brief Show status message (toast)
   */
  void showStatus(const juce::String &message, bool isError = false);

  /**
   * @brief Update status fade-out animation
   */
  void updateStatusAnimation();

  /**
   * @brief Clear search box
   */
  void clearSearch();

  /**
   * @brief Get currently selected track
   */
  zenith::Track *getSelectedTrack() const;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InstrumentBrowserPanel)
};

} // namespace zenith
