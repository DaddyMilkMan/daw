/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/*
    ==============================================================================
    Original file header:
*/

  ==============================================================================

    UndoHistoryPanel.h
    Created: 2025-12-25
    Author:  Zenith DAW Team

    Visual undo/redo history panel with click-to-navigate.

  ==============================================================================

*/

#pragma once

#include "../framework/SkiaComponent.h"
#include "../../engine/ProjectState.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <core/SkCanvas.h>
#include <core/SkFont.h>
#include <core/SkPaint.h>

namespace zenith {

#ifdef ZENITH_USE_SKIA

/**
 * @brief Visual history panel showing undo/redo stack
 * 
 * Features:
 * - Scrollable list of action descriptions
 * - Current position indicator (cyan highlight)
 * - Click any item to jump to that point in history
 * - Keyboard shortcuts display (Ctrl+Z / Ctrl+Shift+Z)
 */
class UndoHistoryPanel : public SkiaComponent {
public:
    explicit UndoHistoryPanel(ProjectState& projectState);
    ~UndoHistoryPanel() override;

    void drawSkia(SkCanvas* canvas) override;
    void resized() override;
    void timerCallback() override;
    
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, 
                        const juce::MouseWheelDetails& wheel) override;

private:
    // Data
    ProjectState& projectState_;
    
    // History cache (rebuilt each frame from UndoManager)
    struct HistoryItem {
        juce::String description;
        bool isUndoable;  // true = in undo stack, false = in redo stack
        int index;        // Position in the combined list
    };
    std::vector<HistoryItem> historyItems_;
    int currentIndex_ = 0;  // Position of "now" in the history
    
    // UI State
    float scrollOffset_ = 0.0f;
    int hoveredIndex_ = -1;
    static constexpr float ITEM_HEIGHT = 28.0f;
    static constexpr float HEADER_HEIGHT = 36.0f;
    static constexpr float BUTTON_SIZE = 24.0f;
    
    // Button bounds for undo/redo
    juce::Rectangle<float> undoButtonBounds_;
    juce::Rectangle<float> redoButtonBounds_;
    bool undoHovered_ = false;
    bool redoHovered_ = false;
    
    // Cached paints
    SkPaint bgPaint_;
    SkPaint headerPaint_;
    SkPaint textPaint_;
    SkPaint dimTextPaint_;
    SkPaint highlightPaint_;
    SkPaint hoverPaint_;
    SkPaint borderPaint_;
    SkPaint buttonPaint_;
    SkFont headerFont_;
    SkFont itemFont_;
    SkFont shortcutFont_;
    
    // Helpers
    void rebuildHistory();
    void updateCachedPaints();
    void drawHeader(SkCanvas* canvas, float width);
    void drawHistoryList(SkCanvas* canvas, const SkRect& bounds);
    void drawHistoryItem(SkCanvas* canvas, const HistoryItem& item, 
                         float y, float width, bool isHovered);
    int getItemAtY(float y) const;
    void jumpToHistoryIndex(int index);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UndoHistoryPanel)
};

#endif // ZENITH_USE_SKIA

} // namespace zenith
