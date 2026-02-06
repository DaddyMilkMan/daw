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

#include "../panels/BrowserRecentSidebar.h"
#include "../../browser/BrowserPreviewEngine.h"
#include "../../browser/BrowserScanner.h"
#include "../panels/BrowserWaveformLoader.h"
#include "../panels/BrowserSearchBar.h"
#include "../panels/BrowserFilterBar.h"
#include "../panels/BrowserListView.h"
#include "../panels/BrowserPreviewPanel.h"

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
  void onAnimationTick(float deltaMs) override;

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

  std::unique_ptr<juce::FileChooser> fileChooser_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BrowserPanel)
};

#endif // ZENITH_USE_SKIA

} // namespace zenith
