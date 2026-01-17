/*
  ==============================================================================
    MixerChannelInput.cpp
    MixerChannelComponent input handling implementation
  ==============================================================================
*/

#include "MixerChannelComponent.h"
#include "MixerComponent.h"
#include "../../engine/ProjectState.h"
#include "../../engine/Engine.h"
#include "../controls/ContextMenuManager.h"
#include "../../engine/Track.h"
#include "../../engine/AuxBus.h"
#include "PluginBrowser.h"
#include "../framework/ConfigurationManager.h"

namespace zenith {

void MixerChannelComponent::mouseDown(const juce::MouseEvent &e) {
  if (e.mods.isRightButtonDown()) {
    auto menu = ContextMenuManager::createMenu();
    
    menu->addSectionHeader("Channel");
    
    menu->addItem(1, "Rename...", true, false, [this]() {
      nameLabel_.showEditor();
    });
    
    menu->addItem(2, "Duplicate Channel", !isMaster_, false, [this]() {
      if (track_) {
        projectState_.duplicateTrack(track_->getTrackId(), "Duplicate Track");
      }
    });
    
    menu->addSeparator();
    
    auto colorMenu = ContextMenuManager::createMenu();
    colorMenu->addItem(100, "Red", true, false, [this]() { track_->setColor(juce::Colour(0xFFFF4444)); });
    colorMenu->addItem(101, "Orange", true, false, [this]() { track_->setColor(juce::Colour(0xFFFF8844)); });
    colorMenu->addItem(102, "Yellow", true, false, [this]() { track_->setColor(juce::Colour(0xFFFFDD44)); });
    colorMenu->addItem(103, "Green", true, false, [this]() { track_->setColor(juce::Colour(0xFF44FF88)); });
    colorMenu->addItem(104, "Cyan", true, false, [this]() { track_->setColor(juce::Colour(0xFF44DDFF)); });
    colorMenu->addItem(105, "Blue", true, false, [this]() { track_->setColor(juce::Colour(0xFF4488FF)); });
    menu->addSubMenu("Change Color", std::move(colorMenu));
    
    menu->addSeparator();
    
    // --- ADAPTIVE CONTEXT MENU LOGIC ---
    using namespace zenith::config;
    auto mode = ThemeConfiguration::getInstance().getContextMenuMode();
    bool isShift = e.mods.isShiftDown();
    bool showPowerFeatures = (mode == ContextMenuMode::Default) || isShift;

    if (showPowerFeatures) {
        menu->addSectionHeader("Routing");
        
        auto routeMenu = ContextMenuManager::createMenu();
        routeMenu->addItem(200, "Master", true, track_->getOutputId() == "master", [this]() {
          track_->setOutputId("master");
        });

        int numAux = engine_.getNumAuxBuses();
        for (int auxIdx = 0; auxIdx < numAux; ++auxIdx) {
          if (auto* bus = engine_.getAuxBus(auxIdx)) {
            juce::String busId = bus->getName();
            bool isCurrent = (track_->getOutputId() == busId);
            routeMenu->addItem(201 + auxIdx, bus->getName(), true, isCurrent, [this, busId]() {
              track_->setOutputId(busId);
            });
          }
        }
        menu->addSubMenu("Route To", std::move(routeMenu));
        menu->addSeparator();
    }
    
    menu->addItem(3, "Reset Channel", true, false, [this]() {
      faderSlider_.setValue(1.0f);
      panKnob_.setValue(0.0f);
      muteButton_.setToggleState(false);
      soloButton_.setToggleState(false);
      onFaderChanged();
      onPanChanged();
      onMuteClicked();
      onSoloClicked();
    });
    
    menu->addSeparator();
    menu->addItemComplete(99, "Delete Channel", SkPath(), "Del", !isMaster_, false, true, [this]() {
      if (track_) {
        projectState_.removeTrack(track_->getTrackId(), "Delete Track");
      }
    });

    if (!showPowerFeatures) {
         menu->addItem(999, "Hold Shift for Routing...", false, false, nullptr);
    }
    
    ContextMenuManager::getInstance().showMenuAt(std::move(menu), this, e.x, e.y);
    return;
  }

  if (onClick) {
    onClick();
  }
  SkiaComponent::mouseDown(e);
}

void MixerChannelComponent::LevelMeter::mouseDown(const juce::MouseEvent& e) {
  if (e.mods.isRightButtonDown()) {
    auto menu = ContextMenuManager::createMenu();
    menu->addItem(1, "Reset Peak", true, false, [this]() {
        peakLevel_ = 0;
        peakLevelL_ = 0;
        peakLevelR_ = 0;
        repaint();
    });
    ContextMenuManager::getInstance().showMenuAt(std::move(menu), this, e.x, e.y);
  }
}

void MixerChannelComponent::InsertSlotIndicator::mouseDown(const juce::MouseEvent& e) {
  if (e.mods.isRightButtonDown()) {
    auto menu = ContextMenuManager::createMenu();
    
    if (isOccupied_) {
      menu->addSectionHeader(pluginName_);
      menu->addItem(1, "Bypass", true, false, [this]() {
          if (auto* track = owner_.getTrack()) {
              if (auto* plugin = track->getPlugin(slotIndex_)) {
                  plugin->suspendProcessing(!plugin->isSuspended());
                  owner_.repaint();
              }
          }
      });
      menu->addItem(2, "Show Editor", true, false, [this]() {
          if (auto* track = owner_.getTrack()) {
              if (auto* plugin = track->getPlugin(slotIndex_)) {
                  owner_.getEngine().getPluginEditorWindowManager().openEditor(plugin, "", slotIndex_);
              }
          }
      });
      menu->addSeparator();
      menu->addItem(3, "Replace Plugin...", true, false, [this]() {
          auto* engine = &owner_.getEngine();
          auto descArr = engine->getPluginHost().getPluginDescriptions();
          if (descArr.size() > 0) {
              if (auto* track = owner_.getTrack()) {
                  if (auto plugin = engine->getPluginHost().createPlugin(descArr[0])) {
                      track->removePlugin(slotIndex_);
                      track->addPlugin(std::move(plugin));
                  }
              }
          }
      });
      menu->addItemComplete(4, "Remove Plugin", SkPath(), "", true, false, true, [this]() {
          if (auto* track = owner_.getTrack()) {
              track->removePlugin(slotIndex_);
          }
      });
    } else {
      menu->addItem(1, "Add Plugin...", true, false, [this]() {
          auto& engine = owner_.getEngine();
          auto* browser = new PluginBrowser(engine.getPluginHost(), [this](const juce::PluginDescription& desc) {
              if (auto* track = owner_.getTrack()) {
                  juce::String error;
                  if (auto plugin = owner_.getEngine().getPluginHost().createInstance(desc, 44100, 512, error)) {
                       track->addPlugin(std::move(plugin));
                  }
              }
          });
          browser->setSize(350, 450);
          juce::CallOutBox::launchAsynchronously(std::unique_ptr<juce::Component>(browser), getScreenBounds(), nullptr);
      });
    }
    ContextMenuManager::getInstance().showMenuAt(std::move(menu), this, e.x, e.y);
  }
}

void MixerChannelComponent::SendIndicator::mouseDown(const juce::MouseEvent& e) {
  if (e.mods.isRightButtonDown()) {
    auto menu = ContextMenuManager::createMenu();
    menu->addSectionHeader("Send " + juce::String(sendIndex_ + 1));
    
    bool isPre = false;
    if (auto* track = owner_.getTrack()) {
        isPre = track->isSendPreFader(sendIndex_);
    }

    menu->addItem(1, "Pre-Fader", true, isPre, [this]() {
        if (auto* track = owner_.getTrack()) {
            track->setSendPreFader(sendIndex_, true);
        }
    });
    menu->addItem(2, "Post-Fader", true, !isPre, [this]() {
        if (auto* track = owner_.getTrack()) {
            track->setSendPreFader(sendIndex_, false);
        }
    });
    
    menu->addSeparator();
    auto destMenu = ContextMenuManager::createMenu();
    destMenu->addItem(100, "None", true, destinationName_.isEmpty(), [this]() {
        if (auto* track = owner_.getTrack()) {
            track->setSendDestination(sendIndex_, -1);
        }
        setDestination("");
        setSendLevel(0.0f);
    });
    
    int numAux = owner_.getEngine().getNumAuxBuses();
    if (numAux > 0) {
        for (int i = 0; i < numAux; ++i) {
            if (auto* bus = owner_.getEngine().getAuxBus(i)) {
                 bool isCurrent = destinationName_ == bus->getName();
                 auto* busPtr = bus;
                 destMenu->addItem(200 + i, bus->getName(), true, isCurrent, [=, this]() {
                     if (auto* track = owner_.getTrack()) {
                         track->setSendDestination(sendIndex_, i); 
                     }
                     setDestination(busPtr->getName());
                 });
            }
        }
    } else {
        destMenu->addItem(999, "No Aux Buses", false, false, nullptr);
    }
    menu->addSubMenu("Set Destination", std::move(destMenu));
    
    menu->addSeparator();
    menu->addItemComplete(3, "Remove Send", SkPath(), "", true, false, true, [this]() {
         if (auto* track = owner_.getTrack()) {
            track->setSendDestination(sendIndex_, -1);
            track->setSendLevel(sendIndex_, 0.0f);
        }
        setDestination("");
        setSendLevel(0.0f);
    });
    
    ContextMenuManager::getInstance().showMenuAt(std::move(menu), this, e.x, e.y);
  }
}

void MixerChannelComponent::focusGained(juce::Component::FocusChangeType cause) {
  juce::ignoreUnused(cause);
  hasFocus_ = true;
  repaint();
}

void MixerChannelComponent::focusLost(juce::Component::FocusChangeType cause) {
  juce::ignoreUnused(cause);
  hasFocus_ = false;
  repaint();
}

bool MixerChannelComponent::keyPressed(const juce::KeyPress& key, juce::Component* origin) {
  juce::ignoreUnused(origin);
  
  if (key.getTextCharacter() == 'm' || key.getTextCharacter() == 'M') {
    muteButton_.onClick();
    return true;
  }
  if (key.getTextCharacter() == 's' || key.getTextCharacter() == 'S') {
    soloButton_.onClick();
    return true;
  }
  if (key.getTextCharacter() == 'r' || key.getTextCharacter() == 'R') {
    armButton_.onClick();
    return true;
  }
  
  if (auto* mixer = findParentComponentOfClass<MixerComponent>()) {
    return mixer->keyPressed(key, origin);
  }
  
  return false;
}

} // namespace zenith
