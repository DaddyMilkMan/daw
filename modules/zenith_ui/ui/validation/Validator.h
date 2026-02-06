/*
  Validator.h

  Base validator interface for UI input validation

  Provides a framework for validating user input with immediate visual feedback
  and async validation capabilities.
*/

#pragma once

#include "../framework/SkiaComponent.h"
#include "../design-system/ZenithDesignSystem.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <string>

namespace zenith::UI {

/**
 * Validation result containing status and error information
 */
struct ValidationResult {
    bool isValid;
    juce::String errorMessage;
    juce::String warningMessage;
    juce::Time timestamp;

    ValidationResult()
        : isValid(true), timestamp(juce::Time::getCurrentTime()) {}

    ValidationResult(bool valid, const juce::String& error = "", const juce::String& warning = "")
        : isValid(valid), errorMessage(error), warningMessage(warning)
        , timestamp(juce::Time::getCurrentTime()) {}

    // Factory methods
    static ValidationResult success() {
        return ValidationResult(true);
    }

    static ValidationResult failure(const juce::String& error) {
        return ValidationResult(false, error);
    }

    static ValidationResult warning(const juce::String& warning) {
        return ValidationResult(true, "", warning);
    }

    // Check if this is a "severe" validation issue
    bool isFailure() const { return !isValid; }
    bool isWarning() const { return isValid && warningMessage.isNotEmpty(); }
    bool hasMessages() const { return !isValid || warningMessage.isNotEmpty(); }
};

/**
 * Base validator interface
 */
class Validator {
public:
    using ValidationCallback = std::function<void(const ValidationResult&)>;

    virtual ~Validator() = default;

    /**
     * Validate the current value
     */
    virtual ValidationResult validate() = 0;

    /**
     * Validate with async callback for long-running validations
     */
    virtual void validateAsync(const ValidationCallback& callback) {
        // Default synchronous implementation
        auto result = validate();
        callback(result);
    }

    /**
     * Get the validator name/description
     */
    virtual juce::String getName() const = 0;

    /**
     * Check if validation is currently running
     */
    virtual bool isValidationRunning() const {
        return false;
    }

    /**
     * Cancel any pending async validation
     */
    virtual void cancelValidation() {
        // Default no-op
    }

    /**
     * Set whether validation should show warnings
     */
    virtual void setShowWarnings(bool show) {
        showWarnings_ = show;
    }

    /**
     * Get whether warnings are shown
     */
    virtual bool getShowWarnings() const {
        return showWarnings_;
    }

protected:
    bool showWarnings_ = true;
};

/**
 * Validator for validating a string value
 */
class StringValidator : public Validator {
public:
    virtual ~StringValidator() = default;

    /**
     * Validate the given string value
     */
    virtual ValidationResult validateString(const juce::String& value) = 0;

    // Override base class
    ValidationResult validate() override {
        if (currentValue_.isNotEmpty()) {
            return validateString(currentValue_);
        }
        return ValidationResult::success();
    }

    /**
     * Set the current value to validate
     */
    void setValue(const juce::String& value) {
        currentValue_ = value;
    }

    /**
     * Get the current value
     */
    juce::String getValue() const {
        return currentValue_;
    }

protected:
    juce::String currentValue_;
};

/**
 * Validator for validating a numeric value
 */
template<typename T>
class NumericValidator : public Validator {
public:
    virtual ~NumericValidator() = default;

    /**
     * Validate the given numeric value
     */
    virtual ValidationResult validateNumber(T value) = 0;

    // Override base class
    ValidationResult validate() override {
        if (hasValue_) {
            return validateNumber(currentValue_);
        }
        // Allow empty/unset values unless required
        if (isRequired_) {
            return ValidationResult::failure("Value is required");
        }
        return ValidationResult::success();
    }

    /**
     * Set the current value to validate
     */
    void setValue(T value, bool hasValue = true) {
        currentValue_ = value;
        hasValue_ = hasValue;
    }

    /**
     * Get the current value
     */
    T getValue() const {
        return currentValue_;
    }

    /**
     * Check if value is set
     */
    bool hasValue() const {
        return hasValue_;
    }

    /**
     * Set whether the field is required
     */
    void setRequired(bool required) {
        isRequired_ = required;
    }

    /**
     * Check if field is required
     */
    bool isRequired() const {
        return isRequired_;
    }

protected:
    T currentValue_ = T();
    bool hasValue_ = false;
    bool isRequired_ = false;
};

/**
 * Validator interface for components that can validate themselves
 */
class ValidatableComponent {
public:
    virtual ~ValidatableComponent() = default;

    /**
     * Validate this component
     */
    virtual ValidationResult validate() = 0;

    /**
     * Get the validator for this component
     */
    virtual Validator* getValidator() = 0;

    /**
     * Set a custom validator for this component
     */
    virtual void setValidator(Validator* validator) = 0;

    /**
     * Set whether validation should run automatically
     */
    virtual void setAutoValidate(bool autoValidate) = 0;

    /**
     * Check if auto-validation is enabled
     */
    virtual bool getAutoValidate() const = 0;

    /**
     * Get the current validation result
     */
    virtual ValidationResult getLastValidationResult() const = 0;

    /**
     * Clear validation state
     */
    virtual void clearValidation() = 0;

    /**
     * Set validation callback
     */
    virtual void setValidationCallback(std::function<void(const ValidationResult&)> callback) = 0;
};

/**
 * Validator factory for creating common validators
 */
class ValidatorFactory {
public:
    // String validators
    static StringValidator* createRequiredValidator(const juce::String& fieldName);
    static StringValidator* createLengthValidator(int minLength, int maxLength, const juce::String& fieldName);
    static StringValidator* createRegexValidator(const juce::String& pattern, const juce::String& fieldName);
    static StringValidator* createEmailValidator(const juce::String& fieldName);
    static StringValidator* createUrlValidator(const juce::String& fieldName);
    static StringValidator* createAlphaNumericValidator(const juce::String& fieldName);

    // Numeric validators
    template<typename T>
    static NumericValidator<T>* createRangeValidator(T min, T max, const juce::String& fieldName);
    template<typename T>
    static NumericValidator<T>* createMinValidator(T min, const juce::String& fieldName);
    template<typename T>
    static NumericValidator<T>* createMaxValidator(T max, const juce::String& fieldName);
};

} // namespace Zenith::UI