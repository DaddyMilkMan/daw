/*
  ==============================================================================

    BrowserPanel.cpp

  ==============================================================================
*/

#include "BrowserPanel.h"
#include "../design-system/ZenithDesignSystem.h"
#include "../design-system/ZenithIcons.h"
#include <algorithm>
#include <juce_audio_formats/juce_audio_formats.h>
#include <map>

#ifdef ZENITH_USE_SKIA

namespace zenith {

namespace {
void drawRoundedFill(SkCanvas* canvas, const juce::Rectangle<int>& r, SkColor fill, float radius) {
  SkPaint p;
  p.setAntiAlias(true);
  p.setColor(fill);
  canvas->drawRoundRect(
      SkRect::MakeXYWH((float)r.getX(), (float)r.getY(), (float)r.getWidth(), (float)r.getHeight()),
      radius, radius, p);
}

void drawRoundedStroke(SkCanvas* canvas, const juce::Rectangle<int>& r, SkColor stroke, float radius, float width = 1.0f) {
  SkPaint p;
  p.setAntiAlias(true);
  p.setColor(stroke);
  p.setStyle(SkPaint::kStroke_Style);
  p.setStrokeWidth(width);
  canvas->drawRoundRect(
      SkRect::MakeXYWH((float)r.getX(), (float)r.getY(), (float)r.getWidth(), (float)r.getHeight()),
      radius, radius, p);
}

void drawSectionIcon(SkCanvas* canvas, const juce::Rectangle<int>& r, int sectionIndex, SkColor color, bool active) {
  const auto badge = juce::Rectangle<int>(r.getX() + 8, r.getCentreY() - 8, 16, 16);
  if (active) {
    SkPaint fill;
    fill.setAntiAlias(true);
    fill.setColor(design::withAlpha(design::colors::ACCENT_PRIMARY, 0.22f));
    canvas->drawRoundRect(SkRect::MakeXYWH((float)badge.getX() - 2.0f, (float)badge.getY() - 2.0f,
                                           (float)badge.getWidth() + 4.0f, (float)badge.getHeight() + 4.0f),
                          design::dimensions::RADIUS_SM, design::dimensions::RADIUS_SM, fill);
  }

  SkPath icon;
  switch (sectionIndex) {
    case 0: icon = icons::Template(); break;
    case 1: icon = icons::Synth(); break;
    case 2: icon = icons::RecordingTemplate(); break;
    case 3: icon = icons::Settings(); break;
    case 4: icon = icons::OrchestralTemplate(); break;
    case 5: icon = icons::File(); break;
    case 6: icon = icons::Project(); break;
    case 7: icon = icons::Sparkles(); break;
    case 8: icon = icons::History(); break;
    default: icon = icons::File(); break;
  }

  icons::IconStyle style;
  style.color = color;
  style.strokeWidth = 1.45f;
  icons::drawIconCentered(
      canvas, icon,
      SkRect::MakeXYWH((float)badge.getX(), (float)badge.getY(), (float)badge.getWidth(), (float)badge.getHeight()),
      11.0f, style);
}

juce::String ellipsize(const juce::String& s, int maxChars) {
  if (maxChars <= 1) return "...";
  if (s.length() <= maxChars) return s;
  return s.substring(0, juce::jmax(0, maxChars - 1)) + "...";
}

SkColor typeColor(BrowserItemType type) {
  switch (type) {
    case BrowserItemType::Instrument: return SkColorSetRGB(97, 160, 230);
    case BrowserItemType::AudioFile: return SkColorSetRGB(88, 189, 168);
    case BrowserItemType::Plugin: return SkColorSetRGB(162, 142, 220);
    case BrowserItemType::MidiFile: return SkColorSetRGB(218, 168, 96);
    case BrowserItemType::Preset: return SkColorSetRGB(199, 138, 176);
    case BrowserItemType::Project: return SkColorSetRGB(144, 158, 180);
    case BrowserItemType::Folder: return SkColorSetRGB(118, 136, 160);
    default: break;
  }
  return SkColorSetRGB(126, 140, 160);
}

SkPath itemTypeIcon(const BrowserItem& item) {
  if (item.isDirectory) return icons::Folder();

  switch (item.type) {
    case BrowserItemType::Instrument: return icons::Synth();
    case BrowserItemType::AudioFile: return icons::Audio();
    case BrowserItemType::Plugin: return icons::Plugin();
    case BrowserItemType::MidiFile: return icons::MIDI();
    case BrowserItemType::Preset: return icons::File();
    case BrowserItemType::Project: return icons::Project();
    case BrowserItemType::Folder: return icons::Folder();
    default: break;
  }

  return icons::File();
}

} // namespace

BrowserPanel::BrowserPanel(BrowserModel &model, CommandAPI &api)
    : model_(model), api_(api) {
  setWantsKeyboardFocus(true);
  setMouseClickGrabsKeyboardFocus(true);
  model_.addChangeListener(this);

  sections_ = {
      {Section::All, "All"},
      {Section::Instruments, "Instruments"},
      {Section::Sounds, "Sounds"},
      {Section::Effects, "Effects"},
      {Section::MIDI, "MIDI"},
      {Section::Presets, "Presets"},
      {Section::Projects, "Projects"},
      {Section::Favorites, "Favorites"},
      {Section::Recent, "Recent"},
  };

  expandedFolderIds_.insert("library_root");
  rowHeight_ = compactDensity_ ? 32 : 40;
  previewPrefsFile_ = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                          .getChildFile("ZenithDAW")
                          .getChildFile("browser_preview_prefs.txt");
  loadPreviewPrefs();
  rebuildVisibleItems();
  if (juce::MessageManager::getInstanceWithoutCreating() != nullptr) startTimerHz(30);
}

BrowserPanel::~BrowserPanel() {
  savePreviewPrefs();
  stopTimer();
  model_.removeChangeListener(this);
}

void BrowserPanel::timerCallback() {
  if (previewEngine_.isPlaying() || instrumentPreviewPlaying_) repaint(previewRect_);
}

void BrowserPanel::changeListenerCallback(juce::ChangeBroadcaster *source) {
  if (source == &model_) {
    rebuildVisibleItems();
    resized();
    repaint();
  }
}

void BrowserPanel::setSearchText(const juce::String &text) {
  searchText_ = text;
  rebuildVisibleItems();
  repaint();
}

bool BrowserPanel::sectionMatchesItem(Section section, const BrowserItem &item) const {
  switch (section) {
    case Section::All: return true;
    case Section::Instruments: return item.type == BrowserItemType::Instrument;
    case Section::Sounds: return item.type == BrowserItemType::AudioFile;
    case Section::Effects: return item.type == BrowserItemType::Plugin;
    case Section::MIDI: return item.type == BrowserItemType::MidiFile;
    case Section::Presets: return item.type == BrowserItemType::Preset;
    case Section::Projects: return item.type == BrowserItemType::Project;
    case Section::Favorites: return item.isFavorite;
    case Section::Recent: return false;
  }
  return true;
}

bool BrowserPanel::itemMatchesSearch(const BrowserItem &item) const {
  if (searchText_.trim().isEmpty()) return true;
  const juce::String q = searchText_.toLowerCase();
  if (item.name.toLowerCase().contains(q)) return true;
  if (item.metadata.category.toLowerCase().contains(q)) return true;
  if (item.metadata.author.toLowerCase().contains(q)) return true;
  for (const auto& tag : item.metadata.tags) {
    if (tag.toLowerCase().contains(q)) return true;
  }
  return false;
}

std::shared_ptr<BrowserItem> BrowserPanel::findNodeById(const std::shared_ptr<BrowserItem>& node,
                                                        const juce::String& id) const {
  if (!node) return nullptr;
  if (node->id == id) return node;
  for (const auto& child : node->children) {
    if (auto found = findNodeById(child, id)) return found;
  }
  return nullptr;
}

bool BrowserPanel::nodeOrDescendantMatches(const std::shared_ptr<BrowserItem>& node) const {
  if (!node) return false;

  const auto userPaths = model_.getUserLibraryPaths();
  if (selectedSourceIndex_ >= 0 && selectedSourceIndex_ < userPaths.size()) {
    const juce::String sourcePath = juce::File(userPaths[selectedSourceIndex_]).getFullPathName();
    if (node->id.isNotEmpty() && !node->id.startsWithIgnoreCase(sourcePath)) {
      bool childInSource = false;
      for (const auto& child : node->children) {
        if (child && child->id.startsWithIgnoreCase(sourcePath)) {
          childInSource = true;
          break;
        }
      }
      if (!childInSource) return false;
    }
  }

  if (!node->isDirectory) {
    return sectionMatchesItem(activeSection_, *node) && itemMatchesSearch(*node);
  }

  if (itemMatchesSearch(*node) && activeSection_ == Section::All) return true;

  for (const auto& child : node->children) {
    if (nodeOrDescendantMatches(child)) return true;
  }

  return false;
}

void BrowserPanel::buildTreeRowsFromNode(const std::shared_ptr<BrowserItem>& node, int depth) {
  if (!node || !nodeOrDescendantMatches(node)) return;

  const bool expanded = expandedFolderIds_.count(node->id) > 0;
  treeRows_.push_back({node, depth, node->isDirectory, expanded});

  if (!node->isDirectory || !expanded) return;

  std::vector<std::shared_ptr<BrowserItem>> children = node->children;
  std::sort(children.begin(), children.end(), [this](const auto& a, const auto& b) {
    if (!a || !b) return (bool)a;
    if (sortMode_ == SortMode::Type && (int)a->type != (int)b->type) return (int)a->type < (int)b->type;
    if (a->isDirectory != b->isDirectory) return a->isDirectory > b->isDirectory;
    return a->name.compareNatural(b->name) < 0;
  });

  for (const auto& child : children) {
    buildTreeRowsFromNode(child, depth + 1);
  }
}

void BrowserPanel::rebuildVisibleItems() {
  visibleItems_.clear();
  treeRows_.clear();

  if (activeSection_ == Section::Recent) {
    visibleItems_ = model_.getRecentItems();
  } else if (activeSection_ == Section::Favorites) {
    visibleItems_ = model_.getFavorites();
  } else if (activeSection_ == Section::All) {
    auto root = model_.getRoot();
    if (root) {
      const auto userPaths = model_.getUserLibraryPaths();
      if (selectedSourceIndex_ >= 0 && selectedSourceIndex_ < userPaths.size()) {
        auto sourceRoot = findNodeById(root, juce::File(userPaths[selectedSourceIndex_]).getFullPathName());
        if (sourceRoot) {
          buildTreeRowsFromNode(sourceRoot, 0);
        }
      } else {
        for (const auto& child : root->children) {
          if (child && child->id != "favorites_root") buildTreeRowsFromNode(child, 0);
        }
      }
    }
    for (const auto& row : treeRows_) {
      if (row.item) visibleItems_.push_back(row.item);
    }
  } else {
    BrowserItemType type = BrowserItemType::Unknown;
    switch (activeSection_) {
      case Section::Instruments: type = BrowserItemType::Instrument; break;
      case Section::Sounds: type = BrowserItemType::AudioFile; break;
      case Section::Effects: type = BrowserItemType::Plugin; break;
      case Section::MIDI: type = BrowserItemType::MidiFile; break;
      case Section::Presets: type = BrowserItemType::Preset; break;
      case Section::Projects: type = BrowserItemType::Project; break;
      default: break;
    }
    visibleItems_ = model_.getItemsByType(type);

    visibleItems_.erase(std::remove_if(visibleItems_.begin(), visibleItems_.end(),
                                       [this](const std::shared_ptr<BrowserItem>& item) {
                                         if (!item) return true;
                                         return !itemMatchesSearch(*item);
                                       }),
                      visibleItems_.end());

    std::sort(visibleItems_.begin(), visibleItems_.end(), [this](const auto& a, const auto& b) {
      if (!a || !b) return (bool)a;
      if (sortMode_ == SortMode::Type && (int)a->type != (int)b->type) return (int)a->type < (int)b->type;
      return a->name.compareNatural(b->name) < 0;
    });
  }

  if (visibleItems_.empty()) {
    selectedIndex_ = -1;
  } else {
    selectedIndex_ = juce::jlimit(0, (int) visibleItems_.size() - 1, selectedIndex_);
  }

  hoveredIndex_ = -1;
  const int maxVisible = std::max(1, listBodyRect_.getHeight() / rowHeight_);
  const int maxScroll = std::max(0, (int) visibleItems_.size() - maxVisible);
  scrollOffset_ = juce::jlimit(0, maxScroll, scrollOffset_);
}

void BrowserPanel::selectItem(int index) {
  if (index < 0 || index >= (int) visibleItems_.size()) return;
  const bool hadPreviewable = selectedIndex_ >= 0 &&
                              selectedIndex_ < (int)visibleItems_.size() &&
                              visibleItems_[(size_t)selectedIndex_] &&
                              (visibleItems_[(size_t)selectedIndex_]->type == BrowserItemType::AudioFile ||
                               visibleItems_[(size_t)selectedIndex_]->type == BrowserItemType::MidiFile);
  selectedIndex_ = index;
  const auto& item = visibleItems_[(size_t) selectedIndex_];
  previewWaveform_.clear();
  previewWaveformPath_.clear();
  previewMidiNotes_.clear();
  previewMidiDuration_ = 0.0f;
  instrumentPreviewPlaying_ = false;
  instrumentPreviewPausedPos_ = 0.0f;
  instrumentPreviewStartMs_ = 0;

  if (!item) {
    previewEngine_.stop();
    return;
  }

  if (item->type == BrowserItemType::AudioFile) {
    const juce::File f(item->id);
    previewEngine_.setAutoPlayEnabled(true);
    previewEngine_.loadFile(f, true);
    requestWaveformForFile(f);
    model_.addToRecent(item);
  } else if (item->type == BrowserItemType::MidiFile) {
    const juce::File f(item->id);
    loadMidiPreviewForFile(f);
    model_.addToRecent(item);
    previewEngine_.stop();
  } else if (item->type == BrowserItemType::Instrument) {
    setupInstrumentPreviewPattern();
    model_.addToRecent(item);
    previewEngine_.stop();
  } else {
    previewEngine_.stop();
  }

  const bool hasPreviewable = item &&
                              (item->type == BrowserItemType::AudioFile ||
                               item->type == BrowserItemType::MidiFile);
  if (hadPreviewable != hasPreviewable) {
    resized();
  }
  repaint(previewRect_);
}

void BrowserPanel::updatePreviewVolumeFromX(int x) {
  if (previewVolumeRect_.getWidth() <= 4) return;
  const float v = juce::jlimit(0.0f, 1.0f, (float)(x - previewVolumeRect_.getX()) / (float) previewVolumeRect_.getWidth());
  previewEngine_.setVolume(v);
  savePreviewPrefs();
  repaint(previewRect_);
}

void BrowserPanel::requestWaveformForFile(const juce::File& file) {
  if (!file.existsAsFile()) return;
  previewWaveformPath_ = file.getFullPathName();
  if (waveformLoader_.isCached(previewWaveformPath_)) {
    previewWaveform_ = waveformLoader_.getCached(previewWaveformPath_);
    repaint(previewRect_);
    return;
  }

  juce::Component::SafePointer<BrowserPanel> safeThis(this);
  waveformLoader_.request(previewWaveformPath_,
                          [safeThis](const juce::String& path, const std::vector<float>& peaks) {
    if (safeThis == nullptr) return;
    if (safeThis->previewWaveformPath_ != path) return;
    safeThis->previewWaveform_ = peaks;
    safeThis->repaint(safeThis->previewRect_);
  });

  // Fallback extractor to guarantee preview even if async loader is slow/blocked.
  juce::Thread::launch([safeThis, path = previewWaveformPath_]() {
    if (safeThis == nullptr) return;
    juce::File f(path);
    if (!f.existsAsFile()) return;

    juce::AudioFormatManager fm;
    fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(fm.createReaderFor(f));
    if (!reader) return;

    constexpr int kPoints = 96;
    std::vector<float> peaks;
    peaks.reserve(kPoints);
    const juce::int64 len = reader->lengthInSamples;
    if (len <= 0) return;
    const juce::int64 step = std::max<juce::int64>(1, len / kPoints);
    juce::AudioBuffer<float> buf(1, 2048);

    for (int i = 0; i < kPoints; ++i) {
      const juce::int64 start = i * step;
      const int n = (int)std::min<juce::int64>((juce::int64)buf.getNumSamples(), len - start);
      if (n <= 0) break;
      buf.clear();
      reader->read(&buf, 0, n, start, true, false);
      float maxV = 0.0f;
      const float* d = buf.getReadPointer(0);
      for (int s = 0; s < n; ++s) maxV = juce::jmax(maxV, std::abs(d[s]));
      peaks.push_back(maxV);
    }

    juce::MessageManager::callAsync([safeThis, path, peaks = std::move(peaks)]() {
      if (safeThis == nullptr) return;
      if (safeThis->previewWaveformPath_ != path) return;
      if (safeThis->previewWaveform_.empty() && !peaks.empty()) {
        safeThis->previewWaveform_ = peaks;
        safeThis->repaint(safeThis->previewRect_);
      }
    });
  });
}

void BrowserPanel::loadMidiPreviewForFile(const juce::File& file) {
  previewMidiNotes_.clear();
  previewMidiDuration_ = 0.0f;
  if (!file.existsAsFile()) return;

  juce::FileInputStream stream(file);
  if (!stream.openedOk()) return;

  juce::MidiFile midi;
  if (!midi.readFrom(stream)) return;

  midi.convertTimestampTicksToSeconds();
  for (int t = 0; t < midi.getNumTracks(); ++t) {
    const auto* seqIn = midi.getTrack(t);
    if (!seqIn) continue;
    juce::MidiMessageSequence seq(*seqIn);
    seq.updateMatchedPairs();

    for (int i = 0; i < seq.getNumEvents(); ++i) {
      const auto* ev = seq.getEventPointer(i);
      if (!ev) continue;
      const auto& msg = ev->message;
      const double ts = msg.getTimeStamp();
      previewMidiDuration_ = juce::jmax(previewMidiDuration_, (float)ts);

      if (msg.isNoteOn() && ev->noteOffObject != nullptr) {
        const float start = (float)ts;
        const float end = (float)ev->noteOffObject->message.getTimeStamp();
        const float dur = juce::jmax(0.02f, end - start);
        previewMidiNotes_.push_back({start, dur, msg.getNoteNumber()});
      }
    }
  }

  if (previewMidiDuration_ <= 0.0f) previewMidiDuration_ = 1.0f;
}

void BrowserPanel::setupInstrumentPreviewPattern() {
  previewMidiNotes_.clear();
  previewMidiDuration_ = 2.0f;

  // Simple deterministic audition phrase for instrument preview.
  previewMidiNotes_.push_back({0.00f, 0.22f, 60});
  previewMidiNotes_.push_back({0.26f, 0.22f, 64});
  previewMidiNotes_.push_back({0.52f, 0.22f, 67});
  previewMidiNotes_.push_back({0.78f, 0.22f, 72});
  previewMidiNotes_.push_back({1.04f, 0.22f, 67});
  previewMidiNotes_.push_back({1.30f, 0.22f, 64});
  previewMidiNotes_.push_back({1.56f, 0.22f, 60});
}

void BrowserPanel::loadPreviewPrefs() {
  if (!previewPrefsFile_.existsAsFile()) return;
  juce::StringArray lines;
  lines.addLines(previewPrefsFile_.loadFileAsString());
  if (lines.size() >= 1) {
    const juce::String a = lines[0].trim();
    previewEngine_.setAutoPlayEnabled(a == "1" || a.equalsIgnoreCase("true"));
  }
  if (lines.size() >= 2) {
    const float v = (float) lines[1].trim().getDoubleValue();
    previewEngine_.setVolume(juce::jlimit(0.0f, 1.0f, v));
  }
}

void BrowserPanel::savePreviewPrefs() const {
  juce::StringArray lines;
  lines.add(previewEngine_.isAutoPlayEnabled() ? "1" : "0");
  lines.add(juce::String(previewEngine_.getVolume(), 4));
  previewPrefsFile_.getParentDirectory().createDirectory();
  previewPrefsFile_.replaceWithText(lines.joinIntoString("\n"));
}

void BrowserPanel::resized() {
  auto b = getLocalBounds().reduced((int)design::spacing::SM, (int)design::spacing::XS);
  const int panelW = b.getWidth();
  const bool narrow = panelW < 340;
  const int headerH = 36;
  const int sectionW = panelW < 320 ? 108 : 136;
  int previewH = narrow ? 64 : 84;
  if (selectedIndex_ >= 0 && selectedIndex_ < (int)visibleItems_.size() &&
      visibleItems_[(size_t)selectedIndex_]) {
    const auto type = visibleItems_[(size_t)selectedIndex_]->type;
    if (type == BrowserItemType::AudioFile || type == BrowserItemType::MidiFile)
      previewH = narrow ? 140 : 190;
  }

  auto top = b.removeFromTop(headerH);
  auto topRow = top;
  auto controls = topRow.removeFromRight(narrow ? 124 : 184);
  densityRect_ = controls.removeFromRight(38).reduced(2, 4);
  sortRect_ = controls.removeFromRight(58).reduced(2, 4);
  removeSourceRect_ = controls.removeFromRight(38).reduced(2, 4);
  addSourceRect_ = controls.removeFromRight(38).reduced(2, 4);

  searchRect_ = top;
  auto searchArea = searchRect_.reduced(2, 3);
  clearSearchRect_ = searchArea.removeFromRight(30).reduced(0, 2);
  searchInputRect_ = searchArea;
  searchInputRect_.removeFromLeft(6);
  sourceStripRect_ = juce::Rectangle<int>(); // Removed source chips to save space

  auto content = b;
  previewRect_ = content.removeFromBottom(previewH).reduced(2, 2);

  sectionRailRect_ = content.removeFromLeft(sectionW).reduced(0, 2);
  resultsRect_ = content.reduced(2, 2);

  const bool multiColumn = resultsRect_.getWidth() >= 300;
  listHeaderRect_ = multiColumn ? resultsRect_.removeFromTop(20) : juce::Rectangle<int>();
  listBodyRect_ = resultsRect_;
  if (multiColumn) {
    colNameRect_ = listHeaderRect_.removeFromLeft((int)(listHeaderRect_.getWidth() * 0.58f));
    colTypeRect_ = listHeaderRect_.removeFromLeft((int)(listHeaderRect_.getWidth() * 0.20f));
    colSourceRect_ = listHeaderRect_;
  } else {
    colNameRect_ = listBodyRect_;
    colTypeRect_ = {};
    colSourceRect_ = {};
  }

  const bool hasSelection = selectedIndex_ >= 0 && selectedIndex_ < (int)visibleItems_.size() &&
                            visibleItems_[(size_t)selectedIndex_];
  const bool previewableSelection = hasSelection &&
                                    (visibleItems_[(size_t)selectedIndex_]->type == BrowserItemType::AudioFile ||
                                     visibleItems_[(size_t)selectedIndex_]->type == BrowserItemType::MidiFile ||
                                     visibleItems_[(size_t)selectedIndex_]->type == BrowserItemType::Instrument);
  if (previewableSelection) {
    const int btnW = panelW < 340 ? 58 : 72;
    const int btnH = 22;
    const int controlsY = previewRect_.getY() + 8;
    const int controlsX = previewRect_.getX() + 10;
    previewPlayRect_ = {controlsX, controlsY, btnW, btnH};
    previewStopRect_ = {previewPlayRect_.getRight() + 8, controlsY, btnW, btnH};

    // Optional controls; never allowed to push play/stop away.
    showPreviewToggle_ = previewRect_.getWidth() >= 300;
    autoPlayToggleRect_ = showPreviewToggle_
                              ? juce::Rectangle<int>(previewStopRect_.getRight() + 10, controlsY, 24, btnH)
                              : juce::Rectangle<int>();

    const int volW = juce::jmin(150, juce::jmax(56, previewRect_.getWidth() / 3));
    previewVolumeRect_ = juce::Rectangle<int>(previewRect_.getRight() - volW - 10, controlsY, volW, btnH);
  } else {
    showPreviewToggle_ = false;
    autoPlayToggleRect_ = {};
    previewPlayRect_ = {};
    previewStopRect_ = {};
    previewVolumeRect_ = {};
  }

  sectionItemBounds_.clear();
  auto sectionArea = sectionRailRect_.reduced(8, 8);
  for (const auto& s : sections_) {
    juce::ignoreUnused(s);
    if (sectionArea.getHeight() < 30) break;
    sectionItemBounds_.push_back(sectionArea.removeFromTop(30));
    sectionArea.removeFromTop(4);
  }

  rowBounds_.clear();
  disclosureBounds_.clear();
  rowPlayBtnBounds_.clear();
  auto listArea = listBodyRect_;
  while (listArea.getHeight() >= rowHeight_) {
    rowBounds_.push_back(listArea.removeFromTop(rowHeight_));
    disclosureBounds_.push_back({});
    rowPlayBtnBounds_.push_back({});
  }

  sourceChipBounds_.clear();

  if (folderChooser_) {
    folderChooser_->setBounds(getLocalBounds().reduced(24));
    folderChooser_->toFront(true);
  }

  rebuildVisibleItems();
}

void BrowserPanel::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds();
  const float w = (float) bounds.getWidth();
  const float h = (float) bounds.getHeight();

  SkPaint bg;
  bg.setAntiAlias(true);
  bg.setColor(design::colors::BG_01);
  canvas->drawRect(SkRect::MakeWH(w, h), bg);

  drawRoundedStroke(canvas,
                    juce::Rectangle<int>(0, 0, (int)w, (int)h).reduced(1),
                    design::colors::BORDER_SUBTLE,
                    design::dimensions::RADIUS_SM);

  drawRoundedFill(canvas, sectionRailRect_, design::colors::BG_02, design::dimensions::RADIUS_SM);
  drawRoundedFill(canvas, resultsRect_, design::colors::BG_02, design::dimensions::RADIUS_SM);
  const bool showColumns = !colTypeRect_.isEmpty();
  if (showColumns)
    drawRoundedFill(canvas, listHeaderRect_, design::colors::BG_03, design::dimensions::RADIUS_SM);
  drawRoundedFill(canvas, previewRect_, design::colors::BG_02, design::dimensions::RADIUS_SM);
  drawRoundedStroke(canvas, sectionRailRect_, design::colors::BORDER_SUBTLE, design::dimensions::RADIUS_SM);
  drawRoundedStroke(canvas, resultsRect_, design::colors::BORDER_SUBTLE, design::dimensions::RADIUS_SM);
  drawRoundedStroke(canvas, previewRect_, design::colors::BORDER_SUBTLE, design::dimensions::RADIUS_SM);

  SkFont titleFont = design::getSkFont(11.0f, design::FontWeight::SemiBold);
  SkFont sectionFont = design::getSkFont(9.5f, design::FontWeight::Medium);
  SkFont rowFont = design::getSkFont(10.0f, design::FontWeight::Medium);
  SkFont metaFont = design::getSkFont(9.0f, design::FontWeight::Regular);

  SkPaint text;
  text.setAntiAlias(true);
  text.setColor(design::colors::TEXT_PRIMARY);
  canvas->drawString("Browser", (float)searchInputRect_.getX(), (float)addSourceRect_.getY() + 11.0f, titleFont, text);

  auto drawButton = [&](const juce::Rectangle<int>& r, const juce::String& label) {
    drawRoundedFill(canvas, r, design::colors::BG_03, design::dimensions::RADIUS_SM);
    drawRoundedStroke(canvas, r, design::colors::BORDER_DEFAULT, design::dimensions::RADIUS_SM);
    SkPaint t;
    t.setAntiAlias(true);
    t.setColor(design::colors::TEXT_SECONDARY);
    canvas->drawString(label.toStdString().c_str(), (float)r.getCentreX() - (float)(label.length() * 2.6f),
                       (float)r.getCentreY() + 3.0f, metaFont, t);
  };

  const bool narrow = getWidth() < 340;
  drawButton(addSourceRect_, "+");
  if (!narrow) drawButton(removeSourceRect_, "-");
  drawButton(sortRect_, sortMode_ == SortMode::Name ? "A-Z" : sortMode_ == SortMode::Type ? "Type" : "Recent");
  drawButton(densityRect_, compactDensity_ ? "C" : "D");

  drawRoundedFill(canvas, searchInputRect_, design::colors::BG_00, design::dimensions::RADIUS_SM);
  drawRoundedStroke(canvas,
                    searchInputRect_,
                    searchFocused_ ? design::colors::BORDER_FOCUS : design::colors::BORDER_DEFAULT,
                    design::dimensions::RADIUS_SM);

  icons::IconStyle searchStyle;
  searchStyle.color = design::withAlpha(design::colors::TEXT_SECONDARY, searchText_.isEmpty() ? 0.8f : 1.0f);
  searchStyle.strokeWidth = 1.8f;
  icons::drawIconCentered(canvas,
                          icons::Search(),
                          SkRect::MakeXYWH((float)searchInputRect_.getX() + 5.0f,
                                           (float)searchInputRect_.getY() + 3.0f,
                                           16.0f,
                                           16.0f),
                          12.0f,
                          searchStyle);

  const juce::String shown = searchText_.isEmpty() ? "Search library..." : searchText_;
  SkPaint searchPaint;
  searchPaint.setAntiAlias(true);
  searchPaint.setColor(searchText_.isEmpty() ? design::colors::TEXT_TERTIARY : design::colors::TEXT_PRIMARY);
  canvas->drawString(shown.toStdString().c_str(), (float)searchInputRect_.getX() + 24.0f,
                     (float)searchInputRect_.getCentreY() + 3.0f, sectionFont, searchPaint);

  if (searchText_.isNotEmpty()) {
      drawRoundedFill(canvas, clearSearchRect_, design::colors::BG_03, design::dimensions::RADIUS_FULL);
      SkPaint xPaint;
      xPaint.setAntiAlias(true);
      xPaint.setColor(design::colors::TEXT_SECONDARY);
      xPaint.setStrokeWidth(1.4f);
      xPaint.setStyle(SkPaint::kStroke_Style);
      const float cx = (float) clearSearchRect_.getCentreX();
      const float cy = (float) clearSearchRect_.getCentreY();
      canvas->drawLine(cx - 3.0f, cy - 3.0f, cx + 3.0f, cy + 3.0f, xPaint);
      canvas->drawLine(cx + 3.0f, cy - 3.0f, cx - 3.0f, cy + 3.0f, xPaint);
  }

  // ... (rest of the drawing code)

  if (!sourceChipBounds_.empty()) {
    const auto sources = model_.getUserLibraryPaths();
    for (size_t i = 0; i < sourceChipBounds_.size() && i < (size_t) sources.size(); ++i) {
      const auto& r = sourceChipBounds_[i];
      const bool active = (int) i == selectedSourceIndex_;
      drawRoundedFill(canvas, r, active ? SkColorSetARGB(120, 111, 170, 245)
                                        : SkColorSetARGB(65, 255, 255, 255), 6.0f);
      SkPaint t;
      t.setAntiAlias(true);
      t.setColor(active ? SkColorSetRGB(237, 246, 255) : SkColorSetRGB(185, 202, 225));
      const juce::String label = ellipsize(juce::File(sources[(int) i]).getFileName(), 16);
      canvas->drawString(label.toStdString().c_str(), (float)r.getX() + 8.0f, (float)r.getCentreY() + 4.0f, metaFont, t);
    }
  }

  for (size_t i = 0; i < sectionItemBounds_.size() && i < sections_.size(); ++i) {
    const auto& r = sectionItemBounds_[i];
    const bool active = sections_[i].first == activeSection_;
    if (active) drawRoundedFill(canvas, r, design::withAlpha(design::colors::ACCENT_PRIMARY, 0.16f), design::dimensions::RADIUS_SM);
    drawSectionIcon(canvas, r, (int) i, active ? design::colors::TEXT_PRIMARY : design::colors::TEXT_SECONDARY, active);

    SkPaint labelPaint;
    labelPaint.setAntiAlias(true);
    labelPaint.setColor(active ? design::colors::TEXT_PRIMARY : design::colors::TEXT_SECONDARY);
    const int maxChars = juce::jmax(6, (r.getWidth() - 34) / 6);
    const juce::String label = ellipsize(sections_[i].second, maxChars);
    canvas->drawString(label.toStdString().c_str(),
                       (float)r.getX() + 30.0f,
                       (float)r.getCentreY() + 3.0f,
                       sectionFont,
                       labelPaint);
  }

  SkPaint headerPaint;
  headerPaint.setAntiAlias(true);
  headerPaint.setColor(design::colors::TEXT_SECONDARY);
  if (showColumns) {
    canvas->drawString("Name", (float)colNameRect_.getX() + 8.0f, (float)colNameRect_.getBottom() - 7.0f, metaFont, headerPaint);
    canvas->drawString("Type", (float)colTypeRect_.getX() + 8.0f, (float)colTypeRect_.getBottom() - 7.0f, metaFont, headerPaint);
    canvas->drawString("Source", (float)colSourceRect_.getX() + 8.0f, (float)colSourceRect_.getBottom() - 7.0f, metaFont, headerPaint);

    SkPaint colLine;
    colLine.setAntiAlias(true);
    colLine.setColor(design::colors::BORDER_SUBTLE);
    colLine.setStrokeWidth(1.0f);
    canvas->drawLine((float)colNameRect_.getRight(), (float)listHeaderRect_.getY(), (float)colNameRect_.getRight(), (float)listBodyRect_.getBottom(), colLine);
    canvas->drawLine((float)colTypeRect_.getRight(), (float)listHeaderRect_.getY(), (float)colTypeRect_.getRight(), (float)listBodyRect_.getBottom(), colLine);
  }

  if (visibleItems_.empty()) {
    SkPaint t;
    t.setAntiAlias(true);
    t.setColor(SkColorSetARGB(210, 166, 186, 220));
    canvas->drawString("No matches.", (float)listBodyRect_.getX() + 14.0f, (float)listBodyRect_.getY() + 20.0f, rowFont, t);
  } else {
    const int start = scrollOffset_;
    const int end = std::min(start + (int)rowBounds_.size(), (int) visibleItems_.size());
    for (int i = start; i < end; ++i) {
      const int row = i - start;
      const auto& r = rowBounds_[(size_t) row];
      const auto& item = visibleItems_[(size_t) i];
      if (!item) continue;

      const bool selected = (i == selectedIndex_);
      const bool hovered = (i == hoveredIndex_);
      const auto rowRect = r.reduced(0, 0);
      drawRoundedFill(canvas, rowRect,
                      selected ? design::withAlpha(design::colors::ACCENT_PRIMARY, 0.18f)
                               : hovered ? design::withAlpha(design::colors::BG_04, 0.95f)
                                         : design::colors::BG_02,
                      design::dimensions::RADIUS_SM);
      drawRoundedStroke(canvas, rowRect, design::colors::BORDER_SUBTLE, design::dimensions::RADIUS_SM);

      int depth = 0;
      bool isFolder = item->isDirectory;
      bool expanded = false;
      if (activeSection_ == Section::All && i < (int) treeRows_.size()) {
        depth = treeRows_[(size_t)i].depth;
        isFolder = treeRows_[(size_t)i].isFolder;
        expanded = treeRows_[(size_t)i].expanded;
      }

      const int indentX = colNameRect_.getX() + 6 + depth * 10;
      if (isFolder) {
        auto disc = juce::Rectangle<int>(indentX, r.getY() + (rowHeight_ / 2) - 5, 10, 10);
        if ((size_t)row < disclosureBounds_.size()) disclosureBounds_[(size_t)row] = disc;
        SkPath tri;
        if (expanded) {
          tri.moveTo((float)disc.getX() + 1.0f, (float)disc.getY() + 2.0f);
          tri.lineTo((float)disc.getX() + 9.0f, (float)disc.getY() + 2.0f);
          tri.lineTo((float)disc.getX() + 5.0f, (float)disc.getY() + 8.0f);
        } else {
          tri.moveTo((float)disc.getX() + 2.0f, (float)disc.getY() + 1.0f);
          tri.lineTo((float)disc.getX() + 8.0f, (float)disc.getY() + 5.0f);
          tri.lineTo((float)disc.getX() + 2.0f, (float)disc.getY() + 9.0f);
        }
        tri.close();
        SkPaint p;
        p.setAntiAlias(true);
        p.setColor(SkColorSetRGB(180, 200, 228));
        p.setStyle(SkPaint::kFill_Style);
        canvas->drawPath(tri, p);
      }

      icons::IconStyle itemIconStyle;
      itemIconStyle.color = typeColor(item->type);
      itemIconStyle.strokeWidth = 1.6f;
      icons::drawIconCentered(
          canvas,
          itemTypeIcon(*item),
          SkRect::MakeXYWH((float)indentX + (isFolder ? 9.0f : 1.0f),
                           (float)r.getY() + 6.0f,
                           16.0f,
                           16.0f),
          12.0f,
          itemIconStyle);

      SkPaint primary;
      primary.setAntiAlias(true);
      primary.setColor(design::colors::TEXT_PRIMARY);
      SkPaint secondary;
      secondary.setAntiAlias(true);
      secondary.setColor(design::colors::TEXT_SECONDARY);

      const int nameChars = juce::jmax(10, (colNameRect_.getWidth() - depth * 10 - 42) / 6);
      const juce::String safeName = ellipsize(item->name, nameChars);
      const float textY = (float)r.getCentreY() + 3.0f;
      canvas->drawString(safeName.toStdString().c_str(),
                         (float)indentX + (isFolder ? 24.0f : 14.0f), textY, rowFont, primary);

      if (showColumns) {
        const juce::String typeText = BrowserItem::getEnumName(item->type);
        canvas->drawString(typeText.toStdString().c_str(), (float)colTypeRect_.getX() + 8.0f, textY, metaFont, secondary);

        juce::String sourceText = "-";
        if (item->id.isNotEmpty()) {
          sourceText = juce::File(item->id).getParentDirectory().getFileName();
          if (sourceText.isEmpty()) sourceText = juce::File(item->id).getFileName();
        }
        sourceText = ellipsize(sourceText, juce::jmax(8, (colSourceRect_.getWidth() - 16) / 7));
        canvas->drawString(sourceText.toStdString().c_str(), (float)colSourceRect_.getX() + 8.0f, textY, metaFont, secondary);
      }

      // Inline play/pause button for audio files – shown on hover or when selected.
      if (item->type == BrowserItemType::AudioFile && (hovered || selected)) {
        const int btnSize = rowHeight_ - 10;
        const int btnX = r.getRight() - btnSize - 4;
        const int btnY = r.getCentreY() - btnSize / 2;
        const juce::Rectangle<int> btnRect(btnX, btnY, btnSize, btnSize);
        if ((size_t)row < rowPlayBtnBounds_.size())
          rowPlayBtnBounds_[(size_t)row] = btnRect;

        const bool isPlayingThis = selected && previewEngine_.isPlaying();
        icons::IconStyle st;
        st.color = isPlayingThis ? design::colors::ACCENT_PRIMARY : design::colors::TEXT_PRIMARY;
        st.strokeWidth = 1.5f;
        st.filled = isPlayingThis;
        icons::drawIconButton(canvas,
                              isPlayingThis ? icons::Pause() : icons::Play(),
                              SkRect::MakeXYWH((float)btnX, (float)btnY, (float)btnSize, (float)btnSize),
                              st, hovered && !selected, isPlayingThis);
      } else if ((size_t)row < rowPlayBtnBounds_.size()) {
        rowPlayBtnBounds_[(size_t)row] = {};
      }
    }
  }

  SkPaint pTitle;
  pTitle.setAntiAlias(true);
  pTitle.setColor(design::colors::TEXT_PRIMARY);
  SkPaint pMeta;
  pMeta.setAntiAlias(true);
  pMeta.setColor(design::colors::TEXT_SECONDARY);

  const bool hasSelection = selectedIndex_ >= 0 && selectedIndex_ < (int)visibleItems_.size() &&
                            visibleItems_[(size_t)selectedIndex_];
  if (!hasSelection) {
    SkPaint tp;
    tp.setAntiAlias(true);
    tp.setColor(SkColorSetARGB(190, 176, 198, 226));
    canvas->drawString("Select an audio or MIDI file to preview.",
                       (float)previewRect_.getX() + 10.0f,
                       (float)previewRect_.getCentreY() + 3.0f,
                       metaFont,
                       tp);
    return;
  }
  const auto& sel = visibleItems_[(size_t)selectedIndex_];
  const bool previewableSelection = (sel->type == BrowserItemType::AudioFile ||
                                     sel->type == BrowserItemType::MidiFile ||
                                     sel->type == BrowserItemType::Instrument);

  if (!previewableSelection) {
    SkPaint tp;
    tp.setAntiAlias(true);
    tp.setColor(SkColorSetARGB(190, 176, 198, 226));
    const juce::String hint = "No preview for this item type.";
    canvas->drawString(hint.toStdString().c_str(),
                       (float)previewRect_.getX() + 10.0f,
                       (float)previewRect_.getCentreY() + 3.0f,
                       metaFont,
                       tp);
    return;
  }

  if (showPreviewToggle_ && !autoPlayToggleRect_.isEmpty()) {
    drawRoundedFill(canvas, autoPlayToggleRect_, previewEngine_.isAutoPlayEnabled()
                                         ? design::withAlpha(design::colors::ACCENT_PRIMARY, 0.26f)
                                         : design::colors::BG_03,
                    design::dimensions::RADIUS_SM);
    SkPath autoIcon = previewEngine_.isAutoPlayEnabled() ? icons::Play() : icons::Stop();
    icons::IconStyle st;
    st.color = design::colors::TEXT_PRIMARY;
    st.strokeWidth = 1.4f;
    icons::drawIconCentered(canvas, autoIcon,
                            SkRect::MakeXYWH((float)autoPlayToggleRect_.getX(),
                                             (float)autoPlayToggleRect_.getY(),
                                             (float)autoPlayToggleRect_.getWidth(),
                                             (float)autoPlayToggleRect_.getHeight()),
                            10.0f, st);
  }

  if (!previewPlayRect_.isEmpty()) {
    drawRoundedFill(canvas, previewPlayRect_, design::colors::ACCENT_PRIMARY, design::dimensions::RADIUS_SM);
    drawRoundedStroke(canvas, previewPlayRect_, design::withAlpha(design::colors::BORDER_DEFAULT, 0.8f), design::dimensions::RADIUS_SM);
    SkPaint t;
    t.setAntiAlias(true);
    t.setColor(design::colors::TEXT_INVERSE);
    canvas->drawString("Play", (float)previewPlayRect_.getX() + 15.0f,
                       (float)previewPlayRect_.getCentreY() + 4.0f, metaFont, t);
  }

  if (!previewStopRect_.isEmpty()) {
    drawRoundedFill(canvas, previewStopRect_, design::colors::BG_03, design::dimensions::RADIUS_SM);
    drawRoundedStroke(canvas, previewStopRect_, design::colors::BORDER_DEFAULT, design::dimensions::RADIUS_SM);
    SkPaint t;
    t.setAntiAlias(true);
    t.setColor(design::colors::TEXT_PRIMARY);
    canvas->drawString("Stop", (float)previewStopRect_.getX() + 13.0f,
                       (float)previewStopRect_.getCentreY() + 4.0f, metaFont, t);
  }

  if (!previewVolumeRect_.isEmpty()) {
    drawRoundedFill(canvas, previewVolumeRect_, design::colors::BG_03, design::dimensions::RADIUS_SM);
    const float vol = previewEngine_.getVolume();
    auto fillW = (int)((float)previewVolumeRect_.getWidth() * vol);
    drawRoundedFill(canvas, previewVolumeRect_.withWidth(fillW), design::withAlpha(design::colors::ACCENT_PRIMARY, 0.45f), design::dimensions::RADIUS_SM);
  }

  auto previewDataRect = previewRect_.reduced(8, 8);
  previewDataRect.removeFromTop(38);

  if (sel->type == BrowserItemType::AudioFile) {
    // Hard-pin controls inside waveform area so they're always visible.
    previewPlayRect_ = juce::Rectangle<int>(previewDataRect.getX() + 8, previewDataRect.getY() + 6, 72, 22);
    previewStopRect_ = juce::Rectangle<int>(previewPlayRect_.getRight() + 8, previewDataRect.getY() + 6, 72, 22);

    if (previewWaveform_.empty()) {
      SkPaint tp;
      tp.setAntiAlias(true);
      tp.setColor(SkColorSetARGB(180, 170, 190, 218));
      canvas->drawString("Loading waveform...", (float)previewDataRect.getX() + 4.0f,
                         (float)previewDataRect.getCentreY() + 3.0f, metaFont, tp);

      // Draw controls even while waveform is loading.
      drawRoundedFill(canvas, previewPlayRect_, design::colors::ACCENT_PRIMARY, design::dimensions::RADIUS_SM);
      drawRoundedFill(canvas, previewStopRect_, design::colors::BG_03, design::dimensions::RADIUS_SM);
      SkPaint t;
      t.setAntiAlias(true);
      t.setColor(design::colors::TEXT_PRIMARY);
      canvas->drawString("Play", (float)previewPlayRect_.getX() + 16.0f,
                         (float)previewPlayRect_.getCentreY() + 4.0f, metaFont, t);
      canvas->drawString("Stop", (float)previewStopRect_.getX() + 15.0f,
                         (float)previewStopRect_.getCentreY() + 4.0f, metaFont, t);
      return;
    }

    SkPaint base;
    base.setAntiAlias(true);
    base.setColor(design::withAlpha(design::colors::BG_04, 0.5f));
    canvas->drawRoundRect(SkRect::MakeXYWH((float)previewDataRect.getX(), (float)previewDataRect.getY(),
                                           (float)previewDataRect.getWidth(), (float)previewDataRect.getHeight()),
                          1.0f, 1.0f, base);

    const float cyWave = (float)previewDataRect.getCentreY();
    SkPaint centerLine;
    centerLine.setAntiAlias(true);
    centerLine.setColor(design::colors::BORDER_SUBTLE);
    centerLine.setStrokeWidth(1.0f);
    canvas->drawLine((float)previewDataRect.getX(), cyWave,
                     (float)previewDataRect.getRight(), cyWave, centerLine);

    const float halfHeight = (float)previewDataRect.getHeight() * 0.42f;
    const float stepX = (float)previewDataRect.getWidth() / (float)juce::jmax(1, (int)previewWaveform_.size());
    SkPaint wave;
    wave.setAntiAlias(true);
    wave.setColor(design::withAlpha(design::colors::ACCENT_PRIMARY, 0.75f));
    wave.setStrokeWidth(1.0f);

    for (size_t i = 0; i < previewWaveform_.size(); ++i) {
      const float x = (float)previewDataRect.getX() + (float)i * stepX;
      const float amp = juce::jlimit(0.0f, 1.0f, previewWaveform_[i]) * halfHeight;
      canvas->drawLine(x, cyWave - amp, x, cyWave + amp, wave);
    }

    const float playPos = previewEngine_.getPlaybackPosition();
    SkPaint playHead;
    playHead.setAntiAlias(true);
    playHead.setColor(SkColorSetRGB(224, 184, 112));
    playHead.setStrokeWidth(1.2f);
    const float px = (float)previewDataRect.getX() + playPos * (float)previewDataRect.getWidth();
    canvas->drawLine(px, (float)previewDataRect.getY(), px, (float)previewDataRect.getBottom(), playHead);

    // Draw overlay controls last so nothing can cover them.
    drawRoundedFill(canvas, previewPlayRect_, design::colors::ACCENT_PRIMARY, design::dimensions::RADIUS_SM);
    drawRoundedFill(canvas, previewStopRect_, design::colors::BG_03, design::dimensions::RADIUS_SM);
    SkPaint t;
    t.setAntiAlias(true);
    t.setColor(design::colors::TEXT_PRIMARY);
    canvas->drawString("Play", (float)previewPlayRect_.getX() + 16.0f,
                       (float)previewPlayRect_.getCentreY() + 4.0f, metaFont, t);
    canvas->drawString("Stop", (float)previewStopRect_.getX() + 15.0f,
                       (float)previewStopRect_.getCentreY() + 4.0f, metaFont, t);
    return;
  }

  if (sel->type == BrowserItemType::MidiFile || sel->type == BrowserItemType::Instrument) {
    SkPaint base;
    base.setAntiAlias(true);
    base.setColor(design::withAlpha(design::colors::BG_04, 0.5f));
    canvas->drawRoundRect(SkRect::MakeXYWH((float)previewDataRect.getX(), (float)previewDataRect.getY(),
                                           (float)previewDataRect.getWidth(), (float)previewDataRect.getHeight()),
                          1.0f, 1.0f, base);

    if (previewMidiNotes_.empty() || previewMidiDuration_ <= 0.0f) {
      SkPaint tp;
      tp.setAntiAlias(true);
      tp.setColor(SkColorSetARGB(180, 170, 190, 218));
      const char* msg = (sel->type == BrowserItemType::Instrument) ? "No instrument preview data" : "No MIDI note data";
      canvas->drawString(msg, (float)previewDataRect.getX() + 4.0f,
                         (float)previewDataRect.getCentreY() + 3.0f, metaFont, tp);
      return;
    }

    int minNote = 127;
    int maxNote = 0;
    for (const auto& n : previewMidiNotes_) {
      minNote = juce::jmin(minNote, n.note);
      maxNote = juce::jmax(maxNote, n.note);
    }
    if (maxNote - minNote < 11) maxNote = minNote + 11;
    const float noteRange = (float)(maxNote - minNote + 1);

    SkPaint lane;
    lane.setAntiAlias(true);
    lane.setColor(design::colors::BORDER_SUBTLE);
    lane.setStrokeWidth(1.0f);
    for (int i = 0; i <= 6; ++i) {
      const float y = (float)previewDataRect.getY() + ((float)i / 6.0f) * (float)previewDataRect.getHeight();
      canvas->drawLine((float)previewDataRect.getX(), y, (float)previewDataRect.getRight(), y, lane);
    }

    SkPaint notePaint;
    notePaint.setAntiAlias(true);
    notePaint.setColor(sel->type == BrowserItemType::Instrument
                           ? design::withAlpha(typeColor(BrowserItemType::Instrument), 0.9f)
                           : design::withAlpha(typeColor(BrowserItemType::MidiFile), 0.9f));
    for (const auto& n : previewMidiNotes_) {
      const float x = (float)previewDataRect.getX() + (n.start / previewMidiDuration_) * (float)previewDataRect.getWidth();
      const float wRect = juce::jmax(2.0f, (n.duration / previewMidiDuration_) * (float)previewDataRect.getWidth());
      const float noteNorm = (float)(n.note - minNote) / noteRange;
      const float y = (float)previewDataRect.getBottom() - noteNorm * (float)previewDataRect.getHeight() - 3.0f;
      canvas->drawRoundRect(SkRect::MakeXYWH(x, y, wRect, 3.0f), 1.0f, 1.0f, notePaint);
    }

    if (sel->type == BrowserItemType::Instrument) {
      float playPos = instrumentPreviewPausedPos_;
      if (instrumentPreviewPlaying_ && previewMidiDuration_ > 0.0f) {
        const juce::uint32 now = juce::Time::getMillisecondCounter();
        const float elapsed = (float)(now - instrumentPreviewStartMs_) / 1000.0f;
        playPos = std::fmod(elapsed, previewMidiDuration_) / previewMidiDuration_;
        instrumentPreviewPausedPos_ = playPos;
      }
      SkPaint playHead;
      playHead.setAntiAlias(true);
      playHead.setColor(SkColorSetRGB(224, 184, 112));
      playHead.setStrokeWidth(1.2f);
      const float px = (float)previewDataRect.getX() + playPos * (float)previewDataRect.getWidth();
      canvas->drawLine(px, (float)previewDataRect.getY(), px, (float)previewDataRect.getBottom(), playHead);
    }
  }
}

void BrowserPanel::mouseDown(const juce::MouseEvent &e) {
  const auto pos = e.getPosition();

  if (showPreviewToggle_ && autoPlayToggleRect_.contains(pos)) {
    previewEngine_.setAutoPlayEnabled(!previewEngine_.isAutoPlayEnabled());
    savePreviewPrefs();
    repaint(previewRect_);
    return;
  }
  if (previewPlayRect_.contains(pos)) {
    const bool hasSelection = selectedIndex_ >= 0 && selectedIndex_ < (int)visibleItems_.size() &&
                              visibleItems_[(size_t)selectedIndex_];
    const auto selectedType = hasSelection ? visibleItems_[(size_t)selectedIndex_]->type : BrowserItemType::Unknown;
    if (selectedType == BrowserItemType::Instrument) {
      instrumentPreviewPlaying_ = true;
      instrumentPreviewStartMs_ = juce::Time::getMillisecondCounter() -
                                  (juce::uint32)(instrumentPreviewPausedPos_ * previewMidiDuration_ * 1000.0f);
    } else {
      previewEngine_.play();
    }
    repaint(previewRect_);
    return;
  }
  if (previewStopRect_.contains(pos)) {
    const bool hasSelection = selectedIndex_ >= 0 && selectedIndex_ < (int)visibleItems_.size() &&
                              visibleItems_[(size_t)selectedIndex_];
    const auto selectedType = hasSelection ? visibleItems_[(size_t)selectedIndex_]->type : BrowserItemType::Unknown;
    const juce::uint32 now = juce::Time::getMillisecondCounter();
    const bool doubleStop = (now - lastStopClickMs_) < 300u;
    if (selectedType == BrowserItemType::Instrument) {
      if (doubleStop) {
        instrumentPreviewPlaying_ = false;
        instrumentPreviewPausedPos_ = 0.0f;
      } else {
        instrumentPreviewPlaying_ = false;
      }
    } else {
      if (doubleStop) {
        previewEngine_.stop(); // resets to start
      } else {
        previewEngine_.pause(); // quick stop without resetting position
      }
    }
    lastStopClickMs_ = now;
    repaint(previewRect_);
    return;
  }
  if (previewVolumeRect_.contains(pos)) {
    draggingPreviewVolume_ = true;
    updatePreviewVolumeFromX(pos.x);
    return;
  }

  if (addSourceRect_.contains(pos)) {
    if (!folderChooser_) {
      folderChooser_ = std::make_unique<SkiaFileChooser>(
          "Add Browser Folder",
          juce::File::getSpecialLocation(juce::File::userHomeDirectory),
          "",
          SkiaFileChooser::Mode::OpenDirectory);
      addAndMakeVisible(folderChooser_.get());
      folderChooser_->setBounds(getLocalBounds().reduced(24));
    }

    folderChooser_->showAsync([this](SkiaFileChooser::Result result,
                                     const juce::File& chosen) {
      if (result == SkiaFileChooser::Result::Approved && chosen.isDirectory()) {
        model_.addUserLibraryPath(chosen.getFullPathName());
        const auto sources = model_.getUserLibraryPaths();
        selectedSourceIndex_ = juce::jmax(0, sources.size() - 1);
      }

      if (folderChooser_) {
        removeChildComponent(folderChooser_.get());
        folderChooser_.reset();
      }
      resized();
      repaint();
    });
    folderChooser_->toFront(true);
    return;
  }

  if (removeSourceRect_.contains(pos)) {
    const auto sources = model_.getUserLibraryPaths();
    if (selectedSourceIndex_ >= 0 && selectedSourceIndex_ < sources.size()) {
      model_.removeUserLibraryPath(sources[selectedSourceIndex_]);
      selectedSourceIndex_ = juce::jmin(selectedSourceIndex_, model_.getUserLibraryPaths().size() - 1);
      resized();
      repaint();
    }
    return;
  }

  if (sortRect_.contains(pos)) {
    sortMode_ = (sortMode_ == SortMode::Name) ? SortMode::Type
             : (sortMode_ == SortMode::Type) ? SortMode::Recent
                                             : SortMode::Name;
    rebuildVisibleItems();
    repaint();
    return;
  }

  if (densityRect_.contains(pos)) {
    compactDensity_ = !compactDensity_;
    rowHeight_ = compactDensity_ ? 32 : 40;
    resized();
    repaint();
    return;
  }

  for (size_t i = 0; i < sourceChipBounds_.size(); ++i) {
    if (sourceChipBounds_[i].contains(pos)) {
      selectedSourceIndex_ = ((int) i == selectedSourceIndex_) ? -1 : (int) i;
      rebuildVisibleItems();
      repaint();
      return;
    }
  }

  searchFocused_ = searchInputRect_.contains(pos);
  if (searchFocused_) grabKeyboardFocus();

  if (clearSearchRect_.contains(pos)) {
    searchText_.clear();
    rebuildVisibleItems();
    repaint();
    return;
  }

  for (size_t i = 0; i < sectionItemBounds_.size() && i < sections_.size(); ++i) {
    if (sectionItemBounds_[i].contains(pos)) {
      activeSection_ = sections_[i].first;
      scrollOffset_ = 0;
      rebuildVisibleItems();
      repaint();
      return;
    }
  }

  for (size_t row = 0; row < rowBounds_.size(); ++row) {
    if (!rowBounds_[row].contains(pos)) continue;

    const int index = scrollOffset_ + (int) row;
    if (index < 0 || index >= (int) visibleItems_.size()) return;

    if (activeSection_ == Section::All && row < disclosureBounds_.size() && disclosureBounds_[row].contains(pos)) {
      const auto& tr = treeRows_[(size_t) index];
      if (tr.isFolder && tr.item) {
        if (expandedFolderIds_.count(tr.item->id)) expandedFolderIds_.erase(tr.item->id);
        else expandedFolderIds_.insert(tr.item->id);
        rebuildVisibleItems();
        repaint(listBodyRect_);
        return;
      }
    }

    // Inline play/pause button hit
    if (row < rowPlayBtnBounds_.size() && !rowPlayBtnBounds_[row].isEmpty()
        && rowPlayBtnBounds_[row].contains(pos)
        && visibleItems_[(size_t)index]->type == BrowserItemType::AudioFile) {
      if (selectedIndex_ != index) {
        selectItem(index); // auto-plays on load
      } else {
        if (previewEngine_.isPlaying()) previewEngine_.pause();
        else previewEngine_.play();
      }
      repaint(listBodyRect_);
      repaint(previewRect_);
      return;
    }

    const bool doubleClick = (selectedIndex_ == index && e.getNumberOfClicks() >= 2);
    selectItem(index);
    repaint();
    if (doubleClick && onItemDoubleClicked) onItemDoubleClicked(visibleItems_[(size_t) index]);
    return;
  }
}

void BrowserPanel::mouseDrag(const juce::MouseEvent &e) {
  if (draggingPreviewVolume_) {
    updatePreviewVolumeFromX(e.getPosition().x);
  }
}

void BrowserPanel::mouseMove(const juce::MouseEvent &e) {
  hoveredIndex_ = -1;
  const auto pos = e.getPosition();
  for (size_t row = 0; row < rowBounds_.size(); ++row) {
    if (rowBounds_[row].contains(pos)) {
      const int index = scrollOffset_ + (int) row;
      if (index >= 0 && index < (int) visibleItems_.size()) hoveredIndex_ = index;
      break;
    }
  }
  repaint(listBodyRect_);
}

void BrowserPanel::mouseWheelMove(const juce::MouseEvent &, const juce::MouseWheelDetails &wheel) {
  if (previewVolumeRect_.contains(getMouseXYRelative())) {
    const float step = wheel.deltaY > 0.0f ? 0.03f : -0.03f;
    previewEngine_.setVolume(juce::jlimit(0.0f, 1.0f, previewEngine_.getVolume() + step));
    savePreviewPrefs();
    repaint(previewRect_);
    return;
  }
  if (visibleItems_.empty() || rowBounds_.empty()) return;
  const int maxVisible = (int) rowBounds_.size();
  const int maxScroll = std::max(0, (int) visibleItems_.size() - maxVisible);
  const int delta = (wheel.deltaY > 0.0f) ? -1 : 1;
  scrollOffset_ = juce::jlimit(0, maxScroll, scrollOffset_ + delta);
  repaint(listBodyRect_);
}

void BrowserPanel::mouseExit(const juce::MouseEvent &) {
  hoveredIndex_ = -1;
  draggingPreviewVolume_ = false;
  repaint(listBodyRect_);
}

bool BrowserPanel::keyPressed(const juce::KeyPress &key) {
  if (key.getModifiers().isCommandDown() && (key.getTextCharacter() == 'f' || key.getTextCharacter() == 'F')) {
    searchFocused_ = true;
    grabKeyboardFocus();
    repaint(searchRect_);
    return true;
  }

  if (key == juce::KeyPress::upKey) {
    if (selectedIndex_ > 0) {
      selectItem(selectedIndex_ - 1);
      if (selectedIndex_ < scrollOffset_) scrollOffset_ = selectedIndex_;
      repaint(listBodyRect_);
    }
    return true;
  }

  if (key == juce::KeyPress::downKey) {
    if (selectedIndex_ + 1 < (int) visibleItems_.size()) {
      selectItem(selectedIndex_ + 1);
      const int visibleCount = juce::jmax(1, (int) rowBounds_.size());
      if (selectedIndex_ >= scrollOffset_ + visibleCount) scrollOffset_ = selectedIndex_ - visibleCount + 1;
      repaint(listBodyRect_);
    }
    return true;
  }

  if (key == juce::KeyPress::leftKey && activeSection_ == Section::All && selectedIndex_ >= 0 && selectedIndex_ < (int) treeRows_.size()) {
    const auto& row = treeRows_[(size_t) selectedIndex_];
    if (row.isFolder && row.expanded && row.item) {
      expandedFolderIds_.erase(row.item->id);
      rebuildVisibleItems();
      repaint(listBodyRect_);
      return true;
    }
  }

  if (key == juce::KeyPress::rightKey && activeSection_ == Section::All && selectedIndex_ >= 0 && selectedIndex_ < (int) treeRows_.size()) {
    const auto& row = treeRows_[(size_t) selectedIndex_];
    if (row.isFolder && !row.expanded && row.item) {
      expandedFolderIds_.insert(row.item->id);
      rebuildVisibleItems();
      repaint(listBodyRect_);
      return true;
    }
  }

  if (!searchFocused_) return false;

  if (key == juce::KeyPress::escapeKey) {
    searchFocused_ = false;
    repaint(searchRect_);
    return true;
  }

  if (key == juce::KeyPress::backspaceKey) {
    if (searchText_.isNotEmpty()) {
      searchText_ = searchText_.dropLastCharacters(1);
      rebuildVisibleItems();
      repaint();
    }
    return true;
  }

  if (key == juce::KeyPress::returnKey) {
    if (selectedIndex_ >= 0 && selectedIndex_ < (int) visibleItems_.size() &&
        onItemDoubleClicked && visibleItems_[(size_t) selectedIndex_]) {
      onItemDoubleClicked(visibleItems_[(size_t) selectedIndex_]);
    }
    return true;
  }

  const juce::juce_wchar c = key.getTextCharacter();
  if (c >= 32 && c != 127) {
    searchText_ += juce::String::charToString(c);
    rebuildVisibleItems();
    repaint();
    return true;
  }

  return false;
}

} // namespace zenith

#endif // ZENITH_USE_SKIA
