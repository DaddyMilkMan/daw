/*
  Validator.cpp

  Implementation of base validator interface and common validators
*/

#include "Validator.h"
#include <regex>
#include <algorithm>

namespace zenith::UI {

// =================================================================
// Required String Validator
// =================================================================

class RequiredValidator : public StringValidator {
public:
    RequiredValidator(const juce::String& fieldName)
        : fieldName_(fieldName) {}

    juce::String getName() const override {
        return "RequiredValidator for " + fieldName_;
    }

    ValidationResult validateString(const juce::String& value) override {
        if (value.trim().isEmpty()) {
            return ValidationResult::failure(fieldName_ + " is required");
        }
        return ValidationResult::success();
    }

private:
    juce::String fieldName_;
};

// =================================================================
// Length String Validator
// =================================================================

class LengthValidator : public StringValidator {
public:
    LengthValidator(int minLength, int maxLength, const juce::String& fieldName)
        : minLength_(minLength), maxLength_(maxLength), fieldName_(fieldName) {}

    juce::String getName() const override {
        return "LengthValidator for " + fieldName_;
    }

    ValidationResult validateString(const juce::String& value) override {
        int length = value.length();

        if (minLength_ > 0 && length < minLength_) {
            return ValidationResult::failure(fieldName_ + " must be at least " + juce::String(minLength_) + " characters");
        }

        if (maxLength_ > 0 && length > maxLength_) {
            return ValidationResult::failure(fieldName_ + " must be no more than " + juce::String(maxLength_) + " characters");
        }

        if (showWarnings_ && length < minLength_ * 2) {
            return ValidationResult::warning(fieldName_ + " is short - consider adding more detail");
        }

        return ValidationResult::success();
    }

private:
    int minLength_;
    int maxLength_;
    juce::String fieldName_;
};

// =================================================================
// Regex String Validator
// =================================================================

class RegexValidator : public StringValidator {
public:
    RegexValidator(const juce::String& pattern, const juce::String& fieldName)
        : pattern_(pattern), fieldName_(fieldName) {
        try {
            regex_ = std::regex(pattern_.toStdString());
        } catch (const std::regex_error& e) {
            zenLogWarning("Invalid regex pattern: " + pattern_);
            valid_ = false;
        }
    }

    juce::String getName() const override {
        return "RegexValidator for " + fieldName_;
    }

    ValidationResult validateString(const juce::String& value) override {
        if (!valid_) {
            return ValidationResult::success(); // Don't fail if regex is invalid
        }

        if (std::regex_match(value.toStdString(), regex_)) {
            return ValidationResult::success();
        } else {
            return ValidationResult::failure(fieldName_ + " format is invalid");
        }
    }

private:
    juce::String pattern_;
    juce::String fieldName_;
    std::regex regex_;
    bool valid_ = true;
};

// =================================================================
// Email Validator
// =================================================================

class EmailValidator : public StringValidator {
public:
    EmailValidator(const juce::String& fieldName)
        : fieldName_(fieldName) {}

    juce::String getName() const override {
        return "EmailValidator for " + fieldName_;
    }

    ValidationResult validateString(const juce::String& value) override {
        if (value.isEmpty()) {
            if (isRequired_) {
                return ValidationResult::failure(fieldName_ + " is required");
            }
            return ValidationResult::success();
        }

        // Basic email validation regex
        static const std::regex emailRegex(
            R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})");

        if (std::regex_match(value.toStdString(), emailRegex)) {
            return ValidationResult::success();
        } else {
            return ValidationResult::failure("Invalid email address format");
        }
    }

private:
    juce::String fieldName_;
};

// =================================================================
// URL Validator
// =================================================================

class UrlValidator : public StringValidator {
public:
    UrlValidator(const juce::String& fieldName)
        : fieldName_(fieldName) {}

    juce::String getName() const override {
        return "UrlValidator for " + fieldName_;
    }

    ValidationResult validateString(const juce::String& value) override {
        if (value.isEmpty()) {
            if (isRequired_) {
                return ValidationResult::failure(fieldName_ + " is required");
            }
            return ValidationResult::success();
        }

        // Basic URL validation
        static const std::regex urlRegex(
            R"((https?:\/\/(?:www\.|(?!www))[a-zA-Z0-9][a-zA-Z0-9-]+[a-zA-Z0-9]\.[^\s]{2,}|www\.[a-zA-Z0-9][a-zA-Z0-9-]+[a-zA-Z0-9]\.[^\s]{2,}|https?:\/\/(?:www\.|(?!www))[a-zA-Z0-9]+\.[^\s]{2,}|www\.[a-zA-Z0-9]+\.[^\s]{2,}))");

        if (std::regex_match(value.toStdString(), urlRegex)) {
            return ValidationResult::success();
        } else {
            return ValidationResult::failure("Invalid URL format");
        }
    }

private:
    juce::String fieldName_;
};

// =================================================================
// Alpha Numeric Validator
// =================================================================

class AlphaNumericValidator : public StringValidator {
public:
    AlphaNumericValidator(const juce::String& fieldName)
        : fieldName_(fieldName) {}

    juce::String getName() const override {
        return "AlphaNumericValidator for " + fieldName_;
    }

    ValidationResult validateString(const juce::String& value) override {
        if (value.isEmpty()) {
            if (isRequired_) {
                return ValidationResult::failure(fieldName_ + " is required");
            }
            return ValidationResult::success();
        }

        // Check if string contains only alphanumeric characters and spaces
        for (auto c : value) {
            if (!juce::CharacterFunctions::isLetterOrDigit(c) && !juce::CharacterFunctions::isWhitespace(c)) {
                return ValidationResult::failure(fieldName_ + " must contain only letters, numbers, and spaces");
            }
        }

        return ValidationResult::success();
    }

private:
    juce::String fieldName_;
};

// =================================================================
// Range Numeric Validator
// =================================================================

template<typename T>
class RangeValidator : public NumericValidator<T> {
public:
    RangeValidator(T min, T max, const juce::String& fieldName)
        : min_(min), max_(max), fieldName_(fieldName) {}

    juce::String getName() const override {
        return "RangeValidator for " + fieldName_;
    }

    ValidationResult validateNumber(T value) override {
        if (value < min_) {
            return ValidationResult::failure(fieldName_ + " must be at least " + juce::String(min_));
        }

        if (value > max_) {
            return ValidationResult::failure(fieldName_ + " must be no more than " + juce::String(max_));
        }

        if (showWarnings_ && value == min_) {
            return ValidationResult::warning(fieldName_ + " is at minimum value");
        }

        if (showWarnings_ && value == max_) {
            return ValidationResult::warning(fieldName_ + " is at maximum value");
        }

        return ValidationResult::success();
    }

private:
    T min_;
    T max_;
    juce::String fieldName_;
};

// =================================================================
// Min Validator
// =================================================================

template<typename T>
class MinValidator : public NumericValidator<T> {
public:
    MinValidator(T min, const juce::String& fieldName)
        : min_(min), fieldName_(fieldName) {}

    juce::String getName() const override {
        return "MinValidator for " + fieldName_;
    }

    ValidationResult validateNumber(T value) override {
        if (value < min_) {
            return ValidationResult::failure(fieldName_ + " must be at least " + juce::String(min_));
        }

        if (showWarnings_ && value == min_) {
            return ValidationResult::warning(fieldName_ + " is at minimum value");
        }

        return ValidationResult::success();
    }

private:
    T min_;
    juce::String fieldName_;
};

// =================================================================
// Max Validator
// =================================================================

template<typename T>
class MaxValidator : public NumericValidator<T> {
public:
    MaxValidator(T max, const juce::String& fieldName)
        : max_(max), fieldName_(fieldName) {}

    juce::String getName() const override {
        return "MaxValidator for " + fieldName_;
    }

    ValidationResult validateNumber(T value) override {
        if (value > max_) {
            return ValidationResult::failure(fieldName_ + " must be no more than " + juce::String(max_));
        }

        if (showWarnings_ && value == max_) {
            return ValidationResult::warning(fieldName_ + " is at maximum value");
        }

        return ValidationResult::success();
    }

private:
    T max_;
    juce::String fieldName_;
};

// =================================================================
// Validator Factory Implementation
// =================================================================

StringValidator* ValidatorFactory::createRequiredValidator(const juce::String& fieldName) {
    return new RequiredValidator(fieldName);
}

StringValidator* ValidatorFactory::createLengthValidator(int minLength, int maxLength, const juce::String& fieldName) {
    return new LengthValidator(minLength, maxLength, fieldName);
}

StringValidator* ValidatorFactory::createRegexValidator(const juce::String& pattern, const juce::String& fieldName) {
    return new RegexValidator(pattern, fieldName);
}

StringValidator* ValidatorFactory::createEmailValidator(const juce::String& fieldName) {
    return new EmailValidator(fieldName);
}

StringValidator* ValidatorFactory::createUrlValidator(const juce::String& fieldName) {
    return new UrlValidator(fieldName);
}

StringValidator* ValidatorFactory::createAlphaNumericValidator(const juce::String& fieldName) {
    return new AlphaNumericValidator(fieldName);
}

template NumericValidator<int>* ValidatorFactory::createRangeValidator<int>(int, int, const juce::String&);
template NumericValidator<double>* ValidatorFactory::createRangeValidator<double>(double, double, const juce::String&);
template NumericValidator<float>* ValidatorFactory::createRangeValidator<float>(float, float, const juce::String&);

template NumericValidator<int>* ValidatorFactory::createMinValidator<int>(int, const juce::String&);
template NumericValidator<double>* ValidatorFactory::createMinValidator<double>(double, const juce::String&);
template NumericValidator<float>* ValidatorFactory::createMinValidator<float>(float, const juce::String&);

template NumericValidator<int>* ValidatorFactory::createMaxValidator<int>(int, const juce::String&);
template NumericValidator<double>* ValidatorFactory::createMaxValidator<double>(double, const juce::String&);
template NumericValidator<float>* ValidatorFactory::createMaxValidator<float>(float, const juce::String&);

} // namespace zenith::UI