/*
  UIErrorHandler.cpp

  Implementation of the centralized UI error handling system
*/

#include "UIErrorHandler.h"
#include <zenith_core/utils/PlatformLogUtils.h>
#include <zenith_core/engine/ZenithLogger.h>
#include "../design-system/ZenithDesignSystem.h"
#include "../controls/ToastNotificationManager.h"

namespace zenith::UI {

UIErrorHandler& UIErrorHandler::getInstance() {
    static UIErrorHandler instance;
    return instance;
}

UIErrorHandler::UIErrorHandler()
    : needsUpdate_(false)
    , maxHistorySize_(100) {
    // Set component properties for debugging
    setName("UIErrorHandler");
    setInterceptsMouseClicks(false, false);
}

UIErrorHandler::~UIErrorHandler() {
    clearAllErrors();
}

void UIErrorHandler::reportError(const UIError& error) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Create error record
    ErrorRecord record;
    record.error = error;
    record.displayed = false;
    record.id = juce::UniqueId();

    // Add to history
    errorHistory_.push_back(record);

    // Log to system
    logErrorToSystem(error);

    // Handle fatal errors specially
    if (error.severity == ErrorSeverity::Fatal) {
        handleFatalError(error);
    }

    // Notify callbacks
    notifyErrorReported(error);

    // Deduplicate if needed
    if (errorHistory_.size() > maxHistorySize_) {
        deduplicateErrors();
    }

    // Mark for update and show toast
    needsUpdate_.store(true);
    repaint();

    // Show toast notification
    ToastNotificationManager::getInstance().showToast(error.message,
        [this, error]() {
            auto toastManager = ToastNotificationManager::getInstance();
            auto* latest = getLatestError();
            if (latest && latest->errorCode == error.errorCode) {
                std::lock_guard<std::mutex> lock(mutex_);
                for (auto& record : errorHistory_) {
                    if (record.error.errorCode == error.errorCode) {
                        record.displayed = true;
                        break;
                    }
                }
            }
        },
        convertSeverityToToastType(error.severity),
        error.dismissible ? 5000 : 0); // Auto-dismiss after 5s if dismissible
}

void UIErrorHandler::reportError(ErrorSeverity severity, ErrorCategory category,
                               const std::string& message, const std::string& details,
                               const juce::String& component) {
    UIError error(severity, category, message, details, component);
    reportError(error);
}

void UIErrorHandler::reportQuickError(const std::string& message, ErrorSeverity severity) {
    UIError error(severity, ErrorCategory::UI, message, "", "Unknown");
    reportError(error);
}

void UIErrorHandler::clearErrors(ErrorCategory category) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto beforeCount = errorHistory_.size();

    // Remove errors of specified category
    errorHistory_.erase(
        std::remove_if(errorHistory_.begin(), errorHistory_.end(),
            [category](const ErrorRecord& record) {
                return record.error.category == category;
            }),
        errorHistory_.end());

    if (errorHistory_.size() != beforeCount) {
        notifyErrorCleared();
        needsUpdate_.store(true);
        repaint();
    }
}

void UIErrorHandler::clearAllErrors() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!errorHistory_.empty()) {
        errorHistory_.clear();
        notifyErrorCleared();
        needsUpdate_.store(true);
        repaint();
    }
}

void UIErrorHandler::setMaxHistorySize(size_t maxSize) {
    std::lock_guard<std::mutex> lock(mutex_);
    maxHistorySize_ = maxSize;

    if (errorHistory_.size() > maxSize) {
        deduplicateErrors();
    }
}

size_t UIErrorHandler::getErrorCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return errorHistory_.size();
}

bool UIErrorHandler::hasErrors() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return !errorHistory_.empty();
}

void UIErrorHandler::onErrorReported(std::function<void(const UIError&)> callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    reportCallbacks_.push_back(callback);
}

void UIErrorHandler::onErrorCleared(std::function<void()> callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    clearCallbacks_.push_back(callback);
}

const UIError* UIErrorHandler::getLatestError() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (errorHistory_.empty()) {
        return nullptr;
    }
    return &errorHistory_.back().error;
}

std::vector<UIError> UIErrorHandler::getErrorsByCategory(ErrorCategory category) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<UIError> result;
    for (const auto& record : errorHistory_) {
        if (record.error.category == category) {
            result.push_back(record.error);
        }
    }
    return result;
}

std::vector<UIError> UIErrorHandler::getErrorsBySeverity(ErrorSeverity severity) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<UIError> result;
    for (const auto& record : errorHistory_) {
        if (record.error.severity == severity) {
            result.push_back(record.error);
        }
    }
    return result;
}

bool UIErrorHandler::hasFatalErrors() const {
    std::lock_guard<std::mutex> lock(mutex_);

    for (const auto& record : errorHistory_) {
        if (record.error.severity == ErrorSeverity::Fatal) {
            return true;
        }
    }
    return false;
}

void UIErrorHandler::addErrorHandler(int errorCode, std::function<void(const UIError&)> handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    customHandlers_[errorCode] = handler;
}

UIErrorHandler::ErrorStats UIErrorHandler::getStatistics() const {
    std::lock_guard<std::mutex> lock(mutex_);

    ErrorStats stats;

    for (const auto& record : errorHistory_) {
        stats.total++;

        switch (record.error.severity) {
            case ErrorSeverity::Fatal:
                stats.fatal++;
                break;
            case ErrorSeverity::Critical:
                stats.critical++;
                break;
            case ErrorSeverity::Error:
                stats.errors++;
                break;
            case ErrorSeverity::Warning:
                stats.warnings++;
                break;
            case ErrorSeverity::Info:
                stats.info++;
                break;
            case ErrorSeverity::Debug:
                // Debug stats not counted in regular stats
                break;
        }
    }

    return stats;
}

void UIErrorHandler::paint(juce::Graphics& g) {
    // Background overlay for debugging purposes (hidden by default)
    if (errorHistory_.empty()) {
        return;
    }

    // Draw subtle background for error indicator
    auto bounds = getLocalBounds();
    g.setColour(ZenithDesignSystem::getColor(ZenithDesignSystem::Colors::Error));
    g.fillRectangle(bounds.toFloat().withTrimmedTop(bounds.getHeight() - 2, 2));
}

void UIErrorHandler::resized() {
    // Component doesn't need visual representation in normal operation
    // Toasts are handled by ToastNotificationManager
}

void UIErrorHandler::notifyErrorReported(const UIError& error) {
    // Notify all registered callbacks
    for (const auto& callback : reportCallbacks_) {
        try {
            callback(error);
        } catch (...) {
            // Callbacks should not throw
            ZENITH_LOG_WARNING("Error in UIErrorHandler callback: " + error.message);
        }
    }

    // Check for custom error handlers
    if (error.errorCode != 0) {
        auto it = customHandlers_.find(error.errorCode);
        if (it != customHandlers_.end()) {
            try {
                it->second(error);
            } catch (...) {
                ZENITH_LOG_WARNING("Error in custom error handler for code " +
                             juce::String(error.errorCode).toStdString());
            }
        }
    }
}

void UIErrorHandler::notifyErrorCleared() {
    // Notify all registered callbacks
    for (const auto& callback : clearCallbacks_) {
        try {
            callback();
        } catch (...) {
            // Callbacks should not throw
            ZENITH_LOG_WARNING("Error in UIErrorHandler clear callback");
        }
    }
}

void UIErrorHandler::logErrorToSystem(const UIError& error) {
    // Convert severity to log level
    LogLevel logLevel;

    switch (error.severity) {
        case ErrorSeverity::Fatal:
        case ErrorSeverity::Critical:
            logLevel = LogLevel::Error;
            break;
        case ErrorSeverity::Error:
            logLevel = LogLevel::Error;
            break;
        case ErrorSeverity::Warning:
            logLevel = LogLevel::Warning;
            break;
        case ErrorSeverity::Info:
            logLevel = LogLevel::Info;
            break;
        case ErrorSeverity::Debug:
            logLevel = LogLevel::Debug;
            break;
    }

    // Create formatted message
    juce::String messageString = "[" + juce::String::formatted("%02d:%02d:%02d",
            error.timestamp.getHours(),
            error.timestamp.getMinutes(),
            error.timestamp.getSeconds()) + "] ";

    messageString += getCategoryName(error.category) + ": " + juce::String(error.message);

    if (error.details.size() > 0) {
        messageString += " (" + juce::String(error.details) + ")";
    }

    if (error.component.isNotEmpty()) {
        messageString += " [Component: " + error.component + "]";
    }

    // Log to system
    ZenithLogger::getInstance().log(logLevel, messageString);
}

void UIErrorHandler::handleFatalError(const UIError& error) {
    // For fatal errors, we might want to show a modal dialog
    // This could be enhanced to show a critical error dialog
    ZENITH_LOG_ERROR("FATAL ERROR: " + error.message);

    // Could add modal dialog here in the future
    // For now, just log and continue
}

void UIErrorHandler::deduplicateErrors() {
    // Simple deduplication - keep only the most recent errors
    if (errorHistory_.size() > maxHistorySize_) {
        size_t excess = errorHistory_.size() - maxHistorySize_;
        errorHistory_.erase(errorHistory_.begin(), errorHistory_.begin() + excess);
    }
}

juce::String UIErrorHandler::getCategoryName(ErrorCategory category) {
    switch (category) {
        case ErrorCategory::UI: return "UI";
        case ErrorCategory::Audio: return "Audio";
        case ErrorCategory::Plugin: return "Plugin";
        case ErrorCategory::File: return "File";
        case ErrorCategory::Network: return "Network";
        case ErrorCategory::Database: return "Database";
        case ErrorCategory::System: return "System";
        case ErrorCategory::Validation: return "Validation";
        case ErrorCategory::Unknown: return "Unknown";
        default: return "Unknown";
    }
}

ToastType UIErrorHandler::convertSeverityToToastType(ErrorSeverity severity) {
    switch (severity) {
        case ErrorSeverity::Fatal:
        case ErrorSeverity::Critical:
            return ToastNotificationManager::ToastType::Critical;
        case ErrorSeverity::Error:
            return ToastNotificationManager::ToastType::Error;
        case ErrorSeverity::Warning:
            return ToastNotificationManager::ToastType::Warning;
        case ErrorSeverity::Info:
            return ToastNotificationManager::ToastType::Info;
        case ErrorSeverity::Debug:
            return ToastNotificationManager::ToastType::Info;
        default:
            return ToastNotificationManager::ToastType::Error;
    }
}

} // namespace zenith::UI