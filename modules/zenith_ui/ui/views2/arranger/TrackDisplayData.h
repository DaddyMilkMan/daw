/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
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
