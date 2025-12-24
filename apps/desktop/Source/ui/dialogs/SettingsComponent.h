#pragma once

#include "../engine/PluginHost.h"
#include "Engine.h"
#include "SkiaButton.h"
#include "SkiaComponent.h"
#include "SkiaSlider.h"
#include <juce_core/juce_core.h>
#include <memory>

namespace zenith {

// Forward declare internal tab classes
class AudioSettingsTab;
class DisplaySettingsTab;
class PluginSettingsTab;

class SettingsComponent : public SkiaComponent {
public:
  SettingsComponent(Engine &engine);
  ~SettingsComponent() override;

  void resized() override;
  void drawSkia(SkCanvas *canvas) override;

private:
  void createNavButton(const juce::String &name, int index);
  void setActiveTab(int index);

  Engine &engine_;
  juce::OwnedArray<SkiaButton> navButtons_;

  std::unique_ptr<AudioSettingsTab> audioTab_;
  std::unique_ptr<DisplaySettingsTab> displayTab_;
  std::unique_ptr<PluginSettingsTab> pluginTab_;

  SkiaComponent *currentTab_ = nullptr;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsComponent)
};

} // namespace zenith