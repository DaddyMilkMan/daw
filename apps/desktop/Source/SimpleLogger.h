#pragma once
// #include "debug/DebugLogOverlay.h"
#include "utils/PlatformLogUtils.h"
#include <fstream>
#include <iostream>
#include <juce_core/juce_core.h>
#include <string>

inline void showDebugConsole() { zenith::PlatformLogUtils::showDebugConsole(); }

inline juce::File getDebugLogFile() {
  // Use portable path: Documents/ZenithDAW/debug_log.txt
  return juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
      .getChildFile("ZenithDAW")
      .getChildFile("debug_log.txt");
}

inline void logToFile(const std::string &msg) {
  // File logging - using portable path
  auto logFile = getDebugLogFile();
  logFile.getParentDirectory().createDirectory();

  std::ofstream outfile;
  outfile.open(logFile.getFullPathName().toStdString(), std::ios_base::app);
  outfile << msg << std::endl;

  // Console logging
  std::cerr << msg << std::endl;

  // UI logging
  // zenith::DebugLogOverlay::getInstance().log(msg);

  // VS Output
  DBG(msg);
}
