#include "ModeManager.h"

namespace zenith::industrial {

ModeManager::ModeManager() { loadPreference(); }

void ModeManager::setMode(UIMode mode) {
  if (mode_ == mode) {
    return;
  }
  mode_ = mode;
  savePreference();
}

int ModeManager::targetHeightForMode(UIMode mode) const {
  switch (mode) {
    case UIMode::Beginner:
      return 500;
    case UIMode::Advanced:
      return 1250;
    case UIMode::Expert:
      return 1700;
  }
  return 1250;
}

juce::String ModeManager::modeLabel(UIMode mode) const {
  switch (mode) {
    case UIMode::Beginner:
      return "BEGINNER";
    case UIMode::Advanced:
      return "ADVANCED";
    case UIMode::Expert:
      return "EXPERT";
  }
  return "ADVANCED";
}

juce::File ModeManager::preferenceFile() const {
  return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
      .getChildFile("zenith_daw")
      .getChildFile("industrial_ui_mode.txt");
}

void ModeManager::loadPreference() {
  const auto file = preferenceFile();
  if (!file.existsAsFile()) {
    mode_ = UIMode::Beginner;
    return;
  }

  const auto text = file.loadFileAsString().trim().toUpperCase();
  if (text == "BEGINNER") {
    mode_ = UIMode::Beginner;
  } else if (text == "ADVANCED") {
    mode_ = UIMode::Advanced;
  } else if (text == "EXPERT") {
    mode_ = UIMode::Expert;
  }
}

void ModeManager::savePreference() {
  const auto file = preferenceFile();
  file.getParentDirectory().createDirectory();
  file.replaceWithText(modeLabel(mode_));
}

} // namespace zenith::industrial
