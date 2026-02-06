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

//==============================================================================
#include "ZenithLogger.h"
#include "../utils/PlatformLogUtils.h"
#include <iostream>




namespace zenith {

ScopedDebugConsole::ScopedDebugConsole() {
  PlatformLogUtils::showDebugConsole();
  allocated = true;
}

ScopedDebugConsole::~ScopedDebugConsole() {
  if (allocated) {
    std::cerr << "\nConsole session ending..." << std::endl;
    PlatformLogUtils::freeDebugConsole();
  }
}

//==============================================================================
ZenithLogger &ZenithLogger::getInstance() {
  static ZenithLogger instance;
  return instance;
}

void ZenithLogger::makeGlobal() {
  juce::Logger::setCurrentLogger(&getInstance());
}

ZenithLogger::ZenithLogger() {
  auto appDataDir =
      juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
  auto logFile =
      appDataDir.getChildFile("Zenith DAW").getChildFile("zenith.log");

  fileLogger_ = std::make_unique<juce::FileLogger>(
      logFile, "Zenith DAW Log Started", 1024 * 1024);
}

ZenithLogger::~ZenithLogger() { juce::Logger::setCurrentLogger(nullptr); }

void ZenithLogger::setLogLevel(LogLevel level) {
  std::lock_guard<std::mutex> lock(logMutex_);
  currentLogLevel_ = level;
}

void ZenithLogger::setLogToFile(bool enabled) {
  std::lock_guard<std::mutex> lock(logMutex_);
  if (!enabled) {
    fileLogger_.reset();
  } else if (!fileLogger_) {
    auto appDataDir = juce::File::getSpecialLocation(
        juce::File::userApplicationDataDirectory);
    auto logFile =
        appDataDir.getChildFile("Zenith DAW").getChildFile("zenith.log");
    fileLogger_ = std::make_unique<juce::FileLogger>(
        logFile, "Zenith DAW Log Re-Started", 1024 * 1024);
  }
}

void ZenithLogger::setLogToConsole(bool enabled) {
  std::lock_guard<std::mutex> lock(logMutex_);
  logToConsole_ = enabled;
}

void ZenithLogger::logMessage(const juce::String &message) {
  log(LogLevel::Info, message, "System");
}

void ZenithLogger::trace(const juce::String &message,
                         const juce::String &category) {
  log(LogLevel::Trace, message, category);
}

void ZenithLogger::debug(const juce::String &message,
                         const juce::String &category) {
  log(LogLevel::Debug, message, category);
}

void ZenithLogger::info(const juce::String &message,
                        const juce::String &category) {
  log(LogLevel::Info, message, category);
}

void ZenithLogger::warning(const juce::String &message,
                           const juce::String &category) {
  log(LogLevel::Warning, message, category);
}

void ZenithLogger::error(const juce::String &message,
                         const juce::String &category) {
  log(LogLevel::Error, message, category);
}

void ZenithLogger::critical(const juce::String &message,
                            const juce::String &category) {
  log(LogLevel::Critical, message, category);
}

void ZenithLogger::log(LogLevel level, const juce::String &message,
                       const juce::String &category) {
  if (static_cast<int>(level) < static_cast<int>(currentLogLevel_)) {
    return;
  }

  juce::String logEntry =
      "[" + juce::Time::getCurrentTime().formatted("%H:%M:%S") + "] ";
  logEntry += "[" + levelToString(level) + "] ";

  if (category.isNotEmpty()) {
    logEntry += "[" + category + "] ";
  }

  logEntry += message;

  // Console output - ALWAYS use stderr to keep stdout clean for MCP/IPC
  if (logToConsole_) {
    #if !JUCE_LINUX
    std::cerr << logEntry.toStdString() << std::endl;
    #endif
  }

  // File output via juce::FileLogger (High performance, thread-safe buffering)
  if (fileLogger_) {
    fileLogger_->logMessage(logEntry);
  }

  // Always keep DBG output for IDE users
  // On Linux, this also writes to stderr, serving as the console output
  juce::Logger::outputDebugString(logEntry);
}

juce::String ZenithLogger::levelToString(LogLevel level) const {
  switch (level) {
  case LogLevel::Trace:
    return "TRACE  ";
  case LogLevel::Debug:
    return "DEBUG  ";
  case LogLevel::Info:
    return "INFO   ";
  case LogLevel::Warning:
    return "WARNING";
  case LogLevel::Error:
    return "ERROR  ";
  case LogLevel::Critical:
    return "FATAL  ";
  default:
    return "UNKNOWN";
  }
}

void ZenithLogger::flush() {
  // FileLogger handles flushing
}

} // namespace zenith
