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

/*
    ==============================================================================
    Original file header:
*/

#pragma once

#include <juce_core/juce_core.h>
#include <memory>
#include <mutex>
#include <iostream>

namespace zenith {

enum class LogLevel {

    Trace,
    Debug,
    Info,
    Warning,
    Error,
    Critical
};

/**
 * @class ScopedDebugConsole
 * @brief RAII wrapper for Windows console allocation and redirection.
 */
class ScopedDebugConsole {
public:
    ScopedDebugConsole();
    ~ScopedDebugConsole();

    ScopedDebugConsole(const ScopedDebugConsole&) = delete;
    ScopedDebugConsole& operator=(const ScopedDebugConsole&) = delete;

private:
    bool allocated = false;
};

/**
 * @class ZenithLogger
 * @brief Professional thread-safe logging system integrated with JUCE's Logger.
 */
class ZenithLogger : public ::juce::Logger {
public:
    static ZenithLogger& getInstance();
    
    // Set as the current global JUCE logger
    static void makeGlobal();

    // Configure logging
    void setLogLevel(LogLevel level);
    void setLogToFile(bool enabled);
    void setLogToConsole(bool enabled);
    
    // JUCE Logger override
    void logMessage(const ::juce::String& message) override;

    // Zenith-specific logging methods
    void trace(const ::juce::String& message, const ::juce::String& category = "");
    void debug(const ::juce::String& message, const ::juce::String& category = "");
    void info(const ::juce::String& message, const ::juce::String& category = "");
    void warning(const ::juce::String& message, const ::juce::String& category = "");
    void error(const ::juce::String& message, const ::juce::String& category = "");
    void critical(const ::juce::String& message, const ::juce::String& category = "");
    
    void log(LogLevel level, const ::juce::String& message, const ::juce::String& category = "");
    
    void flush();
    
private:
    ZenithLogger();
    ~ZenithLogger() override;
    
    ::juce::String levelToString(LogLevel level) const;
    
    LogLevel currentLogLevel_ = LogLevel::Info;
    bool logToConsole_ = true;
    std::unique_ptr<::juce::FileLogger> fileLogger_;
    std::mutex logMutex_;
};

} // namespace zenith

// Convenience macros for easy logging
#ifdef JUCE_DEBUG
    #define ZENITH_LOG_TRACE(msg) ::zenith::ZenithLogger::getInstance().trace(msg, __FILE__)
    #define ZENITH_LOG_DEBUG(msg) ::zenith::ZenithLogger::getInstance().debug(msg, __FILE__)
#else
    #define ZENITH_LOG_TRACE(msg)
    #define ZENITH_LOG_DEBUG(msg)
#endif

#define ZENITH_LOG_INFO(msg)     ::zenith::ZenithLogger::getInstance().info(msg)
#define ZENITH_LOG_WARNING(msg)  ::zenith::ZenithLogger::getInstance().warning(msg)
#define ZENITH_LOG_ERROR(msg)    ::zenith::ZenithLogger::getInstance().error(msg)
#define ZENITH_LOG_WARN(msg)     ZENITH_LOG_WARNING(msg)
#define ZENITH_LOG_CRITICAL(msg) ::zenith::ZenithLogger::getInstance().critical(msg)

#define ZENITH_LOG_UI(level, msg)         ::zenith::ZenithLogger::getInstance().log(level, msg, "UI")
#define ZENITH_LOG_AUDIO(level, msg)      ::zenith::ZenithLogger::getInstance().log(level, msg, "Audio")
