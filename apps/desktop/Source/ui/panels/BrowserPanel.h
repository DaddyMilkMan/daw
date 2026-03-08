/*
  ==============================================================================

    BrowserPanel.h
    Created: 2025-11-28
    Refactored: 2025-12-05 for Universal Browser Model + Drag/Preview/Async

    Universal Media Browser View.
    Features:
    - Tree navigation with icons
    - Async background scanning
    - Audio preview with waveform
    - Drag-and-drop to tracks

  ==============================================================================
*/

#pragma once

#include "../framework/SkiaComponent.h"
#include "../controls/SkiaFileChooser.h"
#include "../../browser/BrowserPreviewEngine.h"
#include "../../browser/BrowserModel.h"
#include "../../commands/CommandAPI.h"
#include "BrowserWaveformLoader.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace zenith {

#ifdef ZENITH_USE_SKIA

/**
 * Main Universal Browser Panel.
 * Now a container for modular sub-components.
 */
class BrowserPanel : public SkiaComponent,
                     public juce::ChangeListener {
public:
  BrowserPanel(BrowserModel &model, CommandAPI &api);
  ~BrowserPanel() override;

  // SkiaComponent overrides
  void drawSkia(SkCanvas *canvas) override;
  void resized() override;
  void timerCallback() override;
  void mouseDown(const juce::MouseEvent &e) override;
  void mouseDrag(const juce::MouseEvent &e) override;
  void mouseMove(const juce::MouseEvent &e) override;
  void mouseWheelMove(const juce::MouseEvent &e, const juce::MouseWheelDetails &wheel) override;
  void mouseExit(const juce::MouseEvent &e) override;
  bool keyPressed(const juce::KeyPress &key) override;

  // ChangeListener override
  void changeListenerCallback(juce::ChangeBroadcaster *source) override;

  void setSearchText(const juce::String &text);
  BrowserPreviewEngine &getPreviewEngine() { return previewEngine_; }

  std::function<void(std::shared_ptr<BrowserItem>)> onItemDoubleClicked;

private:
  enum class Section {
    All,
    Instruments,
    Sounds,
    Effects,
    MIDI,
    Presets,
    Projects,
    Favorites,
    Recent
  };
  enum class SortMode {
    Name,
    Type,
    Recent
  };

  BrowserModel &model_;
  CommandAPI &api_;
  BrowserPreviewEngine previewEngine_;
  BrowserWaveformLoader waveformLoader_;
  std::vector<float> previewWaveform_;
  juce::String previewWaveformPath_;
  struct MidiNotePreview {
    float start = 0.0f;
    float duration = 0.0f;
    int note = 60;
  };
  std::vector<MidiNotePreview> previewMidiNotes_;
  float previewMidiDuration_ = 0.0f;
  bool instrumentPreviewPlaying_ = false;
  float instrumentPreviewPausedPos_ = 0.0f;
  juce::uint32 instrumentPreviewStartMs_ = 0;
  std::vector<std::pair<Section, juce::String>> sections_;
  std::vector<std::shared_ptr<BrowserItem>> visibleItems_;
  struct TreeRow {
    std::shared_ptr<BrowserItem> item;
    int depth = 0;
    bool isFolder = false;
    bool expanded = false;
  };
  std::vector<TreeRow> treeRows_;
  std::set<juce::String> expandedFolderIds_;
  std::vector<juce::Rectangle<int>> sectionItemBounds_;
  std::vector<juce::Rectangle<int>> rowBounds_;
  std::vector<juce::Rectangle<int>> disclosureBounds_;
  std::vector<juce::Rectangle<int>> rowPlayBtnBounds_;
  std::vector<juce::Rectangle<int>> sourceChipBounds_;

  juce::Rectangle<int> searchRect_;
  juce::Rectangle<int> searchInputRect_;
  juce::Rectangle<int> clearSearchRect_;
  juce::Rectangle<int> addSourceRect_;
  juce::Rectangle<int> removeSourceRect_;
  juce::Rectangle<int> sortRect_;
  juce::Rectangle<int> densityRect_;
  juce::Rectangle<int> sourceStripRect_;
  juce::Rectangle<int> sectionRailRect_;
  juce::Rectangle<int> resultsRect_;
  juce::Rectangle<int> listHeaderRect_;
  juce::Rectangle<int> listBodyRect_;
  juce::Rectangle<int> colNameRect_;
  juce::Rectangle<int> colTypeRect_;
  juce::Rectangle<int> colSourceRect_;
  juce::Rectangle<int> previewRect_;
  juce::Rectangle<int> autoPlayToggleRect_;
  juce::Rectangle<int> previewPlayRect_;
  juce::Rectangle<int> previewStopRect_;
  juce::Rectangle<int> previewVolumeRect_;

  Section activeSection_ = Section::All;
  juce::String searchText_;
  bool searchFocused_ = false;
  int selectedIndex_ = -1;
  int hoveredIndex_ = -1;
  int scrollOffset_ = 0;
  int selectedSourceIndex_ = -1;
  int rowHeight_ = 40;
  SortMode sortMode_ = SortMode::Name;
  bool compactDensity_ = false;
  bool draggingPreviewVolume_ = false;
  bool showPreviewToggle_ = true;
  juce::uint32 lastStopClickMs_ = 0;
  juce::File previewPrefsFile_;
  std::unique_ptr<SkiaFileChooser> folderChooser_;

  static constexpr int kHeaderH = 92;
  static constexpr int kSectionW = 72;
  static constexpr int kPreviewH = 120;
  static constexpr int kRowH = 40;

  void rebuildVisibleItems();
  bool sectionMatchesItem(Section section, const BrowserItem &item) const;
  bool itemMatchesSearch(const BrowserItem &item) const;
  void buildTreeRowsFromNode(const std::shared_ptr<BrowserItem>& node, int depth);
  bool nodeOrDescendantMatches(const std::shared_ptr<BrowserItem>& node) const;
  std::shared_ptr<BrowserItem> findNodeById(const std::shared_ptr<BrowserItem>& node,
                                            const juce::String& id) const;
  void selectItem(int index);
  void updatePreviewVolumeFromX(int x);
  void requestWaveformForFile(const juce::File& file);
  void loadMidiPreviewForFile(const juce::File& file);
  void setupInstrumentPreviewPattern();
  void loadPreviewPrefs();
  void savePreviewPrefs() const;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BrowserPanel)
};

#endif // ZENITH_USE_SKIA

} // namespace zenith
