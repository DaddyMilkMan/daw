/*
  ValidationError.h

  Exception types and error handling for validation framework

  Provides structured error information for validation failures with
  proper exception handling and error recovery.
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <stdexcept>
#include <string>

namespace Zenith::UI {

/**
 * Validation error codes for structured error handling
 */
enum class ValidationErrorCode {
    None = 0,

    // String validation errors
    StringEmpty = 100,
    StringTooShort = 101,
    StringTooLong = 102,
    StringInvalidFormat = 103,
    StringContainsInvalidChars = 104,

    // Numeric validation errors
    NumberOutOfRange = 200,
    NumberTooSmall = 201,
    NumberTooLarge = 202,
    NumberInvalidFormat = 203,
    NumberMustBePositive = 204,
    NumberMustBeNegative = 205,

    // Email validation errors
    EmailInvalid = 300,
    EmailDomainInvalid = 301,
    EmailMissingTLD = 302,

    // URL validation errors
    URLInvalid = 400,
    URLMissingProtocol = 401,
    URLInvalidDomain = 402,

    // File validation errors
    FileNotFound = 500,
    FileInvalidFormat = 501,
    FileTooLarge = 502,
    FilePermissionDenied = 503,

    // Custom validation errors
    CustomValidationFailed = 900,

    // System errors
    ValidationTimeout = 1000,
    ValidationCancelled = 1001,
    ValidatorNotSet = 1002,
    InvalidValidatorType = 1003
};

/**
 * Validation exception with detailed error information
 */
class ValidationError : public std::runtime_error {
public:
    /**
     * Constructor with error code and message
     */
    ValidationError(ValidationErrorCode code, const std::string& message)
        : std::runtime_error(message)
        , errorCode_(code)
        , timestamp_(juce::Time::getCurrentTime())
        , severity_(getErrorSeverity(code)) {}

    /**
     * Constructor with error code, message, and field name
     */
    ValidationError(ValidationErrorCode code, const std::string& message, const std::string& fieldName)
        : std::runtime_error(fieldName + ": " + message)
        , errorCode_(code)
        , fieldName_(fieldName)
        , timestamp_(juce::Time::getCurrentTime())
        , severity_(getErrorSeverity(code)) {}

    /**
     * Constructor with error code, message, field name, and invalid value
     */
    ValidationError(ValidationErrorCode code, const std::string& message,
                   const std::string& fieldName, const juce::String& invalidValue)
        : std::runtime_error(buildErrorMessage(code, message, fieldName, invalidValue))
        , errorCode_(code)
        , fieldName_(fieldName)
        , invalidValue_(invalidValue)
        , timestamp_(juce::Time::getCurrentTime())
        , severity_(getErrorSeverity(code)) {}

    // Getters
    ValidationErrorCode getErrorCode() const { return errorCode_; }
    std::string getFieldName() const { return fieldName_; }
    juce::String getInvalidValue() const { return invalidValue_; }
    juce::Timestamp getTimestamp() const { return timestamp_; }
    std::string getSeverity() const { return severity_; }

    /**
     * Get user-friendly error message
     */
    std::string getUserMessage() const {
        switch (errorCode_) {
            case ValidationErrorCode::StringEmpty:
                return "This field is required";
            case ValidationErrorCode::StringTooShort:
                return "Value is too short";
            case ValidationErrorCode::StringTooLong:
                return "Value is too long";
            case ValidationErrorCode::StringInvalidFormat:
                return "Invalid format";
            case ValidationErrorCode::StringContainsInvalidChars:
                return "Contains invalid characters";
            case ValidationErrorCode::NumberOutOfRange:
                return "Value is out of range";
            case ValidationErrorCode::NumberTooSmall:
                return "Value is too small";
            case ValidationErrorCode::NumberTooLarge:
                return "Value is too large";
            case ValidationErrorCode::NumberInvalidFormat:
                return "Invalid number format";
            case ValidationErrorCode::NumberMustBePositive:
                return "Value must be positive";
            case ValidationErrorCode::NumberMustBeNegative:
                return "Value must be negative";
            case ValidationErrorCode::EmailInvalid:
                return "Invalid email address";
            case ValidationErrorCode::EmailDomainInvalid:
                return "Invalid email domain";
            case ValidationErrorCode::EmailMissingTLD:
                return "Email domain is incomplete";
            case ValidationErrorCode::URLInvalid:
                return "Invalid URL";
            case ValidationErrorCode::URLMissingProtocol:
                return "URL must include protocol (http:// or https://)";
            case ValidationErrorCode::URLInvalidDomain:
                return "Invalid domain in URL";
            case ValidationErrorCode::FileNotFound:
                return "File not found";
            case ValidationErrorCode::FileInvalidFormat:
                return "Invalid file format";
            case ValidationErrorCode::FileTooLarge:
                return "File is too large";
            case ValidationErrorCode::FilePermissionDenied:
                return "Permission denied";
            case ValidationErrorCode::CustomValidationFailed:
                return "Validation failed";
            case ValidationErrorCode::ValidationTimeout:
                return "Validation timed out";
            case ValidationErrorCode::ValidationCancelled:
                return "Validation was cancelled";
            case ValidationErrorCode::ValidatorNotSet:
                return "No validator set for this field";
            case ValidationErrorCode::InvalidValidatorType:
                return "Invalid validator type";
            default:
                return what();
        }
    }

    /**
     * Check if error is recoverable
     */
    bool isRecoverable() const {
        switch (errorCode_) {
            case ValidationErrorCode::StringTooShort:
            case ValidationErrorCode::StringTooLong:
            case ValidationErrorCode::NumberTooSmall:
            case ValidationErrorCode::NumberTooLarge:
            case ValidationErrorCode::EmailMissingTLD:
            case ValidationErrorCode::URLMissingProtocol:
                return true;
            default:
                return false;
        }
    }

    /**
     * Get suggestion for fixing the error
     */
    std::string getSuggestion() const {
        switch (errorCode_) {
            case ValidationErrorCode::StringEmpty:
                return "Please enter a value for this field";
            case ValidationErrorCode::StringTooShort:
                return "Please enter more characters";
            case ValidationErrorCode::StringTooLong:
                return "Please shorten the value";
            case ValidationErrorCode::StringInvalidFormat:
                return "Please enter the correct format";
            case ValidationErrorCode::NumberTooSmall:
                return "Please enter a larger value";
            case ValidationErrorCode::NumberTooLarge:
                return "Please enter a smaller value";
            case ValidationErrorCode::EmailInvalid:
                return "Please enter a valid email address";
            case ValidationErrorCode::URLInvalid:
                return "Please enter a valid URL";
            default:
                return "Please check the value and try again";
        }
    }

private:
    ValidationErrorCode errorCode_;
    std::string fieldName_;
    juce::String invalidValue_;
    juce::Timestamp timestamp_;
    std::string severity_;

    static std::string getErrorSeverity(ValidationErrorCode code) {
        switch (code) {
            case ValidationErrorCode::None:
                return "Info";
            case ValidationErrorCode::StringEmpty:
            case ValidationErrorCode::NumberInvalidFormat:
            case ValidationErrorCode::EmailInvalid:
            case ValidationErrorCode::URLInvalid:
            case ValidationErrorCode::FileNotFound:
                return "Warning";
            case ValidationErrorCode::StringInvalidFormat:
            case ValidationErrorCode::StringContainsInvalidChars:
            case ValidationErrorCode::NumberOutOfRange:
            case ValidationErrorCode::CustomValidationFailed:
                return "Error";
            case ValidationErrorCode::ValidationTimeout:
            case ValidationErrorCode::ValidationCancelled:
                return "Info";
            default:
                return "Error";
        }
    }

    static std::string buildErrorMessage(ValidationErrorCode code,
                                      const std::string& message,
                                      const std::string& fieldName,
                                      const juce::String& invalidValue) {
        std::string result = fieldName + ": " + message;
        if (invalidValue.isNotEmpty()) {
            result += " ('" + invalidValue.toStdString() + "')";
        }
        return result;
    }
};

/**
 * Validation error handler for centralized error management
 */
class ValidationErrorHandler {
public:
    /**
     * Handle a validation error by showing appropriate feedback
     */
    static void handleError(const ValidationError& error, juce::Component* component = nullptr) {
        // Log the error
        zenLogWarning("Validation Error: " + std::string(error.what()));

        // Show toast notification
        switch (error.getSeverity()) {
            case "Warning":
                showWarning(error.getUserMessage());
                break;
            case "Error":
                showError(error.getUserMessage());
                break;
            case "Info":
                showInfo(error.getUserMessage());
                break;
        }

        // Focus the component if provided
        if (component) {
            component->grabKeyboardFocus();
        }
    }

    /**
     * Show a validation error toast
     */
    static void showError(const std::string& message) {
        ToastNotificationManager::getInstance().showError(message);
    }

    /**
     * Show a validation warning toast
     */
    static void showWarning(const std::string& message) {
        ToastNotificationManager::getInstance().showWarning(message);
    }

    /**
     * Show validation info toast
     */
    static void showInfo(const std::string& message) {
        ToastNotificationManager::getInstance().showInfo(message);
    }

    /**
     * Handle multiple validation errors
     */
    static void handleErrors(const std::vector<ValidationError>& errors, juce::Component* component = nullptr) {
        if (errors.empty()) {
            return;
        }

        // Group errors by severity
        std::vector<ValidationError> criticalErrors;
        std::vector<ValidationError> warnings;
        std::vector<ValidationError> info;

        for (const auto& error : errors) {
            if (error.getSeverity() == "Error") {
                criticalErrors.push_back(error);
            } else if (error.getSeverity() == "Warning") {
                warnings.push_back(error);
            } else {
                info.push_back(error);
            }
        }

        // Show errors in priority order
        if (!criticalErrors.empty()) {
            showError(criticalErrors[0].getUserMessage());
        } else if (!warnings.empty()) {
            showWarning(warnings[0].getUserMessage());
        } else if (!info.empty()) {
            showInfo(info[0].getUserMessage());
        }

        // Focus first error component if provided
        if (component && !criticalErrors.empty()) {
            component->grabKeyboardFocus();
        }
    }
};

/**
 * RAII-style validation scope for error handling
 */
class ValidationScope {
public:
    /**
     * Constructor
     */
    ValidationScope(juce::Component* component = nullptr)
        : component_(component) {}

    /**
     * Constructor with error callback
     */
    ValidationScope(std::function<void(const ValidationError&)> errorCallback)
        : errorCallback_(errorCallback) {}

    /**
     * Destructor - ensures any pending errors are handled
     */
    ~ValidationScope() {
        if (hasError_ && errorCallback_) {
            errorCallback_(currentError_);
        }
    }

    /**
     * Check if validation has errors
     */
    bool hasErrors() const { return hasError_; }

    /**
     * Get the current error
     */
    const ValidationError* getError() const {
        return hasError_ ? &currentError_ : nullptr;
    }

    /**
     * Set an error and mark validation as failed
     */
    void setError(const ValidationError& error) {
        hasError_ = true;
        currentError_ = error;
        errorCallback_(error);
    }

    /**
     * Clear any error
     */
    void clearError() {
        hasError_ = false;
        currentError_ = ValidationError(ValidationErrorCode::None, "No error");
    }

    /**
     * Execute a function with validation
     */
    template<typename Func>
    auto execute(Func&& func) -> decltype(func()) {
        try {
            return func();
        } catch (const ValidationError& e) {
            setError(e);
            throw;
        } catch (const std::exception& e) {
            setError(ValidationError(ValidationErrorCode::CustomValidationFailed, e.what()));
            throw;
        } catch (...) {
            setError(ValidationError(ValidationErrorCode::CustomValidationFailed, "Unknown validation error"));
            throw;
        }
    }

private:
    juce::Component* component_;
    std::function<void(const ValidationError&)> errorCallback_;
    bool hasError_ = false;
    ValidationError currentError_;
};

} // namespace Zenith::UI