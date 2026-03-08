/*
  ==============================================================================

    BrowserSearchBar.h
    Created: 2025-12-26
    Author:  Zenith DAW

  ==============================================================================
*/

#pragma once

#include "SkiaComponent.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

#ifdef ZENITH_USE_SKIA
#include <include/core/SkCanvas.h>
#endif

namespace zenith {

class BrowserSearchBar : public SkiaComponent {
public:
  BrowserSearchBar();
  ~BrowserSearchBar() override;

  void drawSkia(SkCanvas *canvas) override;
  void mouseDown(const juce::MouseEvent &e) override;
  bool keyPressed(const juce::KeyPress &key) override;
  void resized() override;

  void setSearchText(const juce::String &text);
  juce::String getSearchText() const { return searchText_; }
  void setCommandMode(bool enabled);
  bool isCommandMode() const { return commandMode_; }

  std::function<void(const juce::String&)> onSearchChanged;
  std::function<void(const juce::String&)> onCommandExecuted;
  std::function<void()> onAddFolderRequested;
  std::function<void()> onBackRequested;

  void setBackButtonVisible(bool visible);

private:
  void updateVisibleCommands();
  void executeSelectedCommand();

  juce::String searchText_;
  bool backButtonVisible_ = false;
  bool commandMode_ = false;
  int selectedCommandIndex_ = 0;
  std::vector<int> visibleCommandIndices_;

  juce::Rectangle<int> searchBoxBounds_;
  juce::Rectangle<int> backButtonBounds_;
  juce::Rectangle<int> addFolderButtonBounds_;
  juce::Rectangle<int> commandPaletteBounds_;

  static constexpr int searchBoxHeight_ = 30;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BrowserSearchBar)
};

} // namespace zenith
