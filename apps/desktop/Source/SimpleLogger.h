#pragma once
// #include "../Source/ui/debug/DebugLogOverlay.h"
#include <fstream>
#include <iostream>
#include <string>

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

inline void logToFile(const std::string &msg) {
  // File logging
  std::ofstream outfile;
  outfile.open("C:\\zenith\\daw\\debug_log.txt", std::ios_base::app);
  outfile << msg << std::endl;

  // Console logging
  std::cout << msg << std::endl;

  // UI logging
  // zenith::DebugLogOverlay::getInstance().log(msg);

  // VS Output
  DBG(msg);
}
