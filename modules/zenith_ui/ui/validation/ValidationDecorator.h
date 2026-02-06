/*
  ValidationDecorator.h

  Component decorator that adds validation capabilities to any UI component

  Wraps components to provide visual feedback for validation states:
  - Green outline for valid
  - Red outline for invalid
  - Yellow warning for warnings
  - Loading animation for async validation
*/

#pragma once

#include "Validator.h"
#include "../framework/SkiaComponent.h"
#include "../design-system/ZenithDesignSystem.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

namespace Zenith::UI {

/**
 * Visual style for validation states
 */
enum class ValidationVisualStyle {
    Outline,    // Colored outline around component
    Background, // Colored background
    Icon,       // Icon in corner
    Tooltip,    // Tooltip with message
    Combined    // Combined approach
};

/**
 * Validation decorator that wraps any component to add validation
 */
class ValidationDecorator : public SkiaComponent, public juce::ComponentListener {
public:
    /**
     * Constructor
     */
    ValidationDecorator(ValidatableComponent* component,
                       ValidationVisualStyle style = ValidationVisualStyle::Outline);

    ~ValidationDecorator() override;

    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    void childBoundsChanged(juce::Component* child) override;

    // IUnifiedComponent override
    bool keyPressed(const juce::KeyPress& key) override;

    // Public methods
    /**
     * Set the validation style
     */
    void setValidationStyle(ValidationVisualStyle style);

    /**
     * Get the validation style
     */
    ValidationVisualStyle getValidationStyle() const;

    /**
     * Set whether validation should run automatically
     */
    void setAutoValidate(bool autoValidate);

    /**
     * Get whether auto-validation is enabled
     */
    bool getAutoValidate() const;

    /**
     * Manually trigger validation
     */
    void validate();

    /**
     * Get the current validation result
     */
    ValidationResult getValidationResult() const;

    /**
     * Set the validator for the decorated component
     */
    void setValidator(Validator* validator);

    /**
     * Get the current validator
     */
    Validator* getValidator() const;

    /**
     * Clear validation state
     */
    void clearValidation();

    /**
     * Set validation callback
     */
    void setValidationCallback(std::function<void(const ValidationResult&)> callback);

    /**
     * Set whether to show validation messages
     */
    void setShowMessages(bool show);

    /**
     * Get whether messages are shown
     */
    bool getShowMessages() const;

    /**
     * Get the decorated component
     */
    ValidatableComponent* getDecoratedComponent() const;

    // Layout methods
    void setContentMargin(int margin);
    int getContentMargin() const;

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;

    // Private implementation methods
    void updateValidationState();
    void drawValidationOutline(juce::Graphics& g);
    void drawValidationBackground(juce::Graphics& g);
    void drawValidationIcon(juce::Graphics& g);
    void drawValidationTooltip(juce::Graphics& g);
    void updateTooltipPosition();
    void showTooltip(const ValidationResult& result);
    void hideTooltip();

    // Animation methods
    void startValidationAnimation();
    void stopValidationAnimation();
    void updateAnimation();

    // Event handling
    void handleComponentChanged();
    void handleValidationComplete(const ValidationResult& result);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ValidationDecorator)
};

/**
 * Decorator factory for creating validation decorators
 */
class ValidationDecoratorFactory {
public:
    /**
     * Create a validation decorator with default settings
     */
    static ValidationDecorator* create(ValidatableComponent* component,
                                     Validator* validator = nullptr,
                                     ValidationVisualStyle style = ValidationVisualStyle::Outline);

    /**
     * Create a validation decorator with custom settings
     */
    static ValidationDecorator* createWithSettings(ValidatableComponent* component,
                                                Validator* validator,
                                                ValidationVisualStyle style,
                                                bool autoValidate = true,
                                                bool showMessages = true);

    /**
     * Create a validation decorator for a specific component type
     */
    template<typename ComponentType>
    static ValidationDecorator* createFor(ComponentType* component,
                                        Validator* validator = nullptr,
                                        ValidationVisualStyle style = ValidationVisualStyle::Outline) {
        return create(component, validator, style);
    }
};

} // namespace Zenith::UI