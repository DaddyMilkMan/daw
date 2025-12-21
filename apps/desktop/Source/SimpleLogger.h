#pragma once
// #include "debug/DebugLogOverlay.h"
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
  SetConsoleTitle(L"Zenith Debug Console");
  
  // redirect unbuffered STDOUT to the console
  FILE* fp;
  freopen_s(&fp, "CONOUT$", "w", stdout);
  freopen_s(&fp, "CONOUT$", "w", stderr);
  freopen_s(&fp, "CONIN$", "r", stdin);

  // Clear streams
  std::cout.clear();
  std::cerr.clear();
  std::cin.clear();

  // Sync
  std::ios::sync_with_stdio(true);

  std::cout << "========================================" << std::endl;
  std::cout << "Zenith Debug Console Started" << std::endl;
  std::cout << "========================================" << std::endl;
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
