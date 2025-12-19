/**
 * @file ZenithLogger.cpp
 * @brief Implementation of centralized logging system
 * @author Marcus "The Craftsman" Rodriguez - Operation Polish Phase 2
 */

#include "ZenithLogger.h"
#include <iomanip>

#if JUCE_WINDOWS
#include <windows.h>
#endif

namespace zenith {

ScopedDebugConsole::ScopedDebugConsole() {
#if JUCE_WINDOWS
    if (AllocConsole()) {
        allocated = true;
        SetConsoleTitleW(L"Zenith Debug Console");
        
        FILE* fp;
        freopen_s(&fp, "CONOUT$", "w", stdout);
        freopen_s(&fp, "CONOUT$", "w", stderr);
        freopen_s(&fp, "CONIN$", "r", stdin);

        std::cout.clear();
        std::cerr.clear();
        std::cin.clear();

        std::ios::sync_with_stdio(true);
        
        std::cout << "========================================" << std::endl;
        std::cout << "Zenith Professional Debug Console" << std::endl;
        std::cout << "RAII Management: Active" << std::endl;
        std::cout << "========================================" << std::endl;
    }
#endif
}

ScopedDebugConsole::~ScopedDebugConsole() {
#if JUCE_WINDOWS
    if (allocated) {
        std::cout << "\nConsole session ending..." << std::endl;
        FreeConsole();
    }
#endif
}

//==============================================================================
ZenithLogger& ZenithLogger::getInstance() {
    static ZenithLogger instance;
    return instance;
}

void ZenithLogger::makeGlobal() {
    juce::Logger::setCurrentLogger(&getInstance());
}

ZenithLogger::ZenithLogger() {
    auto appDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
    auto logFile = appDataDir.getChildFile("Zenith DAW").getChildFile("zenith.log");
    
    fileLogger_ = std::make_unique<juce::FileLogger>(logFile, "Zenith DAW Log Started", 1024 * 1024);
}

ZenithLogger::~ZenithLogger() {
    juce::Logger::setCurrentLogger(nullptr);
}

void ZenithLogger::setLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(logMutex_);
    currentLogLevel_ = level;
}

void ZenithLogger::setLogToFile(bool enabled) {
    std::lock_guard<std::mutex> lock(logMutex_);
    if (!enabled) {
        fileLogger_.reset();
    } else if (!fileLogger_) {
        auto appDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
        auto logFile = appDataDir.getChildFile("Zenith DAW").getChildFile("zenith.log");
        fileLogger_ = std::make_unique<juce::FileLogger>(logFile, "Zenith DAW Log Re-Started", 1024 * 1024);
    }
}

void ZenithLogger::setLogToConsole(bool enabled) {
    std::lock_guard<std::mutex> lock(logMutex_);
    logToConsole_ = enabled;
}

void ZenithLogger::logMessage(const juce::String& message) {
    log(LogLevel::Info, message, "System");
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
    if (static_cast<int>(level) < static_cast<int>(currentLogLevel_)) {
        return;
    }
    
    juce::String logEntry = "[" + juce::Time::getCurrentTime().formatted("%H:%M:%S") + "] ";
    logEntry += "[" + levelToString(level) + "] ";
    
    if (category.isNotEmpty()) {
        logEntry += "[" + category + "] ";
    }
    
    logEntry += message;
    
    // Console output
    if (logToConsole_) {
        if (level >= LogLevel::Error) {
            std::cerr << logEntry.toStdString() << std::endl;
        } else {
            std::cout << logEntry.toStdString() << std::endl;
        }
    }
    
    // File output via juce::FileLogger (High performance, thread-safe buffering)
    if (fileLogger_) {
        fileLogger_->logMessage(logEntry);
    }

    // Always keep DBG output for IDE users
    juce::Logger::outputDebugString(logEntry);
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

void ZenithLogger::flush() {
    // FileLogger handles flushing
}

} // namespace zenith

