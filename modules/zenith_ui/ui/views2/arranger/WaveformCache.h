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

} // namespace
