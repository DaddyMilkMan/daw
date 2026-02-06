/*
  TabOrderManager.h

  Manages keyboard tab navigation order for UI components

  Provides a system for registering components and navigating through them
  using the Tab key, supporting both logical and custom navigation orders.
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <map>
#include <memory>
#include <functional>

namespace zenith {
    class SkiaComponent;
}

namespace zenith::UI {

/**
 * Tab direction for navigation
 */
enum class TabDirection {
    Forward,   // Tab key
    Backward,  // Shift+Tab
    Next,      // Arrow keys within container
    Previous   // Arrow keys within container
};

/**
 * Tab order group for organizing components
 */
enum class TabGroup {
    None = 0,
    Main,      // Main application navigation
    Modal,     // Modal dialogs
    Toolbar,   // Toolbar controls
    Settings,  // Settings panels
    Transport, // Transport controls
    Mixer,     // Mixer controls
    PianoRoll, // Piano roll interface
    Arranger   // Arranger interface
};

/**
 * Tab order entry for component registration
 */
struct TabOrderEntry {
    SkiaComponent* component;
    TabGroup group;
    int order;
    juce::String id;
    bool focusable;
    std::function<bool(const SkiaComponent*)> predicate;
    bool isActive;

    TabOrderEntry(SkiaComponent* comp, TabGroup grp, int ord, const juce::String& compId)
        : component(comp)
        , group(grp)
        , order(ord)
        , id(compId)
        , focusable(true)
        , isActive(true) {}
};

/**
 * Tab Order Manager for keyboard navigation
 *
 * Manages focus order for keyboard navigation with support for:
 * - Logical tab order
 * - Group-based navigation
 * - Custom navigation rules
 * - Skip logic based on component state
 * - Accessibility features
 */
class TabOrderManager : public juce::ComponentListener {
public:
    /**
     * Get the singleton instance
     */
    static TabOrderManager& getInstance();

    /**
     * Register a component for tab navigation
     */
    void registerComponent(SkiaComponent* component,
                          TabGroup group = TabGroup::None,
                          int order = 0,
                          const juce::String& id = "",
                          bool focusable = true);

    /**
     * Register a component with custom navigation logic
     */
    void registerComponentWithCondition(SkiaComponent* component,
                                       TabGroup group,
                                       int order,
                                       const juce::String& id,
                                       std::function<bool(const SkiaComponent*)> shouldFocus,
                                       bool focusable = true);

    /**
     * Unregister a component
     */
    void unregisterComponent(SkiaComponent* component);

    /**
     * Move focus to next component
     */
    bool focusNext(TabGroup group = TabGroup::None);

    /**
     * Move focus to previous component
     */
    bool focusPrevious(TabGroup group = TabGroup::None);

    /**
     * Move focus to specific component
     */
    bool focusComponent(SkiaComponent* component);

    /**
     * Move focus to component by ID
     */
    bool focusComponentById(const juce::String& id);

    /**
     * Move focus to first component in group
     */
    bool focusFirstInGroup(TabGroup group);

    /**
     * Move focus to last component in group
     */
    bool focusLastInGroup(TabGroup group);

    /**
     * Get current focused component
     */
    SkiaComponent* getCurrentFocus() const;

    /**
     * Get focus order for a component
     */
    int getFocusOrder(SkiaComponent* component) const;

    /**
     * Set focus order for a component
     */
    void setFocusOrder(SkiaComponent* component, int order);

    /**
     * Skip a component in tab order
     */
    void setComponentSkipped(SkiaComponent* component, bool skipped);

    /**
     * Check if component is skipped
     */
    bool isComponentSkipped(SkiaComponent* component) const;

    /**
     * Set active state for navigation
     */
    void setGroupActive(TabGroup group, bool active);

    /**
     * Check if group is active
     */
    bool isGroupActive(TabGroup group) const;

    /**
     * Set custom focus predicate
     */
    void setFocusPredicate(std::function<bool(const SkiaComponent*)> predicate);

    /**
     * Reset all tab order state
     */
    void reset();

    // Component listener implementation
    void componentBeingDeleted(juce::Component& component) override;

private:
    TabOrderManager();
    ~TabOrderManager() override;


    // Private implementation
    struct Impl;
    std::unique_ptr<Impl> pimpl_;

    // Private methods
    SkiaComponent* findNextComponent(const TabOrderEntry& current, TabDirection direction);
    SkiaComponent* findPreviousComponent(const TabOrderEntry& current, TabDirection direction);
    SkiaComponent* findFirstComponent(TabGroup group);
    SkiaComponent* findLastComponent(TabGroup group);
    bool shouldFocusComponent(const TabOrderEntry& entry) const;
    void updateComponentFocus(SkiaComponent* component, bool focus);
    void drawFocusHighlight(SkiaComponent* component);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TabOrderManager)
};

/**
 * Tab order helper functions
 */
namespace TabOrderHelpers {
    /**
     * Set up standard tab order for a container
     */
    void setupStandardTabOrder(juce::Component* container,
                              const std::vector<SkiaComponent*>& components);

    /**
     * Set up modal dialog tab order
     */
    void setupModalTabOrder(juce::Component* container,
                           SkiaComponent* defaultButton,
                           const std::vector<SkiaComponent*>& components);

    /**
     * Create skip predicate for disabled components
     */
    std::function<bool(const SkiaComponent*)> skipDisabledComponents();

    /**
     * Create skip predicate for hidden components
     */
    std::function<bool(const SkiaComponent*)> skipHiddenComponents();

    /**
     * Create skip predicate for components without keyboard focus
     */
    std::function<bool(const SkiaComponent*)> skipNonFocusableComponents();
}

/**
 * RAII Tab order scope
 */
class TabOrderScope {
public:
    /**
     * Constructor - saves current tab state
     */
    TabOrderScope(TabOrderManager& manager);

    /**
     * Constructor - with custom focus
     */
    TabOrderScope(TabOrderManager& manager, SkiaComponent* initialFocus);

    /**
     * Destructor - restores tab state
     */
    ~TabOrderScope();

    /**
     * Add component to tab order
     */
    void addComponent(SkiaComponent* component,
                     TabGroup group = TabGroup::None,
                     int order = 0,
                     const juce::String& id = "");

    /**
     * Set focus within scope
     */
    void setFocus(SkiaComponent* component);

    /**
     * Navigate to next in scope
     */
    bool next();

    /**
     * Navigate to previous in scope
     */
    bool previous();

private:
    TabOrderManager& manager_;
    SkiaComponent* savedFocus_;
    bool hadFocus_;
    juce::Array<SkiaComponent*> scopeComponents_;
};

} // namespace Zenith::UI