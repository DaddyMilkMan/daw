/*
  ==============================================================================

    GlobalSettingsPanel.cpp
    Created: 2025-12-30
    Author:  Zenith DAW

  ==============================================================================
*/

#include "GlobalSettingsPanel.h"
#include "../design-system/ZenithTheme.h"
#include "../design-system/ZenithDesignSystem.h"
#include "../framework/GlassmorphicPanel.h"

namespace zenith {

GlobalSettingsPanel::GlobalSettingsPanel(juce::AudioDeviceManager& deviceManager)
    : deviceManager_(deviceManager) {
  
  createControls();
  
  // Listen for device changes (e.g. unplugging USB interface)
  deviceManager_.addChangeListener(this);
  
  // Initial population
  refreshAudioDeviceList();

  setSize(600, 500);
}

GlobalSettingsPanel::~GlobalSettingsPanel() {
  deviceManager_.removeChangeListener(this);
}

void GlobalSettingsPanel::createControls() {
  // Output Device
  outputDeviceCombo_ = std::make_unique<SkiaComboBox>("Output Device");
  outputDeviceCombo_->onChange = [this] { applyAudioSettings(); };
  addAndMakeVisible(outputDeviceCombo_.get());

  // Input Device
  inputDeviceCombo_ = std::make_unique<SkiaComboBox>("Input Device");
  inputDeviceCombo_->onChange = [this] { applyAudioSettings(); };
  addAndMakeVisible(inputDeviceCombo_.get());

  // Sample Rate
  sampleRateCombo_ = std::make_unique<SkiaComboBox>("Sample Rate");
  sampleRateCombo_->onChange = [this] { applyAudioSettings(); };
  addAndMakeVisible(sampleRateCombo_.get());

  // Buffer Size
  bufferSizeCombo_ = std::make_unique<SkiaComboBox>("Buffer Size");
  bufferSizeCombo_->onChange = [this] { applyAudioSettings(); };
  addAndMakeVisible(bufferSizeCombo_.get());

  // Test Tone
  testToneBtn_ = std::make_unique<SkiaButton>("Test Tone");
  testToneBtn_->setToggleable(true);
  testToneBtn_->onClick = [this] {
     // TODO: Connect to Engine test tone trigger if available
     // For now just toggle state visual
  };
  addAndMakeVisible(testToneBtn_.get());

  // Close Button
  closeBtn_ = std::make_unique<SkiaButton>("Close");
  closeBtn_->setStyle(SkiaButton::Style::Ghost);
  closeBtn_->onClick = [this] { setVisible(false); };
  addAndMakeVisible(closeBtn_.get());
}

void GlobalSettingsPanel::refreshAudioDeviceList() {
  // Prevent loops during update
  outputDeviceCombo_->onChange = nullptr;
  inputDeviceCombo_->onChange = nullptr;
  sampleRateCombo_->onChange = nullptr;
  bufferSizeCombo_->onChange = nullptr;

  outputDeviceCombo_->clear();
  inputDeviceCombo_->clear();
  sampleRateCombo_->clear();
  bufferSizeCombo_->clear();

  // Populate Devices
  // For simplicity nicely getting current device type's name and re-scanning
  if (auto* currentDevice = deviceManager_.getCurrentAudioDevice()) {
      juce::String typeName = currentDevice->getTypeName();
      
      const auto& types = deviceManager_.getAvailableDeviceTypes();
      for (auto* type : types) {
          if (type->getTypeName() == typeName) {
              type->scanForDevices();
              auto deviceNames = type->getDeviceNames();
              int id = 1;
              for (const auto& name : deviceNames) {
                  outputDeviceCombo_->addItem(name, id);
                  inputDeviceCombo_->addItem(name, id); 
                  id++;
              }
              break; 
          }
      }
  }

  // Populate Sample Rates (based on current device)
  auto* currentDevice = deviceManager_.getCurrentAudioDevice();
  if (currentDevice) {
      auto rates = currentDevice->getAvailableSampleRates();
      int id = 1;
      for (auto rate : rates) {
          sampleRateCombo_->addItem(juce::String(rate), id);
          id++;
      }

      auto buffers = currentDevice->getAvailableBufferSizes();
      id = 1;
      for (auto size : buffers) {
          bufferSizeCombo_->addItem(juce::String(size) + " samples", id);
          id++;
      }
  }

  updateComboBoxes();

  // Restore callbacks
  outputDeviceCombo_->onChange = [this] { applyAudioSettings(); };
  inputDeviceCombo_->onChange = [this] { applyAudioSettings(); };
  sampleRateCombo_->onChange = [this] { applyAudioSettings(); };
  bufferSizeCombo_->onChange = [this] { applyAudioSettings(); };
}

void GlobalSettingsPanel::updateComboBoxes() {
    auto* currentDevice = deviceManager_.getCurrentAudioDevice();
    if (!currentDevice) return;

    auto setup = deviceManager_.getAudioDeviceSetup();
    
    // Attempt to select current settings
    // Since SkiaComboBox uses IDs, and we used 1-based index, we might need a better map.
    // For now, prototype logic: rely on user interaction mostly.
}

void GlobalSettingsPanel::changeListenerCallback(juce::ChangeBroadcaster*) {
    refreshAudioDeviceList();
}

void GlobalSettingsPanel::applyAudioSettings() {
    juce::AudioDeviceManager::AudioDeviceSetup setup = deviceManager_.getAudioDeviceSetup();
    
    // Read from UI (if valid selection)
    if (outputDeviceCombo_->getSelectedId() > 0) {
        setup.outputDeviceName = outputDeviceCombo_->getText();
    }
    if (inputDeviceCombo_->getSelectedId() > 0) {
        setup.inputDeviceName = inputDeviceCombo_->getText();
    }
    
    // Sample Rate
    double newRate = sampleRateCombo_->getText().getDoubleValue();
    if (newRate > 0) setup.sampleRate = newRate;

    // Buffer Size
    int newBufferSize = bufferSizeCombo_->getText().getTrailingIntValue(); // "256 samples" -> 256
    if (newBufferSize > 0) setup.bufferSize = newBufferSize;

    deviceManager_.setAudioDeviceSetup(setup, true);
}

void GlobalSettingsPanel::resized() {
    auto area = getLocalBounds().reduced(40);
    
    // Title space
    area.removeFromTop(60);

    int h = 40;
    int gap = 20;

    outputDeviceCombo_->setBounds(area.removeFromTop(h));
    area.removeFromTop(gap);
    
    inputDeviceCombo_->setBounds(area.removeFromTop(h));
    area.removeFromTop(gap);
    
    auto row = area.removeFromTop(h);
    sampleRateCombo_->setBounds(row.removeFromLeft(row.getWidth() / 2 - 10));
    bufferSizeCombo_->setBounds(row.removeFromRight(row.getWidth() / 2 - 10));
    
    area.removeFromTop(gap);
    testToneBtn_->setBounds(area.removeFromTop(h));

    // Bottom
    auto bottom = getLocalBounds().reduced(40).removeFromBottom(40);
    closeBtn_->setBounds(bottom.removeFromRight(100));
}

void GlobalSettingsPanel::drawSkia(SkCanvas* canvas) {
    // Glass Background
    SkRect bounds = SkRect::MakeWH((float)getWidth(), (float)getHeight());
    GlassmorphicPanel::draw(canvas, bounds, GlassmorphicPanel::Style::Elevated); // Solid glass look
    
    // Title
    SkFont font = zenith::design::getSkFont(28.0f, zenith::design::FontWeight::Bold);
    SkPaint paint;
    paint.setColor(SK_ColorWHITE);
    paint.setAntiAlias(true);
    
    canvas->drawString("Audio Settings", 40, 50, font, paint);
    
    // Labels for combos (optional, or drawn here)
    font = zenith::design::getSkFont(14.0f, zenith::design::FontWeight::Medium);
    paint.setColor(SkColorSetARGB(180, 255, 255, 255));
    
    auto area = getLocalBounds().reduced(40);
    area.removeFromTop(60); // Skip title
    
    // Draw labels above combos based on known layout
    canvas->drawString("Output Device", 40, area.getY() - 5, font, paint);
    // ... etc (Visual Refinement Step)
}

} // namespace zenith
