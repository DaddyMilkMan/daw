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

#ifdef ZENITH_USE_SKIA

namespace zenith {

BrowserPanel::BrowserPanel(BrowserModel &model) : model_(model) {
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

  // Search callbacks
  searchBar_->onSearchChanged = [this](const juce::String &text) {
    listView_->setSearchText(text);
    searchBar_->setBackButtonVisible(false);
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

  // Filter callback
  filterBar_->onFilterChanged = [this]() {
    listView_->updateDisplayItems();
  };

  ZENITH_REGISTER_ANIMATION(zenith::animation::Priority::Medium);
}

BrowserPanel::~BrowserPanel() {
  ZENITH_UNREGISTER_ANIMATION();
  scanner_.cancelScan();
  model_.removeChangeListener(this);
}

void BrowserPanel::onAnimationTick(float deltaMs) {
  SkiaComponent::updateInternalAnimations(deltaMs);
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

void BrowserPanel::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds();
  GlassmorphicPanel::fillBackground(canvas, SkRect::MakeWH((float)bounds.getWidth(), (float)bounds.getHeight()));

  // Progress Bar
  if (scanner_.isScanning()) {
    float prog = scanner_.getProgress();
    float h = progressBarHeight_;
    float y = (float)headerHeight_ + filterBarHeight_;
    SkPaint bg; bg.setColor(zenith::design::colors::BG_01);
    canvas->drawPaint(bg);
    SkPaint fill; fill.setColor(zenith::design::colors::ACCENT_PRIMARY);
    canvas->drawRect(SkRect::MakeXYWH(0, y, (float)getWidth() * prog, h), fill);
  }
  
  // Draw child components (SearchBar, FilterBar, ListView, etc.)
  drawChildren(canvas);
}

void BrowserPanel::resized() {
  auto b = getLocalBounds();
  int y = 0;
  searchBar_->setBounds(0, y, b.getWidth(), headerHeight_); y += headerHeight_;
  filterBar_->setBounds(0, y, b.getWidth(), filterBarHeight_); y += filterBarHeight_;
  
  int listH = b.getHeight() - y - previewHeight_;
  recentSidebar_->setBounds(0, y, recentWidth_, listH);
  listView_->setBounds(recentWidth_, y, b.getWidth() - recentWidth_, listH); y += listH;
  
  previewPanel_->setBounds(0, y, b.getWidth(), previewHeight_);
}

void BrowserPanel::showContextMenu(int itemIndex, juce::Point<int> position) {
  auto item = listView_->getSelectedItem();
  if (!item) return;

  auto menu = ContextMenuManager::createMenu();
  menu->addSectionHeader("Item Options");
  juce::String favText = item->isFavorite ? "Remove from Favorites" : "Add to Favorites";
  menu->addItem(1, favText, true, false, [this, item]() { toggleFavorite(item); });
  menu->addSeparator();

  menu->addItem(2, "Add Tag...", true, false, [this, item]() {
    auto alert = std::make_shared<juce::AlertWindow>("Add Tag", "Enter tag:", juce::MessageBoxIconType::NoIcon);
    alert->addTextEditor("tag", "");
    alert->addButton("Add", 1); alert->addButton("Cancel", 0);
    
    // Use SafePointer just in case 'this' (BrowserPanel) is deleted while dialog is open
    auto safeThis = juce::Component::SafePointer<BrowserPanel>(this);

    alert->enterModalState(true, juce::ModalCallbackFunction::create([safeThis, item, alert](int res) {
      if (res == 1 && safeThis) { 
          juce::String t = alert->getTextEditorContents("tag"); 
          if (t.isNotEmpty()) { 
              safeThis->model_.addTagToItem(item, t); 
              safeThis->repaint(); 
          } 
      }
      // shared_ptr 'alert' is destroyed here, deleting the window
    }), true);
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
  fileChooser_ = std::make_unique<juce::FileChooser>("Select Folder", juce::File::getSpecialLocation(juce::File::userMusicDirectory), "");
  fileChooser_->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories, [this](const juce::FileChooser &c) {
    auto res = c.getResult();
    if (res.isDirectory()) {
      juce::StringArray paths = model_.getUserLibraryPaths();
      if (!paths.contains(res.getFullPathName())) {
        paths.add(res.getFullPathName());
        model_.setUserLibraryPaths(paths);
        scanner_.startScan(res, true, 5);
      }
    }
  });
}

} // namespace zenith

#endif // ZENITH_USE_SKIA
