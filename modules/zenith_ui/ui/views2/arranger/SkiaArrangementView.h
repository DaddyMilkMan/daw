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

    SkiaArrangementView.h
    Created: 2026-02-03
    Author:  Zenith DAW Team

    Timeline-based arrangement view (Logic Pro / Pro Tools style).
    Features:
    - Multi-track timeline with zoom/scroll

    - Waveform/MIDI clip rendering
    - Track headers with solo/mute/arm
    - Timeline ruler with tempo/time signature
    - Playhead with glow effect

  ==============================================================================
*/

#pragma once

#include "../../framework/SkiaComponent.h"
#include "../../framework/GlassmorphicPanel.h"
#include "../../design-system/ZenithTheme.h"
#include <core/SkPicture.h>
#include <core/SkPictureRecorder.h>
#include <vector>
#include <memory>

namespace zenith::ui {

// Forward declarations
class SkiaTrackHeader;
class SkiaTrackLane;
class SkiaTimelineRuler;
class SkiaPlayhead;
class SkiaClipComponent;

//==============================================================================
// Types
//==============================================================================

/**
 * @brief Selection state for arrangement
 */
struct ArrangementSelection {
    std::vector<int> selectedTrackIndices;
    std::vector<std::pair<int, int>> selectedClips;  // (trackIdx, clipIdx)
    double selectionStartBeat = 0.0;
    double selectionEndBeat = 0.0;
    bool hasTimeSelection = false;
};

/**
 * @brief Hit test result for mouse interaction
 */
struct ArrangementHitResult {
    enum class Type { 
        None, 
        TrackHeader, 
        TrackLane,
        Clip, 
        ClipEdgeLeft,
        ClipEdgeRight,
        ClipFadeIn,
        ClipFadeOut,
        Timeline, 
        Playhead,
        LoopRegion
    };
    
    Type type = Type::None;
    int trackIndex = -1;
    int clipIndex = -1;
    double beatPosition = 0.0;
};

/**
 * @brief Drag operation state
 */
struct ArrangementDrag {
    enum class Mode { 
        None, 
        Scroll, 
        Select,
        MoveClip, 
        ResizeClipLeft,
        ResizeClipRight,
        FadeIn,
        FadeOut,
        MovePlayhead,
        AdjustLoop
    };
    
    Mode mode = Mode::None;
    juce::Point<float> startPoint;
    juce::Point<float> currentPoint;
    double startBeat = 0.0;
    ArrangementHitResult initialHit;
    bool isActive = false;
    juce::String draggedClipId;  // For MoveClip/Resize modes
    int draggedClipOriginalTrack = -1;
    double draggedClipOriginalStart = 0.0;
    double draggedClipOriginalLength = 0.0;  // For resize modes
    double draggedClipOriginalEnd = 0.0;     // For resize modes
};

/**
 * @brief Automation lane data
 */
struct AutomationLane {
    juce::String parameterName = "Volume";
    juce::Colour color = juce::Colours::cyan;
    bool isVisible = true;
    bool isExpanded = false;
    float height = 60.0f;
    
    // Automation points (beat, value 0-1)
    std::vector<std::pair<double, float>> points;
};

/**
 * @brief Time signature marker
 */
struct TimeSignatureMarker {
    double beatPosition = 0.0;
    int numerator = 4;
    int denominator = 4;
};

//==============================================================================
// Main View
//==============================================================================

/**
 * @class SkiaArrangementView
 * @brief Main timeline arrangement view
 */
class SkiaArrangementView : public SkiaComponent {
public:
    SkiaArrangementView();
    ~SkiaArrangementView() override;

    //==========================================================================
    // View State
    //==========================================================================
    
    void setScrollPosition(float x, float y);
    juce::Point<float> getScrollPosition() const { return {scrollX_, scrollY_}; }
    
    void setZoom(float horizontal, float vertical);
    juce::Point<float> getZoom() const { return {zoomX_, zoomY_}; }
    
    void setPlayheadPosition(double beats);
    double getPlayheadPosition() const { return playheadBeats_; }
    
    void setLoopRegion(double startBeat, double endBeat);
    void setLoopEnabled(bool enabled);
    bool isLoopEnabled() const { return loopEnabled_; }

    /**
     * @brief Set time signature for grid calculations
     */
    void setTimeSignature(int numerator, int denominator);
    int getTimeSignatureNumerator() const { return timeSigNumerator_; }
    int getTimeSignatureDenominator() const { return timeSigDenominator_; }
    int getBeatsPerBar() const { return timeSigNumerator_; }
    
    //==========================================================================
    // Content Management
    //==========================================================================
    
    void setTrackCount(int count);
    int getTrackCount() const { return trackCount_; }
    
    void setTrackHeight(int trackIndex, float height);
    float getTrackHeight(int trackIndex) const;
    
    /**
     * @brief Set track display data from controller
     */
    void setTrackData(int trackIndex, const juce::String& name, 
                      const juce::Colour& color, bool muted, bool soloed, bool armed);
    
    /**
     * @brief Set track meter level
     */
    void setTrackMeterLevel(int trackIndex, float level);
    
    /**
     * @brief Set clip data for rendering
     */
    struct ClipRenderData {
        juce::String id;
        int trackIndex = 0;
        double startBeats = 0.0;
        double lengthBeats = 0.0;
        juce::String name;
        juce::Colour color;
        bool isMidi = false;
        bool isSelected = false;
        double fadeInBeats = 0.0;
        double fadeOutBeats = 0.0;
        juce::String audioFilePath;  // For loading real waveforms
        int clipIndex = -1;          // Index in the track
    };
    
    /**
     * @brief Waveform cache entry
     */
    struct WaveformCache {
        std::vector<float> peaks;
        bool isLoading = false;
        bool isValid = false;
    };
    
    void setClips(const std::vector<ClipRenderData>& clips);
    void clearClips();
    
    void invalidateClipCache(int trackIndex, int clipIndex);
    void invalidateAllCaches();
    
    /**
     * @brief Load waveform data for an audio clip
     * @param clipId The clip ID
     * @param audioFilePath Path to the audio file
     */
    void loadWaveformForClip(const juce::String& clipId, const juce::String& audioFilePath);

    /**
     * @brief Check if waveform is loaded for a clip
     */
    bool hasWaveform(const juce::String& clipId) const;

    /**
     * @brief Get waveform data for a clip
     */
    const std::vector<float>& getWaveform(const juce::String& clipId) const;

    /**
     * @brief Preload waveforms for visible clips (async, non-blocking)
     * Call this when view becomes visible or scroll position changes significantly
     */
    void preloadWaveformsForVisibleClips();
    
    //==========================================================================
    // Automation
    //==========================================================================
    
    void setAutomationLane(int trackIndex, const AutomationLane& lane);
    void addAutomationPoint(int trackIndex, double beat, float value);
    void setAutomationVisible(int trackIndex, bool visible);
    
    //==========================================================================
    // Time Signatures
    //==========================================================================
    
    void addTimeSignatureMarker(double beat, int numerator, int denominator);
    void clearTimeSignatureMarkers();
    int getBeatsPerBarAt(double beat) const;
    
    //==========================================================================
    // Selection
    //==========================================================================
    
    const ArrangementSelection& getSelection() const { return selection_; }
    void setSelection(const ArrangementSelection& sel);
    void clearSelection();
    
    //==========================================================================
    // Callbacks
    //==========================================================================
    
    std::function<void(const juce::String& clipId, int newTrackIndex, double newStartBeat)> onClipMoved;
    std::function<void(const juce::String& clipId, double newStartBeat, double newLength)> onClipResized;
    std::function<void(const juce::String& clipId)> onClipDoubleClicked;
    std::function<void(int trackIndex, double startBeat)> onClipCreateRequested;
    
    //==========================================================================
    // Coordinate Conversion
    //==========================================================================
    
    float beatsToPixels(double beats) const;
    double pixelsToBeats(float pixels) const;
    float trackIndexToY(int trackIndex) const;
    int yToTrackIndex(float y) const;
    
    //==========================================================================
    // SkiaComponent Overrides
    //==========================================================================
    
    void drawSkia(SkCanvas* canvas) override;
    void resized() override;
    
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, 
                        const juce::MouseWheelDetails& wheel) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    
    void onAnimationTick(float deltaMs) override;

private:
    //==========================================================================
    // Waveform Cache Helpers
    //==========================================================================

    /**
     * @brief Get the cache file path for an audio file
     */
    juce::File getWaveformCacheFile(const juce::File& audioFile) const;

    /**
     * @brief Load waveform peaks from disk cache
     * @return true if cache was loaded successfully
     */
    bool loadWaveformFromCache(const juce::File& audioFile, std::vector<float>& peaks) const;

    /**
     * @brief Save waveform peaks to disk cache
     */
    void saveWaveformToCache(const juce::File& audioFile, const std::vector<float>& peaks) const;

    /**
     * @brief Get the waveform cache directory
     */
    juce::File getWaveformCacheDirectory() const;

    //==========================================================================
    // Layout Constants
    //==========================================================================
    
    static constexpr float kTrackHeaderWidth = 280.0f;
    static constexpr float kTimelineHeight = 40.0f;
    static constexpr float kDefaultTrackHeight = 80.0f;
    static constexpr float kMinTrackHeight = 40.0f;
    static constexpr float kMaxTrackHeight = 200.0f;
    static constexpr float kMinZoom = 0.1f;
    static constexpr float kMaxZoom = 10.0f;
    static constexpr float kDefaultPixelsPerBeat = 40.0f;

    //==========================================================================
    // Interaction Constants
    //==========================================================================

    static constexpr float kHitTestEdgeMargin = 8.0f;  // Edge detection margin for clip resize/playhead
    static constexpr float kMinClipLengthBeats = 1.0;  // Minimum clip length in beats
    
    //==========================================================================
    // View State
    //==========================================================================
    
    float scrollX_ = 0.0f;
    float scrollY_ = 0.0f;
    float zoomX_ = 1.0f;   // Multiplier for pixels per beat
    float zoomY_ = 1.0f;   // Track height multiplier
    double playheadBeats_ = 0.0;
    
    // Loop region
    double loopStartBeat_ = 0.0;
    double loopEndBeat_ = 8.0;
    bool loopEnabled_ = false;

    // Time signature
    int timeSigNumerator_ = 4;
    int timeSigDenominator_ = 4;
    
    // Track data
    int trackCount_ = 8;
    std::vector<float> trackHeights_;
    
    // Track display data
    struct TrackDisplayData {
        juce::String name;
        juce::Colour color;
        bool muted = false;
        bool soloed = false;
        bool armed = false;
        float meterLevel = 0.0f;
    };
    std::vector<TrackDisplayData> trackDisplayData_;
    
    // Clip data for rendering
    std::vector<ClipRenderData> clips_;
    
    // Waveform cache for audio clips
    std::unordered_map<juce::String, WaveformCache> waveformCache_;
    
    // Selection
    ArrangementSelection selection_;
    
    // Interaction
    ArrangementDrag drag_;
    ArrangementHitResult hoverHit_;
    
    //==========================================================================
    // Cached Rendering
    //==========================================================================
    
    sk_sp<SkPicture> gridPicture_;
    sk_sp<SkPicture> trackHeadersPicture_;
    bool needsGridRedraw_ = true;
    bool needsHeadersRedraw_ = true;
    float lastGridWidth_ = 0.0f;
    float lastGridZoom_ = 0.0f;
    
    //==========================================================================
    // Animation State
    //==========================================================================
    
    float playheadGlowPhase_ = 0.0f;  // For subtle pulsing
    
    //==========================================================================
    // Drawing Methods
    //==========================================================================
    
    void drawBackground(SkCanvas* canvas);
    void drawGrid(SkCanvas* canvas, const SkRect& visibleArea);
    void drawTimeline(SkCanvas* canvas);
    void drawTrackHeaders(SkCanvas* canvas);
    void drawTrackLanes(SkCanvas* canvas);
    void drawClips(SkCanvas* canvas);
    void drawPlayhead(SkCanvas* canvas);
    void drawLoopRegion(SkCanvas* canvas);
    void drawSelectionRect(SkCanvas* canvas);
    void drawDropIndicator(SkCanvas* canvas);
    
    // Cached layer management
    void rebuildGridCache(float width, float height);
    void rebuildHeadersCache();
    
    //==========================================================================
    // Hit Testing
    //==========================================================================
    
    ArrangementHitResult hitTest(float x, float y) const;
    juce::MouseCursor getCursorForHit(const ArrangementHitResult& hit) const;
    
    //==========================================================================
    // Interaction Handlers
    //==========================================================================

    void startDrag(const juce::MouseEvent& e, const ArrangementHitResult& hit);
    void updateDrag(const juce::MouseEvent& e);
    void endDrag(const juce::MouseEvent& e);

    void handleScroll(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel);
    void handleZoom(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel);

    /**
     * @brief Get the selection rectangle for drag selection
     */
    SkRect getSelectionRect() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaArrangementView)
};

} // namespace zenith::ui
