/*
  UIFrameworkTest.cpp

  Comprehensive test suite for UI Framework components

  Tests all new framework components including:
  - Error handling system
  - Validation framework
  - Toast notifications
  - Keyboard navigation
  - Tab order management
  - Keyboard shortcuts
*/

#include "UIErrorHandler.h"
#include "ToastNotificationManager.h"
#include "TabOrderManager.h"
#include "KeyboardShortcutManager.h"
#include "Validator.h"
#include "ValidationDecorator.h"
#include "SkiaComponent.h"
#include "../../core/utils/PlatformLogUtils.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <cassert>
#include <iostream>

namespace Zenith::UI {

/**
 * Test suite for UI Framework components
 */
class UIFrameworkTest {
public:
    UIFrameworkTest() {
        std::cout << "=== UI Framework Test Suite ===" << std::endl;
    }

    ~UIFrameworkTest() {
        std::cout << "=== Test Suite Completed ===" << std::endl;
    }

    void runAllTests() {
        testErrorHandler();
        testToastNotifications();
        testValidatorFramework();
        testValidationDecorator();
        testTabOrderManager();
        testKeyboardShortcutManager();
        testKeyboardIntegration();

        std::cout << "All tests passed! ✅" << std::endl;
    }

private:
    void testErrorHandler() {
        std::cout << "Testing ErrorHandler..." << std::endl;

        auto& errorHandler = UIErrorHandler::getInstance();

        // Test error reporting
        errorHandler.reportError(UIError::createUIError("Test error", "Details"));
        errorHandler.reportError(UISeverity::Warning, ErrorCategory::File, "Warning message");
        errorHandler.reportError(UISeverity::Critical, ErrorCategory::Audio, "Critical error");

        assert(errorHandler.getErrorCount() == 3);
        assert(errorHandler.hasErrors());

        // Test error statistics
        auto stats = errorHandler.getStatistics();
        assert(stats.total == 3);
        assert(stats.errors >= 1);

        // Test error clearing
        errorHandler.clearErrors(ErrorCategory::UI);
        assert(errorHandler.getErrorCount() == 2); // Still have file and audio errors

        errorHandler.clearAllErrors();
        assert(errorHandler.getErrorCount() == 0);

        std::cout << "✅ ErrorHandler tests passed" << std::endl;
    }

    void testToastNotifications() {
        std::cout << "Testing Toast Notifications..." << std::endl;

        auto& toastManager = ToastNotificationManager::getInstance();

        // Test basic toast creation
        toastManager.showSuccess("Success message");
        toastManager.showWarning("Warning message");
        toastManager.showError("Error message");
        toastManager.showInfo("Info message");

        assert(toastManager.hasActiveToasts());
        assert(toastManager.getActiveToastCount() >= 4);

        // Test toast with action
        bool actionTriggered = false;
        toastManager.showToast("Click me",
            [&actionTriggered]() { actionTriggered = true; },
            ToastType::Info, "OK", 2000);

        // Test position setting
        toastManager.setDefaultPosition(ToastPosition::TopCenter);
        toastManager.setToastMargin(20);

        // Clear all toasts
        toastManager.clearAll();
        assert(!toastManager.hasActiveToasts());

        std::cout << "✅ Toast Notification tests passed" << std::endl;
    }

    void testValidatorFramework() {
        std::cout << "Testing Validator Framework..." << std::endl;

        // Test required validator
        auto* requiredValidator = ValidatorFactory::createRequiredValidator("Email");
        requiredValidator->setValue("");
        auto result = requiredValidator->validate();
        assert(!result.isValid);
        assert(result.errorMessage == "Email is required");

        // Test email validator
        auto* emailValidator = ValidatorFactory::createEmailValidator("Email");
        emailValidator->setValue("invalid-email");
        result = emailValidator->validate();
        assert(!result.isValid);
        assert(result.errorMessage.contains("invalid"));

        emailValidator->setValue("valid@example.com");
        result = emailValidator->validate();
        assert(result.isValid);

        // Test range validator
        auto* rangeValidator = ValidatorFactory::createRangeValidator(1, 100, "Age");
        rangeValidator->setValue(50);
        result = rangeValidator->validate();
        assert(result.isValid);

        rangeValidator->setValue(150);
        result = rangeValidator->validate();
        assert(!result.isValid);
        assert(result.errorMessage.contains("between 1 and 100"));

        std::cout << "✅ Validator Framework tests passed" << std::endl;
    }

    void testValidationDecorator() {
        std::cout << "Testing Validation Decorator..." << std::endl;

        // Create a mock validatable component
        class MockValidatableComponent : public ValidatableComponent {
        public:
            ValidationResult validate() override {
                return ValidationResult::success();
            }
            Validator* getValidator() override { return nullptr; }
            void setValidator(Validator* validator) override {}
            void setAutoValidate(bool autoValidate) override {}
            bool getAutoValidate() const override { return true; }
            ValidationResult getLastValidationResult() const override { return ValidationResult::success(); }
            void clearValidation() override {}
            void setValidationCallback(std::function<void(const ValidationResult&)> callback) override {}
        };

        auto* component = new MockValidatableComponent();
        auto* decorator = ValidationDecoratorFactory::create(component, nullptr, ValidationVisualStyle::Outline);

        // Test validation state changes
        decorator->validate();
        assert(decorator->getValidationResult().isValid);

        // Test validation style changes
        decorator->setValidationStyle(ValidationVisualStyle::Background);
        assert(decorator->getValidationStyle() == ValidationVisualStyle::Background);

        // Test auto-validation
        decorator->setAutoValidate(false);
        assert(!decorator->getAutoValidate());

        delete component;
        delete decorator;

        std::cout << "✅ Validation Decorator tests passed" << std::endl;
    }

    void testTabOrderManager() {
        std::cout << "Testing Tab Order Manager..." << std::endl;

        auto& tabManager = TabOrderManager::getInstance();

        // Create mock components
        class MockSkiaComponent : public SkiaComponent {
        public:
            MockSkiaComponent(const juce::String& name) { setName(name); }
        };

        auto* comp1 = new MockSkiaComponent("Component1");
        auto* comp2 = new MockSkiaComponent("Component2");
        auto* comp3 = new MockSkiaComponent("Component3");

        // Register components
        tabManager.registerComponent(comp1, TabGroup::Main, 0, "comp1");
        tabManager.registerComponent(comp2, TabGroup::Main, 1, "comp2");
        tabManager.registerComponent(comp3, TabGroup::Main, 2, "comp3");

        // Test navigation
        assert(tabManager.focusFirstInGroup(TabGroup::Main));
        assert(tabManager.getCurrentFocus() == comp1);

        assert(tabManager.focusNext());
        assert(tabManager.getCurrentFocus() == comp2);

        assert(tabManager.focusPrevious());
        assert(tabManager.getCurrentFocus() == comp1);

        assert(tabManager.focusComponentById("comp3"));
        assert(tabManager.getCurrentFocus() == comp3);

        // Test skipping
        tabManager.setComponentSkipped(comp2, true);
        assert(tabManager.focusNext()); // Should skip comp2
        assert(tabManager.getCurrentFocus() == comp3);

        // Clean up
        delete comp1;
        delete comp2;
        delete comp3;
        tabManager.reset();

        std::cout << "✅ Tab Order Manager tests passed" << std::endl;
    }

    void testKeyboardShortcutManager() {
        std::cout << "Testing Keyboard Shortcut Manager..." << std::endl;

        auto& shortcutManager = KeyboardShortcutManager::getInstance();

        // Create context
        auto* context = shortcutManager.createContext("Test Context");

        // Test shortcut registration
        KeyboardShortcut shortcut;
        shortcut.key = juce::KeyPress('s', juce::ModifierKeys::commandModifier, 0);
        shortcut.name = "Save";
        shortcut.description = "Save project";
        shortcut.category = ShortcutCategory::File;
        shortcut.enabled = true;
        shortcut.action = []() { std::cout << "Save triggered!" << std::endl; };

        shortcutManager.registerShortcut(context, shortcut);

        // Test shortcut execution
        assert(shortcutManager.executeShortcut(shortcut.key));

        // Test conflict detection
        KeyboardShortcut conflictingShortcut;
        conflictingShortcut.key = juce::KeyPress('s', juce::ModifierKeys::commandModifier, 0);
        conflictingShortcut.name = "Save As";
        conflictingShortcut.description = "Save project as";
        conflictingShortcut.category = ShortcutCategory::File;

        shortcutManager.registerShortcut(context, conflictingShortcut);
        assert(shortcutManager.hasConflicts());

        // Test conflict resolution
        shortcutManager.resolveConflicts();

        // Clean up
        shortcutManager.removeContext(context);

        std::cout << "✅ Keyboard Shortcut Manager tests passed" << std::endl;
    }

    void testKeyboardIntegration() {
        std::cout << "Testing Keyboard Integration..." << std::endl;

        class TestSkiaComponent : public SkiaComponent {
        public:
            TestSkiaComponent() {
                setTabGroup(TabGroup::Main);
                setTabOrder(0);
            }

            bool handleKeyPress(const juce::KeyPress& key) override {
                return SkiaComponent::handleKeyPress(key);
            }
        };

        auto* component = new TestSkiaComponent();

        // Test tab group and order
        assert(component->getTabGroup() == TabGroup::Main);
        assert(component->getTabOrder() == 0);

        // Test shortcut registration
        KeyboardShortcut shortcut;
        shortcut.key = juce::KeyPress('x', juce::ModifierKeys::commandModifier, 0);
        shortcut.name = "Test Action";
        shortcut.description = "Test shortcut";
        shortcut.action = []() { std::cout << "Test action triggered!" << std::endl; };

        component->registerShortcut(shortcut);
        assert(component->handleKeyPress(shortcut.key));

        // Test focus management
        component->grabFocusWithReason(juce::Component::focusChangedByKeyboard);
        assert(UI::hasFocus(component));

        delete component;

        std::cout << "✅ Keyboard Integration tests passed" << std::endl;
    }
};

// Main test function
void runUIFrameworkTests() {
    UIFrameworkTest test;
    test.runAllTests();
}

} // namespace Zenith::UI

// Entry point for testing
#ifdef UI_FRAMEWORK_TESTING
int main() {
    std::cout << "Running UI Framework Tests..." << std::endl;

    try {
        Zenith::UI::runUIFrameworkTests();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
}
#endif