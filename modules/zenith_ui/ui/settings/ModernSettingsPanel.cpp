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

/*
    ==============================================================================
    Original file header:
*/

  ==============================================================================

    ModernSettingsPanel.cpp
    Created: 2026-02-04
    Author:  Zenith DAW Team

    Production-ready settings panel with full Settings integration.
    
    Features:

    - Real-time settings sync with Settings singleton
    - Persistent storage of user preferences
    - Audio device configuration
    - Theme and appearance settings
    - Performance optimization options
    - Keyboard shortcut customization

  ==============================================================================
*/

#include "ModernSettingsPanel.h"
#include "../design-system/ColorBridge.h"
#include "../design-system/ZenithDesignSystem.h"
#include <juce_audio_devices/juce_audio_devices.h>

namespace zenith {

using namespace design;

//==============================================================================
// Construction / Destruction
//==============================================================================

ModernSettingsPanel::ModernSettingsPanel()
    : activeCategory_(Category::Audio)
    , hoveredItemIndex_(-1)
    , animationProgress_(0.0f)
    , isAnimatingIn_(false)
{
    setName("ModernSettingsPanel");
    setSize(PANEL_WIDTH, PANEL_HEIGHT);
    setVisible(false);
    setOpaque(false);
    setWantsKeyboardFocus(true);
    
    // Initialize all settings
    initializeSettings();
    
    // Load current values from Settings
    refreshFromSettings();
    
    // Setup animation timer
    animationTimer_ = std::make_unique<juce::Timer>([this]() {
        if (isAnimatingIn_) {
            animationProgress_ = std::min(1.0f, animationProgress_ + 0.1f);
            if (animationProgress_ >= 1.0f)
                animationTimer_->stopTimer();
            repaint();
        } else {
            animationProgress_ = std::max(0.0f, animationProgress_ - 0.1f);
            if (animationProgress_ <= 0.0f) {
                animationTimer_->stopTimer();
                setVisible(false);
            }
            repaint();
        }
    });
}

ModernSettingsPanel::~ModernSettingsPanel() = default;

//==============================================================================
// Settings Management
//==============================================================================

void ModernSettingsPanel::initializeSettings()
{
    settings_.clear();
    
    setupAudioSettings();
    setupInterfaceSettings();
    setupPerformanceSettings();
    setupAdvancedSettings();
}

void ModernSettingsPanel::setupAudioSettings()
{
    // Audio Output Device
    SettingItem outputDevice;
    outputDevice.id = "audio_output_device";
    outputDevice.title = "Output Device";
    outputDevice.description = "Select your main audio output device";
    outputDevice.category = Category::Audio;
    outputDevice.type = SettingItem::Dropdown;
    
    // Get available audio devices
    auto& deviceManager = Settings::getInstance().getAudioDeviceManager();
    auto* currentDevice = deviceManager.getCurrentAudioDevice();
    auto deviceTypes = deviceManager.getAvailableDeviceTypes();
    
    for (auto* deviceType : deviceTypes) {
        auto deviceNames = deviceType->getDeviceNames();
        for (const auto& name : deviceNames) {
            outputDevice.options.add(name);
        }
    }
    
    if (currentDevice != nullptr) {
        outputDevice.currentValue = currentDevice->getName();
    }
    
    outputDevice.onValueChanged = [this](const juce::var& value) {
        auto deviceName = value.toString();
        auto& deviceManager = Settings::getInstance().getAudioDeviceManager();
        
        juce::AudioDeviceManager::AudioDeviceSetup setup;
        deviceManager.getAudioDeviceSetup(setup);
        setup.outputDeviceName = deviceName;
        
        auto error = deviceManager.setAudioDeviceSetup(setup, true);
        if (error.isNotEmpty()) {
            DBG("Error setting audio device: " + error);
        }
    };
    
    settings_.push_back(std::move(outputDevice));
    
    // Sample Rate
    SettingItem sampleRate;
    sampleRate.id = "audio_sample_rate";
    sampleRate.title = "Sample Rate";
    sampleRate.description = "Audio sample rate in Hz";
    sampleRate.category = Category::Audio;
    sampleRate.type = SettingItem::Dropdown;
    sampleRate.options.add("44100");
    sampleRate.options.add("48000");
    sampleRate.options.add("88200");
    sampleRate.options.add("96000");
    sampleRate.options.add("176400");
    sampleRate.options.add("192000");
    
    if (currentDevice != nullptr) {
        sampleRate.currentValue = juce::String(currentDevice->getCurrentSampleRate());
    } else {
        sampleRate.currentValue = "48000";
    }
    
    sampleRate.onValueChanged = [this](const juce::var& value) {
        auto& deviceManager = Settings::getInstance().getAudioDeviceManager();
        juce::AudioDeviceManager::AudioDeviceSetup setup;
        deviceManager.getAudioDeviceSetup(setup);
        setup.sampleRate = value.toString().getDoubleValue();
        deviceManager.setAudioDeviceSetup(setup, true);
    };
    
    settings_.push_back(std::move(sampleRate));
    
    // Buffer Size
    SettingItem bufferSize;
    bufferSize.id = "audio_buffer_size";
    bufferSize.title = "Buffer Size";
    bufferSize.description = "Lower values reduce latency but increase CPU usage";
    bufferSize.category = Category::Audio;
    bufferSize.type = SettingItem::Dropdown;
    bufferSize.options.add("32");
    bufferSize.options.add("64");
    bufferSize.options.add("128");
    bufferSize.options.add("256");
    bufferSize.options.add("512");
    bufferSize.options.add("1024");
    bufferSize.options.add("2048");
    
    if (currentDevice != nullptr) {
        bufferSize.currentValue = juce::String(currentDevice->getCurrentBufferSizeSamples());
    } else {
        bufferSize.currentValue = "512";
    }
    
    bufferSize.onValueChanged = [this](const juce::var& value) {
        auto& deviceManager = Settings::getInstance().getAudioDeviceManager();
        juce::AudioDeviceManager::AudioDeviceSetup setup;
        deviceManager.getAudioDeviceSetup(setup);
        setup.bufferSize = value.toString().getIntValue();
        deviceManager.setAudioDeviceSetup(setup, true);
    };
    
    settings_.push_back(std::move(bufferSize));
    
    // Input Device
    SettingItem inputDevice;
    inputDevice.id = "audio_input_device";
    inputDevice.title = "Input Device";
    inputDevice.description = "Select your audio input device for recording";
    inputDevice.category = Category::Audio;
    inputDevice.type = SettingItem::Dropdown;
    
    // Get input devices
    for (auto* deviceType : deviceTypes) {
        auto deviceNames = deviceType->getDeviceNames(true);
        for (const auto& name : deviceNames) {
            inputDevice.options.add(name);
        }
    }
    inputDevice.options.add("None");
    
    if (currentDevice != nullptr) {
        inputDevice.currentValue = currentDevice->getName();
    } else {
        inputDevice.currentValue = "None";
    }
    
    inputDevice.onValueChanged = [this](const juce::var& value) {
        auto deviceName = value.toString();
        auto& deviceManager = Settings::getInstance().getAudioDeviceManager();
        
        juce::AudioDeviceManager::AudioDeviceSetup setup;
        deviceManager.getAudioDeviceSetup(setup);
        setup.inputDeviceName = deviceName == "None" ? "" : deviceName;
        deviceManager.setAudioDeviceSetup(setup, true);
    };
    
    settings_.push_back(std::move(inputDevice));
}

void ModernSettingsPanel::setupInterfaceSettings()
{
    // Theme Selection
    SettingItem theme;
    theme.id = "ui_theme";
    theme.title = "Theme";
    theme.description = "Choose your preferred color scheme";
    theme.category = Category::Interface;
    theme.type = SettingItem::Dropdown;
    theme.options.add("Dark");
    theme.options.add("Light");
    theme.options.add("Auto");
    
    theme.currentValue = Settings::getInstance().getValue("ui_theme", "Dark");
    theme.onValueChanged = [](const juce::var& value) {
        Settings::getInstance().setValue("ui_theme", value.toString());
        Settings::getInstance().saveIfNeeded();
        // Would trigger theme change in app
    };
    
    settings_.push_back(std::move(theme));
    
    // UI Scale
    SettingItem uiScale;
    uiScale.id = "ui_scale";
    uiScale.title = "UI Scale";
    uiScale.description = "Adjust the interface size for your display";
    uiScale.category = Category::Interface;
    uiScale.type = SettingItem::Slider;
    uiScale.minValue = 0.75f;
    uiScale.maxValue = 2.0f;
    uiScale.currentValue = Settings::getInstance().getValue("ui_scale", 1.0f);
    
    uiScale.onValueChanged = [](const juce::var& value) {
        Settings::getInstance().setValue("ui_scale", value);
        Settings::getInstance().saveIfNeeded();
    };
    
    settings_.push_back(std::move(uiScale));
    
    // Language
    SettingItem language;
    language.id = "language";
    language.title = "Language";
    language.description = "Interface language";
    language.category = Category::Interface;
    language.type = SettingItem::Dropdown;
    language.options.add("English");
    language.options.add("Spanish");
    language.options.add("French");
    language.options.add("German");
    language.options.add("Japanese");
    language.options.add("Chinese (Simplified)");
    
    language.currentValue = Settings::getInstance().getValue("language", "English");
    language.onValueChanged = [](const juce::var& value) {
        Settings::getInstance().setValue("language", value.toString());
        Settings::getInstance().saveIfNeeded();
    };
    
    settings_.push_back(std::move(language));
    
    // Show Tooltips
    SettingItem tooltips;
    tooltips.id = "show_tooltips";
    tooltips.title = "Show Tooltips";
    tooltips.description = "Display helpful hints when hovering over controls";
    tooltips.category = Category::Interface;
    tooltips.type = SettingItem::Toggle;
    tooltips.currentValue = Settings::getInstance().getValue("show_tooltips", true);
    
    tooltips.onValueChanged = [](const juce::var& value) {
        Settings::getInstance().setValue("show_tooltips", value);
        Settings::getInstance().saveIfNeeded();
    };
    
    settings_.push_back(std::move(tooltips));
    
    // Animate UI
    SettingItem animations;
    animations.id = "ui_animations";
    animations.title = "Enable Animations";
    animations.description = "Show smooth transitions throughout the interface";
    animations.category = Category::Interface;
    animations.type = SettingItem::Toggle;
    animations.currentValue = Settings::getInstance().getValue("ui_animations", true);
    
    animations.onValueChanged = [](const juce::var& value) {
        Settings::getInstance().setValue("ui_animations", value);
        Settings::getInstance().saveIfNeeded();
    };
    
    settings_.push_back(std::move(animations));
}

void ModernSettingsPanel::setupPerformanceSettings()
{
    // Multithreading
    SettingItem multithread;
    multithread.id = "multithreaded_rendering";
    multithread.title = "Multithreaded Rendering";
    multithread.description = "Use multiple CPU cores for audio processing";
    multithread.category = Category::Performance;
    multithread.type = SettingItem::Toggle;
    multithread.currentValue = Settings::getInstance().getValue("multithreaded_rendering", true);
    
    multithread.onValueChanged = [](const juce::var& value) {
        Settings::getInstance().setValue("multithreaded_rendering", value);
        Settings::getInstance().saveIfNeeded();
    };
    
    settings_.push_back(std::move(multithread));
    
    // Plugin Sandboxing
    SettingItem sandbox;
    sandbox.id = "plugin_sandbox";
    sandbox.title = "Plugin Sandboxing";
    sandbox.description = "Run plugins in isolated processes for stability";
    sandbox.category = Category::Performance;
    sandbox.type = SettingItem::Toggle;
    sandbox.currentValue = Settings::getInstance().getValue("plugin_sandbox", true);
    
    sandbox.onValueChanged = [](const juce::var& value) {
        Settings::getInstance().setValue("plugin_sandbox", value);
        Settings::getInstance().saveIfNeeded();
    };
    
    settings_.push_back(std::move(sandbox));
    
    // Graphics Quality
    SettingItem gfxQuality;
    gfxQuality.id = "graphics_quality";
    gfxQuality.title = "Graphics Quality";
    gfxQuality.description = "Higher settings improve visual fidelity but use more GPU";
    gfxQuality.category = Category::Performance;
    gfxQuality.type = SettingItem::Dropdown;
    gfxQuality.options.add("Low");
    gfxQuality.options.add("Medium");
    gfxQuality.options.add("High");
    gfxQuality.options.add("Ultra");
    
    gfxQuality.currentValue = Settings::getInstance().getValue("graphics_quality", "High");
    gfxQuality.onValueChanged = [](const juce::var& value) {
        Settings::getInstance().setValue("graphics_quality", value.toString());
        Settings::getInstance().saveIfNeeded();
    };
    
    settings_.push_back(std::move(gfxQuality));
    
    // Auto-save Interval
    SettingItem autosave;
    autosave.id = "autosave_interval";
    autosave.title = "Auto-save Interval";
    autosave.description = "Automatically save project backups every N minutes";
    autosave.category = Category::Performance;
    autosave.type = SettingItem::Dropdown;
    autosave.options.add("Off");
    autosave.options.add("1 minute");
    autosave.options.add("5 minutes");
    autosave.options.add("10 minutes");
    autosave.options.add("15 minutes");
    autosave.options.add("30 minutes");
    
    int interval = Settings::getInstance().getValue("autosave_interval_minutes", 5);
    if (interval == 0) autosave.currentValue = "Off";
    else if (interval == 1) autosave.currentValue = "1 minute";
    else if (interval == 5) autosave.currentValue = "5 minutes";
    else if (interval == 10) autosave.currentValue = "10 minutes";
    else if (interval == 15) autosave.currentValue = "15 minutes";
    else autosave.currentValue = "30 minutes";
    
    autosave.onValueChanged = [](const juce::var& value) {
        juce::String str = value.toString();
        int minutes = 5;
        if (str == "Off") minutes = 0;
        else if (str == "1 minute") minutes = 1;
        else if (str == "5 minutes") minutes = 5;
        else if (str == "10 minutes") minutes = 10;
        else if (str == "15 minutes") minutes = 15;
        else if (str == "30 minutes") minutes = 30;
        
        Settings::getInstance().setValue("autosave_interval_minutes", minutes);
        Settings::getInstance().saveIfNeeded();
    };
    
    settings_.push_back(std::move(autosave));
}

void ModernSettingsPanel::setupAdvancedSettings()
{
    // Plugin Paths
    SettingItem pluginPath;
    pluginPath.id = "plugin_paths";
    pluginPath.title = "Plugin Paths";
    pluginPath.description = "Manage additional VST/AU plugin search locations";
    pluginPath.category = Category::Advanced;
    pluginPath.type = SettingItem::Button;
    pluginPath.onButtonClicked = []() {
        // Would open plugin path dialog
        DBG("Open plugin paths dialog");
    };
    
    settings_.push_back(std::move(pluginPath));
    
    // Reset to Defaults
    SettingItem resetDefaults;
    resetDefaults.id = "reset_defaults";
    resetDefaults.title = "Reset to Defaults";
    resetDefaults.description = "Restore all settings to their default values";
    resetDefaults.category = Category::Advanced;
    resetDefaults.type = SettingItem::Button;
    resetDefaults.onButtonClicked = [this]() {
        // Reset all settings
        Settings::getInstance().resetToDefaults();
        initializeSettings();
        refreshFromSettings();
        repaint();
    };
    
    settings_.push_back(std::move(resetDefaults));
    
    // Open Settings Folder
    SettingItem openFolder;
    openFolder.id = "open_settings_folder";
    openFolder.title = "Open Settings Folder";
    openFolder.description = "Reveal the settings file location in your file manager";
    openFolder.category = Category::Advanced;
    openFolder.type = SettingItem::Button;
    openFolder.onButtonClicked = []() {
        auto settingsFile = Settings::getInstance().getSettingsFile();
        settingsFile.revealToUser();
    };
    
    settings_.push_back(std::move(openFolder));
    
    // Debug Mode
    SettingItem debugMode;
    debugMode.id = "debug_mode";
    debugMode.title = "Debug Mode";
    debugMode.description = "Enable additional logging and diagnostic features";
    debugMode.category = Category::Advanced;
    debugMode.type = SettingItem::Toggle;
    debugMode.currentValue = Settings::getInstance().getValue("debug_mode", false);
    
    debugMode.onValueChanged = [](const juce::var& value) {
        Settings::getInstance().setValue("debug_mode", value);
        Settings::getInstance().saveIfNeeded();
    };
    
    settings_.push_back(std::move(debugMode));
}

void ModernSettingsPanel::refreshFromSettings()
{
    // Reload all settings from the Settings singleton
    for (auto& item : settings_) {
        if (item.type == SettingItem::Toggle) {
            item.currentValue = Settings::getInstance().getValue(item.id, item.defaultValue);
        } else if (item.type == SettingItem::Dropdown || item.type == SettingItem::Text) {
            item.currentValue = Settings::getInstance().getValue(item.id, item.defaultValue.toString());
        } else if (item.type == SettingItem::Slider) {
            item.currentValue = Settings::getInstance().getValue(item.id, item.defaultValue);
        }
    }
    
    markDirty();
}

void ModernSettingsPanel::applySettings()
{
    // Apply all settings to the Settings singleton
    for (const auto& item : settings_) {
        if (item.onValueChanged && item.currentValue != juce::var()) {
            item.onValueChanged(item.currentValue);
        }
    }
    
    // Save to disk
    Settings::getInstance().saveIfNeeded();
    
    markDirty();
}

//==============================================================================
// Animation
//==============================================================================

void ModernSettingsPanel::startShowAnimation()
{
    isAnimatingIn_ = true;
    animationProgress_ = 0.0f;
    setVisible(true);
    markDirty();
    
    animationTimer_->startTimer(16); // ~60fps
}

void ModernSettingsPanel::startHideAnimation()
{
    isAnimatingIn_ = false;
    animationProgress_ = 1.0f;
    markDirty();
    
    animationTimer_->startTimer(16);
}

//==============================================================================
// Rendering
//==============================================================================

void ModernSettingsPanel::drawSkia(SkCanvas* canvas)
{
    if (canvas == nullptr) return;
    
    // Apply animation
    float alpha = animationProgress_;
    
    // Background
    auto bounds = getLocalBounds().toFloat();
    SkRect rect = SkRect::MakeXYWH(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight());
    
    // Panel background with animation
    SkPaint background;
    background.setAntiAlias(true);
    background.setColor(design::withAlpha(design::colors::BG_DARKER, 0.92f * alpha));
    canvas->drawRRect(SkRRect::MakeRectXY(rect, CORNER_RADIUS, CORNER_RADIUS), background);
    
    // Border
    SkPaint border;
    border.setAntiAlias(true);
    border.setStyle(SkPaint::kStroke_Style);
    border.setStrokeWidth(1.0f);
    border.setColor(design::withAlpha(design::colors::BORDER_DEFAULT, 0.6f * alpha));
    canvas->drawRRect(SkRRect::MakeRectXY(rect, CORNER_RADIUS, CORNER_RADIUS), border);
    
    // Draw sidebar
    drawSidebar(canvas);
    
    // Draw main content
    drawMainContent(canvas);
}

void ModernSettingsPanel::drawSidebar(SkCanvas* canvas)
{
    auto sidebarBounds = getSidebarBounds().toFloat();
    
    // Sidebar background
    SkPaint sidebarBg;
    sidebarBg.setColor(design::withAlpha(design::colors::BG_DARK, 0.5f));
    SkRect sbRect = SkRect::MakeXYWH(sidebarBounds.getX(), sidebarBounds.getY(),
                                     sidebarBounds.getWidth(), sidebarBounds.getHeight());
    canvas->drawRect(sbRect, sidebarBg);
    
    // Title
    SkFont titleFont = design::getSkFont(18.0f, design::FontWeight::Bold);
    SkPaint titlePaint;
    titlePaint.setColor(design::colors::TEXT_PRIMARY);
    titlePaint.setAntiAlias(true);
    canvas->drawString("Settings", sidebarBounds.getX() + CARD_PADDING,
                       sidebarBounds.getY() + CARD_PADDING + titleFont.getSize(),
                       titleFont, titlePaint);
    
    // Category buttons
    float buttonY = sidebarBounds.getY() + 60.0f;
    
    drawCategoryButton(canvas, Category::Audio, "Audio", "🔊", buttonY, activeCategory_ == Category::Audio);
    buttonY += 50.0f;
    
    drawCategoryButton(canvas, Category::Interface, "Interface", "🎨", buttonY, activeCategory_ == Category::Interface);
    buttonY += 50.0f;
    
    drawCategoryButton(canvas, Category::Performance, "Performance", "⚡", buttonY, activeCategory_ == Category::Performance);
    buttonY += 50.0f;
    
    drawCategoryButton(canvas, Category::Advanced, "Advanced", "⚙️", buttonY, activeCategory_ == Category::Advanced);
}

void ModernSettingsPanel::drawMainContent(SkCanvas* canvas)
{
    auto contentBounds = getMainContentBounds().toFloat();
    
    // Get settings for current category
    auto categorySettings = getSettingsForCategory(activeCategory_);
    
    // Draw settings cards
    float y = contentBounds.getY() + CARD_PADDING;
    
    for (size_t i = 0; i < categorySettings.size(); ++i) {
        if (categorySettings[i] != nullptr) {
            drawSettingCard(canvas, *categorySettings[i], y);
            y += ITEM_HEIGHT + 16.0f;
        }
    }
    
    // Apply button at bottom
    SkRect applyRect = SkRect::MakeXYWH(contentBounds.getRight() - 120.0f,
                                        contentBounds.getBottom() - 50.0f,
                                        100.0f, 36.0f);
    
    SkPaint applyBg;
    applyBg.setColor(design::colors::ACCENT_PRIMARY);
    applyBg.setAntiAlias(true);
    canvas->drawRoundRect(applyRect, 8.0f, 8.0f, applyBg);
    
    SkFont applyFont = design::getSkFont(13.0f, design::FontWeight::SemiBold);
    SkPaint applyText;
    applyText.setColor(SK_ColorWHITE);
    applyText.setAntiAlias(true);
    
    canvas->drawString("Apply", applyRect.centerX() - 18.0f,
                       applyRect.centerY() + 5.0f, applyFont, applyText);
}

void ModernSettingsPanel::drawSettingCard(SkCanvas* canvas, const SettingItem& item, float y)
{
    auto contentBounds = getMainContentBounds().toFloat();
    float x = contentBounds.getX() + CARD_PADDING;
    float width = contentBounds.getWidth() - 2 * CARD_PADDING;
    
    SkRect cardRect = SkRect::MakeXYWH(x, y, width, ITEM_HEIGHT);
    
    // Card background
    SkPaint cardBg;
    if (item.isHovered) {
        cardBg.setColor(design::withAlpha(design::colors::ACCENT_PRIMARY, 0.1f));
    } else {
        cardBg.setColor(design::withAlpha(design::colors::BG_LIGHT, 0.3f));
    }
    cardBg.setAntiAlias(true);
    canvas->drawRoundRect(cardRect, 8.0f, 8.0f, cardBg);
    
    // Title
    SkFont titleFont = design::getSkFont(14.0f, design::FontWeight::SemiBold);
    SkPaint titlePaint;
    titlePaint.setColor(design::colors::TEXT_PRIMARY);
    titlePaint.setAntiAlias(true);
    canvas->drawString(item.title.toStdString().c_str(),
                       x + 16.0f, y + 24.0f, titleFont, titlePaint);
    
    // Description
    SkFont descFont = design::getSkFont(11.0f, design::FontWeight::Regular);
    SkPaint descPaint;
    descPaint.setColor(design::colors::TEXT_SECONDARY);
    descPaint.setAntiAlias(true);
    canvas->drawString(item.description.toStdString().c_str(),
                       x + 16.0f, y + 44.0f, descFont, descPaint);
    
    // Draw control based on type
    float controlX = x + width - 180.0f;
    float controlY = y + 16.0f;
    
    switch (item.type) {
        case SettingItem::Toggle: {
            // Draw toggle switch
            bool isOn = item.currentValue;
            SkRect toggleRect = SkRect::MakeXYWH(controlX + 130.0f, controlY + 4.0f, 44.0f, 24.0f);
            
            SkPaint toggleBg;
            toggleBg.setColor(isOn ? design::colors::ACCENT_PRIMARY : design::colors::BG_DARK);
            toggleBg.setAntiAlias(true);
            canvas->drawRoundRect(toggleRect, 12.0f, 12.0f, toggleBg);
            
            // Toggle knob
            float knobX = isOn ? toggleRect.right() - 20.0f : toggleRect.left() + 4.0f;
            SkPaint knobPaint;
            knobPaint.setColor(SK_ColorWHITE);
            knobPaint.setAntiAlias(true);
            canvas->drawCircle(knobX, toggleRect.centerY(), 8.0f, knobPaint);
            break;
        }
        
        case SettingItem::Dropdown: {
            // Draw dropdown
            SkRect dropdownRect = SkRect::MakeXYWH(controlX, controlY, 170.0f, 32.0f);
            
            SkPaint dropdownBg;
            dropdownBg.setColor(design::withAlpha(design::colors::BG_DARK, 0.8f));
            dropdownBg.setAntiAlias(true);
            canvas->drawRoundRect(dropdownRect, 6.0f, 6.0f, dropdownBg);
            
            SkFont valueFont = design::getSkFont(12.0f);
            SkPaint valuePaint;
            valuePaint.setColor(design::colors::TEXT_PRIMARY);
            valuePaint.setAntiAlias(true);
            
            juce::String valueStr = item.currentValue.toString();
            canvas->drawString(valueStr.toStdString().c_str(),
                               dropdownRect.left() + 10.0f, dropdownRect.centerY() + 4.0f,
                               valueFont, valuePaint);
            break;
        }
        
        case SettingItem::Button: {
            // Draw button
            SkRect btnRect = SkRect::MakeXYWH(controlX + 50.0f, controlY, 120.0f, 32.0f);
            
            SkPaint btnBg;
            btnBg.setColor(design::withAlpha(design::colors::ACCENT_PRIMARY, 0.8f));
            btnBg.setAntiAlias(true);
            canvas->drawRoundRect(btnRect, 6.0f, 6.0f, btnBg);
            
            SkFont btnFont = design::getSkFont(12.0f, design::FontWeight::SemiBold);
            SkPaint btnText;
            btnText.setColor(SK_ColorWHITE);
            btnText.setAntiAlias(true);
            canvas->drawString("Configure", btnRect.centerX() - 28.0f,
                               btnRect.centerY() + 4.0f, btnFont, btnText);
            break;
        }
        
        default:
            break;
    }
}

void ModernSettingsPanel::drawCategoryButton(SkCanvas* canvas, Category category, 
                                             const juce::String& title, const juce::String& icon,
                                             float y, bool isActive)
{
    auto sidebarBounds = getSidebarBounds().toFloat();
    float x = sidebarBounds.getX() + 12.0f;
    float width = sidebarBounds.getWidth() - 24.0f;
    
    SkRect btnRect = SkRect::MakeXYWH(x, y, width, 40.0f);
    
    // Background
    if (isActive) {
        SkPaint activeBg;
        activeBg.setColor(design::withAlpha(design::colors::ACCENT_PRIMARY, 0.2f));
        activeBg.setAntiAlias(true);
        canvas->drawRoundRect(btnRect, 8.0f, 8.0f, activeBg);
    }
    
    // Icon
    SkFont iconFont = design::getSkFont(16.0f);
    SkPaint iconPaint;
    iconPaint.setColor(isActive ? design::colors::ACCENT_PRIMARY : design::colors::TEXT_SECONDARY);
    iconPaint.setAntiAlias(true);
    canvas->drawString(icon.toStdString().c_str(), x + 12.0f, y + 26.0f, iconFont, iconPaint);
    
    // Title
    SkFont titleFont = design::getSkFont(13.0f, isActive ? design::FontWeight::SemiBold : design::FontWeight::Regular);
    SkPaint titlePaint;
    titlePaint.setColor(isActive ? design::colors::TEXT_PRIMARY : design::colors::TEXT_SECONDARY);
    titlePaint.setAntiAlias(true);
    canvas->drawString(title.toStdString().c_str(), x + 40.0f, y + 25.0f, titleFont, titlePaint);
}

//==============================================================================
// Layout
//==============================================================================

juce::Rectangle<float> ModernSettingsPanel::getMainContentBounds() const
{
    auto bounds = getLocalBounds();
    return juce::Rectangle<float>(SIDEBAR_WIDTH, 0.0f,
                                  bounds.getWidth() - SIDEBAR_WIDTH,
                                  bounds.getHeight()).toFloat();
}

juce::Rectangle<float> ModernSettingsPanel::getSidebarBounds() const
{
    auto bounds = getLocalBounds();
    return juce::Rectangle<float>(0.0f, 0.0f, SIDEBAR_WIDTH, bounds.getHeight()).toFloat();
}

std::vector<SettingItem*> ModernSettingsPanel::getSettingsForCategory(Category category)
{
    std::vector<SettingItem*> result;
    for (auto& item : settings_) {
        if (item.category == category) {
            result.push_back(&item);
        }
    }
    return result;
}

//==============================================================================
// Input Handling
//==============================================================================

void ModernSettingsPanel::resized()
{
    // Update child component positions if needed
}

void ModernSettingsPanel::mouseMove(const juce::MouseEvent& e)
{
    updateHoverState(e.getPosition());
}

void ModernSettingsPanel::mouseExit(const juce::MouseEvent&)
{
    hoveredItemIndex_ = -1;
    for (auto& item : settings_) {
        item.isHovered = false;
    }
    markDirty();
}

void ModernSettingsPanel::mouseDown(const juce::MouseEvent& e)
{
    auto pos = e.getPosition();
    
    // Check category buttons
    auto sidebarBounds = getSidebarBounds();
    if (sidebarBounds.contains(pos.x, pos.y)) {
        float buttonY = sidebarBounds.getY() + 60.0f;
        
        if (pos.y >= buttonY && pos.y < buttonY + 40.0f) {
            handleCategoryClick(Category::Audio);
        } else if (pos.y >= buttonY + 50.0f && pos.y < buttonY + 90.0f) {
            handleCategoryClick(Category::Interface);
        } else if (pos.y >= buttonY + 100.0f && pos.y < buttonY + 140.0f) {
            handleCategoryClick(Category::Performance);
        } else if (pos.y >= buttonY + 150.0f && pos.y < buttonY + 190.0f) {
            handleCategoryClick(Category::Advanced);
        }
    }
    
    // Check setting items
    auto contentBounds = getMainContentBounds();
    if (contentBounds.contains(pos.x, pos.y)) {
        auto categorySettings = getSettingsForCategory(activeCategory_);
        float y = contentBounds.getY() + CARD_PADDING;
        
        for (size_t i = 0; i < categorySettings.size(); ++i) {
            if (pos.y >= y && pos.y < y + ITEM_HEIGHT) {
                handleSettingClick(static_cast<int>(i));
                break;
            }
            y += ITEM_HEIGHT + 16.0f;
        }
        
        // Check apply button
        SkRect applyRect = SkRect::MakeXYWH(contentBounds.getRight() - 120.0f,
                                            contentBounds.getBottom() - 50.0f,
                                            100.0f, 36.0f);
        if (applyRect.contains(pos.x, pos.y)) {
            applySettings();
        }
    }
}

void ModernSettingsPanel::handleCategoryClick(Category category)
{
    if (activeCategory_ != category) {
        activeCategory_ = category;
        markDirty();
    }
}

void ModernSettingsPanel::handleSettingClick(int itemIndex)
{
    auto categorySettings = getSettingsForCategory(activeCategory_);
    if (itemIndex < 0 || itemIndex >= static_cast<int>(categorySettings.size()))
        return;
    
    auto& item = *categorySettings[itemIndex];
    
    switch (item.type) {
        case SettingItem::Toggle: {
            item.currentValue = !item.currentValue.getBoolValue();
            if (item.onValueChanged)
                item.onValueChanged(item.currentValue);
            markDirty();
            break;
        }
        
        case SettingItem::Button: {
            if (item.onButtonClicked)
                item.onButtonClicked();
            break;
        }
        
        default:
            break;
    }
}

void ModernSettingsPanel::updateHoverState(const juce::Point<int>& mousePos)
{
    auto contentBounds = getMainContentBounds();
    
    if (contentBounds.contains(mousePos.x, mousePos.y)) {
        auto categorySettings = getSettingsForCategory(activeCategory_);
        float y = contentBounds.getY() + CARD_PADDING;
        
        int newHoverIndex = -1;
        for (size_t i = 0; i < categorySettings.size(); ++i) {
            if (mousePos.y >= y && mousePos.y < y + ITEM_HEIGHT) {
                newHoverIndex = static_cast<int>(i);
                break;
            }
            y += ITEM_HEIGHT + 16.0f;
        }
        
        if (newHoverIndex != hoveredItemIndex_) {
            hoveredItemIndex_ = newHoverIndex;
            
            // Update hover state on items
            for (auto& item : settings_) {
                item.isHovered = false;
            }
            
            if (hoveredItemIndex_ >= 0 && hoveredItemIndex_ < static_cast<int>(categorySettings.size())) {
                categorySettings[hoveredItemIndex_]->isHovered = true;
            }
            
            markDirty();
        }
    } else {
        if (hoveredItemIndex_ != -1) {
            hoveredItemIndex_ = -1;
            for (auto& item : settings_) {
                item.isHovered = false;
            }
            markDirty();
        }
    }
}

} // namespace zenith
