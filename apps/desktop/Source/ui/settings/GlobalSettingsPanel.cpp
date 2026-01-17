/*
  ==============================================================================

    GlobalSettingsPanel.cpp
    Created: 2025-12-30 / Redesigned: 2026-01-13
    Author:  Zenith DAW

    Professional Settings Panel Implementation
    High-quality, searchable, and grouped DAW settings.

  ==============================================================================
*/

#include "GlobalSettingsPanel.h"
#include "../design-system/ZenithTheme.h"
#include "../design-system/ZenithDesignSystem.h"
#include "../design-system/ZenithIcons.h"
#include "../../Settings.h"
#include "../../engine/ZenithLogger.h"
#include <core/SkFont.h>
#include <core/SkPaint.h>
#include <core/SkPath.h>
#include <core/SkRRect.h>
#include <effects/SkImageFilters.h>
#include <effects/SkGradientShader.h>

namespace zenith {

// Helper to convert juce::Rectangle to SkRect
static SkRect toSkRect(const juce::Rectangle<float>& r) {
    return SkRect::MakeLTRB(r.getX(), r.getY(), r.getRight(), r.getBottom());
}

//==============================================================================
// SettingRow - Individual setting with label, description, and control
//==============================================================================

SettingRow::SettingRow(const juce::String& id, const juce::String& label, 
                       const juce::String& description, Type type)
    : id_(id), label_(label), description_(description), type_(type) {
  
  switch (type_) {
    case Type::Toggle:
      toggle_ = std::make_unique<ZenithToggle>();
      toggle_->onToggle = [this](bool state) { 
        if (onToggle) onToggle(state); 
        isChanged_ = true; 
        markDirty(); 
      };
      addAndMakeVisible(toggle_.get());
      break;
      
    case Type::Combo:
      combo_ = std::make_unique<SkiaComboBox>();
      combo_->onChange = [this] { 
        if (onComboChange) onComboChange(); 
        isChanged_ = true; 
        markDirty(); 
      };
      addAndMakeVisible(combo_.get());
      break;
      
    case Type::Button:
      button_ = std::make_unique<ZenithButton>();
      button_->setStyle(ZenithButton::Style::Secondary);
      button_->onClick = [this] { if (onButtonClick) onButtonClick(); };
      addAndMakeVisible(button_.get());
      break;
      
    case Type::Info:
      break;
  }
}

void SettingRow::resized() {
  auto bounds = getLocalBounds();
  int controlW = 200;
  int controlH = 28;
  int rightPadding = 24;
  int x = bounds.getWidth() - controlW - rightPadding;
  int y = (bounds.getHeight() - controlH) / 2;

  if (toggle_) toggle_->setBounds(x + controlW - 52, y, 52, 26);
  if (combo_) combo_->setBounds(x, y, controlW, controlH);
  if (button_) button_->setBounds(x, y, controlW, controlH);
}

void SettingRow::drawSkia(SkCanvas* canvas) {
  auto bounds = getLocalBounds().toFloat();
  float hoverAnim = getAnimatedValue("hover");
  
  // Hover background with animation
  if (hoverAnim > 0.01f) {
    SkPaint hoverPaint;
    hoverPaint.setColor(SkColorSetARGB((int)(20 * hoverAnim), 255, 255, 255));
    hoverPaint.setAntiAlias(true);
    canvas->drawRoundRect(SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()), 6, 6, hoverPaint);
  }
  
  // Change indicator dot
  if (isChanged_) {
    SkPaint dotPaint;
    dotPaint.setColor(design::colors::CYAN);
    dotPaint.setAntiAlias(true);
    canvas->drawCircle(12.0f, bounds.getHeight() / 2, 4.0f, dotPaint);
    
    // Glow effect
    dotPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 4.0f));
    dotPaint.setColor(SkColorSetA(design::colors::CYAN, 100));
    canvas->drawCircle(12.0f, bounds.getHeight() / 2, 4.0f, dotPaint);
  }
  
  // Label text
  float labelX = isChanged_ ? 28.0f : 20.0f;
  SkFont labelFont = design::getSkFont(14.0f, design::FontWeight::Medium);
  SkPaint labelPaint;
  labelPaint.setColor(design::colors::TEXT_PRIMARY);
  labelPaint.setAntiAlias(true);
  canvas->drawString(label_.toRawUTF8(), labelX, 22.0f, labelFont, labelPaint);
  
  // Description text
  SkFont descFont = design::getSkFont(11.0f, design::FontWeight::Regular);
  SkPaint descPaint;
  descPaint.setColor(design::colors::TEXT_SECONDARY);
  descPaint.setAntiAlias(true);
  canvas->drawString(description_.toRawUTF8(), labelX, 40.0f, descFont, descPaint);
  
  // Info text (for button rows showing current value)
  if (type_ == Type::Info || (type_ == Type::Button && infoText_.isNotEmpty())) {
    SkFont infoFont = design::getSkFont(12.0f, design::FontWeight::Regular);
    SkPaint infoPaint;
    infoPaint.setColor(design::colors::TEXT_TERTIARY);
    infoPaint.setAntiAlias(true);
    float infoX = bounds.getWidth() - 230.0f;
    canvas->drawString(infoText_.toRawUTF8(), infoX, 32.0f, infoFont, infoPaint);
  }
  
  // Bottom separator line
  SkPaint linePaint;
  linePaint.setColor(design::colors::BORDER_SUBTLE);
  canvas->drawRect(SkRect::MakeLTRB(20.0f, bounds.getHeight() - 1.0f, 
                                     bounds.getWidth() - 20.0f, bounds.getHeight()), linePaint);
  
  // Draw child controls
  drawChildren(canvas);
}

void SettingRow::mouseEnter(const juce::MouseEvent& e) {
  SkiaComponent::mouseEnter(e);
  animateTo("hover", 1.0f, 150);
}

void SettingRow::mouseExit(const juce::MouseEvent& e) {
  SkiaComponent::mouseExit(e);
  animateTo("hover", 0.0f, 200);
}

bool SettingRow::matchesSearch(const juce::String& query) const {
  if (query.isEmpty()) return true;
  return label_.containsIgnoreCase(query) || description_.containsIgnoreCase(query);
}

void SettingRow::setToggleState(bool state) { 
  if (toggle_) toggle_->setToggleState(state, false); 
}

bool SettingRow::getToggleState() const { 
  return toggle_ ? toggle_->getToggleState() : false; 
}

void SettingRow::setComboItems(const juce::StringArray& items) { 
  if (combo_) { 
    combo_->clear(); 
    for (int i = 0; i < items.size(); ++i) 
      combo_->addItem(items[i], i + 1); 
  } 
}

void SettingRow::setSelectedId(int id) { 
  if (combo_) combo_->setSelectedId(id, false); 
}

int SettingRow::getSelectedId() const { 
  return combo_ ? combo_->getSelectedId() : 0; 
}

juce::String SettingRow::getSelectedText() const { 
  return combo_ ? combo_->getText() : ""; 
}

void SettingRow::setButtonText(const juce::String& text) { 
  if (button_) button_->setButtonText(text); 
}

void SettingRow::setInfoText(const juce::String& text) { 
  infoText_ = text; 
  markDirty(); 
}

//==============================================================================
// SettingGroup - Container for related settings with header
//==============================================================================

SettingGroup::SettingGroup(const juce::String& title) : title_(title) {}

void SettingGroup::addSetting(SettingRow* row) { 
  rows_.push_back(row); 
  addAndMakeVisible(row);
}

void SettingGroup::filterSettings(const juce::String& query) { 
  for (auto* row : rows_) 
    row->setVisible(row->matchesSearch(query)); 
  resized();
  markDirty(); 
}

bool SettingGroup::hasVisibleSettings() const { 
  for (auto* row : rows_) 
    if (row->isVisible()) return true; 
  return false; 
}

int SettingGroup::getContentHeight() const { 
  if (!hasVisibleSettings()) return 0; 
  int h = 48; // Header height
  for (auto* row : rows_) 
    if (row->isVisible()) h += 56; // Row height
  return h + 8; // Bottom padding
}

void SettingGroup::resized() { 
  int y = 48; 
  for (auto* row : rows_) { 
    if (row->isVisible()) { 
      row->setBounds(0, y, getWidth(), 56); 
      y += 56; 
    } 
  } 
}

void SettingGroup::drawSkia(SkCanvas* canvas) {
  if (!hasVisibleSettings()) return;
  
  auto bounds = getLocalBounds().toFloat();
  
  // Group background with subtle gradient
  SkPaint bgPaint;
  bgPaint.setAntiAlias(true);
  SkPoint pts[2] = {{0, 0}, {0, bounds.getHeight()}};
  SkColor colors[2] = {
    SkColorSetARGB(8, 255, 255, 255),
    SkColorSetARGB(0, 255, 255, 255)
  };
  bgPaint.setShader(SkGradientShader::MakeLinear(pts, colors, nullptr, 2, SkTileMode::kClamp));
  canvas->drawRoundRect(SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()), 8, 8, bgPaint);
  
  // Header background
  SkPaint headPaint;
  headPaint.setColor(SkColorSetARGB(15, 0, 200, 255));
  headPaint.setAntiAlias(true);
  SkRect headRect = SkRect::MakeXYWH(0, 0, bounds.getWidth(), 44);
  canvas->drawRoundRect(headRect, 8, 8, headPaint);
  // Square off bottom corners
  canvas->drawRect(SkRect::MakeXYWH(0, 36, bounds.getWidth(), 8), headPaint);
  
  // Header title
  SkFont titleFont = design::getSkFont(11.0f, design::FontWeight::Bold);
  SkPaint titlePaint;
  titlePaint.setColor(design::colors::CYAN);
  titlePaint.setAntiAlias(true);
  canvas->drawString(title_.toUpperCase().toRawUTF8(), 16.0f, 28.0f, titleFont, titlePaint);
  
  // Draw child rows
  drawChildren(canvas);
}

//==============================================================================
// GlobalSettingsPanel - Main settings dialog
//==============================================================================

GlobalSettingsPanel::GlobalSettingsPanel(juce::AudioDeviceManager& deviceManager)
    : deviceManager_(deviceManager) {
  // NOTE: setSize() will trigger resized(), which may be called before
  // member variables are initialized. The resized() method has a guard
  // to handle this case - do NOT remove it or the app will crash.
  // See resized() null check for details.
  setSize(850, 650);
  setVisible(false);

  // Search field
  searchField_ = std::make_unique<SkiaTextInput>();
  searchField_->setPlaceholder("Search settings...");
  searchField_->onTextChanged = [this](const juce::String& text) { filterSettings(text); };
  addAndMakeVisible(searchField_.get());

  // Close button
  closeBtn_ = std::make_unique<ZenithButton>("Done");
  closeBtn_->setStyle(ZenithButton::Style::Primary);
  closeBtn_->onClick = [this] { 
    animateTo("opacity", 0.0f, 200);
    juce::Timer::callAfterDelay(200, [this] {
      if (onClose) onClose(); 
      setVisible(false); 
    });
  };
  addAndMakeVisible(closeBtn_.get());

  // Reset button
  resetBtn_ = std::make_unique<ZenithButton>("Reset Tab");
  resetBtn_->setStyle(ZenithButton::Style::Ghost);
  resetBtn_->onClick = [this] { resetCurrentTabToDefaults(); };
  addAndMakeVisible(resetBtn_.get());

  // Create tabs
  createTabs();
  
  // Create all settings
  createAudioSettings();
  createMidiSettings();
  createRecordingSettings();
  createEditingSettings();
  createDisplaySettings();
  createGeneralSettings();

  // Listen for device changes
  deviceManager_.addChangeListener(this);
  
  // Sync with current settings
  syncWithSettings();
  refreshAudioDevices();
  setCurrentTab(Tab::Audio);
  
  // Start with fade-in animation
  animateTo("opacity", 1.0f, 300);
}

GlobalSettingsPanel::~GlobalSettingsPanel() {
  deviceManager_.removeChangeListener(this);
}

void GlobalSettingsPanel::createTabs() {
  struct TabInfo { const char* name; const char* icon; };
  TabInfo tabs[] = {
    {"Audio", "🔊"},
    {"MIDI", "🎹"},
    {"Recording", "⏺"},
    {"Editing", "✂"},
    {"Display", "🖥"},
    {"General", "⚙"}
  };
  
  for (int i = 0; i < 6; ++i) {
    auto btn = std::make_unique<ZenithButton>(tabs[i].name);
    btn->setStyle(ZenithButton::Style::Ghost);
    btn->setToggleable(true);
    btn->onClick = [this, i] { setCurrentTab(static_cast<Tab>(i)); };
    addAndMakeVisible(btn.get());
    tabButtons_.push_back(std::move(btn));
  }
}


//==============================================================================
// Row Creation Helpers
//==============================================================================

SettingRow* GlobalSettingsPanel::createToggleRow(Tab tab, const juce::String& id, 
    const juce::String& label, const juce::String& desc, const juce::String& tooltip) {
  auto row = std::make_unique<SettingRow>(id, label, desc, SettingRow::Type::Toggle);
  row->setTooltipText(tooltip);
  auto* ptr = row.get();
  rows_[tab].push_back(std::move(row));
  return ptr;
}

SettingRow* GlobalSettingsPanel::createComboRow(Tab tab, const juce::String& id,
    const juce::String& label, const juce::String& desc, const juce::String& tooltip,
    const juce::StringArray& items) {
  auto row = std::make_unique<SettingRow>(id, label, desc, SettingRow::Type::Combo);
  row->setTooltipText(tooltip);
  row->setComboItems(items);
  auto* ptr = row.get();
  rows_[tab].push_back(std::move(row));
  return ptr;
}

SettingRow* GlobalSettingsPanel::createButtonRow(Tab tab, const juce::String& id,
    const juce::String& label, const juce::String& desc, const juce::String& tooltip,
    const juce::String& buttonText) {
  auto row = std::make_unique<SettingRow>(id, label, desc, SettingRow::Type::Button);
  row->setTooltipText(tooltip);
  row->setButtonText(buttonText);
  auto* ptr = row.get();
  rows_[tab].push_back(std::move(row));
  return ptr;
}

SettingRow* GlobalSettingsPanel::createInfoRow(Tab tab, const juce::String& id,
    const juce::String& label, const juce::String& desc) {
  auto row = std::make_unique<SettingRow>(id, label, desc, SettingRow::Type::Info);
  auto* ptr = row.get();
  rows_[tab].push_back(std::move(row));
  return ptr;
}

//==============================================================================
// Audio Settings
//==============================================================================

void GlobalSettingsPanel::createAudioSettings() {
  auto& settings = Settings::getInstance();
  
  // Device Group
  auto deviceGroup = std::make_unique<SettingGroup>("AUDIO DEVICE");
  
  auto* sampleRate = createComboRow(Tab::Audio, "sampleRate", "Sample Rate",
    "Audio sample rate for playback and recording", "Higher rates = better quality, more CPU",
    {"44100 Hz", "48000 Hz", "88200 Hz", "96000 Hz", "176400 Hz", "192000 Hz"});
  sampleRate->onComboChange = [this, sampleRate] {
    auto* device = deviceManager_.getCurrentAudioDevice();
    if (device) {
      double rates[] = {44100, 48000, 88200, 96000, 176400, 192000};
      int idx = sampleRate->getSelectedId() - 1;
      if (idx >= 0 && idx < 6) {
        auto setup = deviceManager_.getAudioDeviceSetup();
        setup.sampleRate = rates[idx];
        deviceManager_.setAudioDeviceSetup(setup, true);
      }
    }
  };
  deviceGroup->addSetting(sampleRate);
  
  auto* bufferSize = createComboRow(Tab::Audio, "bufferSize", "Buffer Size",
    "Audio buffer size (latency vs stability)", "Lower = less latency, higher CPU",
    {"64", "128", "256", "512", "1024", "2048"});
  bufferSize->onComboChange = [this, bufferSize] {
    auto* device = deviceManager_.getCurrentAudioDevice();
    if (device) {
      int sizes[] = {64, 128, 256, 512, 1024, 2048};
      int idx = bufferSize->getSelectedId() - 1;
      if (idx >= 0 && idx < 6) {
        auto setup = deviceManager_.getAudioDeviceSetup();
        setup.bufferSize = sizes[idx];
        deviceManager_.setAudioDeviceSetup(setup, true);
        Settings::getInstance().setBufferSize(sizes[idx]);
      }
    }
  };
  deviceGroup->addSetting(bufferSize);

  auto* testAudio = createButtonRow(Tab::Audio, "testAudio", "Test Audio",
    "Play a test tone to verify audio output", "", "Play Test Tone");
  testAudio->onButtonClick = [this] {
    // Simple test tone implementation - could be expanded
    DBG("Audio test tone requested");
  };
  deviceGroup->addSetting(testAudio);
  
  addAndMakeVisible(deviceGroup.get());
  groups_[Tab::Audio].push_back(std::move(deviceGroup));
  
  // Processing Group
  auto procGroup = std::make_unique<SettingGroup>("PROCESSING");
  
  auto* pdc = createToggleRow(Tab::Audio, "pdc", "Plugin Delay Compensation",
    "Automatically compensate for plugin latency", "Keeps tracks in sync when using plugins");
  pdc->setToggleState(settings.getPluginDelayCompensation());
  pdc->onToggle = [](bool state) { Settings::getInstance().setPluginDelayCompensation(state); };
  procGroup->addSetting(pdc);
  
  auto* monitoring = createToggleRow(Tab::Audio, "monitoring", "Software Monitoring",
    "Monitor input through software", "Enable to hear input with effects");
  monitoring->setToggleState(settings.getSoftwareMonitoring());
  monitoring->onToggle = [](bool state) { Settings::getInstance().setSoftwareMonitoring(state); };
  procGroup->addSetting(monitoring);
  
  addAndMakeVisible(procGroup.get());
  groups_[Tab::Audio].push_back(std::move(procGroup));
}

//==============================================================================
// MIDI Settings
//==============================================================================

void GlobalSettingsPanel::createMidiSettings() {
  auto& settings = Settings::getInstance();
  
  auto midiGroup = std::make_unique<SettingGroup>("MIDI ROUTING");
  
  auto* midiThrough = createToggleRow(Tab::MIDI, "midiThrough", "MIDI Thru",
    "Pass MIDI input directly to output", "Enable for live performance");
  midiThrough->setToggleState(settings.getMIDIThrough());
  midiThrough->onToggle = [](bool state) { Settings::getInstance().setMIDIThrough(state); };
  midiGroup->addSetting(midiThrough);
  
  auto* clockOut = createToggleRow(Tab::MIDI, "clockOut", "Send MIDI Clock",
    "Transmit MIDI clock to external devices", "Sync external gear to Zenith");
  clockOut->setToggleState(settings.getSendMIDIClockOut());
  clockOut->onToggle = [](bool state) { Settings::getInstance().setSendMIDIClockOut(state); };
  midiGroup->addSetting(clockOut);
  
  auto* mtcIn = createToggleRow(Tab::MIDI, "mtcIn", "Receive MTC",
    "Sync to incoming MIDI Time Code", "Slave to external timecode");
  mtcIn->setToggleState(settings.getReceiveMTCIn());
  mtcIn->onToggle = [](bool state) { Settings::getInstance().setReceiveMTCIn(state); };
  midiGroup->addSetting(mtcIn);
  
  auto* latencyComp = createComboRow(Tab::MIDI, "midiLatency", "Latency Compensation",
    "Compensate for MIDI device latency", "Adjust if notes feel late",
    {"0 ms", "5 ms", "10 ms", "15 ms", "20 ms", "25 ms", "30 ms"});
  int currentLatency = settings.getMIDILatencyCompensation();
  latencyComp->setSelectedId((currentLatency / 5) + 1);
  latencyComp->onComboChange = [latencyComp] {
    int ms = (latencyComp->getSelectedId() - 1) * 5;
    Settings::getInstance().setMIDILatencyCompensation(ms);
  };
  midiGroup->addSetting(latencyComp);
  
  addAndMakeVisible(midiGroup.get());
  groups_[Tab::MIDI].push_back(std::move(midiGroup));
}

//==============================================================================
// Recording Settings
//==============================================================================

void GlobalSettingsPanel::createRecordingSettings() {
  auto& settings = Settings::getInstance();
  
  auto recGroup = std::make_unique<SettingGroup>("RECORDING");
  
  auto* bitDepth = createComboRow(Tab::Recording, "bitDepth", "Bit Depth",
    "Recording bit depth", "Higher = better quality, larger files",
    {"16-bit", "24-bit", "32-bit Float"});
  bitDepth->setSelectedId((int)settings.getRecordingBitDepth() + 1);
  bitDepth->onComboChange = [bitDepth] {
    Settings::getInstance().setRecordingBitDepth(
      (Settings::RecordingBitDepth)(bitDepth->getSelectedId() - 1));
  };
  recGroup->addSetting(bitDepth);
  
  auto* fileType = createComboRow(Tab::Recording, "fileType", "File Format",
    "Recording file format", "WAV is most compatible",
    {"WAV", "AIFF", "FLAC"});
  fileType->setSelectedId((int)settings.getRecordingFileType() + 1);
  fileType->onComboChange = [fileType] {
    Settings::getInstance().setRecordingFileType(
      (Settings::RecordingFileType)(fileType->getSelectedId() - 1));
  };
  recGroup->addSetting(fileType);
  
  addAndMakeVisible(recGroup.get());
  groups_[Tab::Recording].push_back(std::move(recGroup));
  
  auto countGroup = std::make_unique<SettingGroup>("COUNT-IN");
  
  auto* metronome = createToggleRow(Tab::Recording, "metronomeCountIn", "Metronome Count-In",
    "Play metronome during count-in", "Hear the beat before recording starts");
  metronome->setToggleState(settings.getMetronomeCountIn());
  metronome->onToggle = [](bool state) { Settings::getInstance().setMetronomeCountIn(state); };
  countGroup->addSetting(metronome);
  
  auto* countBars = createComboRow(Tab::Recording, "countInBars", "Count-In Bars",
    "Number of bars before recording", "",
    {"1 Bar", "2 Bars", "4 Bars"});
  countBars->setSelectedId(settings.getCountInBars());
  countBars->onComboChange = [countBars] {
    Settings::getInstance().setCountInBars(countBars->getSelectedId());
  };
  countGroup->addSetting(countBars);
  
  addAndMakeVisible(countGroup.get());
  groups_[Tab::Recording].push_back(std::move(countGroup));
}

//==============================================================================
// Editing Settings
//==============================================================================

void GlobalSettingsPanel::createEditingSettings() {
  auto& settings = Settings::getInstance();
  
  auto editGroup = std::make_unique<SettingGroup>("EDITING BEHAVIOR");
  
  auto* snapGrid = createToggleRow(Tab::Editing, "snapToGrid", "Snap to Grid",
    "Snap edits to grid divisions", "Keeps edits aligned to beats");
  snapGrid->setToggleState(settings.getSnapToGrid());
  snapGrid->onToggle = [](bool state) { Settings::getInstance().setSnapToGrid(state); };
  editGroup->addSetting(snapGrid);
  
  auto* linkSelection = createToggleRow(Tab::Editing, "linkSelection", "Link Track & Edit Selection",
    "Selecting a clip also selects its track", "Streamlines workflow");
  linkSelection->setToggleState(settings.getLinkTrackAndEditSelection());
  linkSelection->onToggle = [](bool state) { Settings::getInstance().setLinkTrackAndEditSelection(state); };
  editGroup->addSetting(linkSelection);
  
  auto* crossfade = createComboRow(Tab::Editing, "crossfade", "Default Crossfade",
    "Crossfade duration for overlapping clips", "",
    {"0 ms", "5 ms", "10 ms", "20 ms", "50 ms", "100 ms"});
  int cf = settings.getDefaultCrossfadeMs();
  int cfIdx = (cf == 0) ? 1 : (cf == 5) ? 2 : (cf == 10) ? 3 : (cf == 20) ? 4 : (cf == 50) ? 5 : 6;
  crossfade->setSelectedId(cfIdx);
  crossfade->onComboChange = [crossfade] {
    int vals[] = {0, 5, 10, 20, 50, 100};
    int idx = crossfade->getSelectedId() - 1;
    if (idx >= 0 && idx < 6) Settings::getInstance().setDefaultCrossfadeMs(vals[idx]);
  };
  editGroup->addSetting(crossfade);
  
  addAndMakeVisible(editGroup.get());
  groups_[Tab::Editing].push_back(std::move(editGroup));
  
  auto undoGroup = std::make_unique<SettingGroup>("UNDO HISTORY");
  
  auto* undoLevels = createComboRow(Tab::Editing, "undoLevels", "Max Undo Steps",
    "Maximum number of undo operations", "More steps = more memory",
    {"25", "50", "100", "200", "500"});
  int undo = settings.getMaxUndoHistory();
  int undoIdx = (undo <= 25) ? 1 : (undo <= 50) ? 2 : (undo <= 100) ? 3 : (undo <= 200) ? 4 : 5;
  undoLevels->setSelectedId(undoIdx);
  undoLevels->onComboChange = [undoLevels] {
    int vals[] = {25, 50, 100, 200, 500};
    int idx = undoLevels->getSelectedId() - 1;
    if (idx >= 0 && idx < 5) Settings::getInstance().setMaxUndoHistory(vals[idx]);
  };
  undoGroup->addSetting(undoLevels);
  
  addAndMakeVisible(undoGroup.get());
  groups_[Tab::Editing].push_back(std::move(undoGroup));
}

//==============================================================================
// Display Settings
//==============================================================================

void GlobalSettingsPanel::createDisplaySettings() {
  auto& settings = Settings::getInstance();
  
  auto themeGroup = std::make_unique<SettingGroup>("APPEARANCE");
  
  auto* theme = createComboRow(Tab::Display, "theme", "Theme",
    "Visual theme for the interface", "",
    {"Neon Noir", "Dark", "Light"});
  theme->setSelectedId((int)settings.getTheme() + 1);
  theme->onComboChange = [theme] {
    Settings::getInstance().setTheme((Settings::UITheme)(theme->getSelectedId() - 1));
    design::ThemeManager::getInstance().setActiveTheme(
      (design::ThemePreset)(theme->getSelectedId() - 1));
  };
  themeGroup->addSetting(theme);
  
  auto* animations = createToggleRow(Tab::Display, "animations", "Animations",
    "Enable UI animations", "Disable for better performance");
  animations->setToggleState(settings.getAnimationsEnabled());
  animations->onToggle = [](bool state) { Settings::getInstance().setAnimationsEnabled(state); };
  themeGroup->addSetting(animations);
  
  auto* highContrast = createToggleRow(Tab::Display, "highContrast", "High Contrast Mode",
    "Increase contrast for accessibility", "Better visibility");
  highContrast->setToggleState(settings.getHighContrastMode());
  highContrast->onToggle = [](bool state) { Settings::getInstance().setHighContrastMode(state); };
  themeGroup->addSetting(highContrast);
  
  addAndMakeVisible(themeGroup.get());
  groups_[Tab::Display].push_back(std::move(themeGroup));
  
  auto perfGroup = std::make_unique<SettingGroup>("PERFORMANCE");
  
  auto* fps = createComboRow(Tab::Display, "fps", "Target Frame Rate",
    "UI refresh rate", "Higher = smoother, more CPU",
    {"30 FPS", "60 FPS", "120 FPS"});
  int currentFps = settings.getTargetFPS();
  fps->setSelectedId(currentFps <= 30 ? 1 : currentFps <= 60 ? 2 : 3);
  fps->onComboChange = [fps] {
    int vals[] = {30, 60, 120};
    Settings::getInstance().setTargetFPS(vals[fps->getSelectedId() - 1]);
  };
  perfGroup->addSetting(fps);
  
  addAndMakeVisible(perfGroup.get());
  groups_[Tab::Display].push_back(std::move(perfGroup));
}

//==============================================================================
// General Settings
//==============================================================================

void GlobalSettingsPanel::createGeneralSettings() {
  auto& settings = Settings::getInstance();
  
  auto projectGroup = std::make_unique<SettingGroup>("PROJECT");
  
  auto* autoSave = createToggleRow(Tab::General, "autoSave", "Auto-Save",
    "Automatically save project periodically", "Prevents data loss");
  autoSave->setToggleState(settings.getAutoSaveEnabled());
  autoSave->onToggle = [](bool state) { Settings::getInstance().setAutoSaveEnabled(state); };
  projectGroup->addSetting(autoSave);
  
  auto* autoSaveInterval = createComboRow(Tab::General, "autoSaveInterval", "Auto-Save Interval",
    "How often to auto-save", "",
    {"1 min", "2 min", "5 min", "10 min", "15 min"});
  int interval = settings.getAutoSaveIntervalMinutes();
  int intIdx = (interval <= 1) ? 1 : (interval <= 2) ? 2 : (interval <= 5) ? 3 : (interval <= 10) ? 4 : 5;
  autoSaveInterval->setSelectedId(intIdx);
  autoSaveInterval->onComboChange = [autoSaveInterval] {
    int vals[] = {1, 2, 5, 10, 15};
    Settings::getInstance().setAutoSaveIntervalMinutes(vals[autoSaveInterval->getSelectedId() - 1]);
  };
  projectGroup->addSetting(autoSaveInterval);
  
  auto* projectFolder = createButtonRow(Tab::General, "projectFolder", "Default Project Folder",
    "Where new projects are saved", "", "Browse...");
  juce::String folder = settings.getDefaultProjectFolder();
  projectFolder->setInfoText(folder.isEmpty() ? "Not set" : folder);
  projectFolder->onButtonClick = [projectFolder] {
    auto chooser = std::make_shared<juce::FileChooser>("Select Project Folder",
      juce::File(Settings::getInstance().getDefaultProjectFolder()));
    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
      [chooser, projectFolder](const juce::FileChooser& fc) {
        if (fc.getResults().size() > 0) {
          auto path = fc.getResult().getFullPathName();
          Settings::getInstance().setDefaultProjectFolder(path);
          projectFolder->setInfoText(path);
        }
      });
  };
  projectGroup->addSetting(projectFolder);
  
  addAndMakeVisible(projectGroup.get());
  groups_[Tab::General].push_back(std::move(projectGroup));
  
  auto powerGroup = std::make_unique<SettingGroup>("POWER MANAGEMENT");
  
  auto* stayAwake = createToggleRow(Tab::General, "stayAwake", "Prevent Sleep",
    "Keep system awake while project is open", "Prevents interruptions during sessions");
  stayAwake->setToggleState(settings.getStayAwakeDuringProject());
  stayAwake->onToggle = [](bool state) { Settings::getInstance().setStayAwakeDuringProject(state); };
  powerGroup->addSetting(stayAwake);
  
  addAndMakeVisible(powerGroup.get());
  groups_[Tab::General].push_back(std::move(powerGroup));
}

//==============================================================================
// Tab Management
//==============================================================================

void GlobalSettingsPanel::setCurrentTab(Tab tab) {
  currentTab_ = tab;
  scrollY_ = 0.0f;
  
  // Update tab button states
  for (size_t i = 0; i < tabButtons_.size(); ++i) {
    tabButtons_[i]->setToggleState(static_cast<Tab>(i) == tab);
  }
  
  // Show/hide groups for current tab
  for (auto& [t, groupList] : groups_) {
    bool visible = (t == tab);
    for (auto& group : groupList) {
      group->setVisible(visible);
    }
  }
  
  // Apply current search filter
  filterSettings(searchQuery_);
  resized();
  markDirty();
}

void GlobalSettingsPanel::filterSettings(const juce::String& query) {
  searchQuery_ = query;
  
  for (auto& group : groups_[currentTab_]) {
    group->filterSettings(query);
  }
  
  resized();
  markDirty();
}

void GlobalSettingsPanel::resetCurrentTabToDefaults() {
  auto& settings = Settings::getInstance();
  
  switch (currentTab_) {
    case Tab::Audio:
      settings.setPluginDelayCompensation(true);
      settings.setSoftwareMonitoring(true);
      settings.setBufferSize(512);
      break;
    case Tab::MIDI:
      settings.setMIDIThrough(true);
      settings.setSendMIDIClockOut(false);
      settings.setReceiveMTCIn(false);
      settings.setMIDILatencyCompensation(0);
      break;
    case Tab::Recording:
      settings.setRecordingBitDepth(Settings::RecordingBitDepth::Bit24);
      settings.setRecordingFileType(Settings::RecordingFileType::WAV);
      settings.setMetronomeCountIn(true);
      settings.setCountInBars(1);
      break;
    case Tab::Editing:
      settings.setSnapToGrid(true);
      settings.setLinkTrackAndEditSelection(true);
      settings.setDefaultCrossfadeMs(10);
      settings.setMaxUndoHistory(100);
      break;
    case Tab::Display:
      settings.setTheme(Settings::UITheme::Neon);
      settings.setAnimationsEnabled(true);
      settings.setHighContrastMode(false);
      settings.setTargetFPS(60);
      break;
    case Tab::General:
      settings.setAutoSaveEnabled(true);
      settings.setAutoSaveIntervalMinutes(5);
      settings.setStayAwakeDuringProject(true);
      break;
  }
  
  syncWithSettings();
}

//==============================================================================
// Settings Sync
//==============================================================================

void GlobalSettingsPanel::syncWithSettings() {
  auto& settings = Settings::getInstance();
  
  // Sync all toggle and combo states from Settings
  for (auto& [tab, rowList] : rows_) {
    for (auto& row : rowList) {
      juce::String id = row->getId();
      row->setChanged(false);
      
      // Audio
      if (id == "pdc") row->setToggleState(settings.getPluginDelayCompensation());
      else if (id == "monitoring") row->setToggleState(settings.getSoftwareMonitoring());
      // MIDI
      else if (id == "midiThrough") row->setToggleState(settings.getMIDIThrough());
      else if (id == "clockOut") row->setToggleState(settings.getSendMIDIClockOut());
      else if (id == "mtcIn") row->setToggleState(settings.getReceiveMTCIn());
      else if (id == "midiLatency") row->setSelectedId((settings.getMIDILatencyCompensation() / 5) + 1);
      // Recording
      else if (id == "bitDepth") row->setSelectedId((int)settings.getRecordingBitDepth() + 1);
      else if (id == "fileType") row->setSelectedId((int)settings.getRecordingFileType() + 1);
      else if (id == "metronomeCountIn") row->setToggleState(settings.getMetronomeCountIn());
      else if (id == "countInBars") row->setSelectedId(settings.getCountInBars());
      // Editing
      else if (id == "snapToGrid") row->setToggleState(settings.getSnapToGrid());
      else if (id == "linkSelection") row->setToggleState(settings.getLinkTrackAndEditSelection());
      // Display
      else if (id == "theme") row->setSelectedId((int)settings.getTheme() + 1);
      else if (id == "animations") row->setToggleState(settings.getAnimationsEnabled());
      else if (id == "highContrast") row->setToggleState(settings.getHighContrastMode());
      // General
      else if (id == "autoSave") row->setToggleState(settings.getAutoSaveEnabled());
      else if (id == "stayAwake") row->setToggleState(settings.getStayAwakeDuringProject());
    }
  }
  
  markDirty();
}

void GlobalSettingsPanel::refreshAudioDevices() {
  auto* device = deviceManager_.getCurrentAudioDevice();
  if (!device) return;
  
  // Find sample rate and buffer size rows
  for (auto& row : rows_[Tab::Audio]) {
    if (row->getId() == "sampleRate") {
      double sr = device->getCurrentSampleRate();
      int idx = (sr <= 44100) ? 1 : (sr <= 48000) ? 2 : (sr <= 88200) ? 3 : 
                (sr <= 96000) ? 4 : (sr <= 176400) ? 5 : 6;
      row->setSelectedId(idx);
    }
    else if (row->getId() == "bufferSize") {
      int bs = device->getCurrentBufferSizeSamples();
      int idx = (bs <= 64) ? 1 : (bs <= 128) ? 2 : (bs <= 256) ? 3 :
                (bs <= 512) ? 4 : (bs <= 1024) ? 5 : 6;
      row->setSelectedId(idx);
    }
  }
}

void GlobalSettingsPanel::changeListenerCallback(juce::ChangeBroadcaster* source) {
  if (source == &deviceManager_) {
    refreshAudioDevices();
  }
}

bool GlobalSettingsPanel::keyPressed(const juce::KeyPress& key) {
  if (key == juce::KeyPress::escapeKey) {
    if (onClose) onClose();
    setVisible(false);
    return true;
  }
  return false;
}

//==============================================================================
// Layout & Rendering
//==============================================================================

void GlobalSettingsPanel::resized() {
  if (getWidth() == 0 || getHeight() == 0) return;
  
  // CRITICAL: Guard against being called during constructor before members are initialized.
  // In JUCE/SkiaComponent, setSize() triggers resized() immediately during call.
  // If setSize() is called at the start of the constructor (before make_unique calls),
  // all unique_ptr members are still nullptr, causing a segfault.
  // DO NOT REMOVE THIS CHECK - it prevents a startup crash.
  // Future agents: if you refactor this class, move setSize() to the END of the
  // constructor (after all member initialization), or keep this guard.
  if (!searchField_ || !closeBtn_ || !resetBtn_ || tabButtons_.empty()) return;
  
  auto bounds = getLocalBounds();
  int padding = 24;
  int headerH = 70;
  int tabH = 44;
  int footerH = 60;
  
  // Search field (top right)
  searchField_->setBounds(bounds.getWidth() - 250 - padding, padding, 250, 32);
  
  // Tab buttons (below header)
  int tabX = padding;
  int tabY = headerH;
  int tabW = 90;
  for (auto& btn : tabButtons_) {
    btn->setBounds(tabX, tabY, tabW, tabH);
    tabX += tabW + 4;
  }
  
  // Content area
  int contentY = headerH + tabH + 16;
  int contentH = bounds.getHeight() - contentY - footerH;
  int contentW = bounds.getWidth() - padding * 2;
  
  // Layout groups vertically with scroll offset
  int y = contentY - (int)scrollY_;
  maxScroll_ = 0;
  
  for (auto& group : groups_[currentTab_]) {
    if (!group->isVisible() || !group->hasVisibleSettings()) continue;
    int h = group->getContentHeight();
    group->setBounds(padding, y, contentW, h);
    y += h + 16;
    maxScroll_ += h + 16;
  }
  
  maxScroll_ = std::max(0.0f, maxScroll_ - contentH);
  
  // Footer buttons
  int btnW = 100;
  int btnH = 36;
  int btnY = bounds.getHeight() - footerH + 12;
  resetBtn_->setBounds(padding, btnY, btnW, btnH);
  closeBtn_->setBounds(bounds.getWidth() - btnW - padding, btnY, btnW, btnH);
}

void GlobalSettingsPanel::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& d) {
  float delta = d.deltaY * 80.0f;
  scrollY_ = juce::jlimit(0.0f, maxScroll_, scrollY_ - delta);
  resized();
  markDirty();
}

void GlobalSettingsPanel::drawSkia(SkCanvas* canvas) {
  auto bounds = getLocalBounds().toFloat();
  float opacity = getAnimatedValue("opacity");
  
  // Modal backdrop
  SkPaint backdropPaint;
  backdropPaint.setColor(SkColorSetARGB((int)(180 * opacity), 0, 0, 0));
  canvas->drawRect(SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()), backdropPaint);
  
  // Panel dimensions
  float panelW = 850.0f;
  float panelH = 650.0f;
  float panelX = (bounds.getWidth() - panelW) / 2;
  float panelY = (bounds.getHeight() - panelH) / 2;
  SkRect panelRect = SkRect::MakeXYWH(panelX, panelY, panelW, panelH);
  
  // Panel shadow
  SkPaint shadowPaint;
  shadowPaint.setColor(SkColorSetARGB(100, 0, 0, 0));
  shadowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 30.0f));
  canvas->drawRoundRect(panelRect.makeOffset(0, 8), 16, 16, shadowPaint);
  
  // Panel background with glassmorphism
  SkPaint bgPaint;
  bgPaint.setColor(design::colors::BG_02);
  bgPaint.setAntiAlias(true);
  canvas->drawRoundRect(panelRect, 16, 16, bgPaint);
  
  // Subtle gradient overlay
  SkPaint gradPaint;
  SkPoint pts[2] = {{panelX, panelY}, {panelX, panelY + panelH}};
  SkColor gradColors[2] = {SkColorSetARGB(15, 255, 255, 255), SkColorSetARGB(0, 255, 255, 255)};
  gradPaint.setShader(SkGradientShader::MakeLinear(pts, gradColors, nullptr, 2, SkTileMode::kClamp));
  canvas->drawRoundRect(panelRect, 16, 16, gradPaint);
  
  // Border
  SkPaint borderPaint;
  borderPaint.setColor(design::colors::BORDER_DEFAULT);
  borderPaint.setStyle(SkPaint::kStroke_Style);
  borderPaint.setStrokeWidth(1.0f);
  borderPaint.setAntiAlias(true);
  canvas->drawRoundRect(panelRect, 16, 16, borderPaint);
  
  // Header
  SkFont titleFont = design::getSkFont(20.0f, design::FontWeight::Bold);
  SkPaint titlePaint;
  titlePaint.setColor(design::colors::TEXT_PRIMARY);
  titlePaint.setAntiAlias(true);
  canvas->drawString("Preferences", panelX + 24, panelY + 44, titleFont, titlePaint);
  
  // Cyan accent line under header
  SkPaint accentPaint;
  accentPaint.setColor(design::colors::CYAN);
  canvas->drawRect(SkRect::MakeXYWH(panelX + 24, panelY + 58, 80, 2), accentPaint);
  
  // Tab underline indicator
  int tabIdx = static_cast<int>(currentTab_);
  float tabX = panelX + 24 + tabIdx * 94;
  float tabY = panelY + 70 + 44 - 2;
  SkPaint tabIndicator;
  tabIndicator.setColor(design::colors::CYAN);
  canvas->drawRoundRect(SkRect::MakeXYWH(tabX, tabY, 86, 3), 1.5f, 1.5f, tabIndicator);
  
  // Clip content area for scrolling
  int contentY = (int)(panelY + 70 + 44 + 16);
  int contentH = (int)(panelH - 70 - 44 - 16 - 60);
  canvas->save();
  canvas->clipRect(SkRect::MakeXYWH(panelX, contentY, panelW, contentH));
  
  // Draw children (groups, controls)
  drawChildren(canvas);
  
  canvas->restore();
  
  // Footer separator
  SkPaint sepPaint;
  sepPaint.setColor(design::colors::BORDER_SUBTLE);
  float footerY = panelY + panelH - 60;
  canvas->drawRect(SkRect::MakeXYWH(panelX + 24, footerY, panelW - 48, 1), sepPaint);
}

} // namespace zenith
