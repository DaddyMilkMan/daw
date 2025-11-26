#pragma once
#include "../Source/ui/debug/DebugLogOverlay.h"
#include <fstream>
#include <iostream>
#include <string>


inline void logToFile(const std::string &msg) {
  // File logging
  std::ofstream outfile;
  outfile.open("C:\\zenith\\daw\\debug_log.txt", std::ios_base::app);
  outfile << msg << std::endl;

  // UI logging
  // Use try-catch or check if instance is safe?
  // It's a static local, so it's safe.
  zenith::DebugLogOverlay::getInstance().log(msg);

  // Console logging
  DBG(msg);
}
