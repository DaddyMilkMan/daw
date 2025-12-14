#pragma once
// #include "../Source/ui/debug/DebugLogOverlay.h"
#include <fstream>
#include <iostream>
#include <string>
#include <juce_core/juce_core.h>

#if JUCE_WINDOWS
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

inline void showDebugConsole() {
#if JUCE_WINDOWS
  AllocConsole();
  FILE *fp;
  freopen_s(&fp, "CONOUT$", "w", stdout);
  freopen_s(&fp, "CONOUT$", "w", stderr);
  std::cout << "Debug Console Started" << std::endl;
#endif
}

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
  std::cout << msg << std::endl;

  // UI logging
  // zenith::DebugLogOverlay::getInstance().log(msg);

  // VS Output
  DBG(msg);
}
