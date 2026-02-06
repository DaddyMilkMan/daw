/*
  KeyboardShortcutManager.cpp

  Implementation of global keyboard shortcut system
*/

#include "KeyboardShortcutManager.h"
#include "TabOrderManager.h"
#include <zenith_core/utils/PlatformLogUtils.h>

namespace zenith::UI {

// Private implementation structure
struct KeyboardShortcutManager::Impl {
    std::map<juce::KeyPress, KeyboardShortcut> globalShortcuts;
    std::map<juce::KeyPress, KeyboardShortcut> contextShortcuts;
    std::vector<std::unique_ptr<ShortcutContext>> contexts;
    ShortcutContext* activeContext;
    bool showHints;
    std::function<juce::String(const juce::KeyPress&)> hintProvider;

    Impl()
        : activeContext(nullptr)
        , showHints(false) {
    }

    ~Impl() {
        contexts.clear();
        globalShortcuts.clear();
        contextShortcuts.clear();
    }

    void addShortcut(const KeyboardShortcut& shortcut, bool isGlobal = true) {
        if (shortcut.enabled) {
            if (isGlobal) {
                globalShortcuts[shortcut.key] = shortcut;
            } else {
                contextShortcuts[shortcut.key] = shortcut;
            }
        }
    }

    void removeShortcut(const juce::KeyPress& key, bool isGlobal = true) {
        if (isGlobal) {
            globalShortcuts.erase(key);
        } else {
            contextShortcuts.erase(key);
        }
    }

    KeyboardShortcut* findShortcut(const juce::KeyPress& key) {
        // Check context shortcuts first
        auto contextIt = contextShortcuts.find(key);
        if (contextIt != contextShortcuts.end()) {
            return &contextIt->second;
        }

        // Then global shortcuts
        auto globalIt = globalShortcuts.find(key);
        if (globalIt != globalShortcuts.end()) {
            return &globalIt->second;
        }

        return nullptr;
    }

    bool canExecute(const KeyboardShortcut& shortcut) const {
        if (!shortcut.enabled) return false;
        if (shortcut.context && !shortcut.context->isActive()) return false;
        if (shortcut.canExecute && !shortcut.canExecute()) return false;
        return true;
    }

    std::vector<std::pair<juce::KeyPress, std::vector<juce::String>>> getConflicts() const {
        std::map<juce::KeyPress, std::vector<juce::String>> conflictMap;

        // Check for conflicts between global and context shortcuts
        for (const auto& pair : globalShortcuts) {
            const auto& key = pair.first;
            const auto& globalShortcut = pair.second;

            // Check against context shortcuts
            auto contextIt = contextShortcuts.find(key);
            if (contextIt != contextShortcuts.end()) {
                conflictMap[key].push_back("Global: " + globalShortcut.name);
                conflictMap[key].push_back("Context: " + contextIt->second.name);
            }

            // Check other global shortcuts
            for (const auto& otherPair : globalShortcuts) {
                if (otherPair.first != key && otherPair.first.getKeyCode() == key.getKeyCode() &&
                    otherPair.first.getModifiers() == key.getModifiers()) {
                    conflictMap[key].push_back("Global: " + globalShortcut.name);
                    conflictMap[key].push_back("Global: " + otherPair.second.name);
                }
            }
        }

        // Check context shortcuts against each other
        for (const auto& pair : contextShortcuts) {
            const auto& key = pair.first;

            for (const auto& otherPair : contextShortcuts) {
                if (otherPair.first != key && otherPair.first.getKeyCode() == key.getKeyCode() &&
                    otherPair.first.getModifiers() == key.getModifiers()) {
                    if (std::find(conflictMap[key].begin(), conflictMap[key].end(),
                                 "Context: " + otherPair.second.name) == conflictMap[key].end()) {
                        conflictMap[key].push_back("Context: " + pair.second.name);
                        conflictMap[key].push_back("Context: " + otherPair.second.name);
                    }
                }
            }
        }

        return std::vector<std::pair<juce::KeyPress, std::vector<juce::String>>>(
            conflictMap.begin(), conflictMap.end());
    }
};

// ShortcutContext implementation
ShortcutContext::ShortcutContext(const juce::String& name)
    : name_(name)
    , active_(true)
    , enabled_(true) {}

ShortcutContext::~ShortcutContext() {}

juce::String ShortcutContext::getName() const {
    return name_;
}

void ShortcutContext::addShortcut(const KeyboardShortcut& shortcut) {
    shortcuts_.push_back(shortcut);
    shortcutKeys_.insert(shortcut.key);
}

void ShortcutContext::removeShortcut(const juce::KeyPress& key) {
    shortcuts_.erase(
        std::remove_if(shortcuts_.begin(), shortcuts_.end(),
            [&key](const KeyboardShortcut& s) {
                return s.key == key;
            }),
        shortcuts_.end());
    shortcutKeys_.erase(key);
}

bool ShortcutContext::hasShortcut(const juce::KeyPress& key) const {
    return shortcutKeys_.find(key) != shortcutKeys_.end();
}

std::vector<KeyboardShortcut> ShortcutContext::getShortcuts() const {
    return shortcuts_;
}

bool ShortcutContext::isActive() const {
    return active_ && enabled_;
}

void ShortcutContext::setActive(bool active) {
    active_ = active;
}

bool ShortcutContext::isEnabled() const {
    return enabled_;
}

void ShortcutContext::setEnabled(bool enabled) {
    enabled_ = enabled;
}

// KeyboardShortcutManager implementation
KeyboardShortcutManager& KeyboardShortcutManager::getInstance() {
    static KeyboardShortcutManager instance;
    return instance;
}

KeyboardShortcutManager::KeyboardShortcutManager() : pimpl_(new Impl()) {
    setName("KeyboardShortcutManager");
    setWantsKeyboardFocus(true);
    setInterceptsMouseClicks(false, false);
}

KeyboardShortcutManager::~KeyboardShortcutManager() {
    pimpl_.reset();
}

void KeyboardShortcutManager::registerShortcut(const KeyboardShortcut& shortcut) {
    pimpl_->addShortcut(shortcut, true);
}

void KeyboardShortcutManager::registerShortcut(ShortcutContext* context, const KeyboardShortcut& shortcut) {
    if (context) {
        context->addShortcut(shortcut);
        pimpl_->addShortcut(shortcut, false);
    }
}

void KeyboardShortcutManager::unregisterShortcut(const juce::KeyPress& key) {
    pimpl_->removeShortcut(key, true);

    // Remove from all contexts
    for (auto& ctx : pimpl_->contexts) {
        ctx->removeShortcut(key);
    }
}

void KeyboardShortcutManager::unregisterContextShortcuts(ShortcutContext* context) {
    if (context) {
        for (const auto& shortcut : context->getShortcuts()) {
            pimpl_->removeShortcut(shortcut.key, false);
        }
        context->setActive(false);
    }
}

ShortcutContext* KeyboardShortcutManager::createContext(const juce::String& name) {
    auto context = std::make_unique<ShortcutContext>(name);
    auto* result = context.get();
    pimpl_->contexts.push_back(std::move(context));
    return result;
}

void KeyboardShortcutManager::removeContext(ShortcutContext* context) {
    if (context) {
        unregisterContextShortcuts(context);
        pimpl_->contexts.erase(
            std::remove_if(pimpl_->contexts.begin(), pimpl_->contexts.end(),
                [context](const std::unique_ptr<ShortcutContext>& ctx) {
                    return ctx.get() == context;
                }),
            pimpl_->contexts.end());
    }
}

void KeyboardShortcutManager::setActiveContext(ShortcutContext* context) {
    pimpl_->activeContext = context;
}

ShortcutContext* KeyboardShortcutManager::getActiveContext() const {
    return pimpl_->activeContext;
}

KeyboardShortcut* KeyboardShortcutManager::findShortcut(const juce::KeyPress& key) {
    return pimpl_->findShortcut(key);
}

bool KeyboardShortcutManager::executeShortcut(const juce::KeyPress& key) {
    auto* shortcut = pimpl_->findShortcut(key);
    if (shortcut && pimpl_->canExecute(*shortcut)) {
        try {
            if (shortcut->action) {
                shortcut->action();
                return true;
            }
        } catch (const std::exception& e) {
            zenLogError("Error executing shortcut: " + std::string(e.what()));
        }
    }
    return false;
}

bool KeyboardShortcutManager::canExecuteShortcut(const KeyboardShortcut& shortcut) const {
    return pimpl_->canExecute(shortcut);
}

void KeyboardShortcutManager::showShortcutHints() {
    showShortcutHintsInternal();
}

void KeyboardShortcutManager::hideShortcutHints() {
    hideShortcutHintsInternal();
}

std::vector<KeyboardShortcut> KeyboardShortcutManager::getShortcutsByCategory(ShortcutCategory category) const {
    std::vector<KeyboardShortcut> result;

    // Add global shortcuts
    for (const auto& pair : pimpl_->globalShortcuts) {
        if (pair.second.category == category && pair.second.visible) {
            result.push_back(pair.second);
        }
    }

    // Add context shortcuts
    for (const auto& ctx : pimpl_->contexts) {
        for (const auto& shortcut : ctx->getShortcuts()) {
            if (shortcut.category == category && shortcut.visible) {
                result.push_back(shortcut);
            }
        }
    }

    return result;
}

std::vector<std::pair<juce::KeyPress, std::vector<juce::String>>> KeyboardShortcutManager::getConflicts() const {
    return pimpl_->getConflicts();
}

bool KeyboardShortcutManager::hasConflicts() const {
    return !pimpl_->getConflicts().empty();
}

void KeyboardShortcutManager::resolveConflicts() {
    auto conflicts = pimpl_->getConflicts();

    for (const auto& conflict : conflicts) {
        const auto& key = conflict.first;
        const auto& shortcuts = conflict.second;

        // Keep the first shortcut and disable others
        bool first = true;
        for (const auto& name : shortcuts) {
            if (first) {
                first = false;
            } else {
                // Disable conflicting shortcut
                for (auto& pair : pimpl_->globalShortcuts) {
                    if (pair.second.key == key && pair.second.name == name.substr(8)) { // Remove "Global: " prefix
                        pair.second.enabled = false;
                        break;
                    }
                }
            }
        }
    }
}

void KeyboardShortcutManager::setShortcutHintProvider(std::function<juce::String(const juce::KeyPress&)> provider) {
    pimpl_->hintProvider = provider;
}

void KeyboardShortcutManager::paint(juce::Graphics& g) {
    if (pimpl_->showHints) {
        drawShortcutOverlay(g);
    }
}

void KeyboardShortcutManager::resized() {
    // No special handling needed
}

void KeyboardShortcutManager::showShortcutHintsInternal() {
    pimpl_->showHints = true;
    repaint();
}

void KeyboardShortcutManager::hideShortcutHintsInternal() {
    pimpl_->showHints = false;
    repaint();
}

void KeyboardShortcutManager::drawShortcutOverlay(juce::Graphics& g) {
    auto bounds = getLocalBounds();

    // Draw semi-transparent background
    g.setColour(juce::Colours::black.withAlpha(0.7f));
    g.fillRectangle(bounds);

    // Draw shortcut hints
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(14.0f));

    int y = 30;
    int x = 20;

    // Group by category
    std::map<ShortcutCategory, std::vector<KeyboardShortcut>> categorized;

    for (const auto& pair : pimpl_->globalShortcuts) {
        if (pair.second.visible) {
            categorized[pair.second.category].push_back(pair.second);
        }
    }

    for (auto& ctx : pimpl_->contexts) {
        for (const auto& shortcut : ctx->getShortcuts()) {
            if (shortcut.visible) {
                categorized[shortcut.category].push_back(shortcut);
            }
        }
    }

    // Draw each category
    for (const auto& pair : categorized) {
        auto categoryName = getShortcutCategoryName(pair.first);
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(16.0f, juce::Font::bold));
        g.drawText(categoryName, x, y, 200, 20, juce::Justification::left);

        y += 25;

        g.setFont(juce::Font(14.0f));

        for (const auto& shortcut : pair.second) {
            auto keyText = ShortcutUtils::formatKeyForDisplay(shortcut.key);
            auto description = pimpl_->hintProvider ?
                pimpl_->hintProvider(shortcut.key) :
                shortcut.description;

            g.setColour(getShortcutColor(pair.first));
            g.drawText(keyText, x, y, 100, 20, juce::Justification::left);

            g.setColour(juce::Colours::white);
            g.drawText(description, x + 110, y, 300, 20, juce::Justification::left);

            y += 20;
        }

        y += 10;
    }
}

juce::String KeyboardShortcutManager::getShortcutDescription(const KeyboardShortcut& shortcut) const {
    if (pimpl_->hintProvider) {
        return pimpl_->hintProvider(shortcut.key);
    }
    return shortcut.description;
}

juce::Colour KeyboardShortcutManager::getShortcutColor(ShortcutCategory category) const {
    switch (category) {
        case ShortcutCategory::File:
            return ZenithDesignSystem::getColor(ZenithDesignSystem::Colors::Primary);
        case ShortcutCategory::Edit:
            return ZenithDesignSystem::getColor(ZenithDesignSystem::Colors::Warning);
        case ShortcutCategory::View:
            return ZenithDesignSystem::getColor(ZenithDesignSystem::Colors::Success);
        case ShortcutCategory::Transport:
            return juce::Colours::orange;
        case ShortcutCategory::Mixer:
            return juce::Colours::purple;
        case ShortcutCategory::PianoRoll:
            return juce::Colours::cyan;
        case ShortcutCategory::Arranger:
            return juce::Colours::magenta;
        case ShortcutCategory::Window:
            return juce::Colours::lightgrey;
        case ShortcutCategory::Help:
            return ZenithDesignSystem::getColor(ZenithDesignSystem::Colors::Info);
        default:
            return juce::Colours::white;
    }
}

bool KeyboardShortcutManager::keyPressed(const juce::KeyPress& key) {
    return executeShortcut(key);
}

juce::String KeyboardShortcutManager::getShortcutCategoryName(ShortcutCategory category) const {
    switch (category) {
        case ShortcutCategory::None: return "General";
        case ShortcutCategory::File: return "File";
        case ShortcutCategory::Edit: return "Edit";
        case ShortcutCategory::View: return "View";
        case ShortcutCategory::Transport: return "Transport";
        case ShortcutCategory::Mixer: return "Mixer";
        case ShortcutCategory::PianoRoll: return "Piano Roll";
        case ShortcutCategory::Arranger: return "Arranger";
        case ShortcutCategory::Window: return "Window";
        case ShortcutCategory::Help: return "Help";
        default: return "Unknown";
    }
}

// Built-in shortcuts implementation
namespace BuiltInShortcuts {
    const juce::KeyPress NEW_PROJECT = juce::KeyPress('n', juce::ModifierKeys::commandModifier, 0);
    const juce::KeyPress OPEN_PROJECT = juce::KeyPress('o', juce::ModifierKeys::commandModifier, 0);
    const juce::KeyPress SAVE_PROJECT = juce::KeyPress('s', juce::ModifierKeys::commandModifier, 0);
    const juce::KeyPress SAVE_AS_PROJECT = juce::KeyPress('s', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier, 0);
    const juce::KeyPress EXPORT_AUDIO = juce::KeyPress('e', juce::ModifierKeys::commandModifier, 0);

    const juce::KeyPress UNDO = juce::KeyPress('z', juce::ModifierKeys::commandModifier, 0);
    const juce::KeyPress REDO = juce::KeyPress('z', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier, 0);
    const juce::KeyPress CUT = juce::KeyPress('x', juce::ModifierKeys::commandModifier, 0);
    const juce::KeyPress COPY = juce::KeyPress('c', juce::ModifierKeys::commandModifier, 0);
    const juce::KeyPress PASTE = juce::KeyPress('v', juce::ModifierKeys::commandModifier, 0);
    const juce::KeyPress DELETE = juce::KeyPress(juce::KeyPress::deleteKey, juce::ModifierKeys::noModifiers, 0);

    const juce::KeyPress PLAY = juce::KeyPress(juce::KeyPress::spaceKey, juce::ModifierKeys::noModifiers, 0);
    const juce::KeyPress STOP = juce::KeyPress(juce::KeyPress::escapeKey, juce::ModifierKeys::noModifiers, 0);
    const juce::KeyPress RECORD = juce::KeyPress('r', juce::ModifierKeys::ctrlModifier, 0);
    const juce::KeyPress LOOP = juce::KeyPress('l', juce::ModifierKeys::ctrlModifier, 0);
    const juce::KeyPress METRONOME = juce::KeyPress('m', juce::ModifierKeys::ctrlModifier, 0);

    const juce::KeyPress NAVIGATE_LEFT = juce::KeyPress(juce::KeyPress::leftKey, juce::ModifierKeys::noModifiers, 0);
    const juce::KeyPress NAVIGATE_RIGHT = juce::KeyPress(juce::KeyPress::rightKey, juce::ModifierKeys::noModifiers, 0);
    const juce::KeyPress NAVIGATE_UP = juce::KeyPress(juce::KeyPress::upKey, juce::ModifierKeys::noModifiers, 0);
    const juce::KeyPress NAVIGATE_DOWN = juce::KeyPress(juce::KeyPress::downKey, juce::ModifierKeys::noModifiers, 0);
    const juce::KeyPress PAGE_UP = juce::KeyPress(juce::KeyPress::pageUpKey, juce::ModifierKeys::noModifiers, 0);
    const juce::KeyPress PAGE_DOWN = juce::KeyPress(juce::KeyPress::pageDownKey, juce::ModifierKeys::noModifiers, 0);
    const juce::KeyPress HOME = juce::KeyPress(juce::KeyPress::homeKey, juce::ModifierKeys::noModifiers, 0);
    const juce::KeyPress END = juce::KeyPress(juce::KeyPress::endKey, juce::ModifierKeys::noModifiers, 0);

    const juce::KeyPress ZOOM_IN = juce::KeyPress('+', juce::ModifierKeys::commandModifier, 0);
    const juce::KeyPress ZOOM_OUT = juce::KeyPress('-', juce::ModifierKeys::commandModifier, 0);
    const juce::KeyPress ZOOM_RESET = juce::KeyPress('0', juce::ModifierKeys::commandModifier, 0);
}

// ShortcutUtils implementation
namespace ShortcutUtils {
    juce::KeyPress createShortcut(const juce::String& keyString) {
        auto parts = juce::StringArray::fromTokens(keyString, "+", "");
        auto keyCode = juce::KeyPress::backspaceKey;
        auto modifiers = juce::ModifierKeys::noModifiers;

        for (const auto& part : parts) {
            part = part.trim();

            if (part == "Ctrl" || part == "Control") {
                modifiers = modifiers.withFlags(juce::ModifierKeys::ctrlModifier);
            } else if (part == "Alt") {
                modifiers = modifiers.withFlags(juce::ModifierKeys::altModifier);
            } else if (part == "Shift") {
                modifiers = modifiers.withFlags(juce::ModifierKeys::shiftModifier);
            } else if (part == "Cmd" || part == "Command" || part == "Meta") {
                modifiers = modifiers.withFlags(juce::ModifierKeys::commandModifier);
            } else if (part == "Fn") {
                modifiers = modifiers.withFlags(juce::ModifierKeys::fnModifier);
            } else {
                // Parse key code
                if (part.length() == 1) {
                    keyCode = part[0];
                } else {
                    // Map special keys
                    if (part == "Space") keyCode = juce::KeyPress::spaceKey;
                    else if (part == "Return") keyCode = juce::KeyPress::returnKey;
                    else if (part == "Tab") keyCode = juce::KeyPress::tabKey;
                    else if (part == "Backspace") keyCode = juce::KeyPress::backspaceKey;
                    else if (part == "Delete") keyCode = juce::KeyPress::deleteKey;
                    else if (part == "Escape") keyCode = juce::KeyPress::escapeKey;
                    else if (part == "Up") keyCode = juce::KeyPress::upKey;
                    else if (part == "Down") keyCode = juce::KeyPress::downKey;
                    else if (part == "Left") keyCode = juce::KeyPress::leftKey;
                    else if (part == "Right") keyCode = juce::KeyPress::rightKey;
                }
            }
        }

        return juce::KeyPress(keyCode, modifiers, 0);
    }

    juce::String formatKeyForDisplay(const juce::KeyPress& key) {
        juce::String result;

        auto modifiers = key.getModifiers();

        if (modifiers.isCtrlDown()) result += "Ctrl+";
        if (modifiers.isAltDown()) result += "Alt+";
        if (modifiers.isShiftDown()) result += "Shift+";
        if (modifiers.isCommandDown()) result += "Cmd+";
        if (modifiers.isFlagsSet(juce::ModifierKeys::ctrlModifier)) result += "Ctrl+";

        auto keyCode = key.getKeyCode();
        if (keyCode >= ' ' && keyCode <= '~') {
            result += juce::String::charToString(keyCode);
        } else {
            switch (keyCode) {
                case juce::KeyPress::spaceKey: result += "Space"; break;
                case juce::KeyPress::returnKey: result += "Return"; break;
                case juce::KeyPress::tabKey: result += "Tab"; break;
                case juce::KeyPress::backspaceKey: result += "Backspace"; break;
                case juce::KeyPress::deleteKey: result += "Delete"; break;
                case juce::KeyPress::escapeKey: result += "Escape"; break;
                case juce::KeyPress::upKey: result += "↑"; break;
                case juce::KeyPress::downKey: result += "↓"; break;
                case juce::KeyPress::leftKey: result += "←"; break;
                case juce::KeyPress::rightKey: result += "→"; break;
                default: result += "Key" + juce::String(keyCode); break;
            }
        }

        return result;
    }

    std::vector<juce::KeyPress> parseKeyString(const juce::String& keyString) {
        std::vector<juce::KeyPress> result;

        // Simple parsing - in real implementation, this would be more sophisticated
        auto parts = juce::StringArray::fromTokens(keyString, "+", "");

        if (parts.size() >= 2) {
            // Has modifiers
            auto keyCode = parts.getLast().trim()[0];
            auto modifiers = juce::ModifierKeys::noModifiers;

            for (int i = 0; i < parts.size() - 1; i++) {
                auto part = parts[i].trim();
                if (part == "Ctrl" || part == "Control") {
                    modifiers = modifiers.withFlags(juce::ModifierKeys::ctrlModifier);
                } else if (part == "Alt") {
                    modifiers = modifiers.withFlags(juce::ModifierKeys::altModifier);
                } else if (part == "Shift") {
                    modifiers = modifiers.withFlags(juce::ModifierKeys::shiftModifier);
                } else if (part == "Cmd" || part == "Command" || part == "Meta") {
                    modifiers = modifiers.withFlags(juce::ModifierKeys::commandModifier);
                }
            }

            result.push_back(juce::KeyPress(keyCode, modifiers, 0));
        }

        return result;
    }

    bool isModifierKey(const juce::KeyPress& key) {
        auto keyCode = key.getKeyCode();
        return keyCode == juce::KeyPress::shiftKey ||
               keyCode == juce::KeyPress::ctrlKey ||
               keyCode == juce::KeyPress::altKey ||
               keyCode == juce::KeyPress::menuKey ||
               keyCode == juce::KeyPress::rightshiftKey ||
               keyCode == juce::KeyPress::rightctrlKey ||
               keyCode == juce::KeyPress::rightaltKey ||
               keyCode == juce::KeyPress::rightmenuKey;
    }

    bool isPrintableKey(const juce::KeyPress& key) {
        auto keyCode = key.getKeyCode();
        return keyCode >= 32 && keyCode <= 126; // Printable ASCII range
    }
}

// ShortcutScope implementation
ShortcutScope::ShortcutScope(KeyboardShortcutManager& manager, ShortcutContext* context)
    : manager_(manager)
    , context_(context) {
}

ShortcutScope::~ShortcutScope() {
    // Clean up added shortcuts
    for (const auto& shortcut : addedShortcuts_) {
        manager_.unregisterShortcut(shortcut.key);
    }

    // Restore removed shortcuts if they weren't removed from context
    for (const auto& key : removedShortcuts_) {
        if (context_ && context_->hasShortcut(key)) {
            context_->removeShortcut(key);
        }
    }
}

void ShortcutScope::addShortcut(const KeyboardShortcut& shortcut) {
    if (context_) {
        addedShortcuts_.push_back(shortcut);
        manager_.registerShortcut(context_, shortcut);
    }
}

void ShortcutScope::removeShortcut(const juce::KeyPress& key) {
    if (context_) {
        removedShortcuts_.push_back(key);
        manager_.unregisterShortcut(key);
    }
}

} // namespace zenith::UI