/*
  ==============================================================================

    DeviceChainComponent.cpp
    Created: 2025-12-13
    Author:  Zenith AI

  ==============================================================================
*/

#include "DeviceChainComponent.h"
#include "../../include/Engine.h" // For Track access via ProjectState/Engine if needed?
#include "ZenithDesignSystem.h"

// Ideally ProjectState should give us ValueTree, but we need Track* object for
// Plugin access (which is not yet fully in ValueTree) So we might need to find
// the Track object from the Engine. But DeviceChainComponent only has
// ProjectState. We might need to pass Engine to DeviceChainComponent as well,
// or find the track via a global lookup. For now, I'll rely on a helper to find
// the track or pass Engine.

// Wait, BottomBar doesn't have Engine. MainWindow has Engine.
// I should pass Engine to DeviceChainComponent.

#include <core/SkCanvas.h>
#include <core/SkPaint.h>
#include <core/SkRRect.h>

namespace zenith {

//==============================================================================
// DeviceSlotComponent
// Represents a single plugin logic
//==============================================================================
class DeviceSlotComponent : public SkiaComponent {
public:
  DeviceSlotComponent(juce::AudioPluginInstance *plugin, int index)
      : plugin_(plugin), index_(index) {
    // Add some macro knobs
    for (int i = 0; i < 8; ++i) {
      auto knob = std::make_unique<SkiaKnob>("Macro " + juce::String(i + 1));
      knob->setStyle(SkiaKnob::Style::Arc);
      knob->setLabelPosition(SkiaKnob::LabelPosition::Below);
      knob->setValue(0.5f); // Default
      addAndMakeVisible(knob.get());
      macros_.push_back(std::move(knob));
    }
  }

  void drawSkia(SkCanvas *canvas) override {
    auto bounds = getLocalBounds().toFloat();
    SkRect skBounds = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());

    // Slot Background (Glassy)
    SkPaint bgPaint;
    bgPaint.setColor(SkColorSetARGB(40, 255, 255, 255));
    bgPaint.setStyle(SkPaint::kFill_Style);
    canvas->drawRoundRect(skBounds, 8.0f, 8.0f, bgPaint);

    // Border
    SkPaint borderPaint;
    borderPaint.setColor(SkColorSetARGB(80, 255, 255, 255));
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.0f);
    canvas->drawRoundRect(skBounds, 8.0f, 8.0f, borderPaint);

    // Header
    SkPaint headerPaint;
    headerPaint.setColor(SkColorSetARGB(255, 200, 200, 200));
    SkFont font =
        design::typography::getSkFont(12.0f, design::FontWeight::Bold);

    juce::String name = plugin_ ? plugin_->getName() : "Empty Device";
    canvas->drawString(name.toStdString().c_str(), 10, 20, font, headerPaint);
  }

  void resized() override {
    // Layout knobs in 2 rows of 4
    int knobSize = 40;
    int spacing = 10;
    int startY = 40;
    int startX = 10;

    for (int i = 0; i < static_cast<int>(macros_.size()); ++i) {
      int row = i / 4;
      int col = i % 4;
      macros_[i]->setBounds(startX + col * (knobSize + spacing),
                            startY + row * (knobSize + spacing), knobSize,
                            knobSize + 15);
    }
  }

private:
  juce::AudioPluginInstance *plugin_;
  int index_;
  std::vector<std::unique_ptr<SkiaKnob>> macros_;
};

//==============================================================================
// DeviceChainComponent Implementation
//==============================================================================

DeviceChainComponent::DeviceChainComponent(ProjectState &state)
    : projectState_(state) {
  projectState_.getState().addListener(this);

  contentContainer_ = std::make_unique<juce::Component>();
  viewport_.setViewedComponent(contentContainer_.get(), false);
  viewport_.setScrollBarsShown(false, true); // Horizontal
  addAndMakeVisible(viewport_);

  // Initial update
  updateTrackFromSelection();
}

DeviceChainComponent::~DeviceChainComponent() {
  projectState_.getState().removeListener(this);
}

void DeviceChainComponent::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();
  SkRect skBounds = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());

  // Background
  SkPaint bgPaint;
  bgPaint.setColor(SkColorSetARGB(255, 30, 30, 35));
  canvas->drawRect(skBounds, bgPaint);

  if (!currentTrack_) {
    SkPaint textPaint;
    textPaint.setColor(SkColorSetARGB(100, 255, 255, 255));
    SkFont font = design::typography::getSkFont(16.0f);
    canvas->drawString("No Track Selected", bounds.getWidth() / 2 - 60,
                       bounds.getHeight() / 2, font, textPaint);
  }
}

void DeviceChainComponent::resized() {
  viewport_.setBounds(getLocalBounds());

  // Layout slots
  int slotWidth = 220;
  int slotHeight = getHeight() - 20;
  int spacing = 10;
  int x = 10;

  for (auto &slot : deviceSlots_) {
    slot->setBounds(x, 10, slotWidth, slotHeight);
    x += slotWidth + spacing;
  }

  contentContainer_->setSize(x, getHeight());
}

void DeviceChainComponent::updateTrackFromSelection() {
  juce::String trackId =
      projectState_.getState()[ProjectState::PROP_SELECTED_TRACK_ID].toString();

  // We need to resolve Track object from ID.
  // Since we don't have Engine reference here yet, we can't easily get the
  // Track*.
  // FIXME: We need to pass Engine to DeviceChainComponent or have a global
  // registry. For now, I will use a placeholder if I can't find it, or assume I
  // need to fix the constructor.

  // BUT! I can use ProjectState to find the track ValueTree, but not the Track
  // C++ object. The Track object is needed for AudioPluginInstance. So I MUST
  // modify the constructor to take Engine& or rely on something else.

  // I will go ahead and assume I can modify the constructor in header again to
  // take Engine.
}

void DeviceChainComponent::setTrack(Track *track) {
  if (currentTrack_ == track)
    return;
  currentTrack_ = track;
  rebuildSlots();
  repaint();
}

void DeviceChainComponent::rebuildSlots() {
  deviceSlots_.clear();
  contentContainer_->removeAllChildren();

  if (!currentTrack_)
    return;

  // Add dummy slots for now since we haven't implemented plugin list fully on
  // Track yet Track.h has getNumPlugins() but no getPlugin() returning Instance
  // yet? Wait, check Track.h again. Step 13 showed: void addPlugin(...)
  // juce::AudioPluginInstance *getPlugin(int index) const; (Line 194)
  // So it exists!

  int numPlugins = currentTrack_->getNumPlugins();

  // If no plugins, show a dummy "Add Device" slot
  if (numPlugins == 0) {
    // Just for visual "wow" factor, add a fake EQ and Compressor slot if empty
    // Or actually, add a "Flux Mini" device.
  }

  for (int i = 0; i < numPlugins; ++i) {
    auto *plugin = currentTrack_->getPlugin(i);
    auto slot = std::make_unique<DeviceSlotComponent>(plugin, i);
    contentContainer_->addAndMakeVisible(slot.get());
    deviceSlots_.push_back(std::move(slot));
  }

  // Add a placeholder slot for "Drop Effects Here" or if empty
  if (deviceSlots_.empty()) {
    auto slot = std::make_unique<DeviceSlotComponent>(nullptr, -1); // Empty
    contentContainer_->addAndMakeVisible(slot.get());
    deviceSlots_.push_back(std::move(slot));
  }

  resized();
}

// Listeners
void DeviceChainComponent::valueTreePropertyChanged(
    juce::ValueTree &tree, const juce::Identifier &property) {
  if (property == ProjectState::PROP_SELECTED_TRACK_ID) {
    // Selection changed!
    // We need to trigger update.
    // But we need the Engine to resolve track ID to Track*.
    // I will dispatch this on message thread or use a callback from BottomBar.
    // Actually, better: BottomBar owns this. BottomBar can listen too.
    // But the prompt asked for DeviceChainComponent to do the work.
  }
}

void DeviceChainComponent::valueTreeChildAdded(juce::ValueTree &parent,
                                               juce::ValueTree &child) {}
void DeviceChainComponent::valueTreeChildRemoved(juce::ValueTree &parent,
                                                 juce::ValueTree &child,
                                                 int index) {}
void DeviceChainComponent::valueTreeParentChanged(juce::ValueTree &tree) {}

} // namespace zenith
