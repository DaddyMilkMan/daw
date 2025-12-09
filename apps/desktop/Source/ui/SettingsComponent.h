/*
  ==============================================================================

    SettingsComponent.h
    Created: 2025-12-03
    Author:  Zenith DAW

    Comprehensive Settings Panel.
    Tabs: Audio, MIDI, Plugins, Display, General.
  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <include/core/SkColor.h>
#include "../Settings.h"
#include "../../include/Engine.h"
#include "../engine/PluginHost.h"
#include "ZenithLookAndFeel.h"
#include "../ui/skia/ZenithDesignSystem.h"

namespace zenith {

//==============================================================================
// Audio Settings Tab
//==============================================================================
class AudioSettingsTab : public juce::Component {
public:
    AudioSettingsTab(Engine& engine) 
        : selector(engine.getDeviceManager(), 
                   0, 256,  // Min/Max inputs
                   0, 256,  // Min/Max outputs
                   false,   // Show MIDI input (we have separate tab)
                   false,   // Show MIDI output
                   false,   // Show channels as stereo pairs
                   false)   // Hide advanced options
    {
        addAndMakeVisible(selector);
    }

    void resized() override {
        selector.setBounds(getLocalBounds().reduced(10));
    }

private:
    juce::AudioDeviceSelectorComponent selector;
};

//==============================================================================
// MIDI Settings Tab
//==============================================================================
class MidiSettingsTab : public juce::Component, private juce::Timer {
public:
    MidiSettingsTab(Engine& engine) : engine_(engine) {
        addAndMakeVisible(inputList);
        inputList.setText("MIDI Inputs");
        
        startTimer(1000); // Refresh list every second
        refreshList();
    }
    
    ~MidiSettingsTab() override {
        stopTimer();
    }

    void resized() override {
        auto area = getLocalBounds().reduced(20);
        inputList.setBounds(area);
        
        int y = 30;
        for (auto* cb : checkBoxes) {
            cb->setBounds(20, y, 300, 24);
            y += 28;
        }
    }

    void timerCallback() override {
        auto newInputs = juce::MidiInput::getAvailableDevices();
        if (newInputs != cachedInputs) {
            refreshList();
        }
    }

private:
    void refreshList() {
        cachedInputs = juce::MidiInput::getAvailableDevices();
        checkBoxes.clear();
        
        int y = 40;
        for (const auto& input : cachedInputs) {
            auto* cb = new juce::ToggleButton(input.name);
            cb->setToggleState(engine_.getDeviceManager().isMidiInputDeviceEnabled(input.identifier), juce::dontSendNotification);
            
            cb->onClick = [this, id = input.identifier, cb]() {
                engine_.getDeviceManager().setMidiInputDeviceEnabled(id, cb->getToggleState());
            };
            
            addAndMakeVisible(cb);
            cb->setBounds(30, y, 300, 24);
            checkBoxes.add(cb);
            y += 28;
        }
    }

    Engine& engine_;
    juce::GroupComponent inputList;
    juce::Array<juce::MidiDeviceInfo> cachedInputs;
    juce::OwnedArray<juce::ToggleButton> checkBoxes;
};

//==============================================================================
// Plugin Settings Tab
//==============================================================================
class PluginSettingsTab : public juce::Component {
public:
    PluginSettingsTab(PluginHost& host) : pluginHost_(host) {
        addAndMakeVisible(pathList);
        pathList.setMultiLine(true);
        pathList.setReadOnly(true);
        
        addAndMakeVisible(addButton);
        addAndMakeVisible(removeButton);
        addAndMakeVisible(scanButton);
        
        addButton.setButtonText("Add Path");
        removeButton.setButtonText("Remove");
        scanButton.setButtonText("Scan All Plugins");
        
        updateList();
        
        addButton.onClick = [this]() {
            auto chooser = std::make_shared<juce::FileChooser>("Select VST3 Folder", juce::File::getSpecialLocation(juce::File::userHomeDirectory));
            auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories;
            chooser->launchAsync(flags, [this, chooser](const juce::FileChooser& fc) {
                auto result = fc.getResult();
                if (result.exists()) {
                    pluginHost_.addSearchPath(result.getFullPathName());
                    updateList();
                }
            });
        };
        
        removeButton.onClick = [this]() {
            // Simple: Remove last path or prompt user
            // For MVP, we'll just clear and let user re-add
        };
        
        scanButton.onClick = [this]() {
            scanButton.setEnabled(false);
            scanButton.setButtonText("Scanning...");
            
            pluginHost_.scanAsync([this](int progress, int count, const juce::String& msg) {
                juce::ignoreUnused(msg);
                if (progress >= 100) {
                    scanButton.setEnabled(true);
                    scanButton.setButtonText("Scan All Plugins");
                    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Scan Complete", 
                        "Found " + juce::String(count) + " plugins.");
                }
            });
        };
    }
    
    void resized() override {
        auto area = getLocalBounds().reduced(20);
        
        auto buttonRow = area.removeFromBottom(30);
        addButton.setBounds(buttonRow.removeFromLeft(100));
        buttonRow.removeFromLeft(10);
        removeButton.setBounds(buttonRow.removeFromLeft(100));
        buttonRow.removeFromLeft(20);
        scanButton.setBounds(buttonRow.removeFromRight(150));
        
        area.removeFromBottom(10);
        pathList.setBounds(area);
    }

private:
    void updateList() {
        juce::String text;
        for (const auto& path : pluginHost_.getSearchPaths()) {
            text += path + "\n";
        }
        pathList.setText(text);
    }

    PluginHost& pluginHost_;
    juce::TextEditor pathList;
    juce::TextButton addButton;
    juce::TextButton removeButton;
    juce::TextButton scanButton;
};

//==============================================================================
// Display Settings Tab
//==============================================================================
class DisplaySettingsTab : public juce::Component, public juce::ChangeListener {
public:
    DisplaySettingsTab() {
        addAndMakeVisible(backendLabel);
        backendLabel.setText("Graphics Backend:", juce::dontSendNotification);
        addAndMakeVisible(backendCombo);
        
        backendCombo.addItem("Auto", 1);
        backendCombo.addItem("Direct3D 12", 2);
        backendCombo.addItem("Metal", 3);
        backendCombo.addItem("Vulkan", 4);
        backendCombo.addItem("OpenGL", 5);
        backendCombo.addItem("Software", 6);
        
        backendCombo.onChange = [this]() {
            Settings::getInstance().setRenderBackend((SkiaRenderer::Backend)(backendCombo.getSelectedId() - 1)); 
        };
        
        addAndMakeVisible(fpsLabel);
        fpsLabel.setText("Target FPS:", juce::dontSendNotification);
        addAndMakeVisible(fpsSlider);
        fpsSlider.setRange(30, 240, 1);
        fpsSlider.onValueChange = [this]() {
            Settings::getInstance().setTargetFPS((int)fpsSlider.getValue());
        };

        addAndMakeVisible(glowLabel);
        glowLabel.setText("Glow Intensity:", juce::dontSendNotification);
        addAndMakeVisible(glowSlider);
        glowSlider.setRange(0.0, 2.0, 0.1);
        glowSlider.setTooltip("Adjust the neon bloom effect. 0 = Flat, 1 = Standard, 2 = Extra Glow");
        glowSlider.onValueChange = [this]() {
            Settings::getInstance().setGlowIntensity((float)glowSlider.getValue());
        };
        
        updateFromSettings();
    }
    
    ~DisplaySettingsTab() override {}
    
    void changeListenerCallback(juce::ChangeBroadcaster*) override {
        updateFromSettings();
    }
    
    void resized() override {
        auto area = getLocalBounds().reduced(20);
        auto row1 = area.removeFromTop(30);
        backendLabel.setBounds(row1.removeFromLeft(150));
        backendCombo.setBounds(row1.removeFromLeft(200));
        
        area.removeFromTop(10);
        auto row2 = area.removeFromTop(30);
        fpsLabel.setBounds(row2.removeFromLeft(150));
        fpsSlider.setBounds(row2.removeFromLeft(200));

        area.removeFromTop(10);
        auto row3 = area.removeFromTop(30);
        glowLabel.setBounds(row3.removeFromLeft(150));
        glowSlider.setBounds(row3.removeFromLeft(200));
    }

private:
    void updateFromSettings() {
        auto backend = Settings::getInstance().getRenderBackend();
        backendCombo.setSelectedId((int)backend + 1, juce::dontSendNotification);
        
        fpsSlider.setValue(Settings::getInstance().getTargetFPS(), juce::dontSendNotification);
        glowSlider.setValue(Settings::getInstance().getGlowIntensity(), juce::dontSendNotification);
    }

    juce::Label backendLabel;
    juce::ComboBox backendCombo;
    juce::Label fpsLabel;
    juce::Slider fpsSlider;
    juce::Label glowLabel;
    juce::Slider glowSlider;
};

//==============================================================================
// AI Settings Tab
//==============================================================================
class AiSettingsTab : public juce::Component {
public:
    AiSettingsTab() {
        // Placeholder for AI settings
        addAndMakeVisible(placeholderLabel);
        placeholderLabel.setText("AI Integration Settings (Coming Soon)", juce::dontSendNotification);
        placeholderLabel.setJustificationType(juce::Justification::centred);
    }

    void resized() override {
        placeholderLabel.setBounds(getLocalBounds().reduced(20));
    }

private:
    juce::Label placeholderLabel;
};

//==============================================================================
// Color Picker Widget (Skia-based for Theme Editor)
//==============================================================================
class ColorPickerButton : public juce::Component {
public:
    ColorPickerButton(const juce::String& label, SkColor initialColor)
        : label_(label), color_(initialColor) 
    {
        setSize(100, 30);
    }
    
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        
        // Background
        g.setColour(juce::Colour(0xFF1A1A23));
        g.fillRoundedRectangle(bounds, 4.0f);
        
        // Color swatch
        auto swatchBounds = bounds.removeFromRight(40).reduced(4);
        g.setColour(juce::Colour(
            SkColorGetR(color_), 
            SkColorGetG(color_), 
            SkColorGetB(color_)
        ).withAlpha(static_cast<uint8_t>(SkColorGetA(color_))));
        g.fillRoundedRectangle(swatchBounds, 3.0f);
        
        // Border around swatch
        g.setColour(juce::Colours::white.withAlpha(0.3f));
        g.drawRoundedRectangle(swatchBounds, 3.0f, 1.0f);
        
        // Label
        g.setColour(juce::Colours::white);
        g.setFont(12.0f);
        g.drawText(label_, bounds.reduced(8, 0), juce::Justification::centredLeft);
    }
    
    void mouseDown(const juce::MouseEvent&) override {
        // Open color picker
        auto colourSelector = std::make_unique<juce::ColourSelector>(
            juce::ColourSelector::showColourAtTop | 
            juce::ColourSelector::showSliders | 
            juce::ColourSelector::showColourspace
        );
        colourSelector->setCurrentColour(juce::Colour(
            SkColorGetR(color_), 
            SkColorGetG(color_), 
            SkColorGetB(color_)
        ).withAlpha(static_cast<uint8_t>(SkColorGetA(color_))));
        colourSelector->setSize(300, 400);
        
        colourSelector->addChangeListener(new ColorListener(this));
        
        juce::CallOutBox::launchAsynchronously(
            std::move(colourSelector), 
            getScreenBounds(), 
            nullptr
        );
    }
    
    void setColor(SkColor color) { 
        color_ = color; 
        repaint(); 
        if (onColorChange) onColorChange(color_);
    }
    
    SkColor getColor() const { return color_; }
    
    std::function<void(SkColor)> onColorChange;
    
private:
    class ColorListener : public juce::ChangeListener {
    public:
        ColorListener(ColorPickerButton* parent) : parent_(parent) {}
        
        void changeListenerCallback(juce::ChangeBroadcaster* source) override {
            if (auto* selector = dynamic_cast<juce::ColourSelector*>(source)) {
                auto juceColor = selector->getCurrentColour();
                SkColor skColor = SkColorSetARGB(
                    juceColor.getAlpha(),
                    juceColor.getRed(),
                    juceColor.getGreen(),
                    juceColor.getBlue()
                );
                parent_->setColor(skColor);
            }
        }
        
    private:
        ColorPickerButton* parent_;
    };
    
    juce::String label_;
    SkColor color_;
};

//==============================================================================
// Theme Editor Tab (Enhanced Appearance Settings)
//==============================================================================
class AppearanceSettingsTab : public juce::Component, 
                               public juce::ChangeListener {
public:
    AppearanceSettingsTab() {
        // Register for theme changes
        design::ThemeManager::getInstance().addChangeListener(this);
        
        // Theme selection header
        addAndMakeVisible(themeLabel);
        themeLabel.setText("Theme:", juce::dontSendNotification);
        themeLabel.setFont(juce::Font(16.0f, juce::Font::bold));
        
        addAndMakeVisible(themeCombo);
        refreshThemeList();
        themeCombo.onChange = [this]() {
            juce::String themeName = themeCombo.getText();
            if (themeName.startsWith("Default")) {
                design::ThemeManager::getInstance().resetToDefault();
            } else if (themeName.isNotEmpty()) {
                design::ThemeManager::getInstance().loadTheme(themeName);
            }
            updateColorPickers();
            if (auto* top = getTopLevelComponent()) top->repaint();
        };
        
        addAndMakeVisible(saveThemeBtn);
        saveThemeBtn.setButtonText("Save Theme");
        saveThemeBtn.onClick = [this]() {
            auto alertCallback = [this](int result) {
                if (result == 1) {
                    // User clicked OK - get name from alert window
                    // For simplicity, save with current theme name or "CustomTheme"
                    auto themeName = design::ThemeManager::getInstance().getCurrentTheme().name;
                    if (themeName.isEmpty() || themeName.startsWith("Default")) {
                        themeName = "CustomTheme_" + juce::Time::getCurrentTime().formatted("%H%M%S");
                    }
                    design::ThemeManager::getInstance().saveTheme(themeName);
                    refreshThemeList();
                }
            };
            
            juce::AlertWindow::showOkCancelBox(
                juce::AlertWindow::QuestionIcon,
                "Save Theme",
                "Save current colors as a new theme?",
                "Save", "Cancel",
                nullptr,
                juce::ModalCallbackFunction::create(alertCallback)
            );
        };
        
        addAndMakeVisible(resetBtn);
        resetBtn.setButtonText("Reset to Default");
        resetBtn.onClick = [this]() {
            design::ThemeManager::getInstance().resetToDefault();
            updateColorPickers();
            if (auto* top = getTopLevelComponent()) top->repaint();
        };
        
        // Section: Accent Colors
        addAndMakeVisible(accentLabel);
        accentLabel.setText("Accent Colors", juce::dontSendNotification);
        accentLabel.setFont(juce::Font(14.0f, juce::Font::bold));
        
        createColorPicker("primary", "Primary (Cyan)");
        createColorPicker("secondary", "Secondary (Magenta)");
        createColorPicker("accent", "Accent (Green)");
        
        // Section: Status Colors
        addAndMakeVisible(statusLabel);
        statusLabel.setText("Status Colors", juce::dontSendNotification);
        statusLabel.setFont(juce::Font(14.0f, juce::Font::bold));
        
        createColorPicker("warning", "Warning");
        createColorPicker("danger", "Danger");
        createColorPicker("info", "Info");
        
        // Section: Background Colors
        addAndMakeVisible(bgLabel);
        bgLabel.setText("Backgrounds", juce::dontSendNotification);
        bgLabel.setFont(juce::Font(14.0f, juce::Font::bold));
        
        createColorPicker("bgDarkest", "Darkest");
        createColorPicker("bgDarker", "Darker");
        createColorPicker("bgDark", "Dark");
        createColorPicker("bgMedium", "Medium");
        createColorPicker("bgLight", "Light");
        
        // Section: Layout
        addAndMakeVisible(layoutLabel);
        layoutLabel.setText("Layout Options", juce::dontSendNotification);
        layoutLabel.setFont(juce::Font(14.0f, juce::Font::bold));
        
        addAndMakeVisible(editModeToggle);
        editModeToggle.setButtonText("Enable UI Edit Mode");
        editModeToggle.setToggleState(
            design::LayoutManager::getInstance().isEditModeEnabled(), 
            juce::dontSendNotification
        );
        editModeToggle.onClick = [this]() {
            bool enabled = editModeToggle.getToggleState();
            design::LayoutManager::getInstance().setEditModeEnabled(enabled);
            if (auto* top = getTopLevelComponent()) top->repaint();
        };
        
        addAndMakeVisible(saveLayoutBtn);
        saveLayoutBtn.setButtonText("Save Layout");
        saveLayoutBtn.onClick = [this]() {
            design::LayoutManager::getInstance().saveLayout("UserLayout");
        };
        
        // Initialize color pickers with current theme
        updateColorPickers();
    }
    
    ~AppearanceSettingsTab() override {
        design::ThemeManager::getInstance().removeChangeListener(this);
    }
    
    void changeListenerCallback(juce::ChangeBroadcaster*) override {
        // Theme changed externally, update color pickers
        updateColorPickers();
        repaint();
    }
    
    void refreshThemeList() {
        themeCombo.clear();
        auto themes = design::ThemeManager::getInstance().getAvailableThemes();
        int i = 1;
        for (const auto& t : themes) {
            themeCombo.addItem(t, i++);
        }
    }
    
    void resized() override {
        auto area = getLocalBounds().reduced(20);
        const int rowHeight = 35;
        const int sectionGap = 15;
        const int labelWidth = 150;
        
        // Theme selection row
        auto themeRow = area.removeFromTop(rowHeight);
        themeLabel.setBounds(themeRow.removeFromLeft(80));
        themeCombo.setBounds(themeRow.removeFromLeft(180));
        themeRow.removeFromLeft(10);
        saveThemeBtn.setBounds(themeRow.removeFromLeft(100));
        themeRow.removeFromLeft(10);
        resetBtn.setBounds(themeRow.removeFromLeft(120));
        
        area.removeFromTop(sectionGap);
        
        // Accent Colors section
        accentLabel.setBounds(area.removeFromTop(25));
        area.removeFromTop(5);
        
        layoutColorPickers(area, {"primary", "secondary", "accent"}, labelWidth, rowHeight);
        area.removeFromTop(sectionGap);
        
        // Status Colors section
        statusLabel.setBounds(area.removeFromTop(25));
        area.removeFromTop(5);
        
        layoutColorPickers(area, {"warning", "danger", "info"}, labelWidth, rowHeight);
        area.removeFromTop(sectionGap);
        
        // Background Colors section
        bgLabel.setBounds(area.removeFromTop(25));
        area.removeFromTop(5);
        
        layoutColorPickers(area, {"bgDarkest", "bgDarker", "bgDark", "bgMedium", "bgLight"}, labelWidth, rowHeight);
        area.removeFromTop(sectionGap);
        
        // Layout section
        layoutLabel.setBounds(area.removeFromTop(25));
        area.removeFromTop(5);
        editModeToggle.setBounds(area.removeFromTop(rowHeight).removeFromLeft(200));
        saveLayoutBtn.setBounds(area.removeFromTop(rowHeight).removeFromLeft(120));
    }
    
private:
    void createColorPicker(const juce::String& id, const juce::String& label) {
        auto picker = std::make_unique<ColorPickerButton>(label, 0xFF000000);
        picker->onColorChange = [this, id](SkColor color) {
            design::ThemeManager::getInstance().setColor(id, color);
            if (auto* top = getTopLevelComponent()) top->repaint();
        };
        addAndMakeVisible(*picker);
        colorPickers_[id] = std::move(picker);
    }
    
    void updateColorPickers() {
        auto& theme = design::ThemeManager::getInstance().getCurrentTheme();
        auto colorMap = theme.toMap();
        
        for (auto& [id, picker] : colorPickers_) {
            auto it = colorMap.find(id);
            if (it != colorMap.end()) {
                picker->setColor(it->second);
            }
        }
    }
    
    void layoutColorPickers(juce::Rectangle<int>& area, 
                            const std::vector<juce::String>& ids,
                            int pickerWidth, int rowHeight) {
        for (const auto& id : ids) {
            if (colorPickers_.count(id)) {
                colorPickers_[id]->setBounds(area.removeFromTop(rowHeight).removeFromLeft(pickerWidth));
            }
        }
    }
    
    juce::Label themeLabel;
    juce::ComboBox themeCombo;
    juce::TextButton saveThemeBtn;
    juce::TextButton resetBtn;
    
    juce::Label accentLabel;
    juce::Label statusLabel;
    juce::Label bgLabel;
    juce::Label layoutLabel;
    
    juce::ToggleButton editModeToggle;
    juce::TextButton saveLayoutBtn;
    
    std::map<juce::String, std::unique_ptr<ColorPickerButton>> colorPickers_;
};

//==============================================================================
// Main Settings Component
//==============================================================================
class SettingsComponent : public juce::Component {
public:
    SettingsComponent(Engine& engine) 
        : audioTab(engine), 
          midiTab(engine), 
          pluginTab(engine.getPluginHost()), 
          displayTab(),
          aiTab(),
          appearanceTab()
    {
        setLookAndFeel(&ZenithLookAndFeel::getInstance());
        addAndMakeVisible(tabs);
        tabs.addTab("Audio", juce::Colours::darkgrey, &audioTab, false);
        tabs.addTab("MIDI", juce::Colours::darkgrey, &midiTab, false);
        tabs.addTab("Plugins", juce::Colours::darkgrey, &pluginTab, false);
        tabs.addTab("Display", juce::Colours::darkgrey, &displayTab, false);
        tabs.addTab("Appearance", juce::Colours::darkgrey, &appearanceTab, false);
        tabs.addTab("AI", juce::Colours::darkgrey, &aiTab, false);
    }

    ~SettingsComponent() override {
        setLookAndFeel(nullptr);
    }

    void resized() override {
        tabs.setBounds(getLocalBounds());
    }

private:
    juce::TabbedComponent tabs { juce::TabbedButtonBar::TabsAtTop };
    
    AudioSettingsTab audioTab;
    MidiSettingsTab midiTab;
    PluginSettingsTab pluginTab;
    DisplaySettingsTab displayTab;
    AiSettingsTab aiTab;
    AppearanceSettingsTab appearanceTab;
};

} // namespace zenith