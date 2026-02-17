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

#include "BrowserRecentSidebar.h"
#include "../../browser/BrowserPreviewEngine.h"
#include "../../browser/BrowserScanner.h"
#include "BrowserWaveformLoader.h"
#include "BrowserSearchBar.h"
#include "BrowserFilterBar.h"
#include "BrowserListView.h"
#include "BrowserPreviewPanel.h"
#include "../controls/SkiaAlertWindow.h"
#include "../controls/SkiaFileChooser.h"

namespace zenith {

#ifdef ZENITH_USE_SKIA

/**
 * Main Universal Browser Panel.
 * Now a container for modular sub-components.
 */
class BrowserPanel : public SkiaComponent,
                     public juce::ChangeListener,
                     public juce::DragAndDropContainer {
public:
  explicit BrowserPanel(BrowserModel &model);
  ~BrowserPanel() override;

  // SkiaComponent overrides
  void drawSkia(SkCanvas *canvas) override;
  void resized() override;
  void timerCallback() override;
  bool keyPressed(const juce::KeyPress &key) override;

  // ChangeListener override
  void changeListenerCallback(juce::ChangeBroadcaster *source) override;

  void setSearchText(const juce::String &text);
  BrowserPreviewEngine &getPreviewEngine() { return previewEngine_; }

  std::function<void(std::shared_ptr<BrowserItem>)> onItemDoubleClicked;

private:
  BrowserModel &model_;
  BrowserScanner scanner_;
  BrowserPreviewEngine previewEngine_;

  // Modular Sub-Components
  std::unique_ptr<BrowserWaveformLoader> waveformLoader_;
  std::unique_ptr<BrowserSearchBar> searchBar_;
  std::unique_ptr<BrowserFilterBar> filterBar_;
  std::unique_ptr<BrowserRecentSidebar> recentSidebar_;
  std::unique_ptr<BrowserListView> listView_;
  std::unique_ptr<BrowserPreviewPanel> previewPanel_;

  // Layout Constants
  static constexpr int headerHeight_ = 45;
  static constexpr int filterBarHeight_ = 28;
  static constexpr int previewHeight_ = 100;
  static constexpr int recentWidth_ = 120;
  static constexpr int progressBarHeight_ = 4;

  void showContextMenu(int itemIndex, juce::Point<int> position);
  void toggleFavorite(std::shared_ptr<BrowserItem> item);
  void showAddFolderDialog();
  void runBrowserCommand(const juce::String& commandId);

  void dismissDialogOverlays();

  std::unique_ptr<SkiaAlertWindow> activeAlert_;
  std::unique_ptr<SkiaFileChooser> activeFileChooser_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BrowserPanel)
};

#endif // ZENITH_USE_SKIA

} // namespace zenith
