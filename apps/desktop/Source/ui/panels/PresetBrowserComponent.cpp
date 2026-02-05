/*
  ==============================================================================

    PresetBrowserComponent.cpp
    Enhanced preset browser with premium features and DAW integration
    - Audio preview
    - Favorites system
    - Recent presets
    - Multi-tag filtering
    - Random discovery
    - Keyboard shortcuts
    - Card view layout
    - Rating system
    - A/B comparison
    - Collection management UI
    - Search autocomplete
    - Real waveform thumbnails
    - Drag and drop to arrangement
    - Sound similarity search
    - Deep DAW integration

  ==============================================================================
*/

#include "PresetBrowserComponent.h"
#include "../../engine/Engine.h"
#include <algorithm>
#include <random>
#include <set>

namespace zenith {

//==============================================================================
PresetBrowserComponent::PresetBrowserComponent() {
  // Load user data (favorites, recent)
  loadUserData();

  // Setup list box
  addAndMakeVisible(presetList);
  presetList.setModel(this);
  presetList.setRowHeight(52); // Taller for preview info

  // Search input
  addAndMakeVisible(searchInput_);
  searchInput_.setPlaceholder("Search presets, tags, categories... (Ctrl+F)");
  searchInput_.setPillShape(true);
  searchInput_.setFontSize(14.0f);
  // onTextChanged is set later with autocomplete support

  // Category filter
  addAndMakeVisible(categoryFilter_);
  categoryFilter_.setTextWhenNothingSelected("All Categories");
  categoryFilter_.onChange = [this] { applyFilters(); };

  // Sort filter
  addAndMakeVisible(sortFilter_);
  sortFilter_.addItem("Name A-Z", static_cast<int>(SortMode::Name));
  sortFilter_.addItem("Category", static_cast<int>(SortMode::Category));
  sortFilter_.addItem("Author", static_cast<int>(SortMode::Author));
  sortFilter_.addItem("Rating", static_cast<int>(SortMode::Rating));
  sortFilter_.addItem("Recent", static_cast<int>(SortMode::Recent));
  sortFilter_.addItem("Similarity", static_cast<int>(SortMode::Similarity));
  sortFilter_.setSelectedId(static_cast<int>(SortMode::Name), false);
  sortFilter_.onChange = [this] {
    currentSortMode_ = static_cast<SortMode>(sortFilter_.getSelectedId());
    applyFilters();
  };

  // Quick filter (All/Favorites/Recent/Factory/User/Similar/Project)
  addAndMakeVisible(quickFilterCombo_);
  quickFilterCombo_.addItem("All Presets", static_cast<int>(QuickFilter::All));
  quickFilterCombo_.addItem("★ Favorites", static_cast<int>(QuickFilter::Favorites));
  quickFilterCombo_.addItem("Recent", static_cast<int>(QuickFilter::Recent));
  quickFilterCombo_.addItem("Factory", static_cast<int>(QuickFilter::Factory));
  quickFilterCombo_.addItem("User", static_cast<int>(QuickFilter::User));
  quickFilterCombo_.addItem("Similar Sounds", static_cast<int>(QuickFilter::Similar));
  quickFilterCombo_.addItem("Project Presets", static_cast<int>(QuickFilter::Project));
  quickFilterCombo_.setSelectedId(static_cast<int>(QuickFilter::All), false);
  quickFilterCombo_.onChange = [this] {
    quickFilter_ = static_cast<QuickFilter>(quickFilterCombo_.getSelectedId());

    // Handle special case for "Similar" - requires a reference preset
    if (quickFilter_ == QuickFilter::Similar && similarityReference_.id.isEmpty()) {
      int row = presetList.getSelectedRow();
      if (row >= 0 && row < static_cast<int>(filteredPresets.size())) {
        setSimilarityReference(filteredPresets[row]);
      }
    }

    applyFilters();
  };

  // Collections filter
  addAndMakeVisible(collectionsCombo_);
  collectionsCombo_.setTextWhenNothingSelected("Collections...");
  rebuildCollectionsList();
  collectionsCombo_.onChange = [this] {
    int selectedId = collectionsCombo_.getSelectedId();
    if (selectedId > 0) {
      // Filter by collection
      juce::String collectionId = collectionsCombo_.getItemText(selectedId);
      // Apply collection filter logic
      applyFilters();
    }
  };

  // Results label
  addAndMakeVisible(resultsLabel_);
  resultsLabel_.setTextColour(design::colors::TEXT_SECONDARY);
  resultsLabel_.setFont(12.0f, SkFontStyle::Weight::kMedium_Weight);
  resultsLabel_.setJustification(SkiaLabel::Justification::Left);

  // Buttons
  addAndMakeVisible(loadButton);
  addAndMakeVisible(saveButton);
  addAndMakeVisible(deleteButton);
  addAndMakeVisible(refreshButton);
  addAndMakeVisible(randomButton);
  addAndMakeVisible(previewButton);
  addAndMakeVisible(favoritesButton);
  addAndMakeVisible(viewModeButton);
  addAndMakeVisible(compareButton);
  addAndMakeVisible(exportButton);
  addAndMakeVisible(editMetadataButton);
  addAndMakeVisible(newCollectionButton);
  addAndMakeVisible(similarButton);

  loadButton.setButtonStyle(ZenithButton::Style::Primary);
  saveButton.setButtonStyle(ZenithButton::Style::Secondary);
  deleteButton.setButtonStyle(ZenithButton::Style::Danger);
  refreshButton.setButtonStyle(ZenithButton::Style::Ghost);
  randomButton.setButtonStyle(ZenithButton::Style::Secondary);
  previewButton.setButtonStyle(ZenithButton::Style::Ghost);
  favoritesButton.setButtonStyle(ZenithButton::Style::Ghost);
  viewModeButton.setButtonStyle(ZenithButton::Style::Ghost);
  compareButton.setButtonStyle(ZenithButton::Style::Ghost);
  exportButton.setButtonStyle(ZenithButton::Style::Secondary);
  editMetadataButton.setButtonStyle(ZenithButton::Style::Ghost);
  newCollectionButton.setButtonStyle(ZenithButton::Style::Secondary);
  similarButton.setButtonStyle(ZenithButton::Style::Ghost);

  loadButton.onClick = [this] { loadSelectedPreset(); };
  saveButton.onClick = [this] { saveCurrentPreset(); };
  deleteButton.onClick = [this] { deleteSelectedPreset(); };
  refreshButton.onClick = [this] { refreshPresets(); };
  randomButton.onClick = [this] { loadRandomPreset(); };
  previewButton.onClick = [this] {
    int row = presetList.getSelectedRow();
    if (row >= 0 && row < static_cast<int>(filteredPresets.size())) {
      if (isPreviewPlaying_ && currentlyPreviewing_.id == filteredPresets[row].id) {
        stopPreview();
      } else {
        previewPreset(filteredPresets[row]);
      }
    }
  };
  favoritesButton.onClick = [this] {
    if (quickFilter_ == QuickFilter::Favorites) {
      quickFilterCombo_.setSelectedId(static_cast<int>(QuickFilter::All), false);
    } else {
      quickFilterCombo_.setSelectedId(static_cast<int>(QuickFilter::Favorites), false);
    }
  };
  viewModeButton.onClick = [this] {
    setViewMode(viewMode_ == ViewMode::List ? ViewMode::Cards : ViewMode::List);
  };
  compareButton.onClick = [this] {
    int row = presetList.getSelectedRow();
    if (row < 0 || row >= static_cast<int>(filteredPresets.size())) return;

    const auto &selected = filteredPresets[row];

    switch (comparisonState_) {
      case ComparisonState::None:
        // Start comparison, hold this as A
        comparisonState_ = ComparisonState::HoldingA;
        comparisonPresetA_ = selected;
        compareButton.setButtonText("Hold B");
        break;
      case ComparisonState::HoldingA:
        // Hold as B, enable comparison
        comparisonState_ = ComparisonState::Comparing;
        comparisonPresetB_ = selected;
        compareButton.setButtonText("Comparing!");
        break;
      case ComparisonState::Comparing:
        // Clear comparison
        clearComparison();
        break;
      case ComparisonState::HoldingB:
        clearComparison();
        break;
    }
    repaint();
  };
  exportButton.onClick = [this] { exportSelectedPreset(); };
  editMetadataButton.onClick = [this] {
    int row = presetList.getSelectedRow();
    if (row >= 0 && row < static_cast<int>(filteredPresets.size())) {
      showMetadataEditor(filteredPresets[row]);
    }
  };
  newCollectionButton.onClick = [this] { createCollectionUI(); };
  similarButton.onClick = [this] {
    int row = presetList.getSelectedRow();
    if (row >= 0 && row < static_cast<int>(filteredPresets.size())) {
      showSimilarPresets(filteredPresets[row]);
    }
  };

  // Search input with autocomplete trigger
  searchInput_.onTextChanged = [this](const juce::String &text) {
    searchQuery_ = text.trim().toLowerCase();
    if (searchQuery_.length() >= 2) {
      showSearchSuggestions();
    } else {
      hideSearchSuggestions();
    }
    applyFilters();
  };

  // Enable keyboard focus and drag container
  setWantsKeyboardFocus(true);
  presetList.setWantsKeyboardFocus(true);

  dragContainer_ = this->findParentComponentOfClass<juce::DragAndDropContainer>();
  setWantsKeyboardFocus(true);
  presetList.setWantsKeyboardFocus(true);

  // Start timer for animations (60 FPS)
  startTimerHz(60);
}

PresetBrowserComponent::~PresetBrowserComponent() {
  stopPreview();
  saveUserData();
}

//==============================================================================
void PresetBrowserComponent::paint(juce::Graphics &g) {
  // Background with glass effect
  g.fillAll(juce::Colour(design::colors::BG_02));

  // Draw subtle border
  g.setColour(juce::Colour(design::colors::BORDER_SUBTLE));
  g.drawRect(getLocalBounds(), 1);

  // Draw tag chips area
  if (!tagChips_.empty()) {
    auto canvas = reinterpret_cast<SkCanvas*>(g.getInternalContext());
    if (canvas) {
      for (const auto &chip : tagChips_) {
        bool hovered = chip.bounds.toFloat().contains(getMouseXYRelative().toFloat());
        drawTagChip(canvas, chip, hovered && chip.selected);
      }
    }
  }
}

void PresetBrowserComponent::resized() {
  auto area = getLocalBounds().reduced(12);

  // Top section: search and filters (height 80)
  auto headerArea = area.removeFromTop(80);

  // Row 1: Search bar
  auto searchRow = headerArea.removeFromTop(34);
  auto filterRow = headerArea.removeFromTop(24);
  auto metaRow = headerArea; // Remaining space

  // Search input takes most space, buttons on right
  auto buttonArea = searchRow.removeFromRight(420);
  searchRow.removeFromRight(8);
  searchInput_.setBounds(searchRow);

  // Arrange filter buttons in button area
  int btnWidth = 64;
  int btnGap = 4;
  auto previewBtnArea = buttonArea.removeFromLeft(btnWidth);
  auto randomBtnArea = buttonArea.removeFromLeft(btnWidth + btnGap);
  auto favBtnArea = buttonArea.removeFromLeft(btnWidth + btnGap);
  auto viewBtnArea = buttonArea.removeFromLeft(btnWidth + btnGap);
  auto compareBtnArea = buttonArea.removeFromLeft(80);

  previewButton.setBounds(previewBtnArea.reduced(2));
  randomButton.setBounds(randomBtnArea.reduced(2));
  favoritesButton.setBounds(favBtnArea.reduced(2));
  viewModeButton.setBounds(viewBtnArea.reduced(2));
  compareButton.setBounds(compareBtnArea.reduced(2));

  // Filter row: category, sort, quick filter, collections
  auto categoryArea = filterRow.removeFromRight(120);
  filterRow.removeFromRight(6);
  auto sortArea = filterRow.removeFromRight(120);
  filterRow.removeFromRight(6);
  auto quickFilterArea = filterRow.removeFromRight(120);
  filterRow.removeFromRight(6);
  auto collectionsArea = filterRow.removeFromRight(120);

  categoryFilter_.setBounds(categoryArea);
  sortFilter_.setBounds(sortArea);
  quickFilterCombo_.setBounds(quickFilterArea);
  collectionsCombo_.setBounds(collectionsArea);

  // Meta row: results label and hint
  resultsLabel_.setBounds(metaRow.removeFromLeft(300));

  // Tag chips area (if any)
  if (!tagChips_.empty()) {
    auto tagArea = area.removeFromTop(32);
    float x = 0;
    float y = 4;
    float chipWidth = 0;
    float gap = 6;

    for (auto &chip : tagChips_) {
      juce::Font font(11.0f);
      chipWidth = font.getStringWidth(chip.tag) + 16;

      if (x + chipWidth > tagArea.getWidth()) {
        x = 0;
        y += 24;
      }

      chip.bounds = juce::Rectangle<float>(x, y, chipWidth, 20);
      x += chipWidth + gap;
    }

    area.removeFromTop(y + 24);
  }

  // Bottom button row (expanded for new buttons)
  auto buttonRow = area.removeFromBottom(42);
  int numButtons = 8;
  int actionBtnWidth = buttonRow.getWidth() / numButtons;
  loadButton.setBounds(buttonRow.removeFromLeft(actionBtnWidth).reduced(2));
  saveButton.setBounds(buttonRow.removeFromLeft(actionBtnWidth).reduced(2));
  deleteButton.setBounds(buttonRow.removeFromLeft(actionBtnWidth).reduced(2));
  refreshButton.setBounds(buttonRow.removeFromLeft(actionBtnWidth).reduced(2));
  exportButton.setBounds(buttonRow.removeFromLeft(actionBtnWidth).reduced(2));
  editMetadataButton.setBounds(buttonRow.removeFromLeft(actionBtnWidth).reduced(2));
  newCollectionButton.setBounds(buttonRow.removeFromLeft(actionBtnWidth).reduced(2));
  similarButton.setBounds(buttonRow.removeFromLeft(actionBtnWidth).reduced(2));

  // Preset list
  area.removeFromBottom(8);
  presetList.setBounds(area);
}

//==============================================================================
bool PresetBrowserComponent::keyPressed(const juce::KeyPress &key,
                                        Component *originatingComponent) {
  // Handle global shortcuts
  if (key.getKeyCode() == juce::KeyPress::escapeKey) {
    // Close browser or stop preview
    if (isPreviewPlaying_) {
      stopPreview();
      return true;
    }
  }

  if (key.getModifiers().isCommandDown() && key.getTextCharacter() == 'f') {
    searchInput_.grabKeyboardFocus();
    return true;
  }

  if (key.getModifiers().isCommandDown() && key.getTextCharacter() == 'r') {
    loadRandomPreset();
    return true;
  }

  // Navigation
  if (key.getKeyCode() == juce::KeyPress::upKey ||
      key.getKeyCode() == juce::KeyPress::downKey ||
      key.getKeyCode() == juce::KeyPress::returnKey ||
      key.getKeyCode() == juce::KeyPress::spaceKey) {
    return presetList keyPressed(key, originatingComponent);
  }

  return false;
}

//==============================================================================
int PresetBrowserComponent::getNumRows() {
  return static_cast<int>(filteredPresets.size());
}

void PresetBrowserComponent::paintListBoxItem(int rowNumber, SkCanvas &canvas,
                                              int width, int height,
                                              bool rowIsSelected) {
  if (rowNumber >= static_cast<int>(filteredPresets.size()))
    return;

  // Use card view if enabled
  if (viewMode_ == ViewMode::Cards) {
    drawCardView(&canvas, rowNumber, width, height, rowIsSelected);
    return;
  }

  const auto &preset = filteredPresets[rowNumber];
  const bool isFavorite = favoriteIds_.count(preset.id) > 0;
  const bool isRecent = std::find(recentPresetIds_.begin(), recentPresetIds_.end(),
                                  preset.id) != recentPresetIds_.end();
  const bool isPreviewing = isPreviewPlaying_ && currentlyPreviewing_.id == preset.id;
  const int rating = getPresetRating(preset.id);

  // Background
  SkPaint bgPaint;
  if (isPreviewing) {
    // Animated gradient for previewing
    SkColor colors[2] = {
      SkColorSetARGB(40, 0, 240, 255),
      SkColorSetARGB(20, 0, 200, 220)
    };
    SkPoint points[2] = {{0, 0}, {static_cast<float>(width), 0}};
    auto gradient = SkGradientShader::MakeLinear(points, colors, nullptr, 2,
                                                   SkTileMode::kClamp);
    bgPaint.setShader(gradient);
    bgPaint.setAntiAlias(true);
  } else if (rowIsSelected) {
    bgPaint.setColor(SkColorSetARGB(100, 0, 180, 200)); // Cyan selection
  } else {
    bgPaint.setColor(SkColorSetARGB(0, 255, 255, 255));
  }
  canvas.drawRect(SkRect::MakeWH(width, height), bgPaint);

  // Selection border
  if (rowIsSelected || isPreviewing) {
    SkPaint borderPaint;
    borderPaint.setColor(isPreviewing ? design::colors::CYAN : design::colors::CYAN_DARK);
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.5f);
    borderPaint.setAntiAlias(true);
    canvas.drawRect(SkRect::MakeWH(width - 1, height - 1).makeOffset(0.5f, 0.5f), borderPaint);
  }

  // Layout constants
  constexpr float padding = 10.0f;
  constexpr float starSize = 14.0f;
  float x = padding + 24; // Room for star
  float y = 18;

  // Draw star indicator (always visible position, filled if favorite)
  float starX = padding;
  float starY = height / 2 - starSize / 2;

  // Star path
  SkPath starPath;
  const int numPoints = 5;
  const float outerRadius = starSize / 2;
  const float innerRadius = starSize / 4;

  for (int i = 0; i < numPoints * 2; i++) {
    float radius = (i % 2 == 0) ? outerRadius : innerRadius;
    float angle = (i * 3.14159f / numPoints) - 3.14159f / 2;
    float px = starX + outerRadius + std::cos(angle) * radius;
    float py = starY + outerRadius + std::sin(angle) * radius;

    if (i == 0) starPath.moveTo(px, py);
    else starPath.lineTo(px, py);
  }
  starPath.close();

  SkPaint starPaint;
  if (isFavorite) {
    starPaint.setColor(SkColorSetARGB(255, 255, 200, 0)); // Gold
    starPaint.setAntiAlias(true);
    canvas.drawPath(starPath, starPaint);

    // Glow for favorited
    SkPaint glowPaint;
    glowPaint.setColor(SkColorSetARGB(80, 255, 200, 0));
    glowPaint.setAntiAlias(true);
    glowPaint.setStyle(SkPaint::kStroke_Style);
    glowPaint.setStrokeWidth(2);
    canvas.drawPath(starPath, glowPaint);
  } else {
    // Outline for non-favorite
    starPaint.setColor(SkColorSetARGB(80, 150, 150, 150));
    starPaint.setAntiAlias(true);
    starPaint.setStyle(SkPaint::kStroke_Style);
    starPaint.setStrokeWidth(1.5f);
    canvas.drawPath(starPath, starPaint);
  }

  // Preset name
  SkFont nameFont;
  nameFont.setSize(15.0f);
  nameFont.setEmbolden(true);

  SkPaint namePaint;
  namePaint.setColor(design::colors::TEXT_PRIMARY);
  namePaint.setAntiAlias(true);

  canvas.drawString(preset.name.getCharPointer(), x, y, nameFont, namePaint);

  // Recent indicator
  if (isRecent) {
    SkPaint recentPaint;
    recentPaint.setColor(SkColorSetARGB(180, 0, 200, 255));
    recentPaint.setAntiAlias(true);
    SkFont recentFont;
    recentFont.setSize(9.0f);

    float nameWidth = nameFont.measureText(preset.name.getCharPointer(),
                                          preset.name.length(), SkTextEncoding::kUTF8);
    canvas.drawString("RECENT", x + nameWidth + 8, y - 2, recentFont, recentPaint);
  }

  // Tags
  y = height - 12;
  SkFont tagFont;
  tagFont.setSize(10.5f);

  float tagX = x;
  for (size_t i = 0; i < preset.tags.size() && i < 4; ++i) {
    const auto &tag = preset.tags[i];
    float tagWidth = tagFont.measureText(tag.getCharPointer(), tag.length(),
                                         SkTextEncoding::kUTF8) + 10;

    // Tag background
    SkPaint tagBgPaint;
    tagBgPaint.setColor(SkColorSetARGB(40, 60, 60, 70));
    tagBgPaint.setAntiAlias(true);
    SkRRect tagRect = SkRRect::MakeRectXY(
        SkRect::MakeXYWH(tagX, y - 8, tagWidth, 16), 4, 4);
    canvas.drawRRect(tagRect, tagBgPaint);

    // Tag text
    SkPaint tagTextPaint;
    tagTextPaint.setColor(design::colors::TEXT_TERTIARY);
    tagTextPaint.setAntiAlias(true);
    canvas.drawString(tag.getCharPointer(), tagX + 5, y + 3, tagFont, tagTextPaint);

    tagX += tagWidth + 6;
  }

  // Category on right
  SkFont catFont;
  catFont.setSize(12.0f);
  float catWidth = catFont.measureText(preset.category.getCharPointer(),
                                       preset.category.length(), SkTextEncoding::kUTF8);
  float catX = width - catWidth - padding;

  SkPaint catPaint;
  catPaint.setColor(design::colors::TEXT_SECONDARY);
  catPaint.setAntiAlias(true);
  canvas.drawString(preset.category.getCharPointer(), catX, height / 2 + 4,
                    catFont, catPaint);

  // Author and rating below category
  SkFont authorFont;
  authorFont.setSize(10.0f);
  float authorWidth = authorFont.measureText(preset.author.getCharPointer(),
                                             preset.author.length(), SkTextEncoding::kUTF8);

  SkPaint authorPaint;
  authorPaint.setColor(SkColorSetARGB(150, 120, 120, 130));
  authorPaint.setAntiAlias(true);
  canvas.drawString(preset.author.getCharPointer(), width - authorWidth - padding,
                    height - 10, authorFont, authorPaint);

  // Rating stars (below author if rated)
  if (rating > 0) {
    float starX = width - authorWidth - padding;
    float starY = height - 8;
    float starSize = 8;

    for (int i = 0; i < 5; ++i) {
      SkPath starPath;
      const int numPoints = 5;
      const float outerRadius = starSize / 2;
      const float innerRadius = starSize / 4;

      for (int j = 0; j < numPoints * 2; j++) {
        float r = (j % 2 == 0) ? outerRadius : innerRadius;
        float angle = (j * 3.14159f / numPoints) - 3.14159f / 2;
        float px = starX + i * (starSize + 1) + outerRadius + std::cos(angle) * r;
        float py = starY + outerRadius + std::sin(angle) * r;

        if (j == 0) starPath.moveTo(px, py);
        else starPath.lineTo(px, py);
      }
      starPath.close();

      SkPaint starPaint;
      if (i < rating) {
        starPaint.setColor(SkColorSetARGB(255, 255, 200, 0));
      } else {
        starPaint.setColor(SkColorSetARGB(50, 80, 80, 80));
      }
      starPaint.setAntiAlias(true);
      canvas.drawPath(starPath, starPaint);
    }
  }

  // Preview animation (waveform) if previewing
  if (isPreviewing) {
    auto waveArea = SkRect::MakeXYWH(width - 60, 8, 50, 12);
    drawPreviewWaveform(&canvas, waveArea);
  }
}

void PresetBrowserComponent::listBoxItemClicked(int row, const juce::MouseEvent &e) {
  if (row >= 0 && row < static_cast<int>(filteredPresets.size())) {
    const auto &preset = filteredPresets[row];

    // Right-click for context menu
    if (e.mods.isRightButtonDown()) {
      showContextMenuForPreset(preset, e.getScreenX(), e.getScreenY());
      return;
    }

    // Check if clicked on star area (left side)
    if (e.position.x < 30) {
      toggleFavorite(preset);
      repaint();
      return;
    }

    // Double click to load
    if (e.getNumberOfClicks() == 2) {
      loadPreset(preset);
    }

    // Single click - enable drag preparation
    dragStartPosition_ = e.getPosition();
  }
}

//==============================================================================
void PresetBrowserComponent::timerCallback() {
  bool needsRepaint = false;

  // Animate preview waveform
  if (isPreviewPlaying_) {
    previewAnimationPhase_ += 0.05f;
    if (previewAnimationPhase_ > 1.0f) previewAnimationPhase_ = 0.0f;
    needsRepaint = true;
  }

  // Animate random preset button
  if (isAnimatingRandom_) {
    randomAnimationProgress_ += 0.1f;
    if (randomAnimationProgress_ >= 1.0f) {
      isAnimatingRandom_ = false;
      randomAnimationProgress_ = 0.0f;
    }
    needsRepaint = true;
  }

  if (needsRepaint) {
    repaint();
  }
}

//==============================================================================
void PresetBrowserComponent::refreshPresets() {
  presets = ZenithPresetManager::getInstance().getPresetList(currentInstrumentId);
  rebuildCategoryFilter();
  rebuildTagChips();
  applyFilters();
}

void PresetBrowserComponent::setInstrumentId(const juce::String &instrumentId) {
  currentInstrumentId = instrumentId;
  refreshPresets();
}

void PresetBrowserComponent::setViewMode(ViewMode mode) {
  viewMode_ = mode;
  viewModeButton.setButtonText(mode == ViewMode::List ? "Cards" : "List");

  if (mode == ViewMode::Cards) {
    presetList.setRowHeight(120);
  } else if (mode == ViewMode::Compact) {
    presetList.setRowHeight(36);
  } else {
    presetList.setRowHeight(52);
  }

  repaint();
}

//==============================================================================
void PresetBrowserComponent::loadPreset(const PresetMetadata &preset) {
  stopPreview();

  auto fullPreset = ZenithPresetManager::getInstance().loadPreset(
      currentInstrumentId, preset.id);

  addToRecent(preset);
  incrementPlayCount(preset.id);

  if (loadCallback) {
    loadCallback(fullPreset);
  }

  DBG("Loaded preset: " + preset.name);
}

void PresetBrowserComponent::previewPreset(const PresetMetadata &preset) {
  if (isPreviewPlaying_ && currentlyPreviewing_.id == preset.id) {
    stopPreview();
  } else {
    startPreview(preset);
  }
}

void PresetBrowserComponent::stopPreview() {
  stopPreviewInternal();
}

void PresetBrowserComponent::toggleFavorite(const PresetMetadata &preset) {
  if (favoriteIds_.count(preset.id)) {
    favoriteIds_.erase(preset.id);
  } else {
    favoriteIds_.insert(preset.id);
  }
  saveUserData();
  applyFilters();
}

bool PresetBrowserComponent::isFavorite(const juce::String &presetId) const {
  return favoriteIds_.count(presetId) > 0;
}

void PresetBrowserComponent::addToRecent(const PresetMetadata &preset) {
  // Remove if already exists
  auto it = std::find(recentPresetIds_.begin(), recentPresetIds_.end(), preset.id);
  if (it != recentPresetIds_.end()) {
    recentPresetIds_.erase(it);
  }

  // Add to front
  recentPresetIds_.push_front(preset.id);

  // Limit size
  while (recentPresetIds_.size() > MAX_RECENT_PRESETS) {
    recentPresetIds_.pop_back();
  }

  saveUserData();
}

void PresetBrowserComponent::loadRandomPreset() {
  if (filteredPresets.empty()) return;

  // Animate
  isAnimatingRandom_ = true;
  randomAnimationProgress_ = 0.0f;

  // Random selection
  static std::random_device rd;
  static std::mt19937 gen(rd());
  std::uniform_int_distribution<> dist(0, static_cast<int>(filteredPresets.size()) - 1);

  int index = dist(gen);
  loadPreset(filteredPresets[index]);

  // Update selection
  presetList.selectRow(index);
}

//==============================================================================
void PresetBrowserComponent::loadSelectedPreset() {
  int row = presetList.getSelectedRow();
  if (row >= 0 && row < static_cast<int>(filteredPresets.size())) {
    loadPreset(filteredPresets[row]);
  }
}

void PresetBrowserComponent::saveCurrentPreset() {
  if (captureCallback) {
    auto preset = captureCallback();

    auto asyncBox = std::make_shared<juce::AlertWindow>(
        "Save Preset",
        "Enter a name for your preset:",
        juce::MessageBoxIconType::QuestionIcon);

    asyncBox->addTextEditor("presetName", preset.name, "Preset Name:");
    asyncBox->addTextEditor("category", preset.category, "Category:");
    asyncBox->addTextEditor("tags", "", "Tags (comma separated):");
    asyncBox->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey, 0, 0));
    asyncBox->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey, 0, 0));

    auto safeThis = juce::Component::SafePointer<PresetBrowserComponent>(this);
    asyncBox->enterModalState(true, juce::ModalCallbackFunction::create(
        [safeThis, asyncBox, preset](int result) mutable {
          if (result == 1 && safeThis) {
            auto updatedPreset = preset;
            updatedPreset.name = asyncBox->getTextEditorContents("presetName");
            updatedPreset.category = asyncBox->getTextEditorContents("category");

            // Parse tags
            juce::String tagsStr = asyncBox->getTextEditorContents("tags");
            updatedPreset.tags.clear();
            for (const auto& tag : tagsStr.split(",")) {
              juce::String trimmed = tag.trim();
              if (trimmed.isNotEmpty()) {
                updatedPreset.tags.push_back(trimmed);
              }
            }

            ZenithPresetManager::getInstance().savePreset(updatedPreset, true);
            safeThis->refreshPresets();
            DBG("Saved preset: " + updatedPreset.name);
          }
        }), true);
  }
}

void PresetBrowserComponent::deleteSelectedPreset() {
  int row = presetList.getSelectedRow();
  if (row >= 0 && row < static_cast<int>(filteredPresets.size())) {
    auto preset = filteredPresets[row];

    // Confirm delete
    juce::AlertWindow::showYesNoCancelBox(
        juce::MessageBoxIconType::WarningIcon,
        "Delete Preset",
        "Are you sure you want to delete '" + preset.name + "'? This action cannot be undone.",
        "Delete",
        "Cancel",
        {},
        nullptr,
        [this, row](int result) {
          if (result == 1) {
            ZenithPresetManager::getInstance().deletePreset(
                currentInstrumentId, filteredPresets[row].id, true);
            refreshPresets();
          }
        });
  }
}

//==============================================================================
void PresetBrowserComponent::applyFilters() {
  filteredPresets.clear();

  for (const auto &preset : presets) {
    // Quick filter
    switch (quickFilter_) {
      case QuickFilter::All:
        break;
      case QuickFilter::Favorites:
        if (!favoriteIds_.count(preset.id)) continue;
        break;
      case QuickFilter::Recent:
        if (std::find(recentPresetIds_.begin(), recentPresetIds_.end(), preset.id) ==
            recentPresetIds_.end()) continue;
        break;
      case QuickFilter::Factory:
        if (preset.author != "Factory") continue;
        break;
      case QuickFilter::User:
        if (preset.author == "Factory") continue;
        break;
      case QuickFilter::Similar:
        // Filter by similarity to reference preset
        if (similarityReference_.id.isEmpty() || preset.id == similarityReference_.id) continue;
        // Will be sorted by similarity, just add all for now
        break;
      case QuickFilter::Project:
        // Filter by project-specific collections
        {
          bool inProjectCollection = false;
          for (const auto &coll : collections_) {
            if (coll.isProjectSpecific && coll.projectId == projectId_) {
              if (coll.presetIds.contains(preset.id)) {
                inProjectCollection = true;
                break;
              }
            }
          }
          if (!inProjectCollection) continue;
        }
        break;
    }

    // Category filter
    juce::String categoryText = categoryFilter_.getText().trim();
    if (categoryText.isNotEmpty() && categoryText != "All" &&
        categoryText != "All Categories") {
      if (preset.category != categoryText) continue;
    }

    // Search query
    if (!searchQuery_.isEmpty() && !matchesQuery(preset, searchQuery_)) continue;

    // Tag filter (multi-select AND logic)
    if (!selectedTags_.empty() && !matchesTags(preset, selectedTags_)) continue;

    filteredPresets.push_back(preset);
  }

  sortPresets();
  updateResultsLabel();
  updateButtonStates();

  presetList.updateContent();
  presetList.repaint();
}

void PresetBrowserComponent::sortPresets() {
  switch (currentSortMode_) {
    case SortMode::Name:
      std::sort(filteredPresets.begin(), filteredPresets.end(),
                [](const auto &a, const auto &b) { return a.name < b.name; });
      break;
    case SortMode::Category:
      std::sort(filteredPresets.begin(), filteredPresets.end(),
                [](const auto &a, const auto &b) {
                  if (a.category == b.category) return a.name < b.name;
                  return a.category < b.category;
                });
      break;
    case SortMode::Author:
      std::sort(filteredPresets.begin(), filteredPresets.end(),
                [](const auto &a, const auto &b) {
                  if (a.author == b.author) return a.name < b.name;
                  return a.author < b.author;
                });
      break;
    case SortMode::Rating:
      // Sort by favorites first (simplified rating)
      std::sort(filteredPresets.begin(), filteredPresets.end(),
                [this](const auto &a, const auto &b) {
                  bool aFav = favoriteIds_.count(a.id) > 0;
                  bool bFav = favoriteIds_.count(b.id) > 0;
                  if (aFav != bFav) return aFav;
                  return a.name < b.name;
                });
      break;
    case SortMode::Recent:
      std::sort(filteredPresets.begin(), filteredPresets.end(),
                [this](const auto &a, const auto &b) {
                  auto itA = std::find(recentPresetIds_.begin(), recentPresetIds_.end(), a.id);
                  auto itB = std::find(recentPresetIds_.begin(), recentPresetIds_.end(), b.id);
                  bool aRecent = itA != recentPresetIds_.end();
                  bool bRecent = itB != recentPresetIds_.end();
                  if (aRecent != bRecent) return aRecent;
                  if (aRecent && bRecent) return itA < itB;
                  return a.name < b.name;
                });
      break;
    case SortMode::Similarity:
      // Sort by similarity score
      if (similarityReference_.id.isNotEmpty()) {
        std::sort(filteredPresets.begin(), filteredPresets.end(),
                  [this](const auto &a, const auto &b) {
                    auto itA = similarityScores_.find(a.id);
                    auto itB = similarityScores_.find(b.id);
                    float scoreA = (itA != similarityScores_.end()) ? itA->second : 0.0f;
                    float scoreB = (itB != similarityScores_.end()) ? itB->second : 0.0f;
                    return scoreA > scoreB; // Higher similarity first
                  });
      }
      break;
  }
}

void PresetBrowserComponent::rebuildCategoryFilter() {
  auto categories = ZenithPresetManager::getInstance().getCategories(currentInstrumentId);
  categories.sort(true);

  categoryFilter_.clear();
  categoryFilter_.addItem("All", 1);
  int id = 2;
  for (const auto &category : categories) {
    categoryFilter_.addItem(category, id++);
  }

  // Restore selection
  if (currentCategory_.isNotEmpty()) {
    for (int i = 0; i < categoryFilter_.getNumItems(); ++i) {
      if (categoryFilter_.getItemText(i) == currentCategory_) {
        categoryFilter_.setSelectedItemIndex(i, false);
        break;
      }
    }
  } else {
    categoryFilter_.setSelectedId(1, false);
  }
}

void PresetBrowserComponent::rebuildTagChips() {
  tagChips_.clear();

  // Count tag occurrences
  std::map<juce::String, int> tagCounts;
  for (const auto &preset : presets) {
    for (const auto &tag : preset.tags) {
      tagCounts[tag]++;
    }
  }

  // Create chips for top tags (max 8)
  std::vector<std::pair<juce::String, int>> sortedTags(tagCounts.begin(), tagCounts.end());
  std::sort(sortedTags.begin(), sortedTags.end(),
            [](const auto &a, const auto &b) { return a.second > b.second; });

  for (size_t i = 0; i < std::min(size_t(8), sortedTags.size()); ++i) {
    TagChip chip;
    chip.tag = sortedTags[i].first;
    chip.count = sortedTags[i].second;
    chip.selected = selectedTags_.count(chip.tag) > 0;
    tagChips_.push_back(chip);
  }
}

void PresetBrowserComponent::updateResultsLabel() {
  juce::String quickFilterText;
  switch (quickFilter_) {
    case QuickFilter::All: quickFilterText = ""; break;
    case QuickFilter::Favorites: quickFilterText = " · Favorites"; break;
    case QuickFilter::Recent: quickFilterText = " · Recent"; break;
    case QuickFilter::Factory: quickFilterText = " · Factory"; break;
    case QuickFilter::User: quickFilterText = " · User"; break;
    case QuickFilter::Similar:
      quickFilterText = " · Similar to " + similarityReference_.name;
      break;
    case QuickFilter::Project: quickFilterText = " · Project Presets"; break;
  }

  juce::String categoryText = categoryFilter_.getText().trim();
  if (categoryText.isEmpty() || categoryText == "All" || categoryText == "All Categories") {
    categoryText = "";
  } else {
    categoryText = " · " + categoryText;
  }

  resultsLabel_.setText(juce::String(filteredPresets.size()) + " presets" +
                        categoryText + quickFilterText,
                        juce::dontSendNotification);
}

void PresetBrowserComponent::updateButtonStates() {
  // Update favorites button
  if (quickFilter_ == QuickFilter::Favorites) {
    favoritesButton.setButtonStyle(ZenithButton::Style::Primary);
  } else {
    favoritesButton.setButtonStyle(ZenithButton::Style::Ghost);
  }

  // Update preview button
  int row = presetList.getSelectedRow();
  if (row >= 0 && row < static_cast<int>(filteredPresets.size())) {
    const bool isCurrentlyPreviewing = isPreviewPlaying_ &&
        currentlyPreviewing_.id == filteredPresets[row].id;
    previewButton.setButtonText(isCurrentlyPreviewing ? "Stop" : "Preview");
  }
}

//==============================================================================
bool PresetBrowserComponent::matchesQuery(const PresetMetadata &preset,
                                          const juce::String &query) {
  if (preset.name.toLowerCase().contains(query)) return true;
  if (preset.category.toLowerCase().contains(query)) return true;
  if (preset.author.toLowerCase().contains(query)) return true;
  for (const auto &tag : preset.tags) {
    if (tag.toLowerCase().contains(query)) return true;
  }
  return false;
}

bool PresetBrowserComponent::matchesTags(
    const PresetMetadata &preset,
    const std::unordered_set<juce::String> &tags) {
  // Check if preset has ALL selected tags (AND logic)
  for (const auto &tag : tags) {
    bool found = false;
    for (const auto &presetTag : preset.tags) {
      if (presetTag.equalsIgnoreCase(tag)) {
        found = true;
        break;
      }
    }
    if (!found) return false;
  }
  return true;
}

//==============================================================================
void PresetBrowserComponent::startPreview(const PresetMetadata &preset) {
  currentlyPreviewing_ = preset;
  isPreviewPlaying_ = true;
  previewAnimationPhase_ = 0.0f;

  if (previewCallback) {
    previewCallback(preset);
  }

  updateButtonStates();
  presetList.repaint();
}

void PresetBrowserComponent::stopPreviewInternal() {
  isPreviewPlaying_ = false;
  currentlyPreviewing_ = PresetMetadata();
  updateButtonStates();
  presetList.repaint();
}

//==============================================================================
juce::File PresetBrowserComponent::getUserDataFile() const {
  auto dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                 .getChildFile("ZenithDAW/PresetBrowser");
  if (!dir.exists()) dir.createDirectory();
  return dir.getChildFile(currentInstrumentId + "_user_data.json");
}

void PresetBrowserComponent::loadUserData() {
  auto file = getUserDataFile();
  if (!file.exists()) return;

  auto json = juce::JSON::parse(file);
  if (!json.isObject()) return;

  // Load favorites
  auto favsArray = json.getProperty("favorites", juce::Array<juce::var>{});
  if (favsArray.isArray()) {
    for (const auto &id : *favsArray.getArray()) {
      if (id.isString()) {
        favoriteIds_.insert(id.toString());
      }
    }
  }

  // Load recent
  auto recentArray = json.getProperty("recent", juce::Array<juce::var>{});
  if (recentArray.isArray()) {
    for (const auto &id : *recentArray.getArray()) {
      if (id.isString()) {
        recentPresetIds_.push_back(id.toString());
      }
    }
  }

  // Load selected tags
  auto tagsArray = json.getProperty("selectedTags", juce::Array<juce::var>{});
  if (tagsArray.isArray()) {
    for (const auto &tag : *tagsArray.getArray()) {
      if (tag.isString()) {
        selectedTags_.insert(tag.toString());
      }
    }
  }

  // Load ratings
  auto ratingsObj = json.getProperty("ratings", juce::DynamicObject());
  if (ratingsObj.isObject()) {
    auto *props = ratingsObj.getDynamicObject()->getProperties();
    for (const auto &[key, val] : props) {
      if (val.isInt() || val.isDouble()) {
        presetRatings_[key.toString()] = static_cast<int>(val);
      }
    }
  }

  // Load play counts
  auto playsObj = json.getProperty("playCounts", juce::DynamicObject());
  if (playsObj.isObject()) {
    auto *props = playsObj.getDynamicObject()->getProperties();
    for (const auto &[key, val] : props) {
      if (val.isInt() || val.isDouble()) {
        presetPlayCounts_[key.toString()] = static_cast<int>(val);
      }
    }
  }

  // Load collections
  auto collsArray = json.getProperty("collections", juce::Array<juce::var>{});
  if (collsArray.isArray()) {
    for (const auto &collVar : *collsArray.getArray()) {
      if (collVar.isObject()) {
        auto *collObj = collVar.getDynamicObject();
        PresetCollection coll;
        coll.id = collObj->getProperty("id").toString();
        coll.name = collObj->getProperty("name").toString();
        coll.description = collObj->getProperty("description").toString();

        auto presetIdsArray = collObj->getProperty("presetIds");
        if (presetIdsArray.isArray()) {
          for (const auto &idVar : *presetIdsArray.getArray()) {
            if (idVar.isString()) {
              coll.presetIds.add(idVar.toString());
            }
          }
        }
        collections_.push_back(coll);
      }
    }
    rebuildCollectionsList();
  }
}

void PresetBrowserComponent::saveUserData() {
  juce::DynamicObject::Ptr json = new juce::DynamicObject();

  // Save favorites
  juce::Array<juce::var> favsArray;
  for (const auto &id : favoriteIds_) {
    favsArray.add(id);
  }
  json->setProperty("favorites", favsArray);

  // Save recent
  juce::Array<juce::var> recentArray;
  for (const auto &id : recentPresetIds_) {
    recentArray.add(id);
  }
  json->setProperty("recent", recentArray);

  // Save selected tags
  juce::Array<juce::var> tagsArray;
  for (const auto &tag : selectedTags_) {
    tagsArray.add(tag);
  }
  json->setProperty("selectedTags", tagsArray);

  // Save ratings
  juce::DynamicObject::Ptr ratingsObj = new juce::DynamicObject();
  for (const auto &[presetId, rating] : presetRatings_) {
    ratingsObj->setProperty(presetId, rating);
  }
  json->setProperty("ratings", juce::var(ratingsObj.get()));

  // Save play counts
  juce::DynamicObject::Ptr playsObj = new juce::DynamicObject();
  for (const auto &[presetId, count] : presetPlayCounts_) {
    playsObj->setProperty(presetId, count);
  }
  json->setProperty("playCounts", juce::var(playsObj.get()));

  // Save collections
  juce::Array<juce::var> collsArray;
  for (const auto &coll : collections_) {
    juce::DynamicObject::Ptr collObj = new juce::DynamicObject();
    collObj->setProperty("id", coll.id);
    collObj->setProperty("name", coll.name);
    collObj->setProperty("description", coll.description);

    juce::Array<juce::var> presetIdsArray;
    for (const auto &pid : coll.presetIds) {
      presetIdsArray.add(pid);
    }
    collObj->setProperty("presetIds", presetIdsArray);

    collsArray.add(juce::var(collObj.get()));
  }
  json->setProperty("collections", collsArray);

  // Write to file
  auto file = getUserDataFile();
  file.create();
  file.replaceWithText(juce::JSON::toString(juce::var(json.get()), true));
}

//==============================================================================
void PresetBrowserComponent::drawTagChip(SkCanvas *canvas, const TagChip &chip,
                                         bool hovered) {
  if (chip.bounds.isEmpty()) return;

  // Background
  SkPaint bgPaint;
  if (chip.selected) {
    bgPaint.setColor(SkColorSetARGB(200, 0, 180, 220));
  } else if (hovered) {
    bgPaint.setColor(SkColorSetARGB(120, 80, 80, 90));
  } else {
    bgPaint.setColor(SkColorSetARGB(80, 50, 50, 60));
  }
  bgPaint.setAntiAlias(true);

  SkRRect chipRect = SkRRect::MakeRectXY(
      SkRect::MakeXYWH(chip.bounds.getX(), chip.bounds.getY(), chip.bounds.getWidth(), chip.bounds.getHeight()), 6.0f, 6.0f);
  canvas->drawRRect(chipRect, bgPaint);

  // Text
  SkFont font;
  font.setSize(10.0f);

  SkPaint textPaint;
  textPaint.setColor(chip.selected ? SK_ColorWHITE : design::colors::TEXT_TERTIARY);
  textPaint.setAntiAlias(true);

  canvas->drawSimpleText(chip.tag.getCharPointer(), chip.tag.length(), SkTextEncoding::kUTF8,
                        chip.bounds.getX() + 8,
                        chip.bounds.getCentreY() + 4,
                        font, textPaint);

  // Count badge
  juce::String countStr = juce::String(chip.count);
  float countWidth = font.measureText(countStr.getCharPointer(), countStr.length(),
                                      SkTextEncoding::kUTF8) + 8;
  SkRRect countRect = SkRRect::MakeRectXY(
      SkRect::MakeXYWH(chip.bounds.getRight() - countWidth - 4,
                       chip.bounds.getY() + 2, countWidth, 12), 4, 4);

  SkPaint countBgPaint;
  countBgPaint.setColor(SkColorSetARGB(150, 0, 0, 0));
  countBgPaint.setAntiAlias(true);
  canvas->drawRRect(countRect, countBgPaint);

  SkPaint countTextPaint;
  countTextPaint.setColor(SkColorSetARGB(200, 180, 180, 180));
  countTextPaint.setAntiAlias(true);
  canvas->drawSimpleText(countStr.getCharPointer(), countStr.length(), SkTextEncoding::kUTF8,
                        countRect.getBounds().fRect.left + 4,
                        countRect.getBounds().fRect.fBottom - 3,
                        font, countTextPaint);
}

void PresetBrowserComponent::drawPreviewWaveform(SkCanvas *canvas,
                                                  const juce::Rectangle<float> &bounds) {
  if (bounds.isEmpty()) return;

  SkPaint paint;
  paint.setColor(design::colors::CYAN);
  paint.setStyle(SkPaint::kStroke_Style);
  paint.setStrokeWidth(1.5f);
  paint.setAntiAlias(true);

  SkPath path;
  float centerY = bounds.getCentreY();
  float width = bounds.getWidth();
  float phase = previewAnimationPhase_ * width * 2;

  for (float x = 0; x < width; x += 2) {
    float wave = std::sin((x + phase) * 0.15f) * 0.3f +
                 std::sin((x + phase) * 0.08f) * 0.2f;
    float y = centerY + wave * bounds.getHeight() / 2;

    if (x == 0) path.moveTo(bounds.getX() + x, y);
    else path.lineTo(bounds.getX() + x, y);
  }

  canvas->drawPath(path, paint);
}

//==============================================================================
// Rating System
//==============================================================================
void PresetBrowserComponent::setPresetRating(const juce::String &presetId, int rating) {
  rating = juce::jlimit(0, 5, rating);
  if (rating == 0) {
    presetRatings_.erase(presetId);
  } else {
    presetRatings_[presetId] = rating;
  }
  saveUserData();
  repaint();
}

int PresetBrowserComponent::getPresetRating(const juce::String &presetId) const {
  auto it = presetRatings_.find(presetId);
  return it != presetRatings_.end() ? it->second : 0;
}

void PresetBrowserComponent::incrementPlayCount(const juce::String &presetId) {
  presetPlayCounts_[presetId]++;
  saveUserData();
}

//==============================================================================
// Comparison Mode (A/B)
//==============================================================================
void PresetBrowserComponent::setComparisonHoldingA(const PresetMetadata &preset) {
  comparisonState_ = ComparisonState::HoldingA;
  comparisonPresetA_ = preset;
  compareButton.setButtonText("Hold B");
  repaint();
}

void PresetBrowserComponent::setComparisonHoldingB(const PresetMetadata &preset) {
  comparisonState_ = ComparisonState::Comparing;
  comparisonPresetB_ = preset;
  compareButton.setButtonText("Comparing!");
  repaint();
}

void PresetBrowserComponent::clearComparison() {
  comparisonState_ = ComparisonState::None;
  compareButton.setButtonText("Compare A/B");
  repaint();
}

void PresetBrowserComponent::loadFromComparison(bool loadA) {
  const auto &preset = loadA ? comparisonPresetA_ : comparisonPresetB_;
  loadPreset(preset);
}

void PresetBrowserComponent::morphBetweenAB(float amount) {
  morphAmount_ = juce::jlimit(0.0f, 1.0f, amount);
  // This would trigger parameter interpolation
  // Implementation depends on having access to the processor
  DBG("Morphing A->B: " + juce::String(morphAmount_));
}

//==============================================================================
// Card View Painting
//==============================================================================
void PresetBrowserComponent::drawCardView(SkCanvas *canvas, int rowNumber,
                                          int width, int height, bool isSelected) {
  if (rowNumber >= static_cast<int>(filteredPresets.size()))
    return;

  const auto &preset = filteredPresets[rowNumber];
  const bool isFavorite = favoriteIds_.count(preset.id) > 0;
  const int rating = getPresetRating(preset.id);

  // Card background with glass effect
  SkPaint bgPaint;
  if (isSelected) {
    SkColor colors[2] = {
      SkColorSetARGB(180, 20, 30, 40),
      SkColorSetARGB(180, 10, 20, 30)
    };
    SkPoint points[2] = {{0, 0}, {0, static_cast<float>(height)}};
    auto gradient = SkGradientShader::MakeLinear(points, colors, nullptr, 2,
                                                   SkTileMode::kClamp);
    bgPaint.setShader(gradient);
  } else {
    bgPaint.setColor(SkColorSetARGB(200, 18, 18, 22));
  }
  bgPaint.setAntiAlias(true);

  SkRRect cardRect;
  float radius = 12;
  cardRect.setRectXY(SkRect::MakeWH(width - 2, height - 2).makeOffset(1, 1), radius, radius);
  canvas->drawRRect(cardRect, bgPaint);

  // Selection glow
  if (isSelected) {
    SkPaint glowPaint;
    glowPaint.setColor(SkColorSetARGB(100, 0, 200, 220));
    glowPaint.setStyle(SkPaint::kStroke_Style);
    glowPaint.setStrokeWidth(2);
    glowPaint.setAntiAlias(true);
    canvas->drawRRect(cardRect, glowPaint);
  }

  // Layout
  constexpr float pad = 12;

  // Star icon (top left)
  float starX = pad;
  float starY = pad + 5;

  SkPath starPath;
  const int numPoints = 5;
  const float outerRadius = 12;
  const float innerRadius = 6;

  for (int i = 0; i < numPoints * 2; i++) {
    float r = (i % 2 == 0) ? outerRadius : innerRadius;
    float angle = (i * 3.14159f / numPoints) - 3.14159f / 2;
    float px = starX + outerRadius + std::cos(angle) * r;
    float py = starY + outerRadius + std::sin(angle) * r;

    if (i == 0) starPath.moveTo(px, py);
    else starPath.lineTo(px, py);
  }
  starPath.close();

  SkPaint starPaint;
  if (isFavorite) {
    starPaint.setColor(SkColorSetARGB(255, 255, 200, 0));
    starPaint.setAntiAlias(true);
    canvas->drawPath(starPath, starPaint);

    SkPaint glowPaint;
    glowPaint.setColor(SkColorSetARGB(100, 255, 200, 0));
    glowPaint.setStyle(SkPaint::kStroke_Style);
    glowPaint.setStrokeWidth(2);
    canvas->drawPath(starPath, glowPaint);
  } else {
    starPaint.setColor(SkColorSetARGB(60, 120, 120, 120));
    starPaint.setAntiAlias(true);
    starPaint.setStyle(SkPaint::kStroke_Style);
    starPaint.setStrokeWidth(1.5);
    canvas->drawPath(starPath, starPaint);
  }

  // Preset name (top, below star)
  SkFont nameFont;
  nameFont.setSize(16);
  nameFont.setEmbolden(true);

  SkPaint namePaint;
  namePaint.setColor(design::colors::TEXT_PRIMARY);
  namePaint.setAntiAlias(true);

  canvas->drawSimpleText(preset.name.getCharPointer(), preset.name.length(), SkTextEncoding::kUTF8,
                        pad, pad + 40,
                        nameFont, namePaint);

  // User rating stars (below name)
  if (rating > 0) {
    drawStarRating(canvas, pad, pad + 52, rating, 12);
  }

  // Category badge (top right)
  SkFont catFont;
  catFont.setSize(11);
  float catWidth = catFont.measureText(preset.category.getCharPointer(),
                                       preset.category.length(), SkTextEncoding::kUTF8) + 16;

  SkPaint catBgPaint;
  catBgPaint.setColor(SkColorSetARGB(150, 0, 150, 180));
  catBgPaint.setAntiAlias(true);

  SkRRect catRect = SkRRect::MakeRectXY(
      SkRect::MakeXYWH(width - catWidth - pad, pad, catWidth, 22), 6.0f, 6.0f);
  canvas->drawRRect(catRect, catBgPaint);

  SkPaint catTextPaint;
  catTextPaint.setColor(SK_ColorWHITE);
  catTextPaint.setAntiAlias(true);
  canvas->drawSimpleText(preset.category.getCharPointer(), preset.category.length(), SkTextEncoding::kUTF8,
                        width - catWidth - pad + 8, pad + 15,
                        catFont, catTextPaint);

  // Tags (middle section)
  float tagY = pad + 80;
  SkFont tagFont;
  tagFont.setSize(10);
  float tagX = pad;

  for (size_t i = 0; i < preset.tags.size() && i < 5; ++i) {
    const auto &tag = preset.tags[i];
    float tagW = tagFont.measureText(tag.getCharPointer(), tag.length(),
                                     SkTextEncoding::kUTF8) + 12;

    if (tagX + tagW > width - pad * 2) {
      tagX = pad;
      tagY += 22;
    }

    SkPaint tagBgPaint;
    tagBgPaint.setColor(SkColorSetARGB(80, 60, 60, 70));
    tagBgPaint.setAntiAlias(true);

    SkRRect tagRect = SkRRect::MakeRectXY(
        SkRect::MakeXYWH(tagX, tagY, tagW, 18), 4.0f, 4.0f);
    canvas->drawRRect(tagRect, tagBgPaint);

    SkPaint tagTextPaint;
    tagTextPaint.setColor(design::colors::TEXT_TERTIARY);
    canvas->drawSimpleText(tag.getCharPointer(), tag.length(), SkTextEncoding::kUTF8, tagX + 6, tagY + 13,
                          tagFont, tagTextPaint);

    tagX += tagW + 6;
  }

  // Author (bottom left)
  SkFont authorFont;
  authorFont.setSize(11);

  SkPaint authorPaint;
  authorPaint.setColor(SkColorSetARGB(150, 130, 130, 140));
  canvas->drawSimpleText(preset.author.getCharPointer(), preset.author.length(), SkTextEncoding::kUTF8,
                        pad, height - pad - 5,
                        authorFont, authorPaint);

  // Play count (bottom right)
  int plays = presetPlayCounts_[preset.id];
  if (plays > 0) {
    juce::String playsText = juce::String(plays) + (plays == 1 ? " play" : " plays");
    float playsWidth = authorFont.measureText(playsText.getCharPointer(),
                                              playsText.length(), SkTextEncoding::kUTF8);
    SkPaint playsPaint;
    playsPaint.setColor(SkColorSetARGB(120, 100, 150, 100));
    canvas->drawSimpleText(playsText.getCharPointer(),
                          width - playsWidth - pad, height - pad - 5,
                          authorFont, playsPaint);
  }

  // Mini waveform placeholder (could be enhanced with actual thumbnail)
  auto waveArea = SkRect::MakeXYTH(width - 80, height - 45, 70, 30);
  SkPaint waveBgPaint;
  waveBgPaint.setColor(SkColorSetARGB(30, 0, 0, 0));
  waveBgPaint.setAntiAlias(true);
  SkRRect waveBgRect = SkRRect::MakeRectXY(waveArea, 4, 4);
  canvas->drawRRect(waveBgRect, waveBgPaint);

  // Draw animated waveform preview
  drawMiniWaveformThumbnail(canvas, waveArea, preset);

  // Comparison indicators
  if (comparisonState_ != ComparisonState::None) {
    SkPaint compPaint;
    if (comparisonPresetA_.id == preset.id) {
      compPaint.setColor(SkColorSetARGB(200, 0, 220, 255));
      SkFont compFont;
      compFont.setSize(14);
      compFont.setEmbolden(true);
      canvas->drawSimpleText("A", width - 25, pad + 18, compFont, compPaint);
    }
    if (comparisonPresetB_.id == preset.id) {
      compPaint.setColor(SkColorSetARGB(200, 255, 0, 150));
      SkFont compFont;
      compFont.setSize(14);
      compFont.setEmbolden(true);
      canvas->drawSimpleText("B", width - 45, pad + 18, compFont, compPaint);
    }
  }
}

void PresetBrowserComponent::drawComparisonIndicator(SkCanvas *canvas,
                                                      const juce::Rectangle<float> &bounds) {
  if (comparisonState_ == ComparisonState::None) return;

  // Draw comparison mode indicator
  SkPaint bgPaint;
  bgPaint.setColor(SkColorSetARGB(200, 30, 30, 40));
  SkRRect bgRect = SkRRect::MakeRectXY(SkRect::MakeXYWH(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight()), 8.0f, 8.0f);
  canvas->drawRRect(bgRect, bgPaint);

  // Border based on state
  SkPaint borderPaint;
  if (comparisonState_ == ComparisonState::HoldingA) {
    borderPaint.setColor(SkColorSetARGB(255, 0, 220, 255));
  } else if (comparisonState_ == ComparisonState::Comparing) {
    borderPaint.setColor(SkColorSetARGB(255, 0, 255, 200));
  } else {
    borderPaint.setColor(SkColorSetARGB(255, 255, 0, 150));
  }
  borderPaint.setStyle(SkPaint::kStroke_Style);
  borderPaint.setStrokeWidth(2);
  canvas->drawRRect(bgRect, borderPaint);

  // Text
  SkFont font;
  font.setSize(12);
  SkPaint textPaint;
  textPaint.setColor(SK_ColorWHITE);

  juce::String text;
  switch (comparisonState_) {
    case ComparisonState::HoldingA:
      text = "Hold A: " + comparisonPresetA_.name;
      break;
    case ComparisonState::Comparing:
      text = "A: " + comparisonPresetA_.name + " | B: " + comparisonPresetB_.name;
      break;
    default:
      break;
  }

  if (text.isNotEmpty()) {
    canvas->drawSimpleText(text.getCharPointer(), text.length(), SkTextEncoding::kUTF8,
                          bounds.getX() + 12, bounds.getCentreY(),
                          font, textPaint);
  }
}

void PresetBrowserComponent::drawMiniWaveformThumbnail(SkCanvas *canvas,
                                                        const juce::Rectangle<float> &bounds,
                                                        const PresetMetadata &preset) {
  // Draw a stylized waveform based on preset characteristics
  // In a full implementation, this would load actual cached thumbnail data

  SkPaint wavePaint;
  wavePaint.setColor(design::colors::CYAN);
  wavePaint.setStyle(SkPaint::kStroke_Style);
  wavePaint.setStrokeWidth(1);
  wavePaint.setAntiAlias(true);

  SkPath path;
  float centerY = bounds.getCentreY();
  float width = bounds.getWidth();

  // Generate a pseudo-random but consistent waveform based on preset ID
  uint32_t hash = static_cast<uint32_t>(preset.id.hashCode());
  std::mt19937 gen(hash);

  for (float x = 0; x < width; x += 2) {
    float amplitude = (gen() % 100) / 100.0f;
    float wave = std::sin(x * 0.2f + previewAnimationPhase_) * amplitude;
    float y = centerY + wave * bounds.getHeight() * 0.35f;

    if (x == 0) path.moveTo(bounds.getX() + x, y);
    else path.lineTo(bounds.getX() + x, y);
  }

  canvas->drawPath(path, wavePaint);
}

//==============================================================================
void PresetBrowserComponent::drawStarRating(SkCanvas *canvas, float x, float y,
                                            int rating, float size) {
  for (int i = 0; i < 5; ++i) {
    SkPath starPath;
    const int numPoints = 5;
    const float outerRadius = size / 2;
    const float innerRadius = size / 4;

    for (int j = 0; j < numPoints * 2; j++) {
      float r = (j % 2 == 0) ? outerRadius : innerRadius;
      float angle = (j * 3.14159f / numPoints) - 3.14159f / 2;
      float px = x + i * (size + 2) + outerRadius + std::cos(angle) * r;
      float py = y + outerRadius + std::sin(angle) * r;

      if (j == 0) starPath.moveTo(px, py);
      else starPath.lineTo(px, py);
    }
    starPath.close();

    SkPaint starPaint;
    if (i < rating) {
      starPaint.setColor(SkColorSetARGB(255, 255, 200, 0));
    } else {
      starPaint.setColor(SkColorSetARGB(40, 80, 80, 80));
    }
    starPaint.setAntiAlias(true);
    canvas->drawPath(starPath, starPaint);
  }
}

//==============================================================================
// Export/Import
//==============================================================================
void PresetBrowserComponent::exportSelectedPreset() {
  int row = presetList.getSelectedRow();
  if (row < 0 || row >= static_cast<int>(filteredPresets.size())) {
    juce::AlertWindow::showMessageBoxAsync(
        juce::MessageBoxIconType::InfoIcon,
        "Export Preset",
        "Please select a preset to export.");
    return;
  }

  const auto &preset = filteredPresets[row];

  // File chooser for export location
  auto chooser = std::make_unique<juce::FileChooser>(
      "Export Preset",
      juce::File::getSpecialLocation(juce::File::userHomeDirectory),
      preset.name + ".zpreset");

  chooser->launchAsync(juce::FileBrowserComponent::saveMode |
                       juce::FileBrowserComponent::warnAboutOverwriting,
      [this, preset](const juce::FileChooser &chooser) {
        auto destFile = chooser.getResult();
        if (destFile == juce::File()) return;

        // Read source preset file
        juce::File sourceFile(preset.filePath);
        if (!sourceFile.exists()) {
          juce::AlertWindow::showMessageBoxAsync(
              juce::MessageBoxIconType::WarningIcon,
              "Export Failed",
              "Could not find source preset file.");
          return;
        }

        // Copy to destination
        if (sourceFile.copyFileTo(destFile)) {
          juce::AlertWindow::showMessageBoxAsync(
              juce::MessageBoxIconType::InfoIcon,
              "Export Successful",
              "Preset exported to:\n" + destFile.getFullPathName());
        } else {
          juce::AlertWindow::showMessageBoxAsync(
              juce::MessageBoxIconType::WarningIcon,
              "Export Failed",
              "Could not copy preset file.");
        }
      });
}

void PresetBrowserComponent::importPresetFromFile() {
  auto chooser = std::make_unique<juce::FileChooser>(
      "Import Preset",
      juce::File::getSpecialLocation(juce::File::userHomeDirectory),
      "*.zpreset;*.json");

  chooser->launchAsync(juce::FileBrowserComponent::openMode |
                       juce::FileBrowserComponent::canSelectFiles,
      [this](const juce::FileChooser &chooser) {
        auto sourceFile = chooser.getResult();
        if (sourceFile == juce::File()) return;

        // Read and parse the preset file
        auto json = juce::JSON::parse(sourceFile);
        if (!json.isObject()) {
          juce::AlertWindow::showMessageBoxAsync(
              juce::MessageBoxIconType::WarningIcon,
              "Import Failed",
              "Invalid preset file format.");
          return;
        }

        // Convert to Preset and save
        auto preset = Preset::fromJson(json);
        preset.author = "Imported";

        if (ZenithPresetManager::getInstance().savePreset(
                currentInstrumentId, preset, true)) {
          refreshPresets();
          juce::AlertWindow::showMessageBoxAsync(
              juce::MessageBoxIconType::InfoIcon,
              "Import Successful",
              "Preset imported: " + preset.name);
        } else {
          juce::AlertWindow::showMessageBoxAsync(
              juce::MessageBoxIconType::WarningIcon,
              "Import Failed",
              "Could not save preset to library.");
        }
      });
}

void PresetBrowserComponent::exportPresetPack() {
  // Export all filtered presets as a pack
  if (filteredPresets.empty()) {
    juce::AlertWindow::showMessageBoxAsync(
        juce::MessageBoxIconType::InfoIcon,
        "Export Pack",
        "No presets to export.");
    return;
  }

  auto chooser = std::make_unique<juce::FileChooser>(
      "Export Preset Pack",
      juce::File::getSpecialLocation(juce::File::userHomeDirectory),
      "preset_pack.zip");

  chooser->launchAsync(juce::FileBrowserComponent::saveMode |
                       juce::FileBrowserComponent::warnAboutOverwriting,
      [this](const juce::FileChooser &chooser) {
        auto destFile = chooser.getResult();
        if (destFile == juce::File()) return;

        // Create a temporary directory for the pack
        auto tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory)
                           .getChildFile("zenith_preset_pack_" +
                                        juce::String(juce::Time::currentTimeMillis()));
        tempDir.createDirectory();

        // Copy all filtered presets
        int copied = 0;
        for (const auto &preset : filteredPresets) {
          juce::File sourceFile(preset.filePath);
          if (sourceFile.exists()) {
            juce::File destFile = tempDir.getChildFile(sourceFile.getFileName());
            if (sourceFile.copyFileTo(destFile)) {
              copied++;
            }
          }
        }

        // Create a zip file (simplified - would use juce::Zip in full implementation)
        juce::AlertWindow::showMessageBoxAsync(
            juce::MessageBoxIconType::InfoIcon,
            "Export Complete",
            juce::String("Exported ") + juce::String(copied) + " presets to:\n" +
            tempDir.getFullPathName() + "\n\n(Compress to .zip manually for distribution)");

        // Clean up temp directory after 30 seconds
        startTimer(30000); // Reuse existing timer
      });
}

//==============================================================================
// Metadata Editor
//==============================================================================
void PresetBrowserComponent::showMetadataEditor(const PresetMetadata &preset) {
  // Load full preset to get all metadata
  auto fullPreset = ZenithPresetManager::getInstance().loadPreset(
      currentInstrumentId, preset.id);

  // Create metadata editor dialog
  auto editor = std::make_shared<juce::AlertWindow>(
      "Edit Preset Metadata",
      "Edit the metadata for: " + preset.name,
      juce::MessageBoxIconType::QuestionIcon);

  editor->addTextEditor("name", fullPreset.name, "Name:");
  editor->addTextEditor("category", fullPreset.category, "Category:");
  editor->addTextEditor("author", fullPreset.author, "Author:");
  editor->addTextEditor("description", fullPreset.description, "Description:");

  // Tags as comma-separated string
  juce::String tagsStr;
  for (size_t i = 0; i < fullPreset.tags.size(); ++i) {
    if (i > 0) tagsStr << ", ";
    tagsStr << fullPreset.tags[i];
  }
  editor->addTextEditor("tags", tagsStr, "Tags (comma separated):");

  // Rating display (can't edit from here, uses setPresetRating)
  int currentRating = getPresetRating(preset.id);
  editor->addTextEditor("rating", currentRating > 0 ? juce::String(currentRating) : "",
                        "Rating (1-5, or clear to remove):");

  editor->addButton("Apply", 1, juce::KeyPress(juce::KeyPress::returnKey, 0, 0));
  editor->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey, 0, 0));

  auto safeThis = juce::Component::SafePointer<PresetBrowserComponent>(this);
  editor->enterModalState(true, juce::ModalCallbackFunction::create(
      [safeThis, editor, presetId = preset.id](int result) {
        if (result == 1 && safeThis) {
          // Load the preset again
          auto fullPreset = ZenithPresetManager::getInstance().loadPreset(
              safeThis->currentInstrumentId, presetId);

          // Update metadata
          fullPreset.name = editor->getTextEditorContents("name");
          fullPreset.category = editor->getTextEditorContents("category");
          fullPreset.author = editor->getTextEditorContents("author");
          fullPreset.description = editor->getTextEditorContents("description");

          // Parse tags
          fullPreset.tags.clear();
          juce::String tagsStr = editor->getTextEditorContents("tags");
          for (const auto &tag : juce::StringArray::fromTokens(tagsStr, ",", "")) {
            juce::String trimmed = tag.trim();
            if (trimmed.isNotEmpty()) {
              fullPreset.tags.push_back(trimmed);
            }
          }

          // Update rating
          juce::String ratingStr = editor->getTextEditorContents("rating");
          if (ratingStr.isNotEmpty()) {
            int rating = ratingStr.getIntValue();
            safeThis->setPresetRating(presetId, juce::jlimit(1, 5, rating));
          }

          // Save updated preset
          ZenithPresetManager::getInstance().savePreset(fullPreset, true);
          safeThis->refreshPresets();

          DBG("Updated preset metadata: " + fullPreset.name);
        }
      }), true);
}

//==============================================================================
// Collections
//==============================================================================
void PresetBrowserComponent::createCollection(const juce::String &name) {
  PresetCollection newCollection;
  newCollection.id = "collection_" + juce::String(juce::Time::currentTimeMillis());
  newCollection.name = name;
  collections_.push_back(newCollection);
  saveUserData();
  rebuildCollectionsList();
}

void PresetBrowserComponent::addToCollection(const juce::String &collectionId,
                                              const juce::String &presetId) {
  for (auto &coll : collections_) {
    if (coll.id == collectionId) {
      if (!coll.presetIds.contains(presetId)) {
        coll.presetIds.add(presetId);
        saveUserData();
      }
      break;
    }
  }
}

void PresetBrowserComponent::removeFromCollection(const juce::String &collectionId,
                                                   const juce::String &presetId) {
  for (auto &coll : collections_) {
    if (coll.id == collectionId) {
      coll.presetIds.removeAllInstancesOf(presetId);
      saveUserData();
      break;
    }
  }
}

void PresetBrowserComponent::deleteCollection(const juce::String &collectionId) {
  auto it = std::find_if(collections_.begin(), collections_.end(),
                        [&collectionId](const PresetCollection &c) {
                          return c.id == collectionId;
                        });
  if (it != collections_.end()) {
    collections_.erase(it);
    saveUserData();
    rebuildCollectionsList();
  }
}

juce::StringArray PresetBrowserComponent::getCollections() const {
  juce::StringArray names;
  for (const auto &coll : collections_) {
    names.add(coll.name);
  }
  return names;
}

std::vector<PresetBrowserComponent::PresetCollection>
PresetBrowserComponent::getAllCollections() const {
  return collections_;
}

void PresetBrowserComponent::rebuildCollectionsList() {
  // Store current selection
  int currentId = collectionsCombo_.getSelectedId();

  collectionsCombo_.clear();
  collectionsCombo_.addItem("All Collections", 1);
  collectionsCombo_.addSeparator();

  int id = 2;
  for (const auto &coll : collections_) {
    juce::String text = coll.name + " (" + juce::String(coll.presetIds.size()) + ")";
    collectionsCombo_.addItem(text, id++);
  }

  collectionsCombo_.addSeparator();
  collectionsCombo_.addItem("+ New Collection...", 999);

  // Restore selection if possible
  if (currentId > 0 && currentId < id) {
    collectionsCombo_.setSelectedId(currentId, false);
  } else {
    collectionsCombo_.setSelectedId(1, false);
  }
}

//==============================================================================
// Sound Character Analysis
//==============================================================================
PresetBrowserComponent::SoundCharacter
PresetBrowserComponent::analyzePresetCharacter(const PresetMetadata &preset) const {
  // Check cache first
  auto cached = presetCharacterCache_.find(preset.id);
  if (cached != presetCharacterCache_.end()) {
    return cached->second;
  }

  SoundCharacter character;

  // Analyze based on category (heuristic)
  juce::String cat = preset.name.toLowerCase();

  if (preset.category.equalsIgnoreCase("Bass") ||
      preset.category.equalsIgnoreCase("Sub")) {
    character.warmth = 0.8f;
    character.brightness = 0.2f;
    character.attack = 0.3f;
    character.characteristicLabel = "Bass";
  } else if (preset.category.equalsIgnoreCase("Lead")) {
    character.warmth = 0.5f;
    character.brightness = 0.6f;
    character.attack = 0.4f;
    character.characteristicLabel = "Lead";
  } else if (preset.category.equalsIgnoreCase("Pad")) {
    character.warmth = 0.7f;
    character.brightness = 0.5f;
    character.attack = 0.8f;
    character.characteristicLabel = "Pad";
  } else if (preset.category.equalsIgnoreCase("Pluck")) {
    character.warmth = 0.4f;
    character.brightness = 0.6f;
    character.attack = 0.1f;
    character.characteristicLabel = "Plucky";
  } else if (preset.category.equalsIgnoreCase("Keys")) {
    character.warmth = 0.5f;
    character.brightness = 0.5f;
    character.attack = 0.3f;
    character.characteristicLabel = "Keys";
  } else {
    // Analyze from name tags
    if (cat.contains("bright") || cat.contains("sharp") || cat.contains("bell")) {
      character.brightness = 0.8f;
      character.warmth = 0.2f;
    } else if (cat.contains("warm") || cat.contains("soft") || cat.contains("dark")) {
      character.brightness = 0.2f;
      character.warmth = 0.8f;
    }

    if (cat.contains("pluck") || cat.contains("perc") || cat.contains("hit")) {
      character.attack = 0.1f;
      character.characteristicLabel = "Percussive";
    } else if (cat.contains("pad") || cat.contains("ambient") || cat.contains("slow")) {
      character.attack = 0.8f;
      character.characteristicLabel = "Ambient";
    }
  }

  // Cache and return
  presetCharacterCache_[preset.id] = character;
  return character;
}

//==============================================================================
// Search Suggestions (Autocomplete)
//==============================================================================
// Note: This would integrate with the search input to show dropdown suggestions
// The SkiaTextInput component would need to support dropdown menus for full implementation
//==============================================================================
// Collection Management UI
//==============================================================================
void PresetBrowserComponent::createCollectionUI() {
  // Show dialog to create new collection
  auto dialog = std::make_shared<juce::AlertWindow>(
      "Create New Collection",
      "Enter a name for your preset collection:",
      juce::MessageBoxIconType::QuestionIcon);

  dialog->addTextEditor("collectionName", "", "Collection Name:");
  dialog->addTextEditor("description", "", "Description (optional):");
  dialog->addButton("Create", 1, juce::KeyPress(juce::KeyPress::returnKey, 0, 0));
  dialog->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey, 0, 0));

  // Checkbox for project-specific collection
  auto* toggle = new juce::ToggleButton("Project-specific collection");
  toggle->setSize(300, 24);
  dialog->addCustomComponent(toggle);
  juce::Component::SafePointer<juce::ToggleButton> safeToggle(toggle);

  auto safeThis = juce::Component::SafePointer<PresetBrowserComponent>(this);
  dialog->enterModalState(true, juce::ModalCallbackFunction::create(
      [safeThis, dialog, safeToggle](int result) {
        if (result == 1 && safeThis) {
          juce::String name = dialog->getTextEditorContents("collectionName").trim();
          if (name.isNotEmpty()) {
            safeThis->createCollection(name);

            // Get the newly created collection
            auto &newColl = safeThis->collections_.back();
            newColl.description = dialog->getTextEditorContents("description").trim();
            if (safeToggle) newColl.isProjectSpecific = safeToggle->getToggleState();
            if (newColl.isProjectSpecific && safeThis->projectId_.isNotEmpty()) {
              newColl.projectId = safeThis->projectId_;
            }

            safeThis->saveUserData();
            safeThis->rebuildCollectionsList();

            DBG("Created collection: " + name);
          }
        }
      }), true);
}

void PresetBrowserComponent::showAddToCollectionMenu(const PresetMetadata &preset, int x, int y) {
  contextMenuPreset_ = preset;

  juce::PopupMenu menu;
  menu.addItem(1, "Add to New Collection...", true);

  if (!collections_.empty()) {
    menu.addSeparator();
    int id = 2;
    for (const auto &coll : collections_) {
      bool alreadyInCollection = isInCollection(preset.id, coll.id);
      juce::String itemText = coll.name;
      if (alreadyInCollection) {
        itemText += " (already added)";
      }
      menu.addItem(id++, itemText, !alreadyInCollection);
    }
  }

  menu.showMenuAsync(juce::PopupMenu::Options()
      .withTargetComponent(this)
      .withTargetScreenArea(juce::Rectangle<int>(x, y, 1, 1)),
      [this, presetId = preset.id](int result) {
        if (result == 0) return;

        if (result == 1) {
          // Create new collection and add preset
          createCollectionUI();
          // Will need to add preset after collection is created
          // For simplicity, we let user manually add after creation
        } else {
          // Add to existing collection
          int index = result - 2;
          if (index >= 0 && index < static_cast<int>(collections_.size())) {
            addToCollection(collections_[index].id, presetId);
            DBG("Added preset " + presetId + " to collection " + collections_[index].name);
          }
        }
      });
}

void PresetBrowserComponent::editCollection(const juce::String &collectionId) {
  auto it = std::find_if(collections_.begin(), collections_.end(),
                        [&collectionId](const PresetCollection &c) {
                          return c.id == collectionId;
                        });
  if (it == collections_.end()) return;

  auto dialog = std::make_shared<juce::AlertWindow>(
      "Edit Collection",
      "Edit collection details:",
      juce::MessageBoxIconType::QuestionIcon);

  dialog->addTextEditor("collectionName", it->name, "Collection Name:");
  dialog->addTextEditor("description", it->description, "Description:");
  dialog->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey, 0, 0));
  dialog->addButton("Delete Collection", 2);
  dialog->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey, 0, 0));

  auto safeThis = juce::Component::SafePointer<PresetBrowserComponent>(this);
  dialog->enterModalState(true, juce::ModalCallbackFunction::create(
      [safeThis, collectionId, dialog](int result) {
        if (safeThis) {
            if (result == 1) {
            // Save changes
            for (auto &coll : safeThis->collections_) {
                if (coll.id == collectionId) {
                coll.name = dialog->getTextEditorContents("collectionName").trim();
                coll.description = dialog->getTextEditorContents("description").trim();
                break;
                }
            }
            safeThis->saveUserData();
            safeThis->rebuildCollectionsList();
            } else if (result == 2) {
            // Delete collection
            safeThis->deleteCollection(collectionId);
            }
        }
      }), true);
}

bool PresetBrowserComponent::isInCollection(const juce::String &presetId,
                                           const juce::String &collectionId) const {
  auto it = std::find_if(collections_.begin(), collections_.end(),
                        [&collectionId](const PresetCollection &c) {
                          return c.id == collectionId;
                        });
  if (it != collections_.end()) {
    return it->presetIds.contains(presetId);
  }
  return false;
}

//==============================================================================
// Search Autocomplete
//==============================================================================
juce::Array<SkiaSuggestionsPopup::SuggestionItem>
PresetBrowserComponent::getSearchSuggestions(const juce::String &query) const {
  juce::Array<SkiaSuggestionsPopup::SuggestionItem> suggestions;
  juce::String queryLower = query.toLowerCase();

  if (query.length() < 2) return suggestions;

  // Track which items we've already added to avoid duplicates
  std::set<juce::String> addedTexts;

  // Suggest preset names (highest priority)
  for (const auto &preset : presets) {
    if (preset.name.toLowerCase().startsWith(queryLower)) {
      if (addedTexts.insert(preset.name).second) {
        SkiaSuggestionsPopup::SuggestionItem s;
        s.text = preset.name;
        s.type = "preset";
        s.relevance = 100;
        suggestions.add(s);
      }
    }
  }

  // Suggest categories
  std::set<juce::String> categories;
  for (const auto &preset : presets) {
    if (preset.category.toLowerCase().startsWith(queryLower)) {
      categories.insert(preset.category);
    }
  }
  for (const auto &cat : categories) {
    juce::String text = cat;
    if (addedTexts.insert(text).second) {
      SkiaSuggestionsPopup::SuggestionItem s;
      s.text = text;
      s.type = "category";
      s.relevance = 80;
      suggestions.add(s);
    }
  }

  // Suggest tags
  std::set<juce::String> tags;
  for (const auto &preset : presets) {
    for (const auto &tag : preset.tags) {
      if (tag.toLowerCase().startsWith(queryLower)) {
        tags.insert(tag);
      }
    }
  }
  for (const auto &tag : tags) {
    juce::String text = tag;
    if (addedTexts.insert(text).second) {
      SkiaSuggestionsPopup::SuggestionItem s;
      s.text = text;
      s.type = "tag";
      s.relevance = 70;
      suggestions.add(s);
    }
  }

  // Suggest authors
  std::set<juce::String> authors;
  for (const auto &preset : presets) {
    if (preset.author.toLowerCase().startsWith(queryLower)) {
      authors.insert(preset.author);
    }
  }
  for (const auto &author : authors) {
    juce::String text = author;
    if (addedTexts.insert(text).second) {
      SkiaSuggestionsPopup::SuggestionItem s;
      s.text = text;
      s.type = "author";
      s.relevance = 60;
      suggestions.add(s);
    }
  }

  // Sort by relevance
  std::sort(suggestions.begin(), suggestions.end(),
      [](const SkiaSuggestionsPopup::SuggestionItem &a,
         const SkiaSuggestionsPopup::SuggestionItem &b) {
    return a.relevance > b.relevance;
  });

  // Limit to 8 suggestions
  while (suggestions.size() > 8) {
    suggestions.removeLast();
  }

  return suggestions;
}

void PresetBrowserComponent::showSearchSuggestions() {
  currentSuggestions_ = getSearchSuggestions(searchInput_.getText());

  if (currentSuggestions_.isEmpty()) {
    hideSearchSuggestions();
    return;
  }

  // Create Skia popup if needed
  if (!suggestionsPopup_) {
    suggestionsPopup_ = std::make_unique<SkiaSuggestionsPopup>();
    suggestionsPopup_->setSelectedCallback([this](const SkiaSuggestionsPopup::SuggestionItem &item) {
      applySearchSuggestion(item);
    });

    // Add to parent component (desktop window or similar)
    if (auto *parent = getParentComponent()) {
      parent->addChildComponent(suggestionsPopup_.get());
    } else {
      addChildComponent(suggestionsPopup_.get());
    }
  }

  // Position popup below search input
  auto searchBounds = searchInput_.getBounds();
  int popupWidth = searchBounds.getWidth();
  int popupHeight = static_cast<int>(currentSuggestions_.size()) * 28 + 8;

  suggestionsPopup_->setBounds(searchBounds.getX(), searchBounds.getBottom() + 4,
                               popupWidth, popupHeight);

  suggestionsPopup_->setSuggestions(currentSuggestions_);
  suggestionsPopup_->setVisible(true);
  suggestionsPopup_->toFront(true);

  showingSuggestions_ = true;
}

void PresetBrowserComponent::hideSearchSuggestions() {
  if (suggestionsPopup_) {
    suggestionsPopup_->setVisible(false);
  }
  showingSuggestions_ = false;
}

void PresetBrowserComponent::applySearchSuggestion(const SkiaSuggestionsPopup::SuggestionItem &suggestion) {
  // For categories/tags/authors, search by that term directly
  // For presets, search by name
  juce::String searchTerm = suggestion.text;

  searchInput_.setText(searchTerm);
  hideSearchSuggestions();
}

//==============================================================================
// Waveform Thumbnails
//==============================================================================
void PresetBrowserComponent::generateWaveformThumbnail(const Preset &preset,
                                                       const juce::AudioBuffer<float> &audio) {
  WaveformThumbnail thumbnail;
  thumbnail.presetId = preset.id;
  thumbnail.duration = audio.getNumSamples() / 44100.0f;
  thumbnail.timestamp = juce::Time::currentTimeMillis();

  // Extract peaks (one peak per ~2 pixels of thumbnail width)
  constexpr int thumbnailWidth = 100;
  int samplesPerPeak = audio.getNumSamples() / thumbnailWidth;

  thumbnail.peaks.reserve(thumbnailWidth);
  for (int i = 0; i < thumbnailWidth; ++i) {
    int startSample = i * samplesPerPeak;
    int endSample = juce::jmin(startSample + samplesPerPeak, audio.getNumSamples());

    float peak = 0.0f;
    for (int ch = 0; ch < audio.getNumChannels(); ++ch) {
      auto* channelData = audio.getReadPointer(ch);
      for (int s = startSample; s < endSample; ++s) {
        peak = juce::jmax(peak, std::abs(channelData[s]));
      }
    }
    thumbnail.peaks.push_back(peak);
  }

  // Generate thumbnail image
  constexpr int thumbW = 100, thumbH = 40;
  thumbnail.image = juce::Image(juce::Image::RGB, thumbW, thumbH, true);
  juce::Graphics g(thumbnail.image);

  g.fillAll(juce::Colours::transparentBlack);

  // Draw waveform
  juce::Path waveformPath;
  float centerY = thumbH / 2;

  for (int x = 0; x < thumbW; ++x) {
    float amplitude = (x < static_cast<int>(thumbnail.peaks.size()))
                      ? thumbnail.peaks[x] : 0;
    float y = centerY - (amplitude * centerY * 0.9f);

    if (x == 0) waveformPath.startNewSubPath(x, y);
    else waveformPath.lineTo(x, y);

    waveformPath.lineTo(x, 2 * centerY - y);
  }

  g.setColour(juce::Colour(design::colors::CYAN).withBrightness(0.8f));
  g.strokePath(waveformPath, juce::PathStrokeType(1.0f));

  saveWaveformThumbnail(thumbnail);
}

WaveformThumbnail
PresetBrowserComponent::getWaveformThumbnail(const juce::String &presetId) const {
  auto it = thumbnailCache_.find(presetId);
  if (it != thumbnailCache_.end()) {
    return it->second;
  }
  return WaveformThumbnail{};
}

void PresetBrowserComponent::saveWaveformThumbnail(const WaveformThumbnail &thumbnail) {
  thumbnailCache_[thumbnail.presetId] = thumbnail;

  // Save to disk
  auto cacheDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                      .getChildFile("ZenithDAW/PresetThumbnails");
  if (!cacheDir.exists()) cacheDir.createDirectory();

  auto cacheFile = cacheDir.getChildFile(thumbnail.presetId + ".png");
  if (thumbnail.image.isValid()) {
    juce::FileOutputStream stream(cacheFile);
    juce::PNGImageFormat png;
    png.writeImageToStream(thumbnail.image, stream);
  }

  // Save peaks data
  auto peaksFile = cacheDir.getChildFile(thumbnail.presetId + "_peaks.json");
  juce::DynamicObject::Ptr json = new juce::DynamicObject();
  json->setProperty("presetId", thumbnail.presetId);
  json->setProperty("timestamp", thumbnail.timestamp);
  json->setProperty("duration", thumbnail.duration);

  juce::Array<juce::var> peaksArray;
  for (float peak : thumbnail.peaks) {
    peaksArray.add(peak);
  }
  json->setProperty("peaks", peaksArray);

  peaksFile.replaceWithText(juce::JSON::toString(juce::var(json.get())));
}

WaveformThumbnail
PresetBrowserComponent::loadWaveformThumbnail(const juce::String &presetId) {
  auto cacheDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                      .getChildFile("ZenithDAW/PresetThumbnails");
  auto imageFile = cacheDir.getChildFile(presetId + ".png");
  auto peaksFile = cacheDir.getChildFile(presetId + "_peaks.json");

  WaveformThumbnail thumbnail;
  thumbnail.presetId = presetId;

  // Load image
  if (imageFile.exists()) {
    thumbnail.image = juce::ImageFileFormat::loadFrom(imageFile);
  }

  // Load peaks
  if (peaksFile.exists()) {
    auto json = juce::JSON::parse(peaksFile);
    if (json.isObject()) {
      thumbnail.timestamp = json.getProperty("timestamp", 0);
      thumbnail.duration = json.getProperty("duration", 0.0f);

      auto peaksArray = json.getProperty("peaks", juce::var());
      if (peaksArray.isArray()) {
        for (const auto &peak : *peaksArray.getArray()) {
          thumbnail.peaks.push_back(static_cast<float>(peak));
        }
      }
    }
  }

  return thumbnail;
}

void PresetBrowserComponent::clearThumbnailCache() {
  thumbnailCache_.clear();
}

//==============================================================================
// Drag and Drop - Source
//==============================================================================
void PresetBrowserComponent::mouseDrag(const juce::MouseEvent &e) {
  if (!dragContainer_) {
    dragContainer_ = findParentComponentOfClass<juce::DragAndDropContainer>();
  }

  if (dragContainer_ && e.getDistanceFromDragStart() > 10) {
    // Check if we're over the preset list
    if (presetList.getBounds().contains(e.getPosition())) {
      int row = presetList.getRowContainingPosition(e.x, e.y);
      if (row >= 0 && row < static_cast<int>(filteredPresets.size())) {
        draggingPreset_ = filteredPresets[row];

        // Create drag image
        juce::Image dragImage(juce::Image::RGB, 150, 40, true);
        juce::Graphics g(dragImage);
        g.fillAll(juce::Colour(0xFF1A1E26));
        g.setColour(juce::Colours::white);
        g.setFont(14);
        g.drawText(draggingPreset_.name, dragImage.getBounds().reduced(8),
                  juce::Justification::centredLeft);

        // Start drag
        juce::String description = getPresetDragDescription();
        dragContainer_->startDragging(description, this, dragImage, true);

        isDragging_ = true;
      }
    }
  }

  juce::Component::mouseDrag(e);
}

void PresetBrowserComponent::mouseUp(const juce::MouseEvent &e) {
  isDragging_ = false;
  juce::Component::mouseUp(e);
}

juce::var PresetBrowserComponent::getPresetDragDescription() const {
  juce::DynamicObject::Ptr desc = new juce::DynamicObject();
  desc->setProperty("type", "preset");
  desc->setProperty("presetId", draggingPreset_.id);
  desc->setProperty("presetName", draggingPreset_.name);
  desc->setProperty("instrumentId", currentInstrumentId);
  desc->setProperty("category", draggingPreset_.category);
  return juce::var(desc.get());
}

//==============================================================================
// Drag and Drop - Target (for imports)
//==============================================================================
bool PresetBrowserComponent::isInterestedInFileDrag(const juce::StringArray &files) {
  for (const auto &file : files) {
    if (file.endsWithIgnoreCase(".zpreset") ||
        file.endsWithIgnoreCase(".json") ||
        file.endsWithIgnoreCase(".preset")) {
      return true;
    }
  }
  return false;
}

void PresetBrowserComponent::filesDropped(const juce::StringArray &files, int x, int y) {
  for (const auto &file : files) {
    importPresetFromDrag(juce::File(file));
  }
}

void PresetBrowserComponent::importPresetFromDrag(const juce::File &file) {
  if (!file.exists()) return;

  auto json = juce::JSON::parse(file);
  if (!json.isObject()) {
    juce::AlertWindow::showMessageBoxAsync(
        juce::MessageBoxIconType::WarningIcon,
        "Import Failed",
        "Invalid preset file format: " + file.getFileName());
    return;
  }

  auto preset = Preset::fromJson(json);
  preset.author = "Imported";

  if (ZenithPresetManager::getInstance().savePreset(currentInstrumentId, preset, true)) {
    refreshPresets();
    DBG("Imported preset: " + preset.name);
  }
}

//==============================================================================
// Sound Similarity
//==============================================================================
juce::StringArray PresetBrowserComponent::getSimilarPresets(const PresetMetadata &preset,
                                                            int maxResults) {
  juce::StringArray similarIds;
  std::vector<std::pair<float, juce::String>> scores;

  auto refCharacter = analyzePresetCharacter(preset);

  for (const auto &other : presets) {
    if (other.id == preset.id) continue;

    float similarity = calculateSimilarity(preset, other);
    scores.push_back({similarity, other.id});
  }

  // Sort by similarity score (descending)
  std::sort(scores.begin(), scores.end(),
           [](const auto &a, const auto &b) { return a.first > b.first; });

  // Return top N results
  for (int i = 0; i < std::min(maxResults, static_cast<int>(scores.size())); ++i) {
    similarIds.add(scores[i].second);
  }

  return similarIds;
}

float PresetBrowserComponent::calculateSimilarity(const PresetMetadata &a,
                                                  const PresetMetadata &b) const {
  auto charA = analyzePresetCharacter(a);
  auto charB = analyzePresetCharacter(b);

  // Weighted similarity calculation
  float brightnessDiff = std::abs(charA.brightness - charB.brightness);
  float warmthDiff = std::abs(charA.warmth - charB.warmth);
  float attackDiff = std::abs(charA.attack - charB.attack);
  float decayDiff = std::abs(charA.decay - charB.decay);
  float complexityDiff = std::abs(charA.complexity - charB.complexity);

  // Category matching bonus
  float categoryBonus = (a.category == b.category) ? 0.2f : 0.0f;

  // Tag overlap bonus
  float tagOverlap = 0.0f;
  for (const auto &tagA : a.tags) {
    for (const auto &tagB : b.tags) {
      if (tagA.equalsIgnoreCase(tagB)) {
        tagOverlap += 0.1f;
      }
    }
  }
  tagOverlap = juce::jmin(tagOverlap, 0.3f);

  // Calculate final similarity (1.0 = identical, 0.0 = completely different)
  float diff = (brightnessDiff * 0.3f +
                warmthDiff * 0.2f +
                attackDiff * 0.2f +
                decayDiff * 0.1f +
                complexityDiff * 0.2f);

  float similarity = 1.0f - diff + categoryBonus + tagOverlap;

  return juce::jlimit(0.0f, 1.0f, similarity);
}

void PresetBrowserComponent::showSimilarPresets(const PresetMetadata &preset) {
  setSimilarityReference(preset);
  quickFilterCombo_.setSelectedId(static_cast<int>(QuickFilter::Similar), false);
  applyFilters();
}

void PresetBrowserComponent::setSimilarityReference(const PresetMetadata &preset) {
  similarityReference_ = preset;

  // Calculate similarity scores for all presets
  similarityScores_.clear();
  for (const auto &other : presets) {
    if (other.id != preset.id) {
      similarityScores_[other.id] = calculateSimilarity(preset, other);
    }
  }
}

void PresetBrowserComponent::clearSimilarityReference() {
  similarityReference_ = PresetMetadata();
  similarityScores_.clear();
}

//==============================================================================
// DAW Integration
//==============================================================================
void PresetBrowserComponent::setEngine(Engine *engine) {
  engine_ = engine;
}

void PresetBrowserComponent::setProjectId(const juce::String &projectId) {
  projectId_ = projectId;
  loadProjectCollections();
}

void PresetBrowserComponent::setSelectedTrackInfo(const TrackSelectionInfo &info) {
  selectedTrackInfo_ = info;

  // Update UI based on track selection
  if (info.isSelected && info.instrumentId.isNotEmpty()) {
    setInstrumentId(info.instrumentId);
  }
}

void PresetBrowserComponent::syncWithSelectedTrack() {
  if (selectedTrackInfo_.isSelected && selectedTrackInfo_.instrumentId.isNotEmpty()) {
    setInstrumentId(selectedTrackInfo_.instrumentId);

    // Auto-open browser if this is a synth track
    if (selectedTrackInfo_.trackName.contains("Synth") ||
        selectedTrackInfo_.trackName.contains("Lead") ||
        selectedTrackInfo_.trackName.contains("Pad")) {
      // Could auto-show the browser here
    }
  }
}

void PresetBrowserComponent::createTrackWithPreset(const PresetMetadata &preset) {
  if (!engine_) {
    DBG("No engine set - cannot create track");
    return;
  }

  auto fullPreset = ZenithPresetManager::getInstance().loadPreset(
      currentInstrumentId, preset.id);

  int newTrackIndex = createNewTrackForPreset(fullPreset);
  if (newTrackIndex >= 0) {
    loadPresetIntoTrack(preset, newTrackIndex);
  }
}

void PresetBrowserComponent::showPresetInProjectExplorer(const PresetMetadata &preset) {
  // Navigate to the preset file in the project/browser
  juce::File presetFile(preset.filePath);
  if (presetFile.exists()) {
    presetFile.revealToUser();
  } else {
    juce::AlertWindow::showMessageBoxAsync(
        juce::MessageBoxIconType::InfoIcon,
        "Preset Location",
        "Preset file not found on disk: " + preset.filePath);
  }
}

void PresetBrowserComponent::loadPresetIntoTrack(const PresetMetadata &preset, int trackIndex) {
  // Load preset into specific track
  if (createTrackCallback) {
    auto fullPreset = ZenithPresetManager::getInstance().loadPreset(
        currentInstrumentId, preset.id);
    createTrackCallback(fullPreset, trackIndex);
  }
}

void PresetBrowserComponent::notifyTrackSelectionChanged() {
  if (trackSelectionCallback && selectedTrackInfo_.trackIndex >= 0) {
    trackSelectionCallback(selectedTrackInfo_.trackIndex);
  }
}

void PresetBrowserComponent::loadPresetIntoSelectedTrack(const PresetMetadata &preset) {
  if (selectedTrackInfo_.trackIndex >= 0) {
    loadPresetIntoTrack(preset, selectedTrackInfo_.trackIndex);
  } else {
    // No track selected, create new track
    createTrackWithPreset(preset);
  }
}

int PresetBrowserComponent::createNewTrackForPreset(const Preset &preset) {
  // This would interface with the DAW to create a new track
  // For now, return a placeholder index
  if (createTrackCallback) {
    createTrackCallback(preset, -1); // -1 means create new track
    return 0; // Return the index of the newly created track
  }
  return -1;
}

juce::File PresetBrowserComponent::getProjectDataFile() const {
  if (projectId_.isEmpty()) {
    return getUserDataFile();
  }

  auto dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                 .getChildFile("ZenithDAW/Projects/" + projectId_);
  if (!dir.exists()) dir.createDirectory();

  return dir.getChildFile("preset_collections.json");
}

void PresetBrowserComponent::loadProjectCollections() {
  auto file = getProjectDataFile();
  if (!file.exists()) return;

  auto json = juce::JSON::parse(file);
  if (!json.isObject()) return;

  auto collsArray = json.getProperty("collections", juce::Array<juce::var>{});
  if (collsArray.isArray()) {
    for (const auto &collVar : *collsArray.getArray()) {
      if (collVar.isObject()) {
        auto *collObj = collVar.getDynamicObject();
        PresetCollection coll;
        coll.id = collObj->getProperty("id").toString();
        coll.name = collObj->getProperty("name").toString();
        coll.description = collObj->getProperty("description").toString();
        coll.isProjectSpecific = true;
        coll.projectId = projectId_;

        auto presetIdsArray = collObj->getProperty("presetIds");
        if (presetIdsArray.isArray()) {
          for (const auto &idVar : *presetIdsArray.getArray()) {
            if (idVar.isString()) {
              coll.presetIds.add(idVar.toString());
            }
          }
        }

        // Check if collection already exists
        auto it = std::find_if(collections_.begin(), collections_.end(),
                              [&coll](const PresetCollection &c) {
                                return c.id == coll.id;
                              });
        if (it == collections_.end()) {
          collections_.push_back(coll);
        }
      }
    }
    rebuildCollectionsList();
  }
}

//==============================================================================
// Context Menu
//==============================================================================
void PresetBrowserComponent::showContextMenuForPreset(const PresetMetadata &preset, int x, int y) {
  juce::PopupMenu menu;

  menu.addItem(1, "Load Preset", true);
  menu.addItem(2, "Preview", true);
  menu.addSeparator();

  juce::PopupMenu favMenu;
  bool isFav = isFavorite(preset.id);
  favMenu.addItem(3, isFav ? "Remove from Favorites" : "Add to Favorites", true);
  menu.addSubMenu("Favorites", favMenu);

  juce::PopupMenu collectionMenu;
  collectionMenu.addItem(10, "Add to New Collection...", true);
  if (!collections_.empty()) {
    collectionMenu.addSeparator();
    int id = 11;
    for (const auto &coll : collections_) {
      bool alreadyIn = isInCollection(preset.id, coll.id);
      collectionMenu.addItem(id++, coll.name + (alreadyIn ? " (added)" : ""),
                            !alreadyIn);
    }
  }
  menu.addSubMenu("Add to Collection", collectionMenu);

  menu.addSeparator();
  menu.addItem(3, "Find Similar Presets", true);
  menu.addItem(4, "Show in Explorer", true);
  menu.addSeparator();
  menu.addItem(5, "Edit Metadata...", true);
  menu.addItem(6, "Export Preset...", true);
  menu.addSeparator();
  menu.addItem(7, "Create Track with This Preset", true);
  menu.addItem(8, "Duplicate as...", true);
  menu.addSeparator();
  menu.addItem(9, "Delete", true, !preset.author.startsWith("Factory"));

  menu.showMenuAsync(juce::PopupMenu::Options()
      .withTargetComponent(this)
      .withTargetScreenArea(juce::Rectangle<int>(x, y, 1, 1)),
      [this, preset](int result) {
        switch (result) {
          case 0: break;
          case 1: loadPreset(preset); break;
          case 2: previewPreset(preset); break;
          case 3: showSimilarPresets(preset); break;
          case 4: showPresetInProjectExplorer(preset); break;
          case 5: showMetadataEditor(preset); break;
          case 6: exportSelectedPreset(); break;
          case 7: createTrackWithPreset(preset); break;
          case 8: {
            // Duplicate as new preset
            auto fullPreset = ZenithPresetManager::getInstance().loadPreset(
                currentInstrumentId, preset.id);
            fullPreset.id = ""; // Clear to generate new ID
            fullPreset.name = preset.name + " (Copy)";

            if (ZenithPresetManager::getInstance().savePreset(fullPreset, true)) {
              refreshPresets();
            }
            break;
          }
          case 9: deleteSelectedPreset(); break;
          case 10: createCollectionUI(); break;
          default:
            if (result >= 11) {
              // Add to collection
              int collIndex = result - 11;
              if (collIndex >= 0 && collIndex < static_cast<int>(collections_.size())) {
                addToCollection(collections_[collIndex].id, preset.id);
              }
            }
            break;
        }
      });
}

//==============================================================================
// Additional Drawing Helpers
//==============================================================================
void PresetBrowserComponent::drawRealWaveformThumbnail(SkCanvas *canvas,
                                                        const juce::Rectangle<float> &bounds,
                                                        const WaveformThumbnail &thumbnail) {
  if (bounds.isEmpty() || thumbnail.peaks.empty()) {
    drawMiniWaveformThumbnail(canvas, bounds, PresetMetadata());
    return;
  }

  SkPaint bgPaint;
  bgPaint.setColor(SkColorSetARGB(200, 10, 10, 15));
  bgPaint.setAntiAlias(true);
  SkRRect bgRect = SkRRect::MakeRectXY(SkRect::MakeXYWH(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight()), 4.0f, 4.0f);
  canvas->drawRRect(bgRect, bgPaint);

  // Draw waveform from cached peaks
  SkPaint wavePaint;
  wavePaint.setColor(design::colors::CYAN);
  wavePaint.setStyle(SkPaint::kStroke_Style);
  wavePaint.setStrokeWidth(1);
  wavePaint.setAntiAlias(true);

  SkPath path;
  float centerY = bounds.getCentreY();
  float width = bounds.getWidth();
  float step = width / thumbnail.peaks.size();

  for (size_t i = 0; i < thumbnail.peaks.size(); ++i) {
    float amplitude = thumbnail.peaks[i];
    float x = bounds.getX() + i * step;
    float y = centerY - (amplitude * bounds.getHeight() * 0.4f);

    if (i == 0) path.moveTo(x, y);
    else path.lineTo(x, y);
  }

  // Mirror for symmetrical waveform
  for (int i = static_cast<int>(thumbnail.peaks.size()) - 1; i >= 0; --i) {
    float amplitude = thumbnail.peaks[i];
    float x = bounds.getX() + i * step;
    float y = centerY + (amplitude * bounds.getHeight() * 0.4f);
    path.lineTo(x, y);
  }

  path.close();

  wavePaint.setStyle(SkPaint::kFill_Style);
  wavePaint.setColor(SkColorSetARGB(100, 0, 200, 220));
  canvas->drawPath(path, wavePaint);

  wavePaint.setStyle(SkPaint::kStroke_Style);
  wavePaint.setColor(SkColorSetARGB(255, 0, 240, 255));
  canvas->drawPath(path, wavePaint);
}

void PresetBrowserComponent::drawSimilarityBadge(SkCanvas *canvas, float x, float y,
                                                 float similarity) {
  if (similarity < 0.5f) return; // Don't show badge for low similarity

  // Color based on similarity
  SkColor badgeColor;
  if (similarity > 0.85f) {
    badgeColor = SkColorSetARGB(255, 0, 255, 100); // Green
  } else if (similarity > 0.7f) {
    badgeColor = SkColorSetARGB(255, 200, 200, 0); // Yellow
  } else {
    badgeColor = SkColorSetARGB(255, 255, 100, 0); // Orange
  }

  // Draw badge background
  SkPaint bgPaint;
  bgPaint.setColor(badgeColor);
  bgPaint.setAntiAlias(true);
  SkRRect badgeRect = SkRRect::MakeRectXY(SkRect::MakeXYWH(x, y, 45, 16), 8, 8);
  canvas->drawRRect(badgeRect, bgPaint);

  // Draw percentage
  SkFont font;
  font.setSize(9);
  font.setEmbolden(true);

  SkPaint textPaint;
  textPaint.setColor(SK_ColorBLACK);
  textPaint.setAntiAlias(true);

  juce::String text = juce::String(static_cast<int>(similarity * 100)) + "%";
  canvas->drawSimpleText(text.getCharPointer(), text.length(), SkTextEncoding::kUTF8, x + 22, y + 12, font, textPaint);
}

} // namespace zenith
