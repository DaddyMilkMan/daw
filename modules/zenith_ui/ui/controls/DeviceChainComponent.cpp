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

    DeviceChainComponent.cpp
    Created: 2025-12-13
    Author:  Zenith AI

  ==============================================================================
*/


#include "DeviceChainComponent.h"
#include "Engine.h"
#include "ZenithDesignSystem.h"

#include <core/SkCanvas.h>
#include <core/SkFontTypes.h>
#include <core/SkColor.h>
#include <core/SkPaint.h>
#include <core/SkRRect.h>
#include <core/SkFont.h>

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
      knob->setStyle(ZenithKnob::Style::Standard);
      knob->setLabelPosition(15.0f);
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

    // Use SkFont explicitly constructed since design system might be complex
    SkFont font;
    font.setSize(12.0f);
    font.setSubpixel(true);

    juce::String name = plugin_ ? plugin_->getName() : "Empty Device";
    canvas->drawString(name.toStdString().c_str(), 10.0f, 20.0f, font, headerPaint);
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

DeviceChainComponent::DeviceChainComponent(Engine &engine, ProjectState &state)
    : engine_(engine), projectState_(state),
      viewport_("DeviceChainViewport"), // Initialize viewport_ in the
                                        // initializer list
      contentContainer_(
          std::make_unique<juce::Component>()) // Initialize unique_ptr member
{
  // Listen to state changes
  projectState_.addListener(this);

  viewport_.setViewedComponent(contentContainer_.get(), false);
  viewport_.setScrollBarsShown(false, true); // Horizontal
  addAndMakeVisible(viewport_);

  // Initial update
  updateTrackFromSelection();
}

DeviceChainComponent::~DeviceChainComponent() {
  projectState_.removeListener(this);
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

    SkFont font;
    font.setSize(16.0f);
    font.setSubpixel(true);

    juce::String msg = "No Track Selected";
    SkRect textBounds;
    font.measureText(msg.toRawUTF8(), msg.length(), SkTextEncoding::kUTF8, &textBounds);
    
    canvas->drawString(msg.toRawUTF8(), 
                       bounds.getWidth() / 2.0f - textBounds.width() / 2.0f,
                       bounds.getHeight() / 2.0f, 
                       font, textPaint);
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

  Track *foundTrack = nullptr;
  for (const auto &t : engine_.tracks()) {
    if (t->getTrackId() == trackId) {
      foundTrack = t.get();
      break;
    }
  }

  setTrack(foundTrack);
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

  int numPlugins = currentTrack_->getNumPlugins();

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
    updateTrackFromSelection();
  }
}

void DeviceChainComponent::valueTreeChildAdded(juce::ValueTree &parent,
                                               juce::ValueTree &child) {}
void DeviceChainComponent::valueTreeChildRemoved(juce::ValueTree &parent,
                                                 juce::ValueTree &child,
                                                 int index) {}
void DeviceChainComponent::valueTreeParentChanged(juce::ValueTree &tree) {}

} // namespace zenith
