/*
  TabOrderManager.cpp

  Implementation of keyboard tab navigation system
*/

#include "TabOrderManager.h"
#include "SkiaComponent.h"
#include <zenith_core/utils/PlatformLogUtils.h>
#include <algorithm>

namespace zenith::UI {

// Private implementation structure
struct TabOrderManager::Impl {
    std::vector<TabOrderEntry> components;
    SkiaComponent* currentFocus;
    std::map<TabGroup, bool> activeGroups;
    std::function<bool(const SkiaComponent*)> focusPredicate;
    std::map<SkiaComponent*, bool> skippedComponents;

    Impl()
        : currentFocus(nullptr) {
        // Initialize all groups as active
        for (int i = 0; i <= static_cast<int>(TabGroup::Arranger); ++i) {
            activeGroups[static_cast<TabGroup>(i)] = true;
        }
    }

    ~Impl() {
        clear();
    }

    void clear() {
        components.clear();
        currentFocus = nullptr;
        skippedComponents.clear();
        activeGroups.clear();
    }

    void addComponent(SkiaComponent* component,
                     TabGroup group,
                     int order,
                     const juce::String& id,
                     std::function<bool(const SkiaComponent*)> predicate = nullptr,
                     bool focusable = true) {
        if (!component) return;

        TabOrderEntry entry(component, group, order, id.isNotEmpty() ? id : component->getName());
        entry.focusable = focusable;
        entry.predicate = predicate ? predicate : [](const SkiaComponent*) { return true; };

        // Add to vector and sort by order
        components.push_back(entry);
        std::sort(components.begin(), components.end(),
            [](const TabOrderEntry& a, const TabOrderEntry& b) {
                if (a.group != b.group)
                    return static_cast<int>(a.group) < static_cast<int>(b.group);
                return a.order < b.order;
            });
    }

    void removeComponent(SkiaComponent* component) {
        components.erase(
            std::remove_if(components.begin(), components.end(),
                [component](const TabOrderEntry& entry) {
                    return entry.component == component;
                }),
            components.end());

        if (currentFocus == component) {
            currentFocus = nullptr;
        }

        skippedComponents.erase(component);
    }

    SkiaComponent* findNext(const TabOrderEntry& current, TabDirection direction) {
        if (components.empty()) return nullptr;

        auto it = std::find_if(components.begin(), components.end(),
            [&current](const TabOrderEntry& entry) {
                return entry.component == current.component;
            });

        if (it == components.end()) return nullptr;

        SkiaComponent* next = nullptr;
        TabGroup searchGroup = current.group;

        if (direction == TabDirection::Forward) {
            // Find next component
            ++it;
            for (; it != components.end(); ++it) {
                if (shouldFocusComponent(*it) &&
                    (searchGroup == TabGroup::None || it->group == searchGroup)) {
                    next = it->component;
                    break;
                }
                // If we reach a new group and we're only searching within group, stop
                if (searchGroup != TabGroup::None && it->group != searchGroup) {
                    break;
                }
            }
        } else if (direction == TabDirection::Backward) {
            // Find previous component
            if (it != components.begin()) {
                --it;
                for (; it != components.begin(); --it) {
                    if (shouldFocusComponent(*it) &&
                        (searchGroup == TabGroup::None || it->group == searchGroup)) {
                        next = it->component;
                        break;
                    }
                }
                // Check first element
                if (!next && shouldFocusComponent(*it) &&
                    (searchGroup == TabGroup::None || it->group == searchGroup)) {
                    next = it->component;
                }
            }
        }

        return next;
    }

    SkiaComponent* findFirst(TabGroup group) {
        for (const auto& entry : components) {
            if (shouldFocusComponent(entry) &&
                (group == TabGroup::None || entry.group == group)) {
                return entry.component;
            }
        }
        return nullptr;
    }

    SkiaComponent* findLast(TabGroup group) {
        for (auto it = components.rbegin(); it != components.rend(); ++it) {
            if (shouldFocusComponent(*it) &&
                (group == TabGroup::None || it->group == group)) {
                return it->component;
            }
        }
        return nullptr;
    }

    bool shouldFocusComponent(const TabOrderEntry& entry) const {
        if (!entry.isActive || !entry.focusable) return false;
        if (skippedComponents.find(entry.component) != skippedComponents.end()) return false;
        if (!activeGroups[entry.group]) return false;
        return entry.predicate(entry.component);
    }
};

TabOrderManager& TabOrderManager::getInstance() {
    static TabOrderManager instance;
    return instance;
}

TabOrderManager::TabOrderManager() : pimpl_(new Impl()) {
    setName("TabOrderManager");
}

TabOrderManager::~TabOrderManager() {
    pimpl_.reset();
}

void TabOrderManager::registerComponent(SkiaComponent* component,
                                       TabGroup group,
                                       int order,
                                       const juce::String& id,
                                       bool focusable) {
    pimpl_->addComponent(component, group, order, id, nullptr, focusable);
}

void TabOrderManager::registerComponentWithCondition(SkiaComponent* component,
                                                  TabGroup group,
                                                  int order,
                                                  const juce::String& id,
                                                  std::function<bool(const SkiaComponent*)> shouldFocus,
                                                  bool focusable) {
    pimpl_->addComponent(component, group, order, id, shouldFocus, focusable);
}

void TabOrderManager::unregisterComponent(SkiaComponent* component) {
    pimpl_->removeComponent(component);
}

bool TabOrderManager::focusNext(TabGroup group) {
    if (!pimpl_->currentFocus) {
        return focusFirstInGroup(group);
    }

    auto currentEntry = std::find_if(pimpl_->components.begin(), pimpl_->components.end(),
        [this](const TabOrderEntry& entry) {
            return entry.component == pimpl_->currentFocus;
        });

    if (currentEntry != pimpl_->components.end()) {
        auto next = pimpl_->findNext(*currentEntry, TabDirection::Forward);
        if (next) {
            pimpl_->currentFocus = next;
            updateComponentFocus(next, true);
            return true;
        }
    }

    return false;
}

bool TabOrderManager::focusPrevious(TabGroup group) {
    if (!pimpl_->currentFocus) {
        return focusLastInGroup(group);
    }

    auto currentEntry = std::find_if(pimpl_->components.begin(), pimpl_->components.end(),
        [this](const TabOrderEntry& entry) {
            return entry.component == pimpl_->currentFocus;
        });

    if (currentEntry != pimpl_->components.end()) {
        auto prev = pimpl_->findNext(*currentEntry, TabDirection::Backward);
        if (prev) {
            pimpl_->currentFocus = prev;
            updateComponentFocus(prev, true);
            return true;
        }
    }

    return false;
}

bool TabOrderManager::focusComponent(SkiaComponent* component) {
    if (!component) return false;

    if (pimpl_->shouldFocusComponent({component, TabGroup::None, 0, ""})) {
        pimpl_->currentFocus = component;
        updateComponentFocus(component, true);
        return true;
    }

    return false;
}

bool TabOrderManager::focusComponentById(const juce::String& id) {
    auto it = std::find_if(pimpl_->components.begin(), pimpl_->components.end(),
        [&id](const TabOrderEntry& entry) {
            return entry.id == id;
        });

    if (it != pimpl_->components.end()) {
        return focusComponent(it->component);
    }

    return false;
}

bool TabOrderManager::focusFirstInGroup(TabGroup group) {
    auto first = pimpl_->findFirst(group);
    if (first) {
        pimpl_->currentFocus = first;
        updateComponentFocus(first, true);
        return true;
    }
    return false;
}

bool TabOrderManager::focusLastInGroup(TabGroup group) {
    auto last = pimpl_->findLast(group);
    if (last) {
        pimpl_->currentFocus = last;
        updateComponentFocus(last, true);
        return true;
    }
    return false;
}

SkiaComponent* TabOrderManager::getCurrentFocus() const {
    return pimpl_->currentFocus;
}

int TabOrderManager::getFocusOrder(SkiaComponent* component) const {
    auto it = std::find_if(pimpl_->components.begin(), pimpl_->components.end(),
        [component](const TabOrderEntry& entry) {
            return entry.component == component;
        });

    if (it != pimpl_->components.end()) {
        return it->order;
    }
    return -1;
}

void TabOrderManager::setFocusOrder(SkiaComponent* component, int order) {
    auto it = std::find_if(pimpl_->components.begin(), pimpl_->components.end(),
        [component](const TabOrderEntry& entry) {
            return entry.component == component;
        });

    if (it != pimpl_->components.end()) {
        it->order = order;
        // Re-sort
        std::sort(pimpl_->components.begin(), pimpl_->components.end(),
            [](const TabOrderEntry& a, const TabOrderEntry& b) {
                if (a.group != b.group)
                    return static_cast<int>(a.group) < static_cast<int>(b.group);
                return a.order < b.order;
            });
    }
}

void TabOrderManager::setComponentSkipped(SkiaComponent* component, bool skipped) {
    if (component) {
        pimpl_->skippedComponents[component] = skipped;
        if (skipped && pimpl_->currentFocus == component) {
            focusNext();
        }
    }
}

bool TabOrderManager::isComponentSkipped(SkiaComponent* component) const {
    if (!component) return false;
    auto it = pimpl_->skippedComponents.find(component);
    return it != pimpl_->skippedComponents.end() && it->second;
}

void TabOrderManager::setGroupActive(TabGroup group, bool active) {
    pimpl_->activeGroups[group] = active;
}

bool TabOrderManager::isGroupActive(TabGroup group) const {
    auto it = pimpl_->activeGroups.find(group);
    return it != pimpl_->activeGroups.end() && it->second;
}

void TabOrderManager::setFocusPredicate(std::function<bool(const SkiaComponent*)> predicate) {
    pimpl_->focusPredicate = predicate;
}

void TabOrderManager::reset() {
    pimpl_->clear();
}

void TabOrderManager::componentBeingDeleted(juce::Component& component) {
    unregisterComponent(dynamic_cast<SkiaComponent*>(&component));
}

SkiaComponent* TabOrderManager::findNextComponent(const TabOrderEntry& current, TabDirection direction) {
    return pimpl_->findNext(current, direction);
}

SkiaComponent* TabOrderManager::findPreviousComponent(const TabOrderEntry& current, TabDirection direction) {
    return pimpl_->findNext(current, direction);
}

SkiaComponent* TabOrderManager::findFirstComponent(TabGroup group) {
    return pimpl_->findFirst(group);
}

SkiaComponent* TabOrderManager::findLastComponent(TabGroup group) {
    return pimpl_->findLast(group);
}

bool TabOrderManager::shouldFocusComponent(const TabOrderEntry& entry) const {
    return pimpl_->shouldFocusComponent(entry);
}

void TabOrderManager::updateComponentFocus(SkiaComponent* component, bool focus) {
    if (component) {
        if (focus) {
            component->grabKeyboardFocus();
            component->repaint();

            // Draw focus highlight
            drawFocusHighlight(component);
        } else {
            component->repaint();
        }
    }
}

void TabOrderManager::drawFocusHighlight(SkiaComponent* component) {
    // This would integrate with the Skia accessibility system
    // For now, just ensure the component knows it's focused
    if (component) {
        component->repaint();
    }
}

// TabOrderHelpers implementation
void TabOrderHelpers::setupStandardTabOrder(juce::Component* container,
                                          const std::vector<SkiaComponent*>& components) {
    auto& manager = TabOrderManager::getInstance();

    int order = 0;
    for (auto* comp : components) {
        if (comp) {
            manager.registerComponent(comp, TabGroup::Main, order++, comp->getName());
        }
    }
}

void TabOrderHelpers::setupModalTabOrder(juce::Component* container,
                                       SkiaComponent* defaultButton,
                                       const std::vector<SkiaComponent*>& components) {
    auto& manager = TabOrderManager::getInstance();

    // First register default button
    if (defaultButton) {
        manager.registerComponent(defaultButton, TabGroup::Modal, 0, "defaultButton");
    }

    // Then register other components in order
    int order = defaultButton ? 1 : 0;
    for (auto* comp : components) {
        if (comp && comp != defaultButton) {
            manager.registerComponent(comp, TabGroup::Modal, order++, comp->getName());
        }
    }
}

std::function<bool(const SkiaComponent*)> TabOrderHelpers::skipDisabledComponents() {
    return [](const SkiaComponent* comp) {
        return comp->isEnabled();
    };
}

std::function<bool(const SkiaComponent*)> TabOrderHelpers::skipHiddenComponents() {
    return [](const SkiaComponent* comp) {
        return comp->isVisible();
    };
}

std::function<bool(const SkiaComponent*)> TabOrderHelpers::skipNonFocusableComponents() {
    return [](const SkiaComponent* comp) {
        return comp->getWantsKeyboardFocus();
    };
}

// TabOrderScope implementation
TabOrderScope::TabOrderScope(TabOrderManager& manager)
    : manager_(manager)
    , savedFocus_(manager.getCurrentFocus())
    , hadFocus_(savedFocus_ != nullptr) {
}

TabOrderScope::TabOrderScope(TabOrderManager& manager, SkiaComponent* initialFocus)
    : manager_(manager)
    , savedFocus_(manager.getCurrentFocus())
    , hadFocus_(savedFocus_ != nullptr) {
    if (initialFocus) {
        manager.setFocus(initialFocus);
    }
}

TabOrderScope::~TabOrderScope() {
    if (hadFocus_) {
        manager_.focusComponent(savedFocus_);
    }
}

void TabOrderScope::addComponent(SkiaComponent* component,
                                TabGroup group,
                                int order,
                                const juce::String& id) {
    if (component) {
        scopeComponents_.add(component);
        manager_.registerComponent(component, group, order, id);
    }
}

void TabOrderScope::setFocus(SkiaComponent* component) {
    manager_.focusComponent(component);
}

bool TabOrderScope::next() {
    return manager_.focusNext();
}

bool TabOrderScope::previous() {
    return manager_.focusPrevious();
}

} // namespace zenith::UI