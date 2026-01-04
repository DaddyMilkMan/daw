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
#include "../../engine/ZenithLogger.h"
#include "../framework/AnimationCoordinator.h"

namespace zenith {

//==============================================================================
// BASE SETTINGS PANEL
//==============================================================================
class SettingsSubPanel : public SkiaComponent {
public:
    SettingsSubPanel() {}
    virtual ~SettingsSubPanel() = default;
    
    void performLayout(const juce::Rectangle<int>& bounds) {
        int y = bounds.getY();
        int width = bounds.getWidth();
        const int rowHeight = 40;
        const int gap = 12;
        
        for (auto* child : getChildren()) {
            if (child->isVisible()) {
                child->setBounds(bounds.getX(), y, width, rowHeight);
                y += rowHeight + gap;
            }
        }
    }
};

//==============================================================================
// STUB PANELS (Defined BEFORE usage)
//==============================================================================
class PluginSettingsPanel : public SettingsSubPanel {
public: PluginSettingsPanel() {} void drawSkia(SkCanvas* c) override { /* TODO */ } };

class AppearanceSettingsPanel : public SettingsSubPanel {
public: AppearanceSettingsPanel() {} void drawSkia(SkCanvas* c) override { /* TODO */ } };

class CollaborationSettingsPanel : public SettingsSubPanel {
public: CollaborationSettingsPanel() {} void drawSkia(SkCanvas* c) override { /* TODO */ } };

class AdvancedSettingsPanel : public SettingsSubPanel {
public: AdvancedSettingsPanel() {} void drawSkia(SkCanvas* c) override { /* TODO */ } };

//==============================================================================
// AUDIO SETTINGS PANEL
//==============================================================================
class AudioSettingsPanel : public SettingsSubPanel {
public:
    AudioSettingsPanel(juce::AudioDeviceManager& dm) : deviceManager(dm) {
        latencyLabel = std::make_unique<SkiaLabel>();
        addAndMakeVisible(latencyLabel.get());
    }
    
    void visibilityChanged() override {
        if (isVisible()) {
            if (!deviceSelector) {
                deviceSelector = std::make_unique<juce::AudioDeviceSelectorComponent>(
                    deviceManager,
                    0, 256, 0, 256, true, true, true, false
                );
                addAndMakeVisible(deviceSelector.get());
                resized(); 
            }
            startTimer(1000);
            updateLatencyDisplay();
        } else {
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
            int latencySamples = device->getOutputLatencyInSamples() + device->getInputLatencyInSamples();
            double latencyMs = (latencySamples / sampleRate) * 1000.0;
            
            juce::String text = juce::String::formatted("Latency: %.1f ms (%d smp)", latencyMs, latencySamples);
            latencyLabel->setText(text);
        } else {
            latencyLabel->setText("No Device");
        }
    }

private:
    juce::AudioDeviceManager& deviceManager;
    std::unique_ptr<juce::AudioDeviceSelectorComponent> deviceSelector;
    std::unique_ptr<SkiaLabel> latencyLabel;
};

//==============================================================================
// KEYBOARD SETTINGS PANEL
//==============================================================================
class KeyboardSettingsPanel : public SettingsSubPanel, public SkiaListBox::Model {
public:
    KeyboardSettingsPanel() {
        searchBox = std::make_unique<ZenithTextInput>("Search Shortcuts...");
        searchBox->onTextChanged = [this](const juce::String& text) {
            filterShortcuts(text);
        };
        addAndMakeVisible(searchBox.get());
        
        allShortcuts = {
            {"Play / Pause", "Transport", "Space"},
            {"Record", "Transport", "R"},
            {"Undo", "Edit", "Ctrl+Z"},
            {"Redo", "Edit", "Ctrl+Shift+Z"},
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
    
    void drawSkia(SkCanvas* canvas) override {}
    
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
    
    int getNumRows() override { return (int)filteredShortcuts.size(); }
    
    void paintListBoxItem(int row, SkCanvas& canvas, int width, int height, bool rowIsSelected) override {
        if (row >= filteredShortcuts.size()) return;
        const auto& item = filteredShortcuts[row];
        SkPaint paint;
        paint.setAntiAlias(true);
        if (rowIsSelected) {
            paint.setColor(SkColorSetA(SK_ColorCYAN, 50));
            canvas.drawRect(SkRect::MakeWH(width, height), paint);
        }
        paint.setColor(SK_ColorWHITE);
        SkFont font = design::getSkFont(14);
        canvas.drawString(item.action.toRawUTF8(), 10, height/2 + 5, font, paint);
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
        projectPath = std::make_unique<ZenithTextInput>("Default Project Folder");
        projectPath->setText("~/Documents/Zenith Projects");
        addAndMakeVisible(projectPath.get());
        
        autoSaveToggle = std::make_unique<ZenithToggle>("Enable Auto-Save");
        autoSaveToggle->setToggleState(true);
        addAndMakeVisible(autoSaveToggle.get());
    }
    
    void resized() override {
        using namespace juce;
        FlexBox fb;
        fb.flexDirection = FlexBox::Direction::column;
        fb.alignItems = FlexBox::AlignItems::stretch;
        for (auto* c : getChildren()) fb.items.add(FlexItem(*c).withHeight(40).withMargin({0, 0, 10, 0}));
        fb.performLayout(getLocalBounds().reduced(20));
    }
    void drawSkia(SkCanvas* canvas) override {}
private:
    std::unique_ptr<ZenithTextInput> projectPath;
    std::unique_ptr<ZenithToggle> autoSaveToggle;
};

//==============================================================================
// MIDI SETTINGS PANEL
//==============================================================================
class MidiSettingsPanel : public SettingsSubPanel {
public:
    MidiSettingsPanel() {
        mpeToggle = std::make_unique<ZenithToggle>("Enable MPE");
        addAndMakeVisible(mpeToggle.get());
        zoneLower = std::make_unique<ZenithTextInput>("MPE Zone (Lower)");
        addAndMakeVisible(zoneLower.get());
        zoneUpper = std::make_unique<ZenithTextInput>("MPE Zone (Upper)");
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

//==============================================================================
// GLOBAL SETTINGS PANEL IMPLEMENTATION
//==============================================================================

GlobalSettingsPanel::GlobalSettingsPanel(juce::AudioDeviceManager& deviceManager)
    : deviceManager_(deviceManager) {
  
  createTabButtons();
  
  generalPanel_ = std::make_unique<GeneralSettingsPanel>();
  addChildComponent(generalPanel_.get());
  
  audioPanel_ = std::make_unique<AudioSettingsPanel>(deviceManager_);
  addChildComponent(audioPanel_.get());
  
  midiPanel_ = std::make_unique<MidiSettingsPanel>();
  addChildComponent(midiPanel_.get());
  
  keyboardPanel_ = std::make_unique<KeyboardSettingsPanel>();
  addChildComponent(keyboardPanel_.get());
  
  // Now these are complete types
  pluginPanel_ = std::make_unique<PluginSettingsPanel>(); 
  appearancePanel_ = std::make_unique<AppearanceSettingsPanel>();
  collabPanel_ = std::make_unique<CollaborationSettingsPanel>();
  advancedPanel_ = std::make_unique<AdvancedSettingsPanel>();
  
  closeBtn_ = std::make_unique<ZenithButton>();
  closeBtn_->setText("Close");
  closeBtn_->onClick = [this] { hide(); };
  addAndMakeVisible(closeBtn_.get());
  
  switchCategory(Category::General);
  
  // CRITICAL: Register for self-healing tick
  zenith::animation::AnimationCoordinator::getInstance().registerListener(this, zenith::animation::Priority::Normal);
  
  setSize(900, 650); // Start reasonably large
}

GlobalSettingsPanel::~GlobalSettingsPanel() {
    zenith::animation::AnimationCoordinator::getInstance().unregisterListener(this);
    deviceManager_.removeChangeListener(this);
}

void GlobalSettingsPanel::createTabButtons() {
  for (int i = 0; i < static_cast<int>(Category::COUNT); ++i) {
    auto btn = std::make_unique<ZenithButton>();
    btn->setButtonStyle(ZenithButton::Style::Ghost);
    btn->setToggleable(true);
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
    for(size_t i=0; i<tabButtons_.size(); ++i) tabButtons_[i]->setToggleState(i == (int)category);
    
    if(generalPanel_) generalPanel_->setVisible(false);
    if(audioPanel_) audioPanel_->setVisible(false);
    if(midiPanel_) midiPanel_->setVisible(false);
    if(keyboardPanel_) keyboardPanel_->setVisible(false);
    
    switch(category) {
        case Category::General: generalPanel_->setVisible(true); break;
        case Category::Audio: audioPanel_->setVisible(true); break;
        case Category::MIDI: midiPanel_->setVisible(true); break;
        case Category::Keyboard: keyboardPanel_->setVisible(true); break;
        default: break;
    }
    resized();
}

void GlobalSettingsPanel::resized() {
    auto area = getLocalBounds();
    auto sidebar = area.removeFromLeft(200);
    int btnH = 40;
    for(auto& btn : tabButtons_) btn->setBounds(sidebar.removeFromTop(btnH).reduced(5));
    
    auto content = area.reduced(20);
    closeBtn_->setBounds(content.removeFromBottom(40).removeFromRight(100));
    
    if(generalPanel_) generalPanel_->setBounds(content);
    if(audioPanel_) audioPanel_->setBounds(content);
    if(midiPanel_) midiPanel_->setBounds(content);
    if(keyboardPanel_) keyboardPanel_->setBounds(content);
}

void GlobalSettingsPanel::drawSkia(SkCanvas* canvas) {
    GlassmorphicPanel::Options opts;
    opts.style = GlassmorphicPanel::Style::Elevated;
    opts.useBackdropBlur = true;
    GlassmorphicPanel::drawWithOptions(canvas, SkRect::MakeWH(getWidth(), getHeight()), opts);
    SkPaint p;
    p.setColor(SkColorSetA(SK_ColorWHITE, 30));
    canvas->drawLine(200, 0, 200, getHeight(), p);
}

void GlobalSettingsPanel::changeListenerCallback(juce::ChangeBroadcaster*) {}
void GlobalSettingsPanel::show() { 
    setVisible(true); 
    toFront(true);
    
    // Guard against immediate closing (bounce protection)
    canCloseAfter_ = juce::Time::getMillisecondCounter() + 500;
    
    // Ensure we are sized/centered properly
    int targetW = 900;
    int targetH = 650;
    
    if (auto* parent = getParentComponent()) {
        if (parent->getWidth() > 100) {
            targetW = std::max(900, juce::roundToInt(parent->getWidth() * 0.8));
            targetH = std::max(650, juce::roundToInt(parent->getHeight() * 0.85));
        }
    }
    
    centreWithSize(targetW, targetH);
    
    // Force Position Clamping (X,Y >= 0)
    auto b = getBounds();
    if (b.getX() < 0) b.setX(0);
    if (b.getY() < 0) b.setY(0);
    setBounds(b);
}

void GlobalSettingsPanel::hide() { 
    // Bounce guard
    if (juce::Time::getMillisecondCounter() < canCloseAfter_) {
        return;
    }
    setVisible(false); 
    if(onClose) onClose(); 
}
void GlobalSettingsPanel::mouseDown(const juce::MouseEvent& event) {}

// SELF-HEALING LAYOUT LOGIC
void GlobalSettingsPanel::onAnimationTick(float delta) {
    static int tickCount = 0;
    if (tickCount++ % 60 == 0) {
        auto b = getBounds();
        auto p = getParentComponent();
        ZENITH_LOG_INFO(juce::String::formatted("[GlobalSettingsPanel] Tick Bounds: %d %d %d %d | Parent: %s | Vis: %d", 
             b.getX(), b.getY(), b.getWidth(), b.getHeight(), 
             p ? (juce::String(p->getWidth()) + "x" + juce::String(p->getHeight())).toRawUTF8() : "NULL",
             isVisible() ? 1 : 0));
    }
    
    // SELF HEALING
    if (isVisible() && (getWidth() < 800 || getHeight() < 600)) {
         ZENITH_LOG_INFO("[GlobalSettingsPanel] Self-Healing Triggered: Resizing from tiny state.");
         
         int w = 800; 
         int h = 600;
         if (auto* p = getParentComponent()) {
             if (p->getWidth() > 100) {
                 w = std::max(800, juce::roundToInt(p->getWidth() * 0.8));
                 h = std::max(600, juce::roundToInt(p->getHeight() * 0.85));
             }
         }
         centreWithSize(w, h);
         
         // Clamp Position
         auto b = getBounds();
         if (b.getX() < 0) b.setX(0);
         if (b.getY() < 0) b.setY(0);
         setBounds(b);
    }
}

} // namespace zenith