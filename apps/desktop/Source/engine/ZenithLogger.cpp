/**
 * @file ZenithLogger.cpp
 * @brief Implementation of centralized logging system
 * @author Marcus "The Craftsman" Rodriguez - Operation Polish Phase 2
 */

#include "ZenithLogger.h"
#include <iomanip>

namespace zenith {

ZenithLogger& ZenithLogger::getInstance() {
    static ZenithLogger instance;
    return instance;
}

ZenithLogger::ZenithLogger() {
    // Default log file location
    auto appDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
    logFile_ = appDataDir.getChildFile("Zenith DAW").getChildFile("zenith.log");
    
    // Ensure directory exists
    logFile_.getParentDirectory().createDirectory();
    
    // Open log file in append mode
    logFileStream_ = std::make_unique<juce::FileOutputStream>(logFile_);
    
    if (logFileStream_->openedOk()) {
        log(LogLevel::Info, "ZenithLogger initialized", "Logger");
    }
}

ZenithLogger::~ZenithLogger() {
    flush();
    logFileStream_.reset();
}

void ZenithLogger::setLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(logMutex_);
    currentLogLevel_ = level;
}

void ZenithLogger::setLogToFile(bool enabled) {
    std::lock_guard<std::mutex> lock(logMutex_);
    logToFile_ = enabled;
}

void ZenithLogger::setLogToConsole(bool enabled) {
    std::lock_guard<std::mutex> lock(logMutex_);
    logToConsole_ = enabled;
}

void ZenithLogger::setLogFile(const juce::File& file) {
    std::lock_guard<std::mutex> lock(logMutex_);
    logFile_ = file;
    logFile_.getParentDirectory().createDirectory();
    logFileStream_ = std::make_unique<juce::FileOutputStream>(logFile_);
}

void ZenithLogger::trace(const juce::String& message, const juce::String& category) {
    log(LogLevel::Trace, message, category);
}

void ZenithLogger::debug(const juce::String& message, const juce::String& category) {
    log(LogLevel::Debug, message, category);
}

void ZenithLogger::info(const juce::String& message, const juce::String& category) {
    log(LogLevel::Info, message, category);
}

void ZenithLogger::warning(const juce::String& message, const juce::String& category) {
    log(LogLevel::Warning, message, category);
}

void ZenithLogger::error(const juce::String& message, const juce::String& category) {
    log(LogLevel::Error, message, category);
}

void ZenithLogger::critical(const juce::String& message, const juce::String& category) {
    log(LogLevel::Critical, message, category);
}

void ZenithLogger::log(LogLevel level, const juce::String& message, const juce::String& category) {
    // Check if this log level should be output
    if (static_cast<int>(level) < static_cast<int>(currentLogLevel_)) {
        return;
    }
    
    writeLog(level, message, category);
}

void ZenithLogger::writeLog(LogLevel level, const juce::String& message, const juce::String& category) {
    std::lock_guard<std::mutex> lock(logMutex_);
    
    // Format: [TIMESTAMP] [LEVEL] [CATEGORY] Message
    juce::String logEntry = "[" + getCurrentTimestamp() + "] ";
    logEntry += "[" + levelToString(level) + "] ";
    
    if (category.isNotEmpty()) {
        logEntry += "[" + category + "] ";
    }
    
    logEntry += message;
    
    // Output to console (if enabled)
    if (logToConsole_) {
        // Use different output streams based on severity
        if (level >= LogLevel::Error) {
            std::cerr << logEntry.toStdString() << std::endl;
        } else {
            std::cout << logEntry.toStdString() << std::endl;
        }
    }
    
    // Output to file (if enabled and stream is open)
    if (logToFile_ && logFileStream_ && logFileStream_->openedOk()) {
        logFileStream_->writeText(logEntry + "\n", false, false, nullptr);
    }
}

juce::String ZenithLogger::levelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::Trace:    return "TRACE  ";
        case LogLevel::Debug:    return "DEBUG  ";
        case LogLevel::Info:     return "INFO   ";
        case LogLevel::Warning:  return "WARNING";
        case LogLevel::Error:    return "ERROR  ";
        case LogLevel::Critical: return "FATAL  ";
        default:                 return "UNKNOWN";
    }
}

juce::String ZenithLogger::getCurrentTimestamp() const {
    auto now = juce::Time::getCurrentTime();
    return now.formatted("%Y-%m-%d %H:%M:%S");
}

void ZenithLogger::flush() {
    std::lock_guard<std::mutex> lock(logMutex_);
    if (logFileStream_ && logFileStream_->openedOk()) {
        logFileStream_->flush();
    }
}

} // namespace zenith
