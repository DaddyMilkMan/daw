#include "PluginBrowserComponent.h"

#include "Engine.h"
#include "../design-system/ZenithDesignSystem.h"
#include "../engine/PluginHost.h"
#include "../engine/Track.h"

namespace zenith {

PluginBrowserComponent::PluginBrowserComponent(Engine &engine)
    : engine_(engine), listModel_(*this) {
  setSize(860, 620);

  titleLabel_ = std::make_unique<SkiaLabel>("title", "Plugin Browser");
  titleLabel_->setFont(20.0f, SkFontStyle::kBold_Weight);
  titleLabel_->setTextColour(design::colors::TEXT_PRIMARY);
  addAndMakeVisible(titleLabel_.get());

  searchLabel_ = std::make_unique<SkiaLabel>("search_label", "Search");
  searchLabel_->setTextColour(design::colors::TEXT_SECONDARY);
  addAndMakeVisible(searchLabel_.get());

  searchBox_ = std::make_unique<SkiaTextEditor>("search_box");
  searchBox_->setMultiLine(false);
  searchBox_->setTextToShowWhenEmpty(
      "Type plugin name, vendor, or category...",
      design::withAlpha(design::colors::TEXT_SECONDARY, 0.65f));
  searchBox_->onTextChange = [this]() {
    currentFilter_ = searchBox_->getText().toLowerCase().trim();
    updateFilteredList();
  };
  addAndMakeVisible(searchBox_.get());

  pluginList_ = std::make_unique<SkiaListBox>("plugin_list");
  pluginList_->setModel(&listModel_);
  pluginList_->setRowHeight(30);
  pluginList_->onRowClicked = [this](int row) {
    if (row >= 0 && row < filteredPlugins_.size()) {
      pluginList_->selectRow(row, true, true);
    }
  };
  pluginList_->onRowDoubleClicked = [this](int row) { loadPluginAtIndex(row); };
  addAndMakeVisible(pluginList_.get());

  trackLabel_ = std::make_unique<SkiaLabel>("track_label", "Target Track: None");
  trackLabel_->setTextColour(design::colors::TEXT_SECONDARY);
  addAndMakeVisible(trackLabel_.get());

  trackPrevButton_ = std::make_unique<ZenithButton>("<");
  trackPrevButton_->setStyle(ZenithButton::Style::Ghost);
  trackPrevButton_->onClick = [this]() { cycleTargetTrack(-1); };
  addAndMakeVisible(trackPrevButton_.get());

  trackNextButton_ = std::make_unique<ZenithButton>(">");
  trackNextButton_->setStyle(ZenithButton::Style::Ghost);
  trackNextButton_->onClick = [this]() { cycleTargetTrack(1); };
  addAndMakeVisible(trackNextButton_.get());

  loadButton_ = std::make_unique<ZenithButton>("Load On Track");
  loadButton_->setStyle(ZenithButton::Style::Primary);
  loadButton_->onClick = [this]() { loadSelectedPlugin(); };
  addAndMakeVisible(loadButton_.get());

  statusLabel_ = std::make_unique<SkiaLabel>("status", "Ready");
  statusLabel_->setTextColour(design::colors::TEXT_SECONDARY);
  addAndMakeVisible(statusLabel_.get());

  refresh();
}

PluginBrowserComponent::~PluginBrowserComponent() { dismissAlert(); }

void PluginBrowserComponent::drawSkia(SkCanvas *canvas) {
  SkPaint bg;
  bg.setColor(design::colors::BG_DARKEST);
  canvas->drawRect(SkRect::MakeWH((float)getWidth(), (float)getHeight()), bg);

  SkPaint divider;
  divider.setAntiAlias(true);
  divider.setColor(design::withAlpha(design::colors::TEXT_SECONDARY, 0.25f));
  divider.setStrokeWidth(1.0f);
  canvas->drawLine(12.0f, 56.0f, (float)getWidth() - 12.0f, 56.0f, divider);
}

void PluginBrowserComponent::resized() {
  auto area = getLocalBounds().reduced(12);

  auto top = area.removeFromTop(40);
  titleLabel_->setBounds(top.removeFromLeft(260));

  auto searchRow = area.removeFromTop(36);
  searchLabel_->setBounds(searchRow.removeFromLeft(70));
  searchBox_->setBounds(searchRow.reduced(4, 2));

  area.removeFromTop(8);

  auto bottom = area.removeFromBottom(42);
  auto leftBottom = bottom.removeFromLeft(340);

  trackLabel_->setBounds(leftBottom.removeFromLeft(220));
  leftBottom.removeFromLeft(8);
  trackPrevButton_->setBounds(leftBottom.removeFromLeft(32));
  leftBottom.removeFromLeft(4);
  trackNextButton_->setBounds(leftBottom.removeFromLeft(32));

  loadButton_->setBounds(bottom.removeFromRight(160));
  statusLabel_->setBounds(bottom.reduced(8, 0));

  pluginList_->setBounds(area);

  if (activeAlert_) {
    const int w = juce::jlimit(320, 500, (int)std::round((double)getWidth() * 0.55));
    const int h = juce::jlimit(190, 300, (int)std::round((double)getHeight() * 0.32));
    activeAlert_->setBounds((getWidth() - w) / 2, (getHeight() - h) / 2, w, h);
  }
}

void PluginBrowserComponent::setTargetTrack(Track *track) {
  targetTrack_ = track;
  if (targetTrack_ != nullptr) {
    trackLabel_->setText("Target Track: " + targetTrack_->getName(),
                         juce::dontSendNotification);
  } else {
    trackLabel_->setText("Target Track: None", juce::dontSendNotification);
  }
}

int PluginBrowserComponent::getSelectedPluginIndex() const {
  return pluginList_ ? pluginList_->getSelectedRow() : -1;
}

bool PluginBrowserComponent::loadSelectedPlugin() {
  const int selected = getSelectedPluginIndex();
  if (selected < 0 || selected >= filteredPlugins_.size()) {
    showWarning("No Plugin Selected", "Select a plugin from the list first.");
    return false;
  }
  if (targetTrack_ == nullptr) {
    showWarning("No Target Track", "Choose a target track before loading.");
    return false;
  }

  loadPluginAtIndex(selected);
  return true;
}

void PluginBrowserComponent::refresh() {
  updateFilteredList();

  if (targetTrack_ == nullptr) {
    const auto &tracks = engine_.tracks();
    for (const auto &track : tracks) {
      if (track) {
        setTargetTrack(track.get());
        break;
      }
    }
  }
}

int PluginBrowserComponent::PluginListModel::getNumRows() {
  return owner_.filteredPlugins_.size();
}

void PluginBrowserComponent::PluginListModel::paintListBoxItem(
    int rowNumber, SkCanvas &canvas, int width, int height, bool rowIsSelected) {
  if (rowNumber < 0 || rowNumber >= owner_.filteredPlugins_.size()) {
    return;
  }

  const auto &desc = owner_.filteredPlugins_[rowNumber];

  if (rowIsSelected) {
    SkPaint p;
    p.setColor(design::withAlpha(design::colors::ACCENT_PRIMARY, 0.22f));
    canvas.drawRect(SkRect::MakeWH((float)width, (float)height), p);
  } else if ((rowNumber % 2) == 0) {
    SkPaint p;
    p.setColor(design::withAlpha(design::colors::BG_02, 0.8f));
    canvas.drawRect(SkRect::MakeWH((float)width, (float)height), p);
  }

  const juce::String category = desc.category.isNotEmpty() ? desc.category : "Unknown";
  const juce::String right = category + "  |  " + desc.manufacturerName;

  SkFont nameFont = design::getSkFont(12.5f, design::FontWeight::Medium);
  SkFont metaFont = design::getSkFont(11.0f, design::FontWeight::Regular);

  SkPaint namePaint;
  namePaint.setAntiAlias(true);
  namePaint.setColor(design::colors::TEXT_PRIMARY);

  SkPaint metaPaint;
  metaPaint.setAntiAlias(true);
  metaPaint.setColor(design::withAlpha(design::colors::TEXT_SECONDARY, 0.95f));

  canvas.drawString(desc.name.toRawUTF8(), 10.0f, 18.0f, nameFont, namePaint);
  canvas.drawString(right.toRawUTF8(), 10.0f, 28.0f, metaFont, metaPaint);
}

void PluginBrowserComponent::PluginListModel::listBoxItemDoubleClicked(
    int rowNumber, const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  owner_.loadPluginAtIndex(rowNumber);
}

void PluginBrowserComponent::updateFilteredList() {
  filteredPlugins_.clear();
  auto &knownPlugins = engine_.getPluginHost().getKnownPlugins();

  for (const auto &desc : knownPlugins.getTypes()) {
    if (currentFilter_.isEmpty() || desc.name.toLowerCase().contains(currentFilter_) ||
        desc.manufacturerName.toLowerCase().contains(currentFilter_) ||
        desc.category.toLowerCase().contains(currentFilter_)) {
      filteredPlugins_.add(desc);
    }
  }

  pluginList_->updateContent();
  pluginList_->deselectAllRows();
  if (filteredPlugins_.size() > 0) {
    pluginList_->selectRow(0);
  }

  setStatus("Plugins: " + juce::String(filteredPlugins_.size()));
}

void PluginBrowserComponent::loadPluginAtIndex(int index) {
  if (index < 0 || index >= filteredPlugins_.size() || targetTrack_ == nullptr) {
    return;
  }

  const auto desc = filteredPlugins_[index];
  setStatus("Loading " + desc.name + "...");

  auto &formatManager = engine_.getPluginFormatManager();
  const auto sampleRate = engine_.getSampleRate();
  const auto bufferSize = engine_.getBufferSize();

  formatManager.createPluginInstanceAsync(
      desc, sampleRate, bufferSize,
      [this, desc, weakThis = juce::Component::SafePointer<PluginBrowserComponent>(this)](
          std::unique_ptr<juce::AudioPluginInstance> instance,
          const juce::String &error) {
        if (weakThis == nullptr) {
          return;
        }

        if (instance != nullptr && targetTrack_ != nullptr) {
          targetTrack_->addPlugin(std::move(instance));
          setStatus("Loaded: " + desc.name + " -> " + targetTrack_->getName());
        } else {
          setStatus("Failed: " + desc.name);
          showWarning("Plugin Load Failed",
                      "Failed to load plugin: " + desc.name + "\n\nError: " + error);
        }
      });
}

void PluginBrowserComponent::setStatus(const juce::String &text) {
  statusLabel_->setText(text, juce::dontSendNotification);
}

void PluginBrowserComponent::cycleTargetTrack(int direction) {
  const auto &tracks = engine_.tracks();
  if (tracks.empty()) {
    setTargetTrack(nullptr);
    return;
  }

  int currentIndex = -1;
  for (int i = 0; i < (int)tracks.size(); ++i) {
    if (tracks[(size_t)i].get() == targetTrack_) {
      currentIndex = i;
      break;
    }
  }

  if (currentIndex < 0) {
    currentIndex = 0;
  } else {
    currentIndex = (currentIndex + direction + (int)tracks.size()) % (int)tracks.size();
  }

  setTargetTrack(tracks[(size_t)currentIndex].get());
}

void PluginBrowserComponent::showWarning(const juce::String &title,
                                         const juce::String &message) {
  dismissAlert();
  activeAlert_ = std::make_unique<SkiaAlertWindow>(
      title, message, SkiaAlertWindow::IconType::WarningIcon);
  activeAlert_->addButton("OK", SkiaAlertWindow::Result::Button1,
                          SkiaButton::Style::Primary);

  addAndMakeVisible(activeAlert_.get());
  activeAlert_->toFront(true);
  resized();

  activeAlert_->showAsync([this](SkiaAlertWindow::Result) { dismissAlert(); });
}

void PluginBrowserComponent::dismissAlert() {
  if (activeAlert_) {
    removeChildComponent(activeAlert_.get());
    activeAlert_.reset();
  }
}

PluginBrowserWindow::PluginBrowserWindow(Engine &engine)
    : DocumentWindow("Plugin Browser",
                     juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(
                         juce::ResizableWindow::backgroundColourId),
                     DocumentWindow::allButtons) {
  setUsingNativeTitleBar(true);
  setResizable(true, true);

  auto browser = std::make_unique<PluginBrowserComponent>(engine);
  setContentOwned(browser.release(), true);

  centreWithSize(getWidth(), getHeight());
  setVisible(true);
}

PluginBrowserWindow::~PluginBrowserWindow() { clearContentComponent(); }

void PluginBrowserWindow::closeButtonPressed() { setVisible(false); }

PluginBrowserComponent *PluginBrowserWindow::getBrowserComponent() {
  return dynamic_cast<PluginBrowserComponent *>(getContentComponent());
}

} // namespace zenith
