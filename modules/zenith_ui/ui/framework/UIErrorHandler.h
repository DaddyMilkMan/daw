/*
  UIErrorHandler.h

  Centralized UI error handling system for Zenith DAW

  This singleton provides unified error categorization, reporting, and display
  across all UI components.
*/

#pragma once

#include "../framework/SkiaComponent.h"
#include "../design-system/ZenithDesignSystem.h"
#include <ZenithLogger.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <vector>
#include <mutex>
#include <atomic>

namespace zenith::UI {

/**
 * Error severity levels for categorization and handling
 */
enum class ErrorSeverity {
    Fatal,      // Application cannot continue, requires immediate attention
    Critical,   // Serious issue that impacts functionality but app can continue
    Error,      // Regular errors that should be addressed
    Warning,    // Non-critical warnings for user awareness
    Info,       // Informational messages for user guidance
    Debug       // Debug information for development
};

/**
 * Error category for routing and display purposes
 */
enum class ErrorCategory {
    UI,         // UI component errors (rendering, interaction, etc.)
    Audio,      // Audio processing errors
    Plugin,     // Plugin loading/execution errors
    File,       // File I/O errors
    Network,    // Network communication errors
    Database,   // Project data errors
    System,     // System resource errors
    Validation, // Input validation errors
    Unknown     // Uncategorized errors
};

/**
 * Individual error with context information
 */
struct UIError {
    ErrorSeverity severity;
    ErrorCategory category;
    std::string message;
    std::string details;
    juce::String component;
    juce::Time timestamp;
    std::function<void()> action;
    std::string actionText;
    bool dismissible;
    int errorCode;

    UIError(ErrorSeverity sev, ErrorCategory cat, const std::string& msg,
           const std::string& det = "", const juce::String& comp = "")
        : severity(sev), category(cat), message(msg), details(det)
        , component(comp), timestamp(juce::Time::getCurrentTime())
        , dismissible(true), errorCode(0) {}

    // Factory methods for common error types
    static UIError createUIError(const std::string& message, const std::string& details = "") {
        return UIError(ErrorSeverity::Error, ErrorCategory::UI, message, details);
    }

    static UIError createAudioError(const std::string& message, const std::string& details = "") {
        return UIError(ErrorSeverity::Critical, ErrorCategory::Audio, message, details);
    }

    static UIError createFileError(const std::string& message, const std::string& details = "") {
        return UIError(ErrorSeverity::Error, ErrorCategory::File, message, details);
    }

    static UIError createPluginError(const std::string& message, const std::string& details = "") {
        return UIError(ErrorSeverity::Critical, ErrorCategory::Plugin, message, details);
    }

    static UIError createValidationError(const std::string& message, const std::string& details = "") {
        return UIError(ErrorSeverity::Warning, ErrorCategory::Validation, message, details);
    }
};

/**
 * Global UI Error Handler Singleton
 *
 * This provides centralized error management for all UI components.
 * Features include:
 * - Error categorization and severity levels
 * - Error aggregation and deduplication
 * - Toast notification integration
 * - Logging integration
 * - Error callbacks for components
 */
class UIErrorHandler : public juce::Component {
public:
    /**
     * Get the singleton instance
     */
    static UIErrorHandler& getInstance();

    /**
     * Report an error to the handler
     */
    void reportError(const UIError& error);

    /**
     * Report an error with message only (creates UIError with defaults)
     */
    void reportError(ErrorSeverity severity, ErrorCategory category,
                    const std::string& message, const std::string& details = "",
                    const juce::String& component = "");

    /**
     * Report a quick error with minimum parameters
     */
    void reportQuickError(const std::string& message, ErrorSeverity severity = ErrorSeverity::Error);

    /**
     * Clear all errors of a specific category
     */
    void clearErrors(ErrorCategory category);

    /**
     * Clear all errors
     */
    void clearAllErrors();

    /**
     * Set maximum number of errors to keep in history
     */
    void setMaxHistorySize(size_t maxSize);

    /**
     * Get current error count
     */
    size_t getErrorCount() const;

    /**
     * Check if there are any active errors
     */
    bool hasErrors() const;

    /**
     * Register a callback for when errors are reported
     */
    void onErrorReported(std::function<void(const UIError&)> callback);

    /**
     * Register a callback for when errors are cleared
     */
    void onErrorCleared(std::function<void()> callback);

    /**
     * Get the latest error
     */
    const UIError* getLatestError() const;

    /**
     * Get errors by category
     */
    std::vector<UIError> getErrorsByCategory(ErrorCategory category) const;

    /**
     * Get errors by severity
     */
    std::vector<UIError> getErrorsBySeverity(ErrorSeverity severity) const;

    /**
     * Check if system has fatal/critical errors
     */
    bool hasFatalErrors() const;

    /**
     * Add a custom error handler for specific error codes
     */
    void addErrorHandler(int errorCode, std::function<void(const UIError&)> handler);

    /**
     * Get error statistics
     */
    struct ErrorStats {
        size_t total;
        size_t fatal;
        size_t critical;
        size_t errors;
        size_t warnings;
        size_t info;

        ErrorStats() : total(0), fatal(0), critical(0), errors(0), warnings(0), info(0) {}
    };

    ErrorStats getStatistics() const;

    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    UIErrorHandler();
    ~UIErrorHandler() override;

    // Prevent copying
    UIErrorHandler(const UIErrorHandler&) = delete;
    UIErrorHandler& operator=(const UIErrorHandler&) = delete;

    // Private implementation
    struct ErrorRecord {
        UIError error;
        bool displayed;
        juce::UniqueId id;
    };

    std::vector<ErrorRecord> errorHistory_;
    std::map<int, std::function<void(const UIError&)>> customHandlers_;
    std::vector<std::function<void(const UIError&)>> reportCallbacks_;
    std::vector<std::function<void()>> clearCallbacks_;
    mutable std::mutex mutex_;
    std::atomic<bool> needsUpdate_;
    size_t maxHistorySize_;

    // Internal methods
    void notifyErrorReported(const UIError& error);
    void notifyErrorCleared();
    void logErrorToSystem(const UIError& error);
    void handleFatalError(const UIError& error);
    void deduplicateErrors();
    juce::String getCategoryName(ErrorCategory category);
    ToastType convertSeverityToToastType(ErrorSeverity severity);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UIErrorHandler)
};

// Helper functions for quick error reporting
inline void reportUIError(const std::string& message, const std::string& details = "") {
    UIErrorHandler::getInstance().reportError(UIError::createUIError(message, details));
}

inline void reportAudioError(const std::string& message, const std::string& details = "") {
    UIErrorHandler::getInstance().reportError(UIError::createAudioError(message, details));
}

inline void reportFileError(const std::string& message, const std::string& details = "") {
    UIErrorHandler::getInstance().reportError(UIError::createFileError(message, details));
}

inline void reportPluginError(const std::string& message, const std::string& details = "") {
    UIErrorHandler::getInstance().reportError(UIError::createPluginError(message, details));
}

inline void reportValidationError(const std::string& message, const std::string& details = "") {
    UIErrorHandler::getInstance().reportError(UIError::createValidationError(message, details));
}

inline void reportQuickError(const std::string& message, ErrorSeverity severity = ErrorSeverity::Error) {
    UIErrorHandler::getInstance().reportQuickError(message, severity);
}

} // namespace Zenith::UI