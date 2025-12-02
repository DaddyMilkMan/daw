/**
 * @file ZenithLogger.h
 * @brief Centralized logging system for Zenith DAW
 * @author Marcus "The Craftsman" Rodriguez - Operation Polish Phase 2
 *
 * Replaces scattered DBG() calls with proper leveled logging system
 */

#pragma once

#include <juce_core/juce_core.h>
#include <memory>
#include <mutex>

namespace zenith {

/**
 * @enum LogLevel
 * @brief Severity levels for log messages
 */
enum class LogLevel {
    Trace,    // Verbose debugging information
    Debug,    // Debug information
    Info,     // General information
    Warning,  // Warning messages
    Error,    // Error conditions
    Critical  // Critical failures
};

/**
 * @class ZenithLogger
 * @brief Thread-safe logging system with file and console output
 */
class ZenithLogger {
public:
    static ZenithLogger& getInstance();
    
    // Configure logging
    void setLogLevel(LogLevel level);
    void setLogToFile(bool enabled);
    void setLogToConsole(bool enabled);
    void setLogFile(const juce::File& file);
    
    // Logging methods
    void trace(const juce::String& message, const juce::String& category = "");
    void debug(const juce::String& message, const juce::String& category = "");
    void info(const juce::String& message, const juce::String& category = "");
    void warning(const juce::String& message, const juce::String& category = "");
    void error(const juce::String& message, const juce::String& category = "");
    void critical(const juce::String& message, const juce::String& category = "");
    
    // Generic log method
    void log(LogLevel level, const juce::String& message, const juce::String& category = "");
    
    // Flush all pending logs
    void flush();
    
private:
    ZenithLogger();
    ~ZenithLogger();
    
    ZenithLogger(const ZenithLogger&) = delete;
    ZenithLogger& operator=(const ZenithLogger&) = delete;
    
    void writeLog(LogLevel level, const juce::String& message, const juce::String& category);
    juce::String levelToString(LogLevel level) const;
    juce::String getCurrentTimestamp() const;
    
    LogLevel currentLogLevel_ = LogLevel::Info;
    bool logToFile_ = true;
    bool logToConsole_ = true;
    std::unique_ptr<juce::FileOutputStream> logFileStream_;
    juce::File logFile_;
    std::mutex logMutex_;
};

// Convenience macros for easy logging
#ifdef JUCE_DEBUG
    #define ZENITH_LOG_TRACE(msg) zenith::ZenithLogger::getInstance().trace(msg, __FILE__)
    #define ZENITH_LOG_DEBUG(msg) zenith::ZenithLogger::getInstance().debug(msg, __FILE__)
#else
    #define ZENITH_LOG_TRACE(msg) // Disabled in Release
    #define ZENITH_LOG_DEBUG(msg) // Disabled in Release
#endif

#define ZENITH_LOG_INFO(msg)     zenith::ZenithLogger::getInstance().info(msg)
#define ZENITH_LOG_WARNING(msg)  zenith::ZenithLogger::getInstance().warning(msg)
#define ZENITH_LOG_ERROR(msg)    zenith::ZenithLogger::getInstance().error(msg)
#define ZENITH_LOG_CRITICAL(msg) zenith::ZenithLogger::getInstance().critical(msg)

// Category-specific logging (for subsystems)
#define ZENITH_LOG_ENGINE(level, msg)     zenith::ZenithLogger::getInstance().log(level, msg, "Engine")
#define ZENITH_LOG_UI(level, msg)         zenith::ZenithLogger::getInstance().log(level, msg, "UI")
#define ZENITH_LOG_PLUGIN(level, msg)     zenith::ZenithLogger::getInstance().log(level, msg, "Plugin")
#define ZENITH_LOG_AUDIO(level, msg)      zenith::ZenithLogger::getInstance().log(level, msg, "Audio")
#define ZENITH_LOG_INSTRUMENT(level, msg) zenith::ZenithLogger::getInstance().log(level, msg, "Instrument")

} // namespace zenith
