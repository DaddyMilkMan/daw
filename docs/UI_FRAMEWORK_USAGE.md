# UI Framework Usage Guide

This guide provides comprehensive instructions on using the new UI framework components in Zenith DAW.

## Table of Contents

1. [Error Handling System](#error-handling-system)
2. [Validation Framework](#validation-framework)
3. [Toast Notifications](#toast-notifications)
4. [Keyboard Navigation](#keyboard-navigation)
5. [Keyboard Shortcuts](#keyboard-shortcuts)
6. [Integration Guide](#integration-guide)
7. [Best Practices](#best-practices)

## Error Handling System

The `UIErrorHandler` provides centralized error management for all UI components.

### Basic Usage

```cpp
#include "ui/framework/UIErrorHandler.h"

// Report an error
UIErrorHandler::getInstance().reportError(
    UIError::createUIError("Failed to load file", "File not found")
);

// Report with severity
UIErrorHandler::getInstance().reportError(
    UISeverity::Critical, ErrorCategory::File,
    "Critical error", "Cannot continue"
);

// Quick error reporting
reportUIError("Network connection failed");
```

### Error Categories

- **UI**: Component rendering, interaction errors
- **Audio**: Audio processing errors
- **Plugin**: Plugin loading/execution errors
- **File**: File I/O errors
- **Network**: Network communication errors
- **Database**: Project data errors
- **System**: System resource errors
- **Validation**: Input validation errors

### Error Callbacks

```cpp
// Set up error callbacks
UIErrorHandler::getInstance().onErrorReported([](const UIError& error) {
    // Handle error reporting
    std::cout << "Error: " << error.message << std::endl;
});

UIErrorHandler::getInstance().onErrorCleared([]() {
    // Handle error clearing
    std::cout << "Errors cleared" << std::endl;
});
```

## Validation Framework

The validation system provides immediate visual feedback for user input validation.

### Basic Validators

```cpp
#include "ui/validation/Validator.h"

// Required field validation
auto* requiredValidator = ValidatorFactory::createRequiredValidator("Email");

// Email validation
auto* emailValidator = ValidatorFactory::createEmailValidator("Email");

// Length validation
auto* lengthValidator = ValidatorFactory::createLengthValidator(5, 50, "Password");

// Range validation
auto* rangeValidator = ValidatorFactory::createRangeValidator(0, 100, "Volume");

// Regex validation
auto* regexValidator = ValidatorFactory::createRegexValidator(
    "^[A-Za-z0-9]+$", "Username"
);
```

### Using Validators with Components

```cpp
// With ZenithTextInput
auto* input = new ZenithTextInput("Email");
auto* emailValidator = ValidatorFactory::createEmailValidator("Email");
input->setValidator(emailValidator);

// Manual validation
auto result = input->validate();
if (!result.isValid) {
    showError(result.errorMessage);
}
```

### Validation Decorator

```cpp
#include "ui/validation/ValidationDecorator.h"

// Wrap any component with validation
auto* component = new MyComponent();
auto* decorator = ValidationDecoratorFactory::create(component, validator, ValidationVisualStyle::Outline);

// Configure decorator
decorator->setAutoValidate(true);
decorator->setShowMessages(true);

// Set custom validation callback
decorator->setValidationCallback([](const ValidationResult& result) {
    if (result.isFailure()) {
        showToast("Validation failed: " + result.errorMessage);
    }
});
```

## Toast Notifications

Toast notifications provide non-intrusive user feedback.

### Basic Usage

```cpp
#include "ui/controls/ToastNotificationManager.h"

// Show different types of toasts
ToastNotificationManager::getInstance().showSuccess("File saved successfully");
ToastNotificationManager::getInstance().showWarning("Changes will be lost");
ToastNotificationManager::getInstance().showError("Failed to load project");
ToastNotificationManager::getInstance().showInfo("Loading project...");

// Quick helpers
showSuccess("Operation completed");
showWarning("Check your input");
showError("Critical error");
showInfo("Information message");
```

### Custom Toasts

```cpp
// With action button
ToastNotificationManager::getInstance().showToast(
    "Do you want to save changes?",
    []() {
        // Save action
        saveProject();
    },
    ToastType::Info,
    "Save",
    5000 // 5 seconds
);

// With custom positioning
ToastNotificationManager::getInstance().setDefaultPosition(ToastPosition::TopLeft);
ToastNotificationManager::getInstance().setToastMargin(20);
```

## Keyboard Navigation

The `TabOrderManager` manages keyboard tab navigation.

### Basic Usage

```cpp
#include "ui/framework/TabOrderManager.h"

// Register components
auto& tabManager = TabOrderManager::getInstance();

tabManager.registerComponent(button1, TabGroup::Main, 0, "button1");
tabManager.registerComponent(button2, TabGroup::Main, 1, "button2");
tabManager.registerComponent(button3, TabGroup::Main, 2, "button3");

// Navigate
tabManager.focusNext(TabGroup::Main);
tabManager.focusPrevious(TabGroup::Main);

// Focus specific component
tabManager.focusComponentById("button2");

// Group-based navigation
tabManager.focusFirstInGroup(TabGroup::Settings);
tabManager.focusLastInGroup(TabGroup::Settings);
```

### Tab Groups

- **Main**: Main application navigation
- **Modal**: Modal dialogs
- **Toolbar**: Toolbar controls
- **Settings**: Settings panels
- **Transport**: Transport controls
- **Mixer**: Mixer controls
- **PianoRoll**: Piano roll interface
- **Arranger**: Arranger interface

### TabOrderScope RAII

```cpp
// Create temporary tab order context
TabOrderScope scope(tabManager);

// Add components to scope
scope.addComponent(button1, TabGroup::Modal, 0, "modalButton1");
scope.addComponent(button2, TabGroup::Modal, 1, "modalButton2");

// Navigate within scope
scope.next();
scope.previous();

// Scope automatically cleans up when destroyed
```

## Keyboard Shortcuts

The `KeyboardShortcutManager` manages global and context-specific keyboard shortcuts.

### Basic Usage

```cpp
#include "ui/framework/KeyboardShortcutManager.h"

auto& shortcutManager = KeyboardShortcutManager::getInstance();

// Register global shortcut
KeyboardShortcut shortcut;
shortcut.key = juce::KeyPress('s', juce::ModifierKeys::commandModifier, 0);
shortcut.name = "Save";
shortcut.description = "Save project";
shortcut.category = ShortcutCategory::File;
shortcut.enabled = true;
shortcut.action = []() { saveProject(); };

shortcutManager.registerShortcut(shortcut);

// Execute shortcut
shortcutManager.executeShortcut(shortcut.key);

// Find shortcut
auto* found = shortcutManager.findShortcut(shortcut.key);
if (found) {
    found->action();
}
```

### Context-Specific Shortcuts

```cpp
// Create context
auto* projectContext = shortcutManager.createContext("Project");

// Register context-specific shortcut
KeyboardShortcut projectShortcut;
projectShortcut.key = juce::KeyPress('p', juce::ModifierKeys::commandModifier, 0);
projectShortcut.name = "Export Project";
projectShortcut.description = "Export project audio";
projectShortcut.category = ShortcutCategory::File;

shortcutManager.registerShortcut(projectContext, projectShortcut);

// Set active context
shortcutManager.setActiveContext(projectContext);
```

### Built-in Shortcuts

```cpp
#include "ui/framework/KeyboardShortcutManager.h"

// File operations
BuiltInShortcuts::SAVE_PROJECT.trigger();
BuiltInShortcuts::OPEN_PROJECT.trigger();

// Transport
BuiltInShortcuts::PLAY.trigger();
BuiltInShortcuts::STOP.trigger();
BuiltInShortcuts::RECORD.trigger();

// Navigation
BuiltInShortcuts::NAVIGATE_LEFT.trigger();
BuiltInShortcuts::NAVIGATE_RIGHT.trigger();
BuiltInShortcuts::PAGE_UP.trigger();
BuiltInShortcuts::PAGE_DOWN.trigger();
```

## Integration Guide

### Component Integration

All UI components should inherit from `SkiaComponent` and use the keyboard navigation system:

```cpp
class MyComponent : public SkiaComponent {
public:
    MyComponent() {
        // Set up keyboard navigation
        setTabGroup(TabGroup::Main);
        setTabOrder(0);
        setFocusable(true);

        // Register shortcuts
        registerKeyboardShortcuts();
    }

private:
    void registerKeyboardShortcuts() {
        KeyboardShortcut shortcut;
        shortcut.key = juce::KeyPress('k', juce::ModifierKeys::commandModifier, 0);
        shortcut.name = "Toggle Knobs";
        shortcut.description = "Toggle knob display";
        shortcut.action = [this]() { toggleKnobs(); };

        registerShortcut(shortcut);
    }

    void toggleKnobs() {
        // Implementation
    }
};
```

### Error Handling Integration

```cpp
void loadProject(const juce::String& path) {
    try {
        // Load project logic
        if (fileNotFound) {
            throw ValidationError(ValidationErrorCode::FileNotFound,
                                "Project file not found", path);
        }
    } catch (const ValidationError& e) {
        // Handle validation errors
        UIErrorHandler::getInstance().reportError(e);

        // Show toast notification
        ToastNotificationManager::getInstance().showError(
            e.getUserMessage(),
            [this]() { browseForProject(); }
        );
    } catch (const std::exception& e) {
        // Handle other errors
        reportFileError("Failed to load project", e.what());
    }
}
```

### Validation Integration

```cpp
class SettingsComponent : public SkiaComponent, public ValidatableComponent {
public:
    SettingsComponent() {
        // Set up validation
        auto* validator = ValidatorFactory::createRequiredValidator("Username");
        setValidator(validator);
    }

    ValidationResult validate() override {
        // Custom validation logic
        auto result = getValidator()->validate();

        if (!result.isFailure()) {
            // Additional validation
            if (username_.length() < 3) {
                return ValidationResult::failure("Username must be at least 3 characters");
            }
        }

        return result;
    }

    // ... other methods
};
```

## Best Practices

### Error Handling

1. **Use appropriate error severity levels**
   - Fatal for critical errors that stop the application
   - Critical for serious issues that impact functionality
   - Error for regular errors that should be addressed
   - Warning for non-critical issues
   - Info for user guidance

2. **Provide actionable error messages**
   - Tell users what went wrong
   - Suggest how to fix it
   - Provide context about the error

3. **Log errors for debugging**
   - Use the integrated logging system
   - Include error codes for tracking
   - Log timestamps for correlation

### Validation

1. **Validate early and often**
   - Validate on blur for immediate feedback
   - Validate on submit for final check
   - Use async validation for expensive operations

2. **Provide clear validation messages**
   - Use specific, actionable messages
   - Include field names for clarity
   - Show suggestions when possible

3. **Handle validation errors gracefully**
   - Don't block users from navigating away
   - Provide clear visual indicators
   - Allow users to correct errors easily

### Keyboard Navigation

1. **Implement logical tab order**
   - Follow the visual flow of the interface
   - Group related controls together
   - Skip non-interactive elements

2. **Provide keyboard shortcuts for common actions**
   - Use standard shortcuts when possible (Ctrl+S, Ctrl+O)
   - Make shortcuts discoverable via tooltips
   - Allow customization if needed

3. **Handle focus properly**
   - Draw clear focus indicators
   - Manage focus during dialog interactions
   - Return focus to logical destinations

### Performance Considerations

1. **Use dirty rectangles efficiently**
   - Mark only changed regions as dirty
   - Batch repaints when possible
   - Use partial updates for performance

2. **Optimize validation**
   - Debounce validation for expensive operations
   - Cache validation results when possible
   - Use async validation for network requests

3. **Manage memory efficiently**
   - Use RAII for resource management
   - Clean up shortcuts and validators properly
   - Avoid unnecessary copies of data

### Accessibility

1. **Follow WCAG guidelines**
   - Ensure sufficient color contrast
   - Provide alternative text for icons
   - Support screen readers where possible

2. **Provide keyboard access**
   - All interactive elements should be keyboard accessible
   - Provide keyboard shortcuts for common actions
   - Support arrow key navigation in complex controls

3. **Make interfaces usable**
   - Use clear, consistent naming
   - Provide helpful error messages
   - Ensure adequate spacing and sizing

## Troubleshooting

### Common Issues

1. **Shortcuts not working**
   - Check if shortcuts are enabled
   - Verify context is active
   - Check for conflicting shortcuts

2. **Validation not showing**
   - Ensure component is focusable
   - Check validation style settings
   - Verify auto-validation is enabled

3. **Toast notifications not appearing**
   - Check if toast manager is properly initialized
   - Verify parent component exists
   - Check positioning and visibility

### Debug Tips

1. **Enable debug logging**
   ```cpp
   UIErrorHandler::getInstance().setLogLevel(LogLevel::Debug);
   ```

2. **Use validation scopes for debugging**
   ```cpp
   ValidationScope scope;
   scope.addComponent(component);
   ```

3. **Test keyboard navigation**
   - Use Tab key to navigate through components
   - Verify focus indicators are visible
   - Check tab order is logical

This guide provides a comprehensive overview of the UI framework. For more specific information about each component, refer to the individual header files and implementation files.