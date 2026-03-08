#pragma once

#include <juce_core/juce_core.h>

namespace zenith::industrial {

enum class UIMode {
  Beginner,
  Advanced,
  Expert
};

class ModeManager {
public:
  ModeManager();

  UIMode getMode() const { return mode_; }
  void setMode(UIMode mode);

  int targetHeightForMode(UIMode mode) const;
  juce::String modeLabel(UIMode mode) const;

private:
  void loadPreference();
  void savePreference();
  juce::File preferenceFile() const;

  UIMode mode_ = UIMode::Beginner;
};

} // namespace zenith::industrial
