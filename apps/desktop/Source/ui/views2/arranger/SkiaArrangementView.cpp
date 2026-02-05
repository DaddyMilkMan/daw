/*
  ==============================================================================

    SkiaArrangementView.cpp
    Created: 2026-02-03
    Author:  Zenith DAW Team

    Implementation of the timeline arrangement view.

  ==============================================================================
*/

#include "SkiaArrangementView.h"
#include "../../design-system/ZenithTheme.h"
#include "../../framework/GlassmorphicPanel.h"
#include <effects/SkGradientShader.h>
#include <core/SkMaskFilter.h>

namespace zenith::ui {

//==============================================================================
// Construction/Destruction
//==============================================================================

SkiaArrangementView::SkiaArrangementView() {
    setWantsKeyboardFocus(true);
    
    // Initialize track heights
    trackHeights_.resize(trackCount_, kDefaultTrackHeight);
}

SkiaArrangementView::~SkiaArrangementView() = default;

//==============================================================================
// View State
//==============================================================================

void SkiaArrangementView::setScrollPosition(float x, float y) {
    if (scrollX_ != x || scrollY_ != y) {
        scrollX_ = std::max(0.0f, x);
        scrollY_ = std::max(0.0f, y);
        markDirty();

        // Preload waveforms for newly visible clips
        preloadWaveformsForVisibleClips();
    }
}

void SkiaArrangementView::setZoom(float horizontal, float vertical) {
    horizontal = std::clamp(horizontal, kMinZoom, kMaxZoom);
    vertical = std::clamp(vertical, 0.5f, 2.0f);
    
    if (zoomX_ != horizontal || zoomY_ != vertical) {
        zoomX_ = horizontal;
        zoomY_ = vertical;
        needsGridRedraw_ = true;
        markDirty();
    }
}

void SkiaArrangementView::setPlayheadPosition(double beats) {
    if (playheadBeats_ != beats) {
        playheadBeats_ = beats;
        // Only invalidate playhead area, not entire view
        float playheadX = kTrackHeaderWidth + beatsToPixels(beats) - scrollX_;
        markDirtyRect(juce::Rectangle<float>(playheadX - 20, 0, 40, getHeight()));
    }
}

void SkiaArrangementView::setLoopRegion(double startBeat, double endBeat) {
    loopStartBeat_ = startBeat;
    loopEndBeat_ = endBeat;
    markDirty();
}

void SkiaArrangementView::setLoopEnabled(bool enabled) {
    if (loopEnabled_ != enabled) {
        loopEnabled_ = enabled;
        markDirty();
    }
}

void SkiaArrangementView::setTimeSignature(int numerator, int denominator) {
    if (numerator < 1) numerator = 4;
    if (denominator < 1) denominator = 4;

    // Validate denominator is a power of 2
    int validDenom = 1;
    while (validDenom < denominator && validDenom < 32) {
        validDenom *= 2;
    }
    if (validDenom != denominator) {
        denominator = validDenom;
    }

    if (timeSigNumerator_ != numerator || timeSigDenominator_ != denominator) {
        timeSigNumerator_ = numerator;
        timeSigDenominator_ = denominator;
        needsGridRedraw_ = true;
        markDirty();
    }
}

//==============================================================================
// Content Management
//==============================================================================

void SkiaArrangementView::setTrackCount(int count) {
    if (trackCount_ != count) {
        trackCount_ = count;
        trackHeights_.resize(count, kDefaultTrackHeight);
        trackDisplayData_.resize(count);
        needsHeadersRedraw_ = true;
        markDirty();
    }
}

void SkiaArrangementView::setTrackHeight(int trackIndex, float height) {
    if (trackIndex >= 0 && trackIndex < static_cast<int>(trackHeights_.size())) {
        height = std::clamp(height, kMinTrackHeight, kMaxTrackHeight);
        if (trackHeights_[trackIndex] != height) {
            trackHeights_[trackIndex] = height;
            needsHeadersRedraw_ = true;
            markDirty();
        }
    }
}

float SkiaArrangementView::getTrackHeight(int trackIndex) const {
    if (trackIndex >= 0 && trackIndex < static_cast<int>(trackHeights_.size())) {
        return trackHeights_[trackIndex] * zoomY_;
    }
    return kDefaultTrackHeight * zoomY_;
}

void SkiaArrangementView::setTrackData(int trackIndex, const juce::String& name,
                                        const juce::Colour& color, bool muted,
                                        bool soloed, bool armed) {
    if (trackIndex >= 0 && trackIndex < static_cast<int>(trackDisplayData_.size())) {
        auto& data = trackDisplayData_[trackIndex];
        data.name = name;
        data.color = color;
        data.muted = muted;
        data.soloed = soloed;
        data.armed = armed;
        needsHeadersRedraw_ = true;
        markDirty();
    }
}

void SkiaArrangementView::setTrackMeterLevel(int trackIndex, float level) {
    if (trackIndex >= 0 && trackIndex < static_cast<int>(trackDisplayData_.size())) {
        auto& data = trackDisplayData_[trackIndex];
        if (std::abs(data.meterLevel - level) > 0.01f) {  // Only update if changed significantly
            data.meterLevel = level;
            needsHeadersRedraw_ = true;
            markDirty();
        }
    }
}

void SkiaArrangementView::setClips(const std::vector<ClipRenderData>& clips) {
    clips_ = clips;
    markDirty();

    // Preload waveforms for visible clips after setting new clips
    preloadWaveformsForVisibleClips();
}

void SkiaArrangementView::clearClips() {
    clips_.clear();
    markDirty();
}

void SkiaArrangementView::invalidateClipCache(int trackIndex, int clipIndex) {
    if (trackIndex < 0 || clipIndex < 0) {
        // Invalid indices, invalidate all
        invalidateAllCaches();
        return;
    }

    // Find the clip at the given track/clip position
    for (const auto& clip : clips_) {
        if (clip.trackIndex == trackIndex) {
            // Invalidate waveform cache for this clip
            auto it = waveformCache_.find(clip.id);
            if (it != waveformCache_.end()) {
                waveformCache_.erase(it);
            }
            break;
        }
    }

    // Calculate the area this clip occupies for invalidation
    if (trackIndex < static_cast<int>(trackHeights_.size())) {
        float y = trackIndexToY(trackIndex);
        float h = getTrackHeight(trackIndex);

        // Find the clip to get its position
        for (const auto& clip : clips_) {
            if (clip.trackIndex == trackIndex) {
                float x = beatsToPixels(clip.startBeats) - scrollX_;
                float w = beatsToPixels(clip.lengthBeats);

                // Invalidate the clip area plus some margin
                markDirtyRect(juce::Rectangle<float>(
                    x - 10, y - 5,
                    w + 20, h + 10
                ));
                break;
            }
        }
    }
}

void SkiaArrangementView::invalidateAllCaches() {
    needsGridRedraw_ = true;
    needsHeadersRedraw_ = true;
    markDirty();
}

void SkiaArrangementView::loadWaveformForClip(const juce::String& clipId, const juce::String& audioFilePath) {
    if (audioFilePath.isEmpty()) {
        return;  // No file path provided
    }

    auto it = waveformCache_.find(clipId);
    if (it != waveformCache_.end() && (it->second.isValid || it->second.isLoading)) {
        return;  // Already loaded or currently loading
    }

    // Enforce cache size limit - remove oldest entries if needed
    constexpr size_t kMaxWaveformCacheSize = 100;
    if (waveformCache_.size() >= kMaxWaveformCacheSize) {
        // Find and remove the oldest entry (simple FIFO)
        // In production, you'd want to track access time for LRU
        auto firstIt = waveformCache_.begin();
        if (firstIt != waveformCache_.end()) {
            waveformCache_.erase(firstIt);
        }
    }

    // Mark as loading before dispatching
    waveformCache_[clipId].isLoading = true;

    // Get a safe pointer to this view for async callback
    juce::Component::SafePointer<SkiaArrangementView> safeThis(this);

    // Load audio file asynchronously using ThreadPool
    static juce::ThreadPool sharedPool(1);
    sharedPool.addJob(
        [safeThis, clipId, audioFilePath]() {
            // This runs on a background thread
            juce::File file(audioFilePath);
            if (!file.existsAsFile()) {
                // File doesn't exist, update cache on message thread
                juce::MessageManager::callAsync([safeThis, clipId]() {
                    if (auto* view = safeThis.getComponent()) {
                        view->waveformCache_[clipId].isLoading = false;
                        view->waveformCache_[clipId].isValid = false;
                    }
                });
                return;
            }

            // Try loading from disk cache first
            std::vector<float> peaks;
            if (auto* view = safeThis.getComponent()) {
                if (view->loadWaveformFromCache(file, peaks)) {
                    // Cache hit! Use cached peaks
                    juce::MessageManager::callAsync([safeThis, clipId, peaks = std::move(peaks)]() mutable {
                        if (auto* v = safeThis.getComponent()) {
                            v->waveformCache_[clipId].peaks = std::move(peaks);
                            v->waveformCache_[clipId].isValid = true;
                            v->waveformCache_[clipId].isLoading = false;
                            v->markDirty();
                        }
                    });
                    return;
                }
            }

            // Use JUCE's AudioFormatManager to read the file
            juce::AudioFormatManager formatManager;
            formatManager.registerBasicFormats();

            std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
            if (reader == nullptr) {
                // Unsupported format
                juce::MessageManager::callAsync([safeThis, clipId]() {
                    if (auto* view = safeThis.getComponent()) {
                        view->waveformCache_[clipId].isLoading = false;
                        view->waveformCache_[clipId].isValid = false;
                    }
                });
                return;
            }

            // Calculate downsampled peaks (64 samples for preview)
            const int numPeaks = 64;
            peaks.assign(numPeaks, 0.0f);

            const int64_t totalSamples = reader->lengthInSamples;
            const int64_t samplesPerPeak = totalSamples / numPeaks;

            if (samplesPerPeak > 0) {
                // Limit buffer size to prevent large allocations
                const int maxSamplesPerPeak = 65536;
                const int actualSamplesPerPeak = static_cast<int>(std::min(samplesPerPeak, static_cast<int64_t>(maxSamplesPerPeak)));

                juce::AudioBuffer<float> buffer(static_cast<int>(reader->numChannels), actualSamplesPerPeak);

                for (int i = 0; i < numPeaks; ++i) {
                    int64_t sampleStart = i * samplesPerPeak;

                    // Find max absolute value in this segment
                    float maxVal = 0.0f;

                    // Read the segment (with bounds checking)
                    int64_t samplesToRead = std::min(actualSamplesPerPeak, static_cast<int>(totalSamples - sampleStart));
                    if (samplesToRead > 0) {
                        reader->read(&buffer, 0, static_cast<int>(samplesToRead), sampleStart, true, true);

                        for (int ch = 0; ch < static_cast<int>(reader->numChannels); ++ch) {
                            const float* channelData = buffer.getReadPointer(ch);
                            for (int s = 0; s < samplesToRead; ++s) {
                                maxVal = std::max(maxVal, std::abs(channelData[s]));
                            }
                        }
                    }
                    peaks[i] = maxVal;
                }
            }

            // Move peaks to the cache on the message thread
            juce::MessageManager::callAsync([safeThis, clipId, peaks = std::move(peaks), audioFilePath]() mutable {
                if (auto* view = safeThis.getComponent()) {
                    view->waveformCache_[clipId].peaks = peaks;
                    view->waveformCache_[clipId].isValid = true;
                    view->waveformCache_[clipId].isLoading = false;
                    view->markDirty();

                    // Save to disk cache for future sessions
                    juce::File audioFile(audioFilePath);
                    if (audioFile.existsAsFile()) {
                        view->saveWaveformToCache(audioFile, peaks);
                    }
                }
            });
        }
    );
}

bool SkiaArrangementView::hasWaveform(const juce::String& clipId) const {
    auto it = waveformCache_.find(clipId);
    return it != waveformCache_.end() && it->second.isValid;
}

const std::vector<float>& SkiaArrangementView::getWaveform(const juce::String& clipId) const {
    static const std::vector<float> empty;
    auto it = waveformCache_.find(clipId);
    if (it != waveformCache_.end() && it->second.isValid) {
        return it->second.peaks;
    }
    return empty;
}

void SkiaArrangementView::preloadWaveformsForVisibleClips() {
    // Calculate visible beat range
    double startBeat = pixelsToBeats(scrollX_);
    double endBeat = pixelsToBeats(scrollX_ + getWidth() - kTrackHeaderWidth);

    // Queue waveform loading for all visible audio clips
    for (const auto& clip : clips_) {
        if (clip.trackIndex < 0 || clip.trackIndex >= trackCount_) continue;
        if (clip.isMidi || clip.audioFilePath.isEmpty()) continue;

        // Check if clip is visible
        if (clip.startBeats + clip.lengthBeats >= startBeat && clip.startBeats <= endBeat) {
            // Clip is visible, load its waveform if not already loaded/loading
            if (!hasWaveform(clip.id)) {
                auto it = waveformCache_.find(clip.id);
                if (it == waveformCache_.end() || !it->second.isLoading) {
                    loadWaveformForClip(clip.id, clip.audioFilePath);
                }
            }
        }
    }
}

//==============================================================================
// Waveform Disk Cache
//==============================================================================

juce::File SkiaArrangementView::getWaveformCacheDirectory() const {
    // Use user application data directory for waveform cache
    auto cacheDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("ZenithDAW")
        .getChildFile("WaveformCache");

    // Create directory if it doesn't exist
    if (!cacheDir.exists()) {
        cacheDir.createDirectory();
    }

    return cacheDir;
}

juce::File SkiaArrangementView::getWaveformCacheFile(const juce::File& audioFile) const {
    if (!audioFile.existsAsFile()) {
        return juce::File();
    }

    // Create a unique filename based on audio file path and modification time
    // This ensures cache is invalidated when file is modified
    auto cacheDir = getWaveformCacheDirectory();

    // Use hash of file path + modification time as cache key
    juce::String fileKey = audioFile.getFullPathName() + "_" + juce::String(audioFile.getLastModificationTime().toMilliseconds());
    juce::String cacheFileName = fileKey.hashCode() + ".waveform";

    return cacheDir.getChildFile(cacheFileName);
}

bool SkiaArrangementView::loadWaveformFromCache(const juce::File& audioFile, std::vector<float>& peaks) const {
    juce::File cacheFile = getWaveformCacheFile(audioFile);

    if (!cacheFile.existsAsFile()) {
        return false;  // No cache file
    }

    // Check if cache is newer than audio file
    if (audioFile.getLastModificationTime() > cacheFile.getLastModificationTime()) {
        // Audio file was modified after cache was created, invalidate cache
        cacheFile.deleteFile();
        return false;
    }

    // Open cache file for reading
    juce::FileInputStream stream(cacheFile);
    if (!stream.openedOk()) {
        return false;
    }

    // Read number of peaks
    // Read number of peaks
    int32_t numPeaks = stream.readInt();

    if (numPeaks > 0 && numPeaks <= 2048) {
        peaks.resize(static_cast<size_t>(numPeaks));

        // Read peak values
        for (int i = 0; i < numPeaks; ++i) {
            peaks[i] = stream.readFloat();
        }

        return !stream.isExhausted();
    }

    return false;
}

void SkiaArrangementView::saveWaveformToCache(const juce::File& audioFile, const std::vector<float>& peaks) const {
    if (!audioFile.existsAsFile() || peaks.empty()) {
        return;
    }

    juce::File cacheFile = getWaveformCacheFile(audioFile);

    // Open cache file for writing
    juce::FileOutputStream stream(cacheFile);
    if (!stream.openedOk()) {
        return;
    }

    // Write number of peaks
    int32_t numPeaks = static_cast<int32_t>(peaks.size());
    stream.writeInt(numPeaks);

    // Write peak values
    for (const float peak : peaks) {
        stream.writeFloat(peak);
    }

    stream.flush();
}

//==============================================================================
// Selection
//==============================================================================

void SkiaArrangementView::setSelection(const ArrangementSelection& sel) {
    selection_ = sel;
    markDirty();
}

void SkiaArrangementView::clearSelection() {
    selection_ = ArrangementSelection{};
    markDirty();
}

//==============================================================================
// Coordinate Conversion
//==============================================================================

float SkiaArrangementView::beatsToPixels(double beats) const {
    return static_cast<float>(beats * kDefaultPixelsPerBeat * zoomX_);
}

double SkiaArrangementView::pixelsToBeats(float pixels) const {
    return static_cast<double>(pixels / (kDefaultPixelsPerBeat * zoomX_));
}

float SkiaArrangementView::trackIndexToY(int trackIndex) const {
    float y = 0.0f;
    for (int i = 0; i < trackIndex && i < static_cast<int>(trackHeights_.size()); ++i) {
        y += trackHeights_[i] * zoomY_;
    }
    return y;
}

int SkiaArrangementView::yToTrackIndex(float y) const {
    float accum = 0.0f;
    for (int i = 0; i < static_cast<int>(trackHeights_.size()); ++i) {
        accum += trackHeights_[i] * zoomY_;
        if (y < accum) return i;
    }
    return trackCount_ - 1;
}

//==============================================================================
// Layout
//==============================================================================

void SkiaArrangementView::resized() {
    needsGridRedraw_ = true;
    needsHeadersRedraw_ = true;
}

//==============================================================================
// Drawing
//==============================================================================

void SkiaArrangementView::drawSkia(SkCanvas* canvas) {
    SkAutoCanvasRestore acr(canvas, true);
    
    // 1. Background
    drawBackground(canvas);
    
    // 2. Grid (cached when possible)
    canvas->save();
    canvas->clipRect(SkRect::MakeXYWH(
        kTrackHeaderWidth, kTimelineHeight,
        getWidth() - kTrackHeaderWidth, getHeight() - kTimelineHeight));
    canvas->translate(kTrackHeaderWidth - scrollX_, kTimelineHeight - scrollY_);
    
    SkRect visibleArea = SkRect::MakeXYWH(
        scrollX_, scrollY_,
        getWidth() - kTrackHeaderWidth,
        getHeight() - kTimelineHeight);
    drawGrid(canvas, visibleArea);
    canvas->restore();
    
    // 3. Track lanes with clips
    canvas->save();
    canvas->clipRect(SkRect::MakeXYWH(
        kTrackHeaderWidth, kTimelineHeight,
        getWidth() - kTrackHeaderWidth, getHeight() - kTimelineHeight));
    canvas->translate(kTrackHeaderWidth - scrollX_, kTimelineHeight - scrollY_);
    drawTrackLanes(canvas);
    drawClips(canvas);
    canvas->restore();
    
    // 4. Loop region (overlay on track area)
    if (loopEnabled_) {
        canvas->save();
        canvas->clipRect(SkRect::MakeXYWH(
            kTrackHeaderWidth, 0,
            getWidth() - kTrackHeaderWidth, getHeight()));
        canvas->translate(kTrackHeaderWidth - scrollX_, 0);
        drawLoopRegion(canvas);
        canvas->restore();
    }
    
    // 5. Timeline ruler (fixed Y, scrolled X)
    canvas->save();
    canvas->clipRect(SkRect::MakeXYWH(kTrackHeaderWidth, 0, 
                                       getWidth() - kTrackHeaderWidth, kTimelineHeight));
    canvas->translate(kTrackHeaderWidth - scrollX_, 0);
    drawTimeline(canvas);
    canvas->restore();
    
    // 6. Track headers (fixed X, scrolled Y)
    canvas->save();
    canvas->clipRect(SkRect::MakeXYWH(0, kTimelineHeight, 
                                       kTrackHeaderWidth, getHeight() - kTimelineHeight));
    canvas->translate(0, kTimelineHeight - scrollY_);
    drawTrackHeaders(canvas);
    canvas->restore();
    
    // 7. Top-left corner (project info)
    SkRect cornerRect = SkRect::MakeXYWH(0, 0, kTrackHeaderWidth, kTimelineHeight);
    SkPaint cornerPaint;
    cornerPaint.setColor(ZenithTheme::Colors::bg_01.getARGB());
    canvas->drawRect(cornerRect, cornerPaint);
    
    // Project name
    SkPaint textPaint;
    textPaint.setColor(ZenithTheme::Colors::text_primary.getARGB());
    textPaint.setAntiAlias(true);
    SkFont font = design::typography::getSkFont(14.0f);
    canvas->drawString("Untitled Project", 16, 26, font, textPaint);
    
    // 8. Playhead (always on top)
    drawPlayhead(canvas);

    // 9. Selection rectangle (during drag)
    if (drag_.isActive && drag_.mode == ArrangementDrag::Mode::Select) {
        drawSelectionRect(canvas);
    }

    // 10. Drop indicator (when dragging clips)
    if (drag_.isActive && drag_.mode == ArrangementDrag::Mode::MoveClip) {
        drawDropIndicator(canvas);
    }
}

void SkiaArrangementView::drawBackground(SkCanvas* canvas) {
    SkPaint bgPaint;
    bgPaint.setColor(ZenithTheme::Colors::bg_00.getARGB());
    canvas->drawRect(SkRect::MakeWH(getWidth(), getHeight()), bgPaint);
}

void SkiaArrangementView::drawGrid(SkCanvas* canvas, const SkRect& visibleArea) {
    // Calculate visible beat range
    double startBeat = pixelsToBeats(visibleArea.fLeft);
    double endBeat = pixelsToBeats(visibleArea.fRight);

    // Use time signature from state
    int beatsPerBar = timeSigNumerator_;
    float pixelsPerBar = beatsToPixels(beatsPerBar);

    // Adaptive grid: show finer grid when zoomed in
    int subdivisions = 1;
    if (pixelsPerBar > 200) subdivisions = 4;
    else if (pixelsPerBar > 100) subdivisions = 2;

    float subdiv = static_cast<float>(beatsPerBar) / subdivisions;

    // Grid line paint
    SkPaint barPaint;
    barPaint.setColor(ZenithTheme::Colors::border_default.getARGB());
    barPaint.setStrokeWidth(1.0f);
    barPaint.setAntiAlias(true);

    SkPaint beatPaint;
    beatPaint.setColor(ZenithTheme::Colors::border_subtle.getARGB());
    beatPaint.setStrokeWidth(0.5f);
    beatPaint.setAntiAlias(true);

    // Draw vertical lines
    double beat = std::floor(startBeat / beatsPerBar) * beatsPerBar;
    while (beat <= endBeat) {
        float x = beatsToPixels(beat);

        int barNumber = static_cast<int>(beat / beatsPerBar);
        bool isBar = (static_cast<int>(beat) % beatsPerBar == 0);

        if (isBar) {
            canvas->drawLine(x, 0, x, visibleArea.height() + 500, barPaint);
        } else {
            canvas->drawLine(x, 0, x, visibleArea.height() + 500, beatPaint);
        }
        
        beat += subdiv;
    }
    
    // Draw horizontal track dividers
    float y = 0;
    for (int i = 0; i < trackCount_; ++i) {
        y += getTrackHeight(i);
        canvas->drawLine(0, y, visibleArea.fRight + scrollX_, y, beatPaint);
    }
}

void SkiaArrangementView::drawTimeline(SkCanvas* canvas) {
    // Background
    SkPaint bgPaint;
    bgPaint.setColor(ZenithTheme::Colors::bg_02.getARGB());
    canvas->drawRect(SkRect::MakeXYWH(0, 0, getWidth() + scrollX_, kTimelineHeight), bgPaint);

    // Calculate visible bar range
    float visibleWidth = getWidth() - kTrackHeaderWidth;
    double startBeat = pixelsToBeats(scrollX_);
    double endBeat = pixelsToBeats(scrollX_ + visibleWidth);
    int beatsPerBar = timeSigNumerator_;

    int startBar = static_cast<int>(std::floor(startBeat / beatsPerBar));
    int endBar = static_cast<int>(std::ceil(endBeat / beatsPerBar));

    SkPaint textPaint;
    textPaint.setColor(ZenithTheme::Colors::text_secondary.getARGB());
    textPaint.setAntiAlias(true);
    SkFont font = design::typography::getSkFont(12.0f);

    SkPaint tickPaint;
    tickPaint.setColor(ZenithTheme::Colors::text_tertiary.getARGB());
    tickPaint.setStrokeWidth(1.0f);

    for (int bar = startBar; bar <= endBar; ++bar) {
        float x = beatsToPixels(bar * beatsPerBar);

        // Bar number
        juce::String barStr = juce::String(bar + 1);
        canvas->drawString(barStr.toRawUTF8(), x + 4, 24, font, textPaint);

        // Tick mark
        canvas->drawLine(x, kTimelineHeight - 8, x, kTimelineHeight, tickPaint);

        // Beat subdivisions
        for (int beat = 1; beat < beatsPerBar; ++beat) {
            float beatX = beatsToPixels(bar * beatsPerBar + beat);
            canvas->drawLine(beatX, kTimelineHeight - 4, beatX, kTimelineHeight, tickPaint);
        }
    }

    // Bottom border
    SkPaint borderPaint;
    borderPaint.setColor(ZenithTheme::Colors::border_default.getARGB());
    canvas->drawLine(0, kTimelineHeight - 1, getWidth() + scrollX_, kTimelineHeight - 1, borderPaint);
}

void SkiaArrangementView::drawTrackHeaders(SkCanvas* canvas) {
    float y = 0;
    
    for (int i = 0; i < trackCount_; ++i) {
        float trackHeight = getTrackHeight(i);
        SkRect headerRect = SkRect::MakeXYWH(0, y, kTrackHeaderWidth, trackHeight);
        
        // Background
        bool isSelected = std::find(selection_.selectedTrackIndices.begin(),
                                    selection_.selectedTrackIndices.end(), i) 
                         != selection_.selectedTrackIndices.end();
        
        SkPaint bgPaint;
        if (isSelected) {
            bgPaint.setColor(ZenithTheme::Colors::accent_subtle.getARGB());
        } else {
            bgPaint.setColor(i % 2 == 0 ? 
                ZenithTheme::Colors::bg_02.getARGB() : 
                ZenithTheme::Colors::bg_01.getARGB());
        }
        canvas->drawRect(headerRect, bgPaint);
        
        // Track color bar (left edge)
        juce::Colour trackColor = ZenithTheme::Colors::getTrackColor(i);
        SkPaint colorPaint;
        colorPaint.setColor(trackColor.getARGB());
        canvas->drawRect(SkRect::MakeXYWH(0, y, 4, trackHeight), colorPaint);
        
        // Track name
        SkPaint textPaint;
        textPaint.setColor(ZenithTheme::Colors::text_primary.getARGB());
        textPaint.setAntiAlias(true);
        SkFont font = design::typography::getSkFont(14.0f);
        
        juce::String trackName = "Track " + juce::String(i + 1);
        canvas->drawString(trackName.toRawUTF8(), 16, y + 22, font, textPaint);
        
        // Solo/Mute/Arm buttons (simplified rendering)
        float buttonY = y + trackHeight - 28;
        float buttonX = 16;
        float buttonSize = 20;
        float buttonSpacing = 24;
        
        // Record arm button
        SkPaint armPaint;
        armPaint.setColor(SkColorSetARGB(180, 239, 68, 68));  // Red
        armPaint.setAntiAlias(true);
        canvas->drawCircle(buttonX + buttonSize/2, buttonY + buttonSize/2, 8, armPaint);
        buttonX += buttonSpacing;
        
        // Solo button
        SkPaint soloPaint;
        soloPaint.setStyle(SkPaint::kStroke_Style);
        soloPaint.setStrokeWidth(2);
        soloPaint.setColor(SkColorSetARGB(180, 234, 179, 8));  // Yellow
        soloPaint.setAntiAlias(true);
        SkRect soloRect = SkRect::MakeXYWH(buttonX, buttonY, buttonSize, buttonSize);
        canvas->drawRoundRect(soloRect, 4, 4, soloPaint);
        
        SkPaint soloTextPaint;
        soloTextPaint.setColor(SkColorSetARGB(180, 234, 179, 8));
        soloTextPaint.setAntiAlias(true);
        SkFont smallFont = design::typography::getSkFont(10.0f);
        canvas->drawString("S", buttonX + 6, buttonY + 14, smallFont, soloTextPaint);
        buttonX += buttonSpacing;
        
        // Mute button
        SkPaint mutePaint;
        mutePaint.setStyle(SkPaint::kStroke_Style);
        mutePaint.setStrokeWidth(2);
        mutePaint.setColor(SkColorSetARGB(180, 100, 116, 139));  // Slate
        mutePaint.setAntiAlias(true);
        SkRect muteRect = SkRect::MakeXYWH(buttonX, buttonY, buttonSize, buttonSize);
        canvas->drawRoundRect(muteRect, 4, 4, mutePaint);
        
        SkPaint muteTextPaint;
        muteTextPaint.setColor(SkColorSetARGB(180, 100, 116, 139));
        muteTextPaint.setAntiAlias(true);
        canvas->drawString("M", buttonX + 5, buttonY + 14, smallFont, muteTextPaint);
        
        // Volume meter (right side)
        float meterX = kTrackHeaderWidth - 30;
        float meterY = y + 8;
        float meterHeight = trackHeight - 16;
        
        SkPaint meterBgPaint;
        meterBgPaint.setColor(ZenithTheme::Colors::bg_00.getARGB());
        canvas->drawRect(SkRect::MakeXYWH(meterX, meterY, 8, meterHeight), meterBgPaint);
        
        // Real level from audio engine
        float level = trackDisplayData_[i].meterLevel;
        SkPaint meterPaint;
        // Color based on level: green -> yellow -> red
        if (level < 0.7f) {
            meterPaint.setColor(SkColorSetRGB(34, 197, 94));  // Green
        } else if (level < 0.9f) {
            meterPaint.setColor(SkColorSetRGB(234, 179, 8));  // Yellow
        } else {
            meterPaint.setColor(SkColorSetRGB(239, 68, 68));  // Red
        }
        float levelHeight = meterHeight * level;
        canvas->drawRect(SkRect::MakeXYWH(meterX, meterY + meterHeight - levelHeight, 
                                          8, levelHeight), meterPaint);
        
        // Right border
        SkPaint borderPaint;
        borderPaint.setColor(ZenithTheme::Colors::border_subtle.getARGB());
        canvas->drawLine(kTrackHeaderWidth - 1, y, kTrackHeaderWidth - 1, y + trackHeight, borderPaint);
        
        y += trackHeight;
    }
}

void SkiaArrangementView::drawTrackLanes(SkCanvas* canvas) {
    // Track backgrounds are drawn as part of grid, clips drawn separately
}

void SkiaArrangementView::drawClips(SkCanvas* canvas) {
    // Render actual clips from the model
    for (const auto& clip : clips_) {
        if (clip.trackIndex >= trackCount_) continue;
        if (clip.trackIndex < 0) continue;
        
        float x = beatsToPixels(clip.startBeats);
        float y = trackIndexToY(clip.trackIndex);
        float w = beatsToPixels(clip.lengthBeats);
        float h = getTrackHeight(clip.trackIndex) - 4;
        
        // Skip if completely outside visible area
        if (x + w < scrollX_ || x > scrollX_ + getWidth()) continue;
        
        SkRect clipRect = SkRect::MakeXYWH(x, y + 2, std::max(w - 2.0f, 4.0f), h);
        SkRRect clipRRect = SkRRect::MakeRectXY(clipRect, 6, 6);
        
        // Use clip color if set, otherwise default based on type
        juce::Colour clipColor = clip.color;
        if (clipColor == juce::Colour()) {
            clipColor = clip.isMidi ? 
                ZenithTheme::Colors::waveform_midi : 
                ZenithTheme::Colors::waveform_audio;
        }
        
        // Clip background
        SkPaint clipPaint;
        clipPaint.setColor(clipColor.withAlpha(clip.isSelected ? 0.95f : 0.8f).getARGB());
        clipPaint.setAntiAlias(true);
        canvas->drawRRect(clipRRect, clipPaint);
        
        // Selection highlight
        if (clip.isSelected) {
            SkPaint selPaint;
            selPaint.setStyle(SkPaint::kStroke_Style);
            selPaint.setStrokeWidth(2.0f);
            selPaint.setColor(ZenithTheme::Colors::accent_primary.getARGB());
            selPaint.setAntiAlias(true);
            canvas->drawRRect(clipRRect, selPaint);
        } else {
            // Normal border
            SkPaint borderPaint;
            borderPaint.setStyle(SkPaint::kStroke_Style);
            borderPaint.setStrokeWidth(1.0f);
            borderPaint.setColor(clipColor.brighter(0.3f).getARGB());
            canvas->drawRRect(clipRRect, borderPaint);
        }
        
        // Clip name (only if clip is wide enough)
        if (w > 40) {
            SkPaint textPaint;
            textPaint.setColor(SK_ColorWHITE);
            textPaint.setAntiAlias(true);
            SkFont font = design::typography::getSkFont(10.0f);
            
            canvas->save();
            canvas->clipRRect(clipRRect);
            canvas->drawString(clip.name.toRawUTF8(), x + 6, y + 16, font, textPaint);

            // Waveform (real data if available, otherwise synthetic)
            // Note: Waveform loading should be triggered when clips become visible,
            // not during the render pass. Use preloadWaveformsForVisibleClips() instead.
            if (!clip.isMidi) {
                SkPaint wavePaint;
                wavePaint.setColor(SkColorSetARGB(100, 255, 255, 255));
                wavePaint.setAntiAlias(true);

                float waveY = y + h/2 + 10;
                float waveH = h/2 - 16;

                // Use real waveform data if available
                const auto& waveform = getWaveform(clip.id);
                if (!waveform.empty()) {
                    // Draw real waveform
                    float barWidth = (w - 8) / waveform.size();
                    for (size_t i = 0; i < waveform.size(); ++i) {
                        float wx = x + 4 + i * barWidth;
                        float amplitude = waveform[i];
                        float blockH = waveH * amplitude;
                        canvas->drawRect(SkRect::MakeXYWH(wx, waveY - blockH/2, barWidth * 0.8f, blockH), wavePaint);
                    }
                } else {
                    // Check if loading is in progress and show indicator
                    auto it = waveformCache_.find(clip.id);
                    if (it != waveformCache_.end() && it->second.isLoading) {
                        // Draw loading indicator
                        SkPaint loadPaint;
                        loadPaint.setColor(SkColorSetARGB(80, 255, 255, 255));
                        loadPaint.setAntiAlias(true);
                        float centerX = x + w / 2;
                        float centerY = waveY + waveH / 2;
                        for (int i = 0; i < 3; ++i) {
                            float offset = (i - 1) * 8;
                            canvas->drawRect(SkRect::MakeXYWH(centerX + offset, centerY - 2, 4, 4), loadPaint);
                        }
                    } else {
                        // Fallback to synthetic waveform based on clip ID for consistency
                        uint32_t seed = static_cast<uint32_t>(clip.id.hashCode());

                        for (float wx = x + 4; wx < x + w - 6 && wx < x + w; wx += 4) {
                            float pseudoRand = std::sin(wx * 0.1f + seed * 0.01f) * 0.5f + 0.5f;
                            float amplitude = 0.2f + 0.8f * pseudoRand;
                            float blockH = waveH * amplitude;
                            canvas->drawRect(SkRect::MakeXYWH(wx, waveY - blockH/2, 2, blockH), wavePaint);
                        }
                    }
                }
            } else {
                // MIDI note blocks
                SkPaint notePaint;
                notePaint.setColor(SkColorSetARGB(150, 255, 255, 255));

                float noteY = y + 24;
                float noteH = h - 30;

                uint32_t midiSeed = static_cast<uint32_t>(clip.id.hashCode());
                
                for (float nx = x + 4; nx < x + w - 6 && nx < x + w; nx += 12) {
                    float noteTop = noteY + noteH * (0.2f + 0.3f * std::sin(nx * 0.05f + midiSeed * 0.01f));
                    float noteBot = noteTop + 8;
                    canvas->drawRect(SkRect::MakeXYWH(nx, noteTop, 8, noteBot - noteTop), notePaint);
                }
            }
            
            canvas->restore();
        }
    }
}

void SkiaArrangementView::drawPlayhead(SkCanvas* canvas) {
    float x = kTrackHeaderWidth + beatsToPixels(playheadBeats_) - scrollX_;
    
    // Only draw if visible
    if (x < kTrackHeaderWidth - 10 || x > getWidth() + 10) return;
    
    // Glow effect (pulsing)
    float glowIntensity = 0.3f + 0.1f * std::sin(playheadGlowPhase_);
    
    SkPaint glowPaint;
    glowPaint.setColor(ZenithTheme::Colors::accent_primary.getARGB());
    glowPaint.setAlphaf(glowIntensity);
    glowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 8.0f));
    canvas->drawLine(x, 0, x, getHeight(), glowPaint);
    
    // Main line
    SkPaint linePaint;
    linePaint.setColor(ZenithTheme::Colors::accent_primary.getARGB());
    linePaint.setStrokeWidth(2.0f);
    linePaint.setAntiAlias(true);
    canvas->drawLine(x, 0, x, getHeight(), linePaint);
    
    // Top handle (triangle)
    SkPath handlePath;
    handlePath.moveTo(x - 8, 0);
    handlePath.lineTo(x + 8, 0);
    handlePath.lineTo(x, 14);
    handlePath.close();
    
    SkPaint handlePaint;
    handlePaint.setColor(ZenithTheme::Colors::accent_primary.getARGB());
    handlePaint.setAntiAlias(true);
    canvas->drawPath(handlePath, handlePaint);
}

void SkiaArrangementView::drawLoopRegion(SkCanvas* canvas) {
    float startX = beatsToPixels(loopStartBeat_);
    float endX = beatsToPixels(loopEndBeat_);
    
    // Loop region overlay
    SkPaint loopPaint;
    loopPaint.setColor(ZenithTheme::Colors::accent_primary.withAlpha(0.15f).getARGB());
    canvas->drawRect(SkRect::MakeXYWH(startX, 0, endX - startX, getHeight()), loopPaint);
    
    // Loop markers
    SkPaint markerPaint;
    markerPaint.setColor(ZenithTheme::Colors::accent_primary.getARGB());
    markerPaint.setStrokeWidth(2.0f);
    
    canvas->drawLine(startX, 0, startX, getHeight(), markerPaint);
    canvas->drawLine(endX, 0, endX, getHeight(), markerPaint);
    
    // Loop bracket at top
    SkPaint bracketPaint;
    bracketPaint.setColor(ZenithTheme::Colors::accent_primary.getARGB());
    bracketPaint.setAntiAlias(true);
    
    SkRect bracketRect = SkRect::MakeXYWH(startX, 0, endX - startX, 8);
    canvas->drawRect(bracketRect, bracketPaint);
}

void SkiaArrangementView::drawSelectionRect(SkCanvas* canvas) {
    float x1 = std::min(drag_.startPoint.x, drag_.currentPoint.x);
    float y1 = std::min(drag_.startPoint.y, drag_.currentPoint.y);
    float x2 = std::max(drag_.startPoint.x, drag_.currentPoint.x);
    float y2 = std::max(drag_.startPoint.y, drag_.currentPoint.y);
    
    SkRect selRect = SkRect::MakeXYWH(x1, y1, x2 - x1, y2 - y1);
    
    // Fill
    SkPaint fillPaint;
    fillPaint.setColor(ZenithTheme::Colors::accent_primary.withAlpha(0.2f).getARGB());
    canvas->drawRect(selRect, fillPaint);
    
    // Border
    SkPaint borderPaint;
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.0f);
    borderPaint.setColor(ZenithTheme::Colors::accent_primary.getARGB());
    canvas->drawRect(selRect, borderPaint);
}

void SkiaArrangementView::drawDropIndicator(SkCanvas* canvas) {
    if (!drag_.isActive || drag_.mode != ArrangementDrag::Mode::MoveClip) {
        return;
    }

    if (drag_.draggedClipId.isEmpty()) {
        return;
    }

    // Find the clip being dragged
    const ClipRenderData* draggedClip = nullptr;
    for (const auto& clip : clips_) {
        if (clip.id == drag_.draggedClipId) {
            draggedClip = &clip;
            break;
        }
    }

    if (!draggedClip) {
        return;
    }

    // Calculate drop position based on current mouse position
    juce::Point<float> currentPos = drag_.currentPoint;
    float trackAreaX = currentPos.x - kTrackHeaderWidth + scrollX_;
    float trackAreaY = currentPos.y - kTimelineHeight + scrollY_;

    int targetTrackIndex = yToTrackIndex(static_cast<float>(trackAreaY));
    targetTrackIndex = std::clamp(targetTrackIndex, 0, trackCount_ - 1);

    double targetStartBeat = pixelsToBeats(trackAreaX);
    // Snap to grid (1 beat resolution)
    targetStartBeat = std::round(targetStartBeat);

    // Calculate drop position in pixels
    float dropX = kTrackHeaderWidth + beatsToPixels(targetStartBeat) - scrollX_;
    float dropY = trackIndexToY(targetTrackIndex) + 2;  // +2 for padding
    float dropW = beatsToPixels(draggedClip->lengthBeats);
    float dropH = getTrackHeight(targetTrackIndex) - 4;

    // Draw drop indicator (outline with glow)
    SkRect dropRect = SkRect::MakeXYWH(dropX, dropY, std::max(dropW - 2.0f, 4.0f), dropH);
    SkRRect dropRRect = SkRRect::MakeRectXY(dropRect, 6, 6);

    // Glow effect
    SkPaint glowPaint;
    glowPaint.setColor(ZenithTheme::Colors::accent_primary.getARGB());
    glowPaint.setAlphaf(0.4f);
    glowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 12.0f));
    glowPaint.setAntiAlias(true);
    canvas->drawRRect(dropRRect, glowPaint);

    // Dashed outline
    SkPaint outlinePaint;
    outlinePaint.setColor(ZenithTheme::Colors::accent_primary.getARGB());
    outlinePaint.setStyle(SkPaint::kStroke_Style);
    outlinePaint.setStrokeWidth(2.0f);
    outlinePaint.setAntiAlias(true);

    // Create dashed effect manually
    float dashLength = 8.0f;
    float gapLength = 4.0f;
    float perimeter = 2.0f * (dropRect.width() + dropRect.height());

    // Draw simple outline for now (dashed lines are complex in Skia)
    canvas->drawRRect(dropRRect, outlinePaint);

    // Draw beat position indicator (vertical line at snap position)
    SkPaint snapLinePaint;
    snapLinePaint.setColor(ZenithTheme::Colors::accent_primary.withAlpha(0.5f).getARGB());
    snapLinePaint.setStrokeWidth(2.0f);
    snapLinePaint.setAntiAlias(true);

    float snapX = dropX;
    canvas->drawLine(snapX, kTimelineHeight, snapX, getHeight(), snapLinePaint);

    // Draw track highlight
    float trackY = trackIndexToY(targetTrackIndex);
    float trackH = getTrackHeight(targetTrackIndex);

    SkPaint trackHighlightPaint;
    trackHighlightPaint.setColor(ZenithTheme::Colors::accent_primary.withAlpha(0.15f).getARGB());
    canvas->drawRect(SkRect::MakeXYWH(kTrackHeaderWidth, trackY,
                                          getWidth() - kTrackHeaderWidth, trackH), trackHighlightPaint);
}

//==============================================================================
// Animation
//==============================================================================

void SkiaArrangementView::onAnimationTick(float deltaMs) {
    // Update playhead glow phase
    playheadGlowPhase_ += deltaMs * 0.005f;
    if (playheadGlowPhase_ > juce::MathConstants<float>::twoPi) {
        playheadGlowPhase_ -= juce::MathConstants<float>::twoPi;
    }
    
    // Invalidate playhead area for animation
    float playheadX = kTrackHeaderWidth + beatsToPixels(playheadBeats_) - scrollX_;
    markDirtyRect(juce::Rectangle<float>(playheadX - 20, 0, 40, getHeight()));
}

//==============================================================================
// Hit Testing
//==============================================================================

ArrangementHitResult SkiaArrangementView::hitTest(float x, float y) const {
    ArrangementHitResult result;
    constexpr float kHitTestEdgeMargin = 6.0f;
    
    // Check track headers
    if (x < kTrackHeaderWidth && y > kTimelineHeight) {
        result.type = ArrangementHitResult::Type::TrackHeader;
        result.trackIndex = yToTrackIndex(y - kTimelineHeight + scrollY_);
        return result;
    }
    
    // Check timeline
    if (y < kTimelineHeight && x > kTrackHeaderWidth) {
        result.type = ArrangementHitResult::Type::Timeline;
        result.beatPosition = pixelsToBeats(x - kTrackHeaderWidth + scrollX_);
        return result;
    }
    
    // Check playhead
    float playheadX = kTrackHeaderWidth + beatsToPixels(playheadBeats_) - scrollX_;
    if (std::abs(x - playheadX) < kHitTestEdgeMargin) {
        result.type = ArrangementHitResult::Type::Playhead;
        return result;
    }
    
    // Check clips
    float trackAreaY = y - kTimelineHeight + scrollY_;
    float trackAreaX = x - kTrackHeaderWidth + scrollX_;
    int trackIdx = yToTrackIndex(trackAreaY);

    if (trackIdx >= 0 && trackIdx < trackCount_) {
        for (const auto& clip : clips_) {
            if (clip.trackIndex != trackIdx) continue;

            float clipX = beatsToPixels(clip.startBeats);
            float clipW = beatsToPixels(clip.lengthBeats);
            float clipY = trackIndexToY(trackIdx);
            float clipH = getTrackHeight(trackIdx);

            // Check if point is inside clip bounds
            if (trackAreaX >= clipX && trackAreaX <= clipX + clipW &&
                trackAreaY >= clipY && trackAreaY <= clipY + clipH) {

                // Check for edge hits (for resize)
                if (trackAreaX <= clipX + kHitTestEdgeMargin) {
                    result.type = ArrangementHitResult::Type::ClipEdgeLeft;
                    result.trackIndex = trackIdx;
                    result.clipIndex = static_cast<int>(&clip - &clips_[0]);
                    result.beatPosition = pixelsToBeats(trackAreaX);
                    return result;
                }
                if (trackAreaX >= clipX + clipW - kHitTestEdgeMargin) {
                    result.type = ArrangementHitResult::Type::ClipEdgeRight;
                    result.trackIndex = trackIdx;
                    result.clipIndex = static_cast<int>(&clip - &clips_[0]);
                    result.beatPosition = pixelsToBeats(trackAreaX);
                    return result;
                }

                // Otherwise it's a clip body hit
                result.type = ArrangementHitResult::Type::Clip;
                result.trackIndex = trackIdx;
                result.clipIndex = static_cast<int>(&clip - &clips_[0]);
                result.beatPosition = pixelsToBeats(trackAreaX);
                return result;
            }
        }
    }
    
    // Default to track lane
    if (x > kTrackHeaderWidth && y > kTimelineHeight) {
        result.type = ArrangementHitResult::Type::TrackLane;
        result.trackIndex = yToTrackIndex(y - kTimelineHeight + scrollY_);
        result.beatPosition = pixelsToBeats(x - kTrackHeaderWidth + scrollX_);
    }
    
    return result;
}

juce::MouseCursor SkiaArrangementView::getCursorForHit(const ArrangementHitResult& hit) const {
    switch (hit.type) {
        case ArrangementHitResult::Type::Playhead:
            return juce::MouseCursor::LeftRightResizeCursor;
        case ArrangementHitResult::Type::ClipEdgeLeft:
        case ArrangementHitResult::Type::ClipEdgeRight:
            return juce::MouseCursor::LeftRightResizeCursor;
        default:
            return juce::MouseCursor::NormalCursor;
    }
}

//==============================================================================
// Mouse Handling
//==============================================================================

void SkiaArrangementView::mouseDown(const juce::MouseEvent& e) {
    auto hit = hitTest(static_cast<float>(e.x), static_cast<float>(e.y));
    startDrag(e, hit);
}

void SkiaArrangementView::mouseDrag(const juce::MouseEvent& e) {
    updateDrag(e);
}

void SkiaArrangementView::mouseUp(const juce::MouseEvent& e) {
    endDrag(e);
}

void SkiaArrangementView::mouseMove(const juce::MouseEvent& e) {
    auto hit = hitTest(static_cast<float>(e.x), static_cast<float>(e.y));
    if (hit.type != hoverHit_.type) {
        hoverHit_ = hit;
        setMouseCursor(getCursorForHit(hit));
    }
}

void SkiaArrangementView::mouseWheelMove(const juce::MouseEvent& e, 
                                          const juce::MouseWheelDetails& wheel) {
    if (e.mods.isCommandDown() || e.mods.isCtrlDown()) {
        handleZoom(e, wheel);
    } else {
        handleScroll(e, wheel);
    }
}

void SkiaArrangementView::mouseDoubleClick(const juce::MouseEvent& e) {
    auto hit = hitTest(static_cast<float>(e.x), static_cast<float>(e.y));

    if (hit.type == ArrangementHitResult::Type::TrackLane) {
        // Create new clip at position
        float trackAreaX = static_cast<float>(e.x) - kTrackHeaderWidth + scrollX_;
        double startBeat = pixelsToBeats(trackAreaX);

        // Snap to grid (1 beat resolution)
        startBeat = std::round(startBeat);
        if (startBeat < 0.0) startBeat = 0.0;

        int trackIndex = hit.trackIndex;
        if (trackIndex >= 0 && onClipCreateRequested) {
            onClipCreateRequested(trackIndex, startBeat);
        }
    } else if (hit.type == ArrangementHitResult::Type::Clip) {
        // Open clip in editor
        if (hit.clipIndex >= 0 && hit.clipIndex < static_cast<int>(clips_.size())) {
            const auto& clip = clips_[hit.clipIndex];
            if (onClipDoubleClicked) {
                onClipDoubleClicked(clip.id);
            }
        }
    }
}

void SkiaArrangementView::startDrag(const juce::MouseEvent& e, const ArrangementHitResult& hit) {
    drag_.startPoint = e.position;
    drag_.currentPoint = e.position;
    drag_.initialHit = hit;
    drag_.isActive = true;
    drag_.draggedClipId.clear();

    switch (hit.type) {
        case ArrangementHitResult::Type::Playhead:
            drag_.mode = ArrangementDrag::Mode::MovePlayhead;
            break;
        case ArrangementHitResult::Type::TrackLane:
            drag_.mode = e.mods.isShiftDown() ?
                ArrangementDrag::Mode::Select : ArrangementDrag::Mode::Scroll;
            break;
        case ArrangementHitResult::Type::ClipEdgeLeft:
            drag_.mode = ArrangementDrag::Mode::ResizeClipLeft;
            // Store which clip we're resizing
            if (hit.clipIndex >= 0 && hit.clipIndex < static_cast<int>(clips_.size())) {
                const auto& clip = clips_[hit.clipIndex];
                drag_.draggedClipId = clip.id;
                drag_.draggedClipOriginalTrack = clip.trackIndex;
                drag_.draggedClipOriginalStart = clip.startBeats;
                drag_.draggedClipOriginalLength = clip.lengthBeats;
                drag_.draggedClipOriginalEnd = clip.startBeats + clip.lengthBeats;
            }
            break;
        case ArrangementHitResult::Type::ClipEdgeRight:
            drag_.mode = ArrangementDrag::Mode::ResizeClipRight;
            // Store which clip we're resizing
            if (hit.clipIndex >= 0 && hit.clipIndex < static_cast<int>(clips_.size())) {
                const auto& clip = clips_[hit.clipIndex];
                drag_.draggedClipId = clip.id;
                drag_.draggedClipOriginalTrack = clip.trackIndex;
                drag_.draggedClipOriginalStart = clip.startBeats;
                drag_.draggedClipOriginalLength = clip.lengthBeats;
                drag_.draggedClipOriginalEnd = clip.startBeats + clip.lengthBeats;
            }
            break;
        case ArrangementHitResult::Type::Clip: {
            drag_.mode = ArrangementDrag::Mode::MoveClip;
            // Store which clip we're dragging
            if (hit.clipIndex >= 0 && hit.clipIndex < static_cast<int>(clips_.size())) {
                const auto& clip = clips_[hit.clipIndex];
                drag_.draggedClipId = clip.id;
                drag_.draggedClipOriginalTrack = clip.trackIndex;
                drag_.draggedClipOriginalStart = clip.startBeats;
            }
            break;
        }
        default:
            drag_.mode = ArrangementDrag::Mode::None;
            break;
    }
}

void SkiaArrangementView::updateDrag(const juce::MouseEvent& e) {
    if (!drag_.isActive) return;
    
    juce::Point<float> delta = e.position - drag_.currentPoint;
    drag_.currentPoint = e.position;
    
    switch (drag_.mode) {
        case ArrangementDrag::Mode::Scroll:
            setScrollPosition(scrollX_ - delta.x, scrollY_ - delta.y);
            break;
            
        case ArrangementDrag::Mode::MovePlayhead: {
            float newX = e.position.x - kTrackHeaderWidth + scrollX_;
            double newBeat = pixelsToBeats(newX);
            setPlayheadPosition(std::max(0.0, newBeat));
            break;
        }
            
        case ArrangementDrag::Mode::Select:
            markDirty();  // Redraw selection rectangle
            break;
            
        case ArrangementDrag::Mode::MoveClip: {
            // Visual feedback during drag - just mark dirty, actual move happens on endDrag
            markDirty();
            break;
        }

        case ArrangementDrag::Mode::ResizeClipLeft:
        case ArrangementDrag::Mode::ResizeClipRight: {
            // Visual feedback during resize - actual resize happens on endDrag
            markDirty();
            break;
        }

        default:
            break;
    }
}

void SkiaArrangementView::endDrag(const juce::MouseEvent& e) {
    if (drag_.mode == ArrangementDrag::Mode::Select) {
        // Finalize selection
        SkRect selectionRect = getSelectionRect();
        float trackAreaY = selectionRect.fTop - kTimelineHeight + scrollY_;
        int startTrack = yToTrackIndex(trackAreaY);
        float trackAreaBottom = selectionRect.fBottom - kTimelineHeight + scrollY_;
        int endTrack = yToTrackIndex(trackAreaBottom);

        // Select clips within the selection rectangle
        clearSelection();
        for (const auto& clip : clips_) {
            float clipX = kTrackHeaderWidth - scrollX_ + static_cast<float>(beatsToPixels(clip.startBeats));
            float clipRight = clipX + static_cast<float>(beatsToPixels(clip.lengthBeats));
            float clipTop = kTimelineHeight - scrollY_ + trackIndexToY(clip.trackIndex);
            float clipBottom = clipTop + trackHeights_[clip.trackIndex];

            if (clipRight >= selectionRect.fLeft && clipX <= selectionRect.fRight &&
                clipBottom >= selectionRect.fTop && clipTop <= selectionRect.fBottom) {
                selection_.selectedClips.push_back({clip.trackIndex, clip.clipIndex});
            }
        }
    } else if (drag_.mode == ArrangementDrag::Mode::MoveClip && !drag_.draggedClipId.isEmpty()) {
        // Calculate new position
        float trackAreaX = e.position.x - kTrackHeaderWidth + scrollX_;
        float trackAreaY = e.position.y - kTimelineHeight + scrollY_;

        int newTrackIndex = yToTrackIndex(trackAreaY);
        double newStartBeat = pixelsToBeats(trackAreaX);

        // Snap to grid (1 beat resolution)
        newStartBeat = std::round(newStartBeat);
        newTrackIndex = std::clamp(newTrackIndex, 0, trackCount_ - 1);

        // Only move if position actually changed
        if (newTrackIndex != drag_.draggedClipOriginalTrack ||
            std::abs(newStartBeat - drag_.draggedClipOriginalStart) > 0.01) {

            // Trigger callback
            if (onClipMoved) {
                onClipMoved(drag_.draggedClipId, newTrackIndex, newStartBeat);
            }
        }
    } else if (drag_.mode == ArrangementDrag::Mode::ResizeClipLeft && !drag_.draggedClipId.isEmpty()) {
        // Calculate new start position
        float trackAreaX = e.position.x - kTrackHeaderWidth + scrollX_;
        double newStartBeat = pixelsToBeats(trackAreaX);

        // Snap to grid (1 beat resolution)
        newStartBeat = std::round(newStartBeat);

        // Enforce minimum clip length and don't go past original end
        double maxStart = drag_.draggedClipOriginalEnd - kMinClipLengthBeats;
        newStartBeat = std::clamp(newStartBeat, 0.0, maxStart);

        // Calculate new length
        double newLength = drag_.draggedClipOriginalEnd - newStartBeat;

        // Only resize if position actually changed
        if (std::abs(newStartBeat - drag_.draggedClipOriginalStart) > 0.01) {
            // Trigger callback
            if (onClipResized) {
                onClipResized(drag_.draggedClipId, newStartBeat, newLength);
            }
        }
    } else if (drag_.mode == ArrangementDrag::Mode::ResizeClipRight && !drag_.draggedClipId.isEmpty()) {
        // Calculate new end position
        float trackAreaX = e.position.x - kTrackHeaderWidth + scrollX_;
        double newEndBeat = pixelsToBeats(trackAreaX);

        // Snap to grid (1 beat resolution)
        newEndBeat = std::round(newEndBeat);

        // Enforce minimum clip length
        double minEnd = drag_.draggedClipOriginalStart + kMinClipLengthBeats;
        newEndBeat = std::max(newEndBeat, minEnd);

        // Calculate new length
        double newLength = newEndBeat - drag_.draggedClipOriginalStart;

        // Only resize if length actually changed
        if (std::abs(newLength - drag_.draggedClipOriginalLength) > 0.01) {
            // Trigger callback
            if (onClipResized) {
                onClipResized(drag_.draggedClipId, drag_.draggedClipOriginalStart, newLength);
            }
        }
    }

    drag_.isActive = false;
    drag_.mode = ArrangementDrag::Mode::None;
    drag_.draggedClipId.clear();
    markDirty();
}

void SkiaArrangementView::handleScroll(const juce::MouseEvent& e, 
                                        const juce::MouseWheelDetails& wheel) {
    juce::ignoreUnused(e);
    
    float scrollSpeed = 40.0f;
    
    if (wheel.isReversed) {
        setScrollPosition(scrollX_ + wheel.deltaX * scrollSpeed,
                          scrollY_ - wheel.deltaY * scrollSpeed);
    } else {
        setScrollPosition(scrollX_ - wheel.deltaX * scrollSpeed,
                          scrollY_ + wheel.deltaY * scrollSpeed);
    }
}

void SkiaArrangementView::handleZoom(const juce::MouseEvent& e, 
                                      const juce::MouseWheelDetails& wheel) {
    float zoomFactor = 1.0f + wheel.deltaY * 0.2f;
    
    // Zoom centered on mouse position
    float mouseX = static_cast<float>(e.x) - kTrackHeaderWidth;
    double beatAtMouse = pixelsToBeats(mouseX + scrollX_);
    
    float newZoomX = zoomX_ * zoomFactor;
    setZoom(newZoomX, zoomY_);
    
    // Adjust scroll to keep mouse position stable
    float newPixelAtMouse = beatsToPixels(beatAtMouse);
    setScrollPosition(newPixelAtMouse - mouseX, scrollY_);
}

SkRect SkiaArrangementView::getSelectionRect() const {
    if (!drag_.isActive || drag_.mode != ArrangementDrag::Mode::Select) {
        return SkRect::MakeEmpty();
    }

    float x1 = drag_.startPoint.x;
    float y1 = drag_.startPoint.y;
    float x2 = drag_.currentPoint.x;
    float y2 = drag_.currentPoint.y;

    // Normalize rectangle (x1,y1 is top-left, x2,y2 is bottom-right)
    if (x1 > x2) std::swap(x1, x2);
    if (y1 > y2) std::swap(y1, y2);

    return SkRect::MakeXYWH(x1, y1, x2 - x1, y2 - y1);
}

} // namespace zenith::ui
