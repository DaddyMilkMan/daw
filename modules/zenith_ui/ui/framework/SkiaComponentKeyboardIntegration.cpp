/*
  SkiaComponentKeyboardIntegration.cpp

  Keyboard navigation integration for SkiaComponent base class

  This file implements keyboard navigation support for all UI components
  derived from SkiaComponent.
*/

#include "SkiaComponent.h"
#include "TabOrderManager.h"
#include "KeyboardShortcutManager.h"
#include "UIErrorHandler.h"
#include "../../core/utils/PlatformLogUtils.h"

namespace zenith {

// Tab order manager instance
static SkiaComponent* lastFocusedComponent = nullptr;

// Focus change tracking
static std::map<SkiaComponent*, juce::Timestamp> lastFocusTime;

void SkiaComponent::setTabGroup(zenith::UI::TabGroup group) {
    tabGroup_ = group;

    // Register with tab order manager
    auto& tabManager = zenith::UI::TabOrderManager::getInstance();
    tabManager.registerComponent(this, group, tabOrder_, getName());
}

zenith::UI::TabGroup SkiaComponent::getTabGroup() const {
    return tabGroup_;
}

void SkiaComponent::setTabOrder(int order) {
    tabOrder_ = order;

    // Update with tab order manager
    auto& tabManager = zenith::UI::TabOrderManager::getInstance();
    tabManager.setFocusOrder(this, order);
}

int SkiaComponent::getTabOrder() const {
    return tabOrder_;
}

void SkiaComponent::setFocusable(bool focusable) {
    isFocusable_ = focusable;

    // Update with tab order manager
    auto& tabManager = zenith::UI::TabOrderManager::getInstance();
    tabManager.setComponentSkipped(this, !focusable);
}

bool SkiaComponent::isFocusable() const {
    return isFocusable_;
}

bool SkiaComponent::isInTabOrder() const {
    return isInTabOrder_;
}

void SkiaComponent::registerShortcut(const zenith::UI::KeyboardShortcut& shortcut) {
    if (!shortcut.action) {
        zenLogWarning("Cannot register shortcut with no action: " + shortcut.name);
        return;
    }

    // Create shortcut context if not exists
    if (!shortcutContext_) {
        shortcutContext_ = zenith::UI::KeyboardShortcutManager::getInstance()
            .createContext(getName() + " Shortcuts");
    }

    // Register shortcut
    registeredShortcuts_[shortcut.key] = shortcut;

    auto& shortcutManager = zenith::UI::KeyboardShortcutManager::getInstance();
    shortcutManager.registerShortcut(shortcutContext_.get(), shortcut);

    zenLogInfo("Registered shortcut: " + zenith::UI::ShortcutUtils::formatKeyForDisplay(shortcut.key) +
               " -> " + shortcut.name);
}

void SkiaComponent::unregisterShortcut(const juce::KeyPress& key) {
    auto it = registeredShortcuts_.find(key);
    if (it != registeredShortcuts_.end()) {
        registeredShortcuts_.erase(it);

        auto& shortcutManager = zenith::UI::KeyboardShortcutManager::getInstance();
        shortcutManager.unregisterShortcut(key);

        zenLogInfo("Unregistered shortcut: " + zenith::UI::ShortcutUtils::formatKeyForDisplay(key));
    }
}

bool SkiaComponent::handleKeyPress(const juce::KeyPress& key) {
    // Check registered shortcuts first
    auto shortcutIt = registeredShortcuts_.find(key);
    if (shortcutIt != registeredShortcuts_.end()) {
        const auto& shortcut = shortcutIt->second;

        if (shortcut.enabled && shortcut.action) {
            try {
                shortcut.action();
                return true;
            } catch (const std::exception& e) {
                zenith::UI::ValidationErrorHandler::handleError(
                    zenith::UI::ValidationError(
                        zenith::UI::ValidationErrorCode::CustomValidationFailed,
                        e.what(), "Shortcut Action"));
                return true;
            }
        }
    }

    // Handle navigation keys
    switch (key.getKeyCode()) {
        case juce::KeyPress::tabKey: {
            if (key.getModifiers().isShiftDown()) {
                return requestFocusPrevious();
            } else {
                return requestFocusNext();
            }
        }

        case juce::KeyPress::upKey:
        case juce::KeyPress::downKey:
        case juce::KeyPress::leftKey:
        case juce::KeyPress::rightKey: {
            // Handle directional navigation
            return handleDirectionalKey(key);
        }

        case juce::KeyPress::returnKey:
        case juce::KeyPress::enterKey: {
            onEnterPressed();
            return true;
        }

        case juce::KeyPress::escapeKey: {
            onEscapePressed();
            return true;
        }

        default:
            return false;
    }
}

void SkiaComponent::grabFocusWithReason(juce::Component::FocusChangeType reason) {
    if (isFocusable_) {
        juce::Component::grabKeyboardFocus();
        lastFocusedComponent = this;
        lastFocusTime[this] = juce::Time::getCurrentTime();

        // Trigger focus event
        focusGained(reason);

        // Draw focus indicator
        repaint();
    }
}

bool SkiaComponent::requestFocusNext() {
    if (!isFocusable_ || !isInTabOrder_) {
        return false;
    }

    auto& tabManager = zenith::UI::TabOrderManager::getInstance();
    return tabManager.focusNext(tabGroup_);
}

bool SkiaComponent::requestFocusPrevious() {
    if (!isFocusable_ || !isInTabOrder_) {
        return false;
    }

    auto& tabManager = zenith::UI::TabOrderManager::getInstance();
    return tabManager.focusPrevious(tabGroup_);
}

void SkiaComponent::mouseEnter(const juce::MouseEvent& e) {
    isHovered_ = true;
    markDirty();
}

void SkiaComponent::mouseExit(const juce::MouseEvent& e) {
    isHovered_ = false;
    markDirty();
}

void SkiaComponent::focusGained(juce::Component::FocusChangeType cause) {
    // Update last focused component
    lastFocusedComponent = this;
    lastFocusTime[this] = juce::Time::getCurrentTime();

    // Trigger focus event
    if (cause == juce::Component::focusChangedByMouse) {
        // Mouse focus - don't draw keyboard focus indicator
    } else {
        // Keyboard focus - draw focus indicator
        markDirty();
    }

    // Log focus gain
    if (getName().isNotEmpty()) {
        zenLogInfo("Focus gained: " + getName().toStdString());
    }
}

void SkiaComponent::focusLost(juce::Component::FocusChangeType cause) {
    // Clear focus state
    if (cause == juce::Component::focusChangedByMouse) {
        // Mouse focus - only clear if another component didn't gain focus
        if (lastFocusedComponent == this) {
            lastFocusedComponent = nullptr;
        }
    } else {
        // Keyboard focus - clear focus indicator
        markDirty();
    }

    // Log focus loss
    if (getName().isNotEmpty()) {
        zenLogInfo("Focus lost: " + getName().toStdString());
    }
}

bool SkiaComponent::keyPressed(const juce::KeyPress& key, juce::Component* origin) {
    // Handle custom key press
    if (handleKeyPress(key)) {
        return true;
    }

    // Default key handling
    return juce::Component::keyPressed(key);
}

bool SkiaComponent::handleDirectionalKey(const juce::KeyPress& key) {
    // Default directional navigation
    // In a real implementation, this would handle navigation within containers
    switch (key.getKeyCode()) {
        case juce::KeyPress::upKey:
            // Navigate up
            return requestFocusPrevious();

        case juce::KeyPress::downKey:
            // Navigate down
            return requestFocusNext();

        case juce::KeyPress::leftKey:
            // Navigate left
            return requestFocusPrevious();

        case juce::KeyPress::rightKey:
            // Navigate right
            return requestFocusNext();

        default:
            return false;
    }
}

// Static helper functions
namespace UI {

SkiaComponent* getLastFocusedComponent() {
    return lastFocusedComponent;
}

juce::Timestamp getLastFocusTime(SkiaComponent* component) {
    auto it = lastFocusTime.find(component);
    return it != lastFocusTime.end() ? it->second : juce::Timestamp();
}

bool hasFocus(SkiaComponent* component) {
    return lastFocusedComponent == component;
}

void setFocus(SkiaComponent* component) {
    if (component && component->isFocusable()) {
        component->grabFocusWithReason(juce::Component::focusChangedByKeyboard);
    }
}

} // namespace UI

} // namespace zenith