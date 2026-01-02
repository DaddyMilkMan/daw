/*
  ==============================================================================

    GlobalSettingsPanel.cpp
    Created: 2025-12-30
    Updated: 2026-01-02
    Author:  Zenith DAW

    Premium Settings Modal with modular tab architecture.

  ==============================================================================
*/

#include "GlobalSettingsPanel.h"
#include "../design-system/ZenithTheme.h"
#include "../design-system/ZenithDesignSystem.h"
#include "../framework/GlassmorphicPanel.h"
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_utils/juce_audio_utils.h>

namespace zenith {

//==============================================================================
// BASE SETTINGS PANEL
//==============================================================================
class SettingsSubPanel : public SkiaComponent {
public:
    SettingsSubPanel() {}
    virtual ~SettingsSubPanel() = default;
    
    // Helper for consistent layout rows
    void performLayout(const juce::Rectangle<int>& bounds) {
        int y = bounds.getY();
        int width = bounds.getWidth();
        const int rowHeight = 40;
        const int gap = 12;
        
        for (auto* child : getChildren()) {
            if (child->isVisible()) {
                // Simple layout: Label (if exists) -> Control
                // For now, we assume children are added in order
                child->setBounds(bounds.getX(), y, width, rowHeight);
                y += rowHeight + gap;
            }
        }
    }
};

//==============================================================================
// AUDIO SETTINGS PANEL (Refactored)
//==============================================================================
class AudioSettingsPanel : public SettingsSubPanel {
public:
    AudioSettingsPanel(juce::AudioDeviceManager& dm) : deviceManager(dm) {
        // Latency Display
        latencyLabel = std::make_unique<SkiaLabel>();
        addAndMakeVisible(latencyLabel.get());
        
        // Timer to update latency (only runs when visible)
        // startTimer(1000); // Moved to visibilityChanged
    }
    
    void visibilityChanged() override {
        if (isVisible()) {
            if (!deviceSelector) {
                deviceSelector = std::make_unique<juce::AudioDeviceSelectorComponent>(
                    deviceManager,
                    0, 256, 0, 256, true, true, true, false
                );
                addAndMakeVisible(deviceSelector.get());
                resized(); // Ensure layout
            }
            startTimer(1000);
            updateLatencyDisplay();
        } else {
            // Optional: Destroy to free resources, or keep it. 
            // Keeping it is safer for state, but destroying avoids background polling.
            // Let's keep it for now but stop timer.
            stopTimer();
        }
    }
    
    void resized() override {
        auto area = getLocalBounds();
        latencyLabel->setBounds(area.removeFromTop(30));
        if (deviceSelector) {
            deviceSelector->setBounds(area);
        }
    }
    
    void drawSkia(SkCanvas* canvas) override {}
    
    void timerCallback() override {
        updateLatencyDisplay();
    }
    
    void updateLatencyDisplay() {
        if (auto* device = deviceManager.getCurrentAudioDevice()) {
            double sampleRate = device->getCurrentSampleRate();
            int bufferSize = device->getCurrentBufferSizeSamples();
            int latencySamples = device->getOutputLatencyInSamples() + device->getInputLatencyInSamples();
            double latencyMs = (latencySamples / sampleRate) * 1000.0;
            
            juce::String text = juce::String::formatted("Total Roundtrip Latency: %.1f ms (%d samples @ %.0f Hz)", 
                                                      latencyMs, latencySamples, sampleRate);
            latencyLabel->setText(text);
        } else {
            latencyLabel->setText("No Audio Device Selected");
        }
    }

private:
    juce::AudioDeviceManager& deviceManager;
    std::unique_ptr<juce::AudioDeviceSelectorComponent> deviceSelector;
    std::unique_ptr<SkiaLabel> latencyLabel;
};

//==============================================================================
// KEYBOARD SETTINGS PANEL (Refactored with Search)
//==============================================================================
class KeyboardSettingsPanel : public SettingsSubPanel, public SkiaListBox::Model {
public:
    KeyboardSettingsPanel() {
        searchBox = std::make_unique<ZenithTextInput>("Search Shortcuts...");
        searchBox->onTextChanged = [this](const juce::String& text) {
            filterShortcuts(text);
        };
        addAndMakeVisible(searchBox.get());
        
        // Initialize shortcuts
        allShortcuts = {
            {"Play / Pause", "Transport", "Space"},
            {"Record", "Transport", "R"},
            {"Undo", "Edit", "Ctrl+Z"},
            {"Redo", "Edit", "Ctrl+Shift+Z"},
            {"Quantize", "MIDI", "Q"},
            {"Slice", "Edit", "S"},
            {"Duplicate", "Edit", "Ctrl+D"},
            {"Open Settings", "General", "Ctrl+,"}
        };
        filteredShortcuts = allShortcuts;
        
        listBox = std::make_unique<SkiaListBox>();
        listBox->setModel(this); 
        addAndMakeVisible(listBox.get());
    }
    
    void resized() override {
        auto area = getLocalBounds();
        searchBox->setBounds(area.removeFromTop(40).reduced(0, 5));
        listBox->setBounds(area.reduced(0, 10));
    }
    
    void drawSkia(SkCanvas* canvas) override {
        // ListBox handles drawing
    }
    
    void filterShortcuts(const juce::String& text) {
        filteredShortcuts.clear();
        for (const auto& s : allShortcuts) {
            if (s.action.containsIgnoreCase(text) || s.category.containsIgnoreCase(text)) {
                filteredShortcuts.push_back(s);
            }
        }
        listBox->updateContent(); 
        repaint();
    }
    
    // ListBoxModel implementation
    int getNumRows() override { return (int)filteredShortcuts.size(); }
    
    void paintListBoxItem(int row, SkCanvas& canvas, int width, int height, bool rowIsSelected) override {
        if (row >= filteredShortcuts.size()) return;
        
        const auto& item = filteredShortcuts[row];
        SkPaint paint;
        paint.setAntiAlias(true);
        
        // Background
        if (rowIsSelected) {
            paint.setColor(SkColorSetA(SK_ColorCYAN, 50));
            canvas.drawRect(SkRect::MakeWH(width, height), paint);
        }
        
        // Text
        paint.setColor(SK_ColorWHITE);
        SkFont font = design::getSkFont(14);
        
        canvas.drawString(item.action.toRawUTF8(), 10, height/2 + 5, font, paint);
        
        paint.setColor(SkColorSetA(SK_ColorWHITE, 150));
        canvas.drawString(item.category.toRawUTF8(), width * 0.4f, height/2 + 5, font, paint);
        
        paint.setColor(SK_ColorCYAN);
        canvas.drawString(item.key.toRawUTF8(), width * 0.7f, height/2 + 5, font, paint);
    }

private:
    struct Shortcut { juce::String action; juce::String category; juce::String key; };
    std::vector<Shortcut> allShortcuts;
    std::vector<Shortcut> filteredShortcuts;
    
    std::unique_ptr<ZenithTextInput> searchBox;
    std::unique_ptr<SkiaListBox> listBox;
};

//==============================================================================
// GENERAL SETTINGS PANEL
//==============================================================================
class GeneralSettingsPanel : public SettingsSubPanel {
public:
    GeneralSettingsPanel() {
        addLabel("Project Settings");
        projectPath = std::make_unique<ZenithTextInput>("Default Project Folder");
        projectPath->setText("~/Documents/Zenith Projects");
        addAndMakeVisible(projectPath.get());
        
        addLabel("Behavior");
        autoSaveToggle = std::make_unique<ZenithToggle>("Enable Auto-Save (5 min)");
        autoSaveToggle->setToggleState(true);
        addAndMakeVisible(autoSaveToggle.get());
        
        restoreToggle = std::make_unique<ZenithToggle>("Restore Last Project on Startup");
        restoreToggle->setToggleState(true);
        addAndMakeVisible(restoreToggle.get());
    }
    
    void resized() override {
        using namespace juce;
        FlexBox fb;
        fb.flexDirection = FlexBox::Direction::column;
        fb.alignItems = FlexBox::AlignItems::stretch;
        
        for (auto* c : getChildren()) {
            fb.items.add(FlexItem(*c).withHeight(40).withMargin({0, 0, 10, 0}));
        }
        
        fb.performLayout(getLocalBounds().reduced(20));
    }

    void drawSkia(SkCanvas* canvas) override {}
    
private:
    void addLabel(const juce::String& text) {
        auto l = std::make_unique<SkiaLabel>();
        l->setText(text);
        addAndMakeVisible(l.get());
        labels.push_back(std::move(l));
    }
    
    std::vector<std::unique_ptr<SkiaLabel>> labels;
    std::unique_ptr<ZenithTextInput> projectPath;
    std::unique_ptr<ZenithToggle> autoSaveToggle;
    std::unique_ptr<ZenithToggle> restoreToggle;
};

//==============================================================================
// MIDI SETTINGS PANEL (Refactored)
//==============================================================================
class MidiSettingsPanel : public SettingsSubPanel {
public:
    MidiSettingsPanel() {
        mpeToggle = std::make_unique<ZenithToggle>("Enable MPE");
        addAndMakeVisible(mpeToggle.get());
        
        zoneLower = std::make_unique<ZenithTextInput>("MPE Zone (Lower)");
        zoneLower->setText("Channels 2-8");
        addAndMakeVisible(zoneLower.get());
        
        zoneUpper = std::make_unique<ZenithTextInput>("MPE Zone (Upper)");
        zoneUpper->setText("Channels 9-16");
        addAndMakeVisible(zoneUpper.get());
    }
    
    void resized() override {
        using namespace juce;
        FlexBox fb;
        fb.flexDirection = FlexBox::Direction::column;
        
        fb.items.add(FlexItem(*mpeToggle).withHeight(40));
        fb.items.add(FlexItem(*zoneLower).withHeight(40).withMargin({10,0,0,0}));
        fb.items.add(FlexItem(*zoneUpper).withHeight(40).withMargin({10,0,0,0}));
        
        fb.performLayout(getLocalBounds().reduced(20));
    }
    void drawSkia(SkCanvas* canvas) override {}
    
private:
    std::unique_ptr<ZenithToggle> mpeToggle;
    std::unique_ptr<ZenithTextInput> zoneLower;
    std::unique_ptr<ZenithTextInput> zoneUpper;
};

// ... (Other panels would follow similar pattern: Appearance, Plugins, etc.)

class PlaceholderPanel : public SettingsSubPanel {
public:
    PlaceholderPanel(const juce::String& name) : name_(name) {}
    void drawSkia(SkCanvas* canvas) override {
        SkPaint p;
        p.setColor(SK_ColorWHITE);
        canvas->drawString(name_.toRawUTF8(), 20, 40, design::getSkFont(20), p);
    }
private:
    juce::String name_;
};

//==============================================================================
// GLOBAL SETTINGS PANEL IMPLEMENTATION
//==============================================================================

GlobalSettingsPanel::GlobalSettingsPanel(juce::AudioDeviceManager& deviceManager)
    : deviceManager_(deviceManager) {
  
  createTabButtons();
  
  // Create Panels
  generalPanel_ = std::make_unique<GeneralSettingsPanel>();
  addChildComponent(generalPanel_.get());
  
  audioPanel_ = std::make_unique<AudioSettingsPanel>(deviceManager_);
  addChildComponent(audioPanel_.get());
  
  midiPanel_ = std::make_unique<MidiSettingsPanel>();
  addChildComponent(midiPanel_.get());
  
  keyboardPanel_ = std::make_unique<KeyboardSettingsPanel>();
  addChildComponent(keyboardPanel_.get());
  
  // Stubs for others
  pluginPanel_ = std::make_unique<PluginSettingsPanel>(); 
  appearancePanel_ = std::make_unique<AppearanceSettingsPanel>();
  collabPanel_ = std::make_unique<CollaborationSettingsPanel>();
  advancedPanel_ = std::make_unique<AdvancedSettingsPanel>();
  
  // Close Button
  closeBtn_ = std::make_unique<ZenithButton>();
  closeBtn_->setText("Close");
  closeBtn_->onClick = [this] { hide(); };
  addAndMakeVisible(closeBtn_.get());
  
  // Initial State
  switchCategory(Category::General);
  
  setSize(900, 600);
}

GlobalSettingsPanel::~GlobalSettingsPanel() {
    deviceManager_.removeChangeListener(this);
}

void GlobalSettingsPanel::createTabButtons() {
  for (int i = 0; i < static_cast<int>(Category::COUNT); ++i) {
    auto btn = std::make_unique<ZenithButton>();
    btn->setButtonStyle(ZenithButton::Style::Ghost);
    btn->setToggleable(true);
    
    // Set Text from Categories
    btn->setText(categories_[i].name);
    
    const int categoryIndex = i;
    btn->onClick = [this, categoryIndex] {
      switchCategory(static_cast<Category>(categoryIndex));
    };
    
    addAndMakeVisible(btn.get());
    tabButtons_.push_back(std::move(btn));
  }
}

void GlobalSettingsPanel::switchCategory(Category category) {
    currentCategory_ = category;
    
    // Update Buttons
    for(size_t i=0; i<tabButtons_.size(); ++i) {
        tabButtons_[i]->setToggleState(i == (int)category);
    }
    
    // Hide All
    if(generalPanel_) generalPanel_->setVisible(false);
    if(audioPanel_) audioPanel_->setVisible(false);
    if(midiPanel_) midiPanel_->setVisible(false);
    if(keyboardPanel_) keyboardPanel_->setVisible(false);
    // ... others
    
    // Show Current
    switch(category) {
        case Category::General: generalPanel_->setVisible(true); break;
        case Category::Audio: audioPanel_->setVisible(true); break;
        case Category::MIDI: midiPanel_->setVisible(true); break;
        case Category::Keyboard: keyboardPanel_->setVisible(true); break;
        // ...
        default: break;
    }
    
    resized();
}

void GlobalSettingsPanel::resized() {
    auto area = getLocalBounds();
    
    // Sidebar for Tabs
    auto sidebar = area.removeFromLeft(200);
    
    // Layout Tabs
    int btnH = 40;
    for(auto& btn : tabButtons_) {
        btn->setBounds(sidebar.removeFromTop(btnH).reduced(5));
    }
    
    // Content Area
    auto content = area.reduced(20);
    
    // Close button at bottom right
    closeBtn_->setBounds(content.removeFromBottom(40).removeFromRight(100));
    
    // Panels
    if(generalPanel_) generalPanel_->setBounds(content);
    if(audioPanel_) audioPanel_->setBounds(content);
    if(midiPanel_) midiPanel_->setBounds(content);
    if(keyboardPanel_) keyboardPanel_->setBounds(content);
}

void GlobalSettingsPanel::drawSkia(SkCanvas* canvas) {
    // Draw Backdrop (Glass)
    GlassmorphicPanel::Options opts;
    opts.style = GlassmorphicPanel::Style::Elevated;
    opts.useBackdropBlur = true;
    GlassmorphicPanel::drawWithOptions(canvas, SkRect::MakeWH(getWidth(), getHeight()), opts);
    
    // Draw Sidebar divider
    SkPaint p;
    p.setColor(SkColorSetA(SK_ColorWHITE, 30));
    canvas->drawLine(200, 0, 200, getHeight(), p);
}

void GlobalSettingsPanel::changeListenerCallback(juce::ChangeBroadcaster*) {
    // Pass to Audio Panel if active
}

void GlobalSettingsPanel::show() {
    setVisible(true);
    toFront(true);
    
    // Guard against immediate closing (bounce protection)
    canCloseAfter_ = juce::Time::getMillisecondCounter() + 500;
    
    // Ensure we are sized/centered properly
    // Fallback to 800x600 if parent is small
    int targetW = 800;
    int targetH = 600;
    
    if (auto* parent = getParentComponent()) {
        auto bounds = parent->getLocalBounds();
        if (bounds.getWidth() > 100) {
            targetW = juce::roundToInt(bounds.getWidth() * 0.8);
            targetH = juce::roundToInt(bounds.getHeight() * 0.85);
        }
    }
    
    // Enforce minimums
    targetW = std::max(targetW, 800);
    targetH = std::max(targetH, 600);
    
    centreWithSize(targetW, targetH);
    
    // FIX: Clamp negative positions (if parent is smaller than modal)
    // This prevents the "Top-Left Tiny Box" where the modal is shifted off-screen
    auto bounds = getBounds();
    if (bounds.getX() < 0) bounds.setX(0);
    if (bounds.getY() < 0) bounds.setY(0);
    setBounds(bounds);
}

void GlobalSettingsPanel::hide() {
    // Bounce guard
    if (juce::Time::getMillisecondCounter() < canCloseAfter_) {
        return;
    }

    setVisible(false);
    if(onClose) onClose();
}

void GlobalSettingsPanel::mouseDown(const juce::MouseEvent& event) {
    // Consume
}

} // namespace zenith

// Stub definitions for classes I didn't fully implement above to satisfy linker if needed
namespace zenith {
    class PluginSettingsPanel : public SettingsSubPanel {
    public: PluginSettingsPanel() {} void drawSkia(SkCanvas*) override {} };
    
    class AppearanceSettingsPanel : public SettingsSubPanel {
    public: AppearanceSettingsPanel() {} void drawSkia(SkCanvas*) override {} };
    
    class CollaborationSettingsPanel : public SettingsSubPanel {
    public: CollaborationSettingsPanel() {} void drawSkia(SkCanvas*) override {} };
    
    class AdvancedSettingsPanel : public SettingsSubPanel {
    public: AdvancedSettingsPanel() {} void drawSkia(SkCanvas*) override {} };
}
