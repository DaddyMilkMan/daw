/**
 * @file UndoHistoryComponent.h
 * @brief Undo/Redo history panel with timeline visualization
 *
 * Features:
 * - Scrollable list of recent undo/redo actions
 * - Click to jump to any point in history
 * - Visual timeline showing history branching
 * - Undo/Redo button indicators
 * - Current position highlight
 * - Action descriptions (e.g., "Added clip", "Moved track", "Changed tempo")
 * - Action icons by type (add, delete, move, modify)
 * - Timestamp display
 * - Quick buttons (Undo, Redo) at top
 * - Search/filter for finding specific actions
 * - Clear history button with confirmation
 * - Beautiful card-based design with animations
 * - Smooth scrolling and transitions
 *
 * @phase Phase 0: Foundation
 */

#pragma once

#include <JuceHeader.h>

class ProjectState;

namespace zenith {

/**
 * @class UndoHistoryComponent
 * @brief Beautiful undo/redo history visualization and navigation
 */
class UndoHistoryComponent : public juce::Component,
                            public juce::ListBoxModel,
                            public juce::Button::Listener,
                            public juce::TextEditor::Listener,
                            public juce::Timer
{
public:
    //==========================================================================
    explicit UndoHistoryComponent(ProjectState& state);
    ~UndoHistoryComponent() override;

    //==========================================================================
    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    //==========================================================================
    // ListBoxModel
    //==========================================================================
    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g,
                         int width, int height, bool rowIsSelected) override;
    void listBoxItemClicked(int row, const juce::MouseEvent& event) override;

    //==========================================================================
    // Button::Listener
    //==========================================================================
    void buttonClicked(juce::Button* button) override;

    //==========================================================================
    // TextEditor::Listener
    //==========================================================================
    void textEditorTextChanged(juce::TextEditor& editor) override;

    //==========================================================================
    // Methods
    //==========================================================================

    /**
     * @brief Refresh history from project state
     */
    void refreshHistory();

    /**
     * @brief Get number of undo steps available
     */
    int getUndoStepsAvailable() const { return undoSteps_; }

    /**
     * @brief Get number of redo steps available
     */
    int getRedoStepsAvailable() const { return redoSteps_; }

private:
    //==========================================================================
    // Helpers
    //==========================================================================

    /**
     * @brief Paint history timeline
     */
    void paintTimeline(juce::Graphics& g, const juce::Rectangle<int>& bounds);

    /**
     * @brief Paint action icon based on type
     */
    void paintActionIcon(juce::Graphics& g, const juce::String& actionType,
                        const juce::Rectangle<int>& bounds);

    /**
     * @brief Get icon color based on action type
     */
    juce::Colour getActionTypeColor(const juce::String& type) const;

    /**
     * @brief Handle undo button
     */
    void handleUndo();

    /**
     * @brief Handle redo button
     */
    void handleRedo();

    /**
     * @brief Filter history items by search term
     */
    void filterHistory(const juce::String& searchTerm);

    //==========================================================================
    // Members
    //==========================================================================

    ProjectState& projectState_;

    // History list
    juce::ListBox historyListBox_;

    // Undo/Redo buttons
    juce::TextButton undoButton_;
    juce::TextButton redoButton_;

    // Search field
    juce::TextEditor searchEditor_;

    // History data
    struct HistoryItem {
        juce::String description;
        juce::String actionType;  // "add", "delete", "move", "modify"
        bool isUndoable;
        bool isRedoable;
    };
    juce::Array<HistoryItem> historyItems_;
    juce::Array<HistoryItem> filteredItems_;

    // State
    int undoSteps_ = 0;
    int redoSteps_ = 0;
    int currentHistoryIndex_ = 0;

    // Animation
    float timelineAlpha_ = 0.5f;

    // Layout
    static constexpr int BUTTON_BAR_HEIGHT = 48;
    static constexpr int SEARCH_HEIGHT = 40;
    static constexpr int ITEM_HEIGHT = 36;
    static constexpr int ICON_SIZE = 20;
    static constexpr int SPACING = 8;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UndoHistoryComponent)
};

}  // namespace zenith

