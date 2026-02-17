/*
  ==============================================================================

    BrowserPanel.cpp
    Refactored: 2025-12-26
    Author:  Zenith DAW

  ==============================================================================
*/

#include "BrowserPanel.h"
#include "../design-system/ZenithDesignSystem.h"
#include "../design-system/ColorBridge.h"
#include "../framework/GlassmorphicPanel.h"
#include "../controls/ContextMenuManager.h"
#include <effects/SkGradientShader.h>
#include <cmath>

#ifdef ZENITH_USE_SKIA

namespace zenith {

BrowserPanel::BrowserPanel(BrowserModel &model) : model_(model) {
  setWantsKeyboardFocus(true);
  setMouseClickGrabsKeyboardFocus(true);
  model_.addChangeListener(this);
  
  waveformLoader_ = std::make_unique<BrowserWaveformLoader>();
  searchBar_ = std::make_unique<BrowserSearchBar>();
  filterBar_ = std::make_unique<BrowserFilterBar>(model_);
  recentSidebar_ = std::make_unique<BrowserRecentSidebar>(model_);
  listView_ = std::make_unique<BrowserListView>(model_, *waveformLoader_);
  previewPanel_ = std::make_unique<BrowserPreviewPanel>(previewEngine_);

  addAndMakeVisible(searchBar_.get());
  addAndMakeVisible(filterBar_.get());
  addAndMakeVisible(recentSidebar_.get());
  addAndMakeVisible(listView_.get());
  addAndMakeVisible(previewPanel_.get());
  listView_->setWantsKeyboardFocus(true);
  listView_->setMouseClickGrabsKeyboardFocus(true);

  // Search callbacks
  searchBar_->onSearchChanged = [this](const juce::String &text) {
    listView_->setSearchText(text);
    searchBar_->setBackButtonVisible(listView_->getCurrentRoot() != model_.getRoot());
  };
  searchBar_->onBackRequested = [this]() {
    listView_->navigateUp();
    searchBar_->setBackButtonVisible(listView_->getCurrentRoot() != model_.getRoot());
  };
  searchBar_->onAddFolderRequested = [this]() {
    showAddFolderDialog();
  };

  // Recent callback
  recentSidebar_->onItemSelected = [this](std::shared_ptr<BrowserItem> item) {
    listView_->onItemSelected(item);
  };

  // List callbacks
  listView_->onItemSelected = [this](std::shared_ptr<BrowserItem> item) {
    if (item && item->type == BrowserItemType::AudioFile) {
      juce::File f(item->id);
      previewPanel_->loadWaveform(f);
      previewEngine_.loadFile(f, previewEngine_.isAutoPlayEnabled());
      model_.addToRecent(item);
    } else {
      previewPanel_->clearWaveform();
      previewEngine_.stop();
    }
    searchBar_->setBackButtonVisible(listView_->getCurrentRoot() != model_.getRoot());
  };
  listView_->onItemDoubleClicked = [this](std::shared_ptr<BrowserItem> item) {
    if (onItemDoubleClicked) onItemDoubleClicked(item);
    searchBar_->setBackButtonVisible(listView_->getCurrentRoot() != model_.getRoot());
  };
  listView_->onItemRightClicked = [this](int index, juce::Point<int> pos) {
    showContextMenu(index, pos);
  };
  listView_->onCommandPaletteRequested = [this]() {
    searchBar_->setCommandMode(true);
  };

  // Filter callback
  filterBar_->onFilterChanged = [this]() {
    listView_->updateDisplayItems();
  };

  searchBar_->onCommandExecuted = [this](const juce::String& commandId) {
    runBrowserCommand(commandId);
  };

  if (juce::MessageManager::getInstanceWithoutCreating() != nullptr) startTimerHz(30);
}

BrowserPanel::~BrowserPanel() {
  stopTimer();
  scanner_.cancelScan();
  dismissDialogOverlays();
  model_.removeChangeListener(this);
}

void BrowserPanel::timerCallback() {
  if (scanner_.isScanning() || previewEngine_.isPlaying()) {
    repaint();
  }
}

void BrowserPanel::changeListenerCallback(juce::ChangeBroadcaster *source) {
  if (source == &model_) {
    listView_->updateDisplayItems();
    repaint();
  }
}

void BrowserPanel::setSearchText(const juce::String &text) {
  searchBar_->setSearchText(text);
}

bool BrowserPanel::keyPressed(const juce::KeyPress &key) {
  if (key == juce::KeyPress('k', juce::ModifierKeys::commandModifier, 0) ||
      key == juce::KeyPress('k', juce::ModifierKeys::ctrlModifier, 0)) {
    searchBar_->setCommandMode(true);
    return true;
  }
  return SkiaComponent::keyPressed(key);
}

void BrowserPanel::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds();
  GlassmorphicPanel::fillBackground(canvas, SkRect::MakeWH((float)bounds.getWidth(), (float)bounds.getHeight()));

  // Progress Bar
  if (scanner_.isScanning()) {
    float prog = scanner_.getProgress();
    float h = progressBarHeight_;
    float y = (float)headerHeight_ + filterBarHeight_;
    SkPaint bg;
    bg.setColor(zenith::design::colors::BG_01);
    canvas->drawRect(SkRect::MakeXYWH(0.0f, y, (float)getWidth(), h), bg);
    SkPaint fill; fill.setColor(zenith::design::colors::ACCENT_PRIMARY);
    canvas->drawRect(SkRect::MakeXYWH(0, y, (float)getWidth() * prog, h), fill);

    SkFont progressFont = design::getSkFont(10.0f, design::FontWeight::SemiBold);
    SkPaint progressPaint;
    progressPaint.setAntiAlias(true);
    progressPaint.setColor(design::withAlpha(design::colors::TEXT_PRIMARY, 0.82f));
    const juce::String progressText = "Scanning libraries  " + juce::String((int)std::round(prog * 100.0f)) + "%";
    canvas->drawString(progressText.toStdString().c_str(), 10.0f, y - 4.0f, progressFont, progressPaint);
  }
}

void BrowserPanel::resized() {
  auto b = getLocalBounds();
  int y = 0;
  searchBar_->setBounds(0, y, b.getWidth(), headerHeight_); y += headerHeight_;
  filterBar_->setBounds(0, y, b.getWidth(), filterBarHeight_); y += filterBarHeight_;

  const int previewH = juce::jlimit(92, 180, (int)std::round((double)b.getHeight() * 0.20));
  const int listH = juce::jmax(80, b.getHeight() - y - previewH);
  const bool compactMode = b.getWidth() < 760;
  const int recentW = compactMode ? 0 : juce::jlimit(132, 240, (int)std::round((double)b.getWidth() * 0.18));

  recentSidebar_->setVisible(!compactMode);
  if (!compactMode) {
    recentSidebar_->setBounds(0, y, recentW, listH);
  }

  listView_->setBounds(recentW, y, b.getWidth() - recentW, listH);
  y += listH;

  previewPanel_->setBounds(0, y, b.getWidth(), previewH);
}

void BrowserPanel::showContextMenu(int itemIndex, juce::Point<int> position) {
  auto item = listView_->getItemAt(itemIndex);
  if (!item) return;

  auto menu = ContextMenuManager::createMenu();
  menu->addSectionHeader("Item Options");
  juce::String favText = item->isFavorite ? "Remove from Favorites" : "Add to Favorites";
  menu->addItem(1, favText, true, false, [this, item]() { toggleFavorite(item); });
  menu->addSeparator();

  menu->addItem(2, "Add Tag...", true, false, [this, item]() {
    dismissDialogOverlays();
    activeAlert_ = std::make_unique<SkiaAlertWindow>(
        "Add Tag", "Create a tag for this browser item",
        SkiaAlertWindow::IconType::NoIcon);
    activeAlert_->addTextEditor("tag", "", "Tag");
    activeAlert_->addButton("Cancel", SkiaAlertWindow::Result::Cancelled,
                            SkiaButton::Style::Secondary);
    activeAlert_->addButton("Add", SkiaAlertWindow::Result::Button1,
                            SkiaButton::Style::Primary);

    const int w = juce::jlimit(360, 520, (int)std::round((double)getWidth() * 0.56));
    const int h = juce::jlimit(210, 300, (int)std::round((double)getHeight() * 0.34));
    activeAlert_->setBounds((getWidth() - w) / 2, (getHeight() - h) / 2, w, h);
    addAndMakeVisible(activeAlert_.get());
    activeAlert_->toFront(true);

    activeAlert_->showAsync([this, item](SkiaAlertWindow::Result result) {
      if (result == SkiaAlertWindow::Result::Button1 && activeAlert_) {
        const juce::String tag =
            activeAlert_->getTextEditorContents("tag").trim();
        if (tag.isNotEmpty()) {
          model_.addTagToItem(item, tag);
          repaint();
        }
      }
      dismissDialogOverlays();
    });
  });

  if (!item->metadata.tags.empty()) {
    auto tagMenu = ContextMenuManager::createMenu();
    for (const auto &tag : item->metadata.tags) {
      auto colorMenu = ContextMenuManager::createMenu();
      std::map<juce::String, juce::String> presets = {
        {"Red", "#FF5555"}, {"Green", "#55FF55"}, {"Blue", "#5555FF"},
        {"Cyan", "#55FFFF"}, {"Yellow", "#FFFF55"}, {"Purple", "#FF55FF"}
      };
      for (const auto &[name, hex] : presets) {
        colorMenu->addItem(0, name, true, false, [this, tag, hex]() {
          model_.setTagColor(tag, hex);
          repaint();
        });
      }
      tagMenu->addSubMenu(tag, std::move(colorMenu));
    }
    menu->addSubMenu("Set Tag Color", std::move(tagMenu));
  }

  if (item->type == BrowserItemType::AudioFile || item->type == BrowserItemType::MidiFile) {
    menu->addSeparator();
    menu->addItem(10, "Show in Explorer", true, false, [item]() { juce::File(item->id).revealToUser(); });
  }

  ContextMenuManager::getInstance().showMenuAt(std::move(menu), this, position.x - getScreenPosition().x, position.y - getScreenPosition().y);
}

void BrowserPanel::toggleFavorite(std::shared_ptr<BrowserItem> item) {
  if (item) { if (item->isFavorite) model_.removeFromFavorites(item); else model_.addToFavorites(item); repaint(); }
}

void BrowserPanel::showAddFolderDialog() {
  dismissDialogOverlays();
  activeFileChooser_ = std::make_unique<SkiaFileChooser>(
      "Add Library Folder",
      juce::File::getSpecialLocation(juce::File::userMusicDirectory), "",
      SkiaFileChooser::Mode::OpenDirectory);

  const int w = juce::jlimit(540, 980, (int)std::round((double)getWidth() * 0.82));
  const int h = juce::jlimit(360, 700, (int)std::round((double)getHeight() * 0.78));
  activeFileChooser_->setBounds((getWidth() - w) / 2, (getHeight() - h) / 2, w,
                                h);
  addAndMakeVisible(activeFileChooser_.get());
  activeFileChooser_->toFront(true);

  activeFileChooser_->showAsync([this](SkiaFileChooser::Result result,
                                       const juce::File& selected) {
    if (result == SkiaFileChooser::Result::Approved && selected.isDirectory()) {
      juce::StringArray paths = model_.getUserLibraryPaths();
      if (!paths.contains(selected.getFullPathName())) {
        paths.add(selected.getFullPathName());
        model_.setUserLibraryPaths(paths);
        scanner_.startScan(selected, true, 5);
      }
    }
    dismissDialogOverlays();
  });
}

void BrowserPanel::runBrowserCommand(const juce::String& commandId) {
  if (commandId == "add-folder") {
    showAddFolderDialog();
    return;
  }

  if (commandId == "refresh-library") {
    model_.refresh();
    listView_->updateDisplayItems();
    repaint();
    return;
  }

  if (commandId == "toggle-autoplay") {
    previewEngine_.setAutoPlayEnabled(!previewEngine_.isAutoPlayEnabled());
    repaint();
    return;
  }

  if (commandId == "clear-search") {
    setSearchText("");
    listView_->grabKeyboardFocus();
    return;
  }

  if (commandId == "focus-list") {
    listView_->grabKeyboardFocus();
    return;
  }
}

void BrowserPanel::dismissDialogOverlays() {
  if (activeAlert_) {
    removeChildComponent(activeAlert_.get());
    activeAlert_.reset();
  }
  if (activeFileChooser_) {
    removeChildComponent(activeFileChooser_.get());
    activeFileChooser_.reset();
  }
}

} // namespace zenith

#endif // ZENITH_USE_SKIA
