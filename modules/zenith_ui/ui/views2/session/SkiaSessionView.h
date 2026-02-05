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

    SkiaSessionView.h
    Created: 2026-02-03
    Author:  Zenith DAW Team

    Clip launcher grid view (Ableton Live style).
    Features:
    - Scene/clip grid with launch buttons

    - Track headers with mixer strip
    - Scene launcher column
    - Real-time clip state visualization

  ==============================================================================
*/

#pragma once

#include "../../framework/SkiaComponent.h"
#include "../../framework/GlassmorphicPanel.h"
#include "../../design-system/ZenithTheme.h"
#include "../ViewTheme.h"
#include <vector>

namespace zenith::ui {

//==============================================================================
// Types
//==============================================================================

/**
 * @brief State of a single clip slot
 */
enum class ClipSlotState {
    Empty,       ///< No clip in slot
    Stopped,     ///< Clip loaded but not playing
    Playing,     ///< Clip currently playing
    Queued,      ///< Clip queued to play on next quantize
    Recording,   ///< Recording into this slot
    Stopping     ///< Queued to stop on next quantize
};

/**
 * @brief Data for a single clip slot
 */
struct ClipSlotData {
    ClipSlotState state = ClipSlotState::Empty;
    juce::String name;
    juce::Colour color = juce::Colours::grey;
    bool isSelected = false;
    float playProgress = 0.0f;  ///< 0-1 for playing clips
    bool isMidi = false;
};

/**
 * @brief Track data for session view
 */
struct SessionTrackData {
    juce::String name;
    juce::Colour color;
    bool isSolo = false;
    bool isMuted = false;
    bool isArmed = false;
    float volume = 0.8f;
    float pan = 0.5f;
    float meterLevel = 0.0f;
    std::vector<ClipSlotData> clips;
};

/**
 * @brief Scene data
 */
struct SceneData {
    juce::String name;
    juce::Colour color = juce::Colours::grey;
    bool isPlaying = false;
    bool isQueued = false;
};

//==============================================================================
// Main View
//==============================================================================

/**
 * @class SkiaSessionView
 * @brief Clip launcher grid view
 */
class SkiaSessionView : public SkiaComponent {
public:
    SkiaSessionView();
    ~SkiaSessionView() override;

    //==========================================================================
    // Data Access
    //==========================================================================
    
    void setTracks(const std::vector<SessionTrackData>& tracks);
    void setScenes(const std::vector<SceneData>& scenes);
    
    void setClipState(int trackIndex, int sceneIndex, const ClipSlotData& data);
    void setTrackMeterLevel(int trackIndex, float level);
    void setMasterMeterLevel(float left, float right);
    
    //==========================================================================
    // Selection
    //==========================================================================
    
    void setSelectedSlot(int trackIndex, int sceneIndex);
    std::pair<int, int> getSelectedSlot() const { return selectedSlot_; }
    
    //==========================================================================
    // Scrolling
    //==========================================================================
    
    void setScrollPosition(float x, float y);
    juce::Point<float> getScrollPosition() const { return {scrollX_, scrollY_}; }

    //==========================================================================
    // Callbacks
    //==========================================================================

    /**
     * @brief Callback when a clip slot is clicked
     * Arguments: trackIndex, sceneIndex
     */
    std::function<void(int, int)> onClipSlotClicked;

    /**
     * @brief Callback when a clip slot is double-clicked
     * Arguments: trackIndex, sceneIndex
     */
    std::function<void(int, int)> onClipSlotDoubleClicked;

    /**
     * @brief Callback when a scene is clicked
     * Arguments: sceneIndex
     */
    std::function<void(int)> onSceneClicked;

    /**
     * @brief Callback when stop button is clicked for a track
     * Arguments: trackIndex
     */
    std::function<void(int)> onTrackStopClicked;

    //==========================================================================
    // SkiaComponent Overrides
    //==========================================================================
    
    void drawSkia(SkCanvas* canvas) override;
    void resized() override;
    
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e,
                        const juce::MouseWheelDetails& wheel) override;
    
    void onAnimationTick(float deltaMs) override;

private:
    //==========================================================================
    // Theme System
    //==========================================================================

    const zenith::ui::ViewTheme::Layout& layout_;
    const zenith::ui::ViewTheme::Colors& colors_;
    const zenith::ui::ViewTheme::Animation& animation_;
    const zenith::ui::ViewTheme::ErrorHandling& errorHandling_;

    //==========================================================================
    // Legacy Constants (for migration - to be removed)
    //==========================================================================

    static constexpr float kSceneLauncherWidth = 100.0f;
    static constexpr float kTrackWidth = 140.0f;
    static constexpr float kClipSlotHeight = 80.0f;
    static constexpr float kTrackHeaderHeight = 44.0f;
    static constexpr float kMixerHeight = 100.0f;
    static constexpr float kStopRowHeight = 36.0f;
    
    //==========================================================================
    // State
    //==========================================================================
    
    std::vector<SessionTrackData> tracks_;
    std::vector<SceneData> scenes_;
    std::pair<int, int> selectedSlot_ = {-1, -1};
    std::pair<int, int> hoveredSlot_ = {-1, -1};
    
    float scrollX_ = 0.0f;
    float scrollY_ = 0.0f;
    
    // Animation
    float animPhase_ = 0.0f;
    
    // Master meter
    float masterMeterLeft_ = 0.0f;
    float masterMeterRight_ = 0.0f;
    
    //==========================================================================
    // Drawing Methods
    //==========================================================================
    
    void drawBackground(SkCanvas* canvas);
    void drawSceneLauncher(SkCanvas* canvas);
    void drawTrackHeaders(SkCanvas* canvas);
    void drawClipGrid(SkCanvas* canvas);
    void drawStopRow(SkCanvas* canvas);
    void drawMixerStrips(SkCanvas* canvas);
    void drawMasterTrack(SkCanvas* canvas);
    
    void drawClipSlot(SkCanvas* canvas, float x, float y, 
                      float w, float h, const ClipSlotData& data,
                      bool isHovered, bool isSelected);
    
    void drawPlayingIndicator(SkCanvas* canvas, float cx, float cy, 
                              float radius, float progress);
    void drawQueuedIndicator(SkCanvas* canvas, float cx, float cy, float radius);
    void drawRecordingIndicator(SkCanvas* canvas, float cx, float cy, float radius);
    
    //==========================================================================
    // Hit Testing
    //==========================================================================

    std::pair<int, int> hitTestSlot(float x, float y) const;
    int hitTestScene(float y) const;
    int hitTestTrack(float x) const;
    int hitTestStopButton(float x, float y) const;
    
    //==========================================================================
    // Utilities
    //==========================================================================

    float getTrackX(int trackIndex) const;
    float getSceneY(int sceneIndex) const;
    int getVisibleTrackCount() const;
    int getVisibleSceneCount() const;

    //==========================================================================
    // Error Handling
    //==========================================================================

    void handleError(const std::string& operation, const std::string& error);
    void drawFallbackBackground(SkCanvas* canvas);
    void initializeThemeSystem();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaSessionView)
};

} // namespace zenith::ui
