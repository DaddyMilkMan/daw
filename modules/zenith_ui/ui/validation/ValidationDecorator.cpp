/*
  ValidationDecorator.cpp

  Implementation of validation decorator for UI components
*/

#include "ValidationDecorator.h"
#include "../../core/utils/PlatformLogUtils.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace Zenith::UI {

// Private implementation structure
struct ValidationDecorator::Impl {
    ValidatableComponent* component;
    Validator* validator;
    ValidationVisualStyle style;
    ValidationResult lastResult;
    bool autoValidate;
    bool showMessages;
    int contentMargin;
    bool isAnimating;
    float animationProgress;
    juce::TooltipWindow tooltipWindow;
    std::function<void(const ValidationResult&)> callback;

    // Animation timer
    juce::Timer animationTimer;

    Impl(ValidatableComponent* comp, Validator* val, ValidationVisualStyle s)
        : component(comp)
        , validator(val)
        , style(s)
        , autoValidate(true)
        , showMessages(true)
        , contentMargin(4)
        , isAnimating(false)
        , animationProgress(0.0f) {
        // Set up animation timer
        animationTimer.startTimerHz(60); // 60 FPS
    }

    ~Impl() {
        animationTimer.stopTimer();
    }

    void startAnimation() {
        isAnimating = true;
        animationProgress = 0.0f;
    }

    void updateAnimation() {
        if (isAnimating) {
            animationProgress += 0.05f;
            if (animationProgress >= 1.0f) {
                animationProgress = 1.0f;
                isAnimating = false;
                animationTimer.stopTimer();
            }
        }
    }

    float getEaseInOut(float t) {
        return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
    }
};

ValidationDecorator::ValidationDecorator(ValidatableComponent* component,
                                         ValidationVisualStyle style)
    : pimpl_(new Impl(component, nullptr, style)) {
    setName("ValidationDecorator");

    // Add the component as a child
    addAndMakeVisible(component);

    // Listen to component changes
    component->addComponentListener(this);

    // Set up tooltip window
    pimpl_->tooltipWindow.setMillisecondsBeforeTipAppears(500);
    pimpl_->tooltipWindow.setOpaque(false);
    pimpl_->tooltipWindow.setLookAndFeel(&getLookAndFeel());
}

ValidationDecorator::~ValidationDecorator() {
    if (pimpl_->component) {
        pimpl_->component->removeComponentListener(this);
    }
}

void ValidationDecorator::paint(juce::Graphics& g) {
    if (!pimpl_->component) {
        return;
    }

    // Draw base background if needed
    if (pimpl_->style == ValidationVisualStyle::Background) {
        g.setColour(juce::Colours::transparentWhite);
        g.fillRectangle(getLocalBounds());
    }

    // Draw validation decorations based on style
    switch (pimpl_->style) {
        case ValidationVisualStyle::Outline:
            drawValidationOutline(g);
            break;
        case ValidationVisualStyle::Background:
            drawValidationBackground(g);
            break;
        case ValidationVisualStyle::Icon:
            drawValidationIcon(g);
            break;
        case ValidationVisualStyle::Tooltip:
            // Tooltip is drawn in showTooltip()
            break;
        case ValidationVisualStyle::Combined:
            drawValidationOutline(g);
            drawValidationIcon(g);
            break;
    }

    // Update animation if needed
    if (pimpl_->isAnimating) {
        pimpl_->updateAnimation();
    }
}

void ValidationDecorator::resized() {
    if (!pimpl_->component) {
        return;
    }

    // Position the decorated component
    auto bounds = getLocalBounds().reduced(pimpl_->contentMargin);
    pimpl_->component->setBounds(bounds);

    // Update tooltip position if shown
    updateTooltipPosition();
}

void ValidationDecorator::childBoundsChanged(juce::Component* child) {
    // Repaint when child bounds change
    repaint();
}

bool ValidationDecorator::keyPressed(const juce::KeyPress& key) {
    // Forward key events to decorated component
    if (pimpl_->component) {
        return pimpl_->component->keyPressed(key);
    }
    return false;
}

void ValidationDecorator::setValidationStyle(ValidationVisualStyle style) {
    pimpl_->style = style;
    repaint();
}

ValidationVisualStyle ValidationDecorator::getValidationStyle() const {
    return pimpl_->style;
}

void ValidationDecorator::setAutoValidate(bool autoValidate) {
    pimpl_->autoValidate = autoValidate;
}

bool ValidationDecorator::getAutoValidate() const {
    return pimpl_->autoValidate;
}

void ValidationDecorator::validate() {
    if (!pimpl_->component || !pimpl_->validator) {
        return;
    }

    // Start validation animation
    pimpl_->startAnimation();

    // Perform validation
    auto result = pimpl_->component->validate();

    // Update state
    pimpl_->lastResult = result;
    repaint();

    // Handle async validation if validator supports it
    if (pimpl_->validator->isValidationRunning()) {
        // Will be handled by callback
    } else {
        handleValidationComplete(result);
    }
}

ValidationResult ValidationDecorator::getValidationResult() const {
    return pimpl_->lastResult;
}

void ValidationDecorator::setValidator(Validator* validator) {
    pimpl_->validator = validator;
    clearValidation();

    // If we have a validator and auto-validate is enabled, validate immediately
    if (validator && pimpl_->autoValidate) {
        validate();
    }
}

Validator* ValidationDecorator::getValidator() const {
    return pimpl_->validator;
}

void ValidationDecorator::clearValidation() {
    pimpl_->lastResult = ValidationResult::success();
    hideTooltip();
    repaint();
}

void ValidationDecorator::setValidationCallback(std::function<void(const ValidationResult&)> callback) {
    pimpl_->callback = callback;
}

void ValidationDecorator::setShowMessages(bool show) {
    pimpl_->showMessages = show;
    if (!show) {
        hideTooltip();
    }
}

bool ValidationDecorator::getShowMessages() const {
    return pimpl_->showMessages;
}

ValidatableComponent* ValidationDecorator::getDecoratedComponent() const {
    return pimpl_->component;
}

void ValidationDecorator::setContentMargin(int margin) {
    pimpl_->contentMargin = juce::jmax(0, margin);
    resized();
}

int ValidationDecorator::getContentMargin() const {
    return pimpl_->contentMargin;
}

void ValidationDecorator::updateValidationState() {
    if (pimpl_->autoValidate) {
        validate();
    }
}

void ValidationDecorator::drawValidationOutline(juce::Graphics& g) {
    if (!pimpl_->component || !pimpl_->lastResult.hasMessages()) {
        return;
    }

    auto bounds = getLocalBounds().toFloat();
    bounds = bounds.reduced(pimpl_->contentMargin);

    // Calculate outline width with animation
    float baseWidth = 2.0f;
    float animatedWidth = baseWidth;

    if (pimpl_->isAnimating) {
        animatedWidth = baseWidth + (2.0f * pimpl_->animationProgress);
    }

    // Set color based on validation state
    juce::Colour outlineColor;
    if (pimpl_->lastResult.isFailure()) {
        outlineColor = ZenithDesignSystem::getColor(ZenithDesignSystem::Colors::Error);
    } else if (pimpl_->lastResult.isWarning()) {
        outlineColor = ZenithDesignSystem::getColor(ZenithDesignSystem::Colors::Warning);
    } else {
        outlineColor = ZenithDesignSystem::getColor(ZenithDesignSystem::Colors::Success);
    }

    // Draw outline
    g.setColour(outlineColor);
    g.drawRoundedRectangle(bounds, 4.0f, animatedWidth);
}

void ValidationDecorator::drawValidationBackground(juce::Graphics& g) {
    if (!pimpl_->component || !pimpl_->lastResult.hasMessages()) {
        return;
    }

    auto bounds = getLocalBounds().toFloat();
    bounds = bounds.reduced(pimpl_->contentMargin);

    // Set background color with transparency
    juce::Colour bgColor;
    if (pimpl_->lastResult.isFailure()) {
        bgColor = ZenithDesignSystem::getColor(ZenithDesignSystem::Colors::Error).withAlpha(0.2f);
    } else if (pimpl_->lastResult.isWarning()) {
        bgColor = ZenithDesignSystem::getColor(ZenithDesignSystem::Colors::Warning).withAlpha(0.2f);
    } else {
        bgColor = ZenithDesignSystem::getColor(ZenithDesignSystem::Colors::Success).withAlpha(0.2f);
    }

    // Draw background
    g.setColour(bgColor);
    g.fillRoundedRectangle(bounds, 4.0f);
}

void ValidationDecorator::drawValidationIcon(juce::Graphics& g) {
    if (!pimpl_->component || !pimpl_->lastResult.hasMessages()) {
        return;
    }

    auto bounds = getLocalBounds().toFloat();
    auto iconSize = 16.0f;
    auto iconBounds = bounds.withTrimmedRight(bounds.getWidth() - iconSize - 8)
                           .withTrimmedTop(bounds.getHeight() - iconSize - 8)
                           .withSizeKeepingCentre(iconSize, iconSize);

    // Set color
    juce::Colour iconColor;
    if (pimpl_->lastResult.isFailure()) {
        iconColor = ZenithDesignSystem::getColor(ZenithDesignSystem::Colors::Error);
    } else if (pimpl_->lastResult.isWarning()) {
        iconColor = ZenithDesignSystem::getColor(ZenithDesignSystem::Colors::Warning);
    } else {
        iconColor = ZenithDesignSystem::getColor(ZenithDesignSystem::Colors::Success);
    }

    g.setColour(iconColor);

    // Draw icon (simple circle with mark)
    g.fillEllipse(iconBounds);
    g.setColour(juce::Colours::white);

    if (pimpl_->lastResult.isFailure()) {
        // X for error
        g.drawLine(iconBounds.getX() + 3, iconBounds.getY() + 3,
                  iconBounds.getRight() - 3, iconBounds.getBottom() - 3, 2);
        g.drawLine(iconBounds.getRight() - 3, iconBounds.getY() + 3,
                  iconBounds.getX() + 3, iconBounds.getBottom() - 3, 2);
    } else if (pimpl_->lastResult.isWarning()) {
        // ! for warning
        g.drawLine(iconBounds.getCentreX(), iconBounds.getY() + 2,
                  iconBounds.getCentreX(), iconBounds.getCentreY(), 2);
        g.fillEllipse(iconBounds.getCentreX() - 1, iconBounds.getCentreY(), 2, 2);
    } else {
        // ✓ for success
        auto path = juce::Path();
        path.startNewSubPath(iconBounds.getX() + 4, iconBounds.getCentreY());
        path.lineTo(iconBounds.getCentreX() - 2, iconBounds.getBottom() - 4);
        path.lineTo(iconBounds.getRight() - 4, iconBounds.getY() + 4);
        g.strokePath(path, juce::PathStrokeType(2.0f));
    }
}

void ValidationDecorator::drawValidationTooltip(juce::Graphics& g) {
    // Tooltip drawing is handled by juce::TooltipWindow
}

void ValidationDecorator::updateTooltipPosition() {
    // Tooltip position is handled by juce::TooltipWindow
}

void ValidationDecorator::showTooltip(const ValidationResult& result) {
    if (!pimpl_->showMessages || !result.hasMessages()) {
        return;
    }

    // Determine tooltip message
    juce::String message;
    if (result.isFailure()) {
        message = "Error: " + result.errorMessage;
    } else if (result.isWarning()) {
        message = "Warning: " + result.warningMessage;
    }

    // Show tooltip at component position
    auto* tooltipParent = getParentComponent();
    if (tooltipParent) {
        auto screenPos = getScreenPosition();
        pimpl_->tooltipWindow.displayTooltip(message, screenPos + juce::Point<int>(0, -30));
    }
}

void ValidationDecorator::hideTooltip() {
    pimpl_->tooltipWindow.hideTip();
}

void ValidationDecorator::startValidationAnimation() {
    pimpl_->startAnimation();
    repaint();
}

void ValidationDecorator::stopValidationAnimation() {
    pimpl_->isAnimating = false;
    repaint();
}

void ValidationDecorator::updateAnimation() {
    if (pimpl_->isAnimating) {
        pimpl_->updateAnimation();
        repaint();
    }
}

void ValidationDecorator::handleComponentChanged() {
    updateValidationState();
}

void ValidationDecorator::handleValidationComplete(const ValidationResult& result) {
    // Update state
    pimpl_->lastResult = result;
    repaint();

    // Show tooltip if needed
    if (pimpl_->showMessages) {
        showTooltip(result);
    }

    // Fire callback
    if (pimpl_->callback) {
        pimpl_->callback(result);
    }

    // Fire validation complete event
    sendChangeMessage();
}

// Factory implementation
ValidationDecorator* ValidationDecoratorFactory::create(ValidatableComponent* component,
                                                       Validator* validator,
                                                       ValidationVisualStyle style) {
    auto decorator = new ValidationDecorator(component, style);
    if (validator) {
        decorator->setValidator(validator);
    }
    return decorator;
}

ValidationDecorator* ValidationDecoratorFactory::createWithSettings(ValidatableComponent* component,
                                                                Validator* validator,
                                                                ValidationVisualStyle style,
                                                                bool autoValidate,
                                                                bool showMessages) {
    auto decorator = new ValidationDecorator(component, style);
    decorator->setValidator(validator);
    decorator->setAutoValidate(autoValidate);
    decorator->setShowMessages(showMessages);
    return decorator;
}

} // namespace Zenith::UI