/*
  ==============================================================================

    SkiaGridComponent.cpp
    Created: 2025-12-29
    Author:  Zenith DAW

    Implementation of high-performance Session View grid.

  ==============================================================================
*/

#include "SkiaGridComponent.h"

#ifdef ZENITH_USE_SKIA
#include <include/core/SkSurface.h>
#include <include/effects/SkGradientShader.h>
#endif

namespace zenith {

using namespace design;

//==============================================================================
// Constants
//==============================================================================
static constexpr float CELL_CORNER_RADIUS = 6.0f;
static constexpr float CELL_PADDING = 4.0f;
static constexpr float GAIN_ZONE_RATIO = 0.15f;   // Top 15%
static constexpr float LOOP_ZONE_RATIO = 0.10f;   // Left/Right 10%
static constexpr float WAVEFORM_HEIGHT_RATIO = 0.25f;  // Bottom 25% for sparkline
static constexpr float GLOW_BLUR_SIGMA = 6.0f;
static constexpr float CELL_GAP = 2.0f;

//==============================================================================
// Constructor / Destructor
//==============================================================================
SkiaGridComponent::SkiaGridComponent(Engine& engine, ProjectState& projectState)
    : engine_(engine), projectState_(projectState)
{
    startTimerHz(60);
    setOpaque(true);
    
    // Initialize default grid
    setGridSize(8, 8);
    
    // Initialize playhead positions
    for (int i = 0; i < 32; ++i) {
        playheadPositions_[i].store(0.0f);
    }
}

SkiaGridComponent::~SkiaGridComponent() = default;

//==============================================================================
// Grid Configuration
//==============================================================================
void SkiaGridComponent::setGridSize(int numTracks, int numScenes)
{
    numTracks_ = juce::jmax(1, numTracks);
    numScenes_ = juce::jmax(1, numScenes);
    
    cells_.resize(static_cast<size_t>(numTracks_));
    for (int t = 0; t < numTracks_; ++t) {
        cells_[static_cast<size_t>(t)].resize(static_cast<size_t>(numScenes_));
        for (int s = 0; s < numScenes_; ++s) {
            auto& cell = cells_[static_cast<size_t>(t)][static_cast<size_t>(s)];
            cell.trackIndex = t;
            cell.sceneIndex = s;
            // Assign track colors from palette
            cell.trackColor = colors::TRACK_COLORS[static_cast<size_t>(t) % colors::TRACK_COLORS.size()];
        }
    }
    
    invalidateBackgroundCache();
    repaint();
}

void SkiaGridComponent::setClipData(int trackIndex, int sceneIndex, const SessionClipCell& cell)
{
    if (trackIndex >= 0 && trackIndex < numTracks_ && 
        sceneIndex >= 0 && sceneIndex < numScenes_) {
        cells_[static_cast<size_t>(trackIndex)][static_cast<size_t>(sceneIndex)] = cell;
        repaint();
    }
}

SessionClipCell* SkiaGridComponent::getClipCell(int trackIndex, int sceneIndex)
{
    if (trackIndex >= 0 && trackIndex < numTracks_ && 
        sceneIndex >= 0 && sceneIndex < numScenes_) {
        return &cells_[static_cast<size_t>(trackIndex)][static_cast<size_t>(sceneIndex)];
    }
    return nullptr;
}

//==============================================================================
// Component Overrides
//==============================================================================
void SkiaGridComponent::resized()
{
    // Calculate cell dimensions based on available space
    float availableWidth = static_cast<float>(getWidth()) - sceneHeaderWidth_;
    float availableHeight = static_cast<float>(getHeight()) - trackHeaderHeight_;
    
    cellWidth_ = availableWidth / static_cast<float>(numTracks_);
    cellHeight_ = availableHeight / static_cast<float>(numScenes_);
    
    // Minimum cell sizes
    cellWidth_ = juce::jmax(cellWidth_, 80.0f);
    cellHeight_ = juce::jmax(cellHeight_, 60.0f);
    
    // Update cell bounds
    for (int t = 0; t < numTracks_; ++t) {
        for (int s = 0; s < numScenes_; ++s) {
            cells_[static_cast<size_t>(t)][static_cast<size_t>(s)].bounds = getCellBounds(t, s);
        }
    }
    
    invalidateBackgroundCache();
}

void SkiaGridComponent::timerCallback()
{
    // Update hover animations
    bool needsRepaint = false;
    
    for (int t = 0; t < numTracks_; ++t) {
        for (int s = 0; s < numScenes_; ++s) {
            auto& cell = cells_[static_cast<size_t>(t)][static_cast<size_t>(s)];
            float target = (t == hoveredCell_.x && s == hoveredCell_.y) ? 1.0f : 0.0f;
            
            if (std::abs(cell.hoverProgress - target) > 0.01f) {
                cell.hoverProgress += (target - cell.hoverProgress) * 0.2f;
                needsRepaint = true;
            }
        }
    }
    
    if (needsRepaint) {
        repaint();
    }
}

//==============================================================================
// Main Drawing
//==============================================================================
#ifdef ZENITH_USE_SKIA
void SkiaGridComponent::drawSkia(SkCanvas* canvas)
{
    if (!canvas) return;
    
    // Clear background
    // Clear background
    canvas->clear(colors::BG_00);
    
    // Draw layers
    drawBackground(canvas);
    drawTrackHeaders(canvas);
    drawSceneHeaders(canvas);
    drawGrid(canvas);
}

void SkiaGridComponent::drawBackground(SkCanvas* canvas)
{
    // Subtle gradient background
    SkPaint bgPaint;
    SkPoint pts[2] = {{0, 0}, {0, static_cast<float>(getHeight())}};
    SkColor colors[2] = {
        SkColorSetRGB(20, 20, 25),
        SkColorSetRGB(15, 15, 20)
    };
    bgPaint.setShader(SkGradientShader::MakeLinear(pts, colors, nullptr, 2, SkTileMode::kClamp));
    canvas->drawRect(SkRect::MakeWH(static_cast<float>(getWidth()), static_cast<float>(getHeight())), bgPaint);
}

void SkiaGridComponent::drawTrackHeaders(SkCanvas* canvas)
{
    SkPaint headerPaint;
    headerPaint.setAntiAlias(true);
    
    for (int t = 0; t < numTracks_; ++t) {
        float x = sceneHeaderWidth_ + t * cellWidth_;
        SkRect headerRect = SkRect::MakeXYWH(x, 0, cellWidth_ - CELL_GAP, trackHeaderHeight_ - CELL_GAP);
        
        // Header background
        headerPaint.setColor(SkColorSetARGB(200, 30, 30, 35));
        canvas->drawRoundRect(headerRect, CELL_CORNER_RADIUS, CELL_CORNER_RADIUS, headerPaint);
        
        // Track color strip at top
        auto& cell = cells_[static_cast<size_t>(t)][0];
        SkPaint colorPaint;
        colorPaint.setColor(SkColorSetRGB(
            static_cast<uint8_t>(cell.trackColor.getRed()),
            static_cast<uint8_t>(cell.trackColor.getGreen()),
            static_cast<uint8_t>(cell.trackColor.getBlue())
        ));
        canvas->drawRect(SkRect::MakeXYWH(x + 2, 2, cellWidth_ - CELL_GAP - 4, 4), colorPaint);
        
        // Track name
        SkPaint textPaint;
        textPaint.setColor(SkColorSetRGB(200, 200, 200));
        textPaint.setAntiAlias(true);
        
        SkFont font;
        font.setSize(11.0f);
        
        juce::String trackName = "Track " + juce::String(t + 1);
        canvas->drawString(trackName.toRawUTF8(), x + 8, trackHeaderHeight_ * 0.6f, font, textPaint);
    }
}

void SkiaGridComponent::drawSceneHeaders(SkCanvas* canvas)
{
    SkPaint headerPaint;
    headerPaint.setAntiAlias(true);
    
    for (int s = 0; s < numScenes_; ++s) {
        float y = trackHeaderHeight_ + s * cellHeight_;
        SkRect headerRect = SkRect::MakeXYWH(0, y, sceneHeaderWidth_ - CELL_GAP, cellHeight_ - CELL_GAP);
        
        // Header background
        headerPaint.setColor(SkColorSetARGB(200, 30, 30, 35));
        canvas->drawRoundRect(headerRect, CELL_CORNER_RADIUS, CELL_CORNER_RADIUS, headerPaint);
        
        // Scene number
        SkPaint textPaint;
        textPaint.setColor(SkColorSetRGB(180, 180, 180));
        textPaint.setAntiAlias(true);
        
        SkFont font;
        font.setSize(12.0f);
        
        juce::String sceneName = juce::String(s + 1);
        canvas->drawString(sceneName.toRawUTF8(), sceneHeaderWidth_ * 0.4f, y + cellHeight_ * 0.55f, font, textPaint);
        
        // Scene trigger button (triangle)
        float btnX = sceneHeaderWidth_ - 20;
        float btnY = y + cellHeight_ * 0.5f;
        
        SkPath triggerPath;
        triggerPath.moveTo(btnX, btnY - 8);
        triggerPath.lineTo(btnX + 12, btnY);
        triggerPath.lineTo(btnX, btnY + 8);
        triggerPath.close();
        
        SkPaint triggerPaint;
        triggerPaint.setColor(SkColorSetRGB(80, 200, 120));
        triggerPaint.setAntiAlias(true);
        canvas->drawPath(triggerPath, triggerPaint);
    }
}

void SkiaGridComponent::drawGrid(SkCanvas* canvas)
{
    // Draw all cells
    for (int t = 0; t < numTracks_; ++t) {
        for (int s = 0; s < numScenes_; ++s) {
            drawCell(canvas, t, s);
        }
    }
}

void SkiaGridComponent::drawCell(SkCanvas* canvas, int trackIndex, int sceneIndex)
{
    auto& cell = cells_[static_cast<size_t>(trackIndex)][static_cast<size_t>(sceneIndex)];
    SkRect cellRect = SkRect::MakeXYWH(
        cell.bounds.getX(),
        cell.bounds.getY(),
        cell.bounds.getWidth() - CELL_GAP,
        cell.bounds.getHeight() - CELL_GAP
    );
    
    SkRRect cellRRect;
    cellRRect.setRectXY(cellRect, CELL_CORNER_RADIUS, CELL_CORNER_RADIUS);
    
    // Draw glow for active/queued clips
    if (cell.isPlaying) {
        drawGlowEffect(canvas, cellRRect, SkColorSetRGB(0, 255, 255));  // Cyan glow
    } else if (cell.isQueued) {
        drawGlowEffect(canvas, cellRRect, SkColorSetRGB(255, 180, 0));  // Orange glow
    }
    
    // Cell background
    SkPaint cellPaint;
    cellPaint.setAntiAlias(true);
    
    if (cell.hasClip) {
        // Filled cell with track color
        uint8_t alpha = cell.isPlaying ? 180 : static_cast<uint8_t>(100 + cell.hoverProgress * 50);
        cellPaint.setColor(SkColorSetARGB(
            alpha,
            static_cast<uint8_t>(cell.trackColor.getRed()),
            static_cast<uint8_t>(cell.trackColor.getGreen()),
            static_cast<uint8_t>(cell.trackColor.getBlue())
        ));
    } else {
        // Empty cell - dark
        cellPaint.setColor(SkColorSetARGB(static_cast<uint8_t>(40 + cell.hoverProgress * 20), 40, 40, 45));
    }
    
    canvas->drawRRect(cellRRect, cellPaint);
    
    // Draw border
    SkPaint borderPaint;
    borderPaint.setAntiAlias(true);
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(cell.isSelected ? 2.0f : 1.0f);
    
    if (cell.isSelected) {
        borderPaint.setColor(SkColorSetRGB(0, 255, 255));  // Cyan selection
    } else if (cell.hasClip) {
        borderPaint.setColor(SkColorSetARGB(150, 
            static_cast<uint8_t>(cell.trackColor.getRed()),
            static_cast<uint8_t>(cell.trackColor.getGreen()),
            static_cast<uint8_t>(cell.trackColor.getBlue())
        ));
    } else {
        borderPaint.setColor(SkColorSetARGB(60, 100, 100, 100));
    }
    
    canvas->drawRRect(cellRRect, borderPaint);
    
    // Draw cell content if has clip
    if (cell.hasClip) {
        drawCellContent(canvas, cell, cellRect);
    }
    
    // Draw interaction zone highlight on hover
    if (trackIndex == hoveredCell_.x && sceneIndex == hoveredCell_.y && hoveredZone_ != InteractionZone::None) {
        drawInteractionZoneHighlight(canvas, cellRect, hoveredZone_);
    }
}

void SkiaGridComponent::drawCellContent(SkCanvas* canvas, const SessionClipCell& cell, const SkRect& cellRect)
{
    float padding = CELL_PADDING;
    SkRect contentRect = SkRect::MakeLTRB(
        cellRect.fLeft + padding,
        cellRect.fTop + padding + cellRect.height() * GAIN_ZONE_RATIO,
        cellRect.fRight - padding,
        cellRect.fBottom - padding
    );
    
    // Draw waveform or MIDI sparkline at bottom
    SkRect sparklineRect = SkRect::MakeLTRB(
        contentRect.fLeft,
        contentRect.fBottom - contentRect.height() * WAVEFORM_HEIGHT_RATIO,
        contentRect.fRight,
        contentRect.fBottom
    );
    
    if (cell.isAudio) {
        drawWaveformSparkline(canvas, cell, sparklineRect);
    } else {
        drawMidiSparkline(canvas, cell, sparklineRect);
    }
    
    // Draw playhead position if playing
    if (cell.isPlaying) {
        float playheadX = contentRect.fLeft + contentRect.width() * 0.5f;  // TODO: Use actual playhead
        
        SkPaint playheadPaint;
        playheadPaint.setColor(SkColorSetRGB(255, 255, 255));
        playheadPaint.setStrokeWidth(2.0f);
        canvas->drawLine(playheadX, contentRect.fTop, playheadX, contentRect.fBottom, playheadPaint);
    }
}

void SkiaGridComponent::drawWaveformSparkline(SkCanvas* canvas, const SessionClipCell& cell, const SkRect& rect)
{
    if (cell.waveformPeaks.empty()) {
        // Generate dummy waveform
        SkPaint wavePaint;
        wavePaint.setColor(SkColorSetARGB(180, 255, 255, 255));
        wavePaint.setAntiAlias(true);
        
        SkPath path;
        float centerY = rect.centerY();
        float halfHeight = rect.height() * 0.4f;
        
        for (int i = 0; i < 20; ++i) {
            float x = rect.fLeft + (rect.width() * i / 20.0f);
            float peak = std::sin(i * 0.5f) * 0.5f + 0.5f;
            float y = centerY - halfHeight * peak;
            
            if (i == 0) path.moveTo(x, y);
            else path.lineTo(x, y);
        }
        
        wavePaint.setStyle(SkPaint::kStroke_Style);
        wavePaint.setStrokeWidth(1.5f);
        canvas->drawPath(path, wavePaint);
    } else {
        // Draw actual waveform peaks
        SkPaint wavePaint;
        wavePaint.setColor(SkColorSetARGB(200, 255, 255, 255));
        wavePaint.setAntiAlias(true);
        
        size_t numPeaks = cell.waveformPeaks.size();
        float peakWidth = rect.width() / static_cast<float>(numPeaks);
        float centerY = rect.centerY();
        float halfHeight = rect.height() * 0.45f;
        
        for (size_t i = 0; i < numPeaks; ++i) {
            float x = rect.fLeft + peakWidth * i;
            float peak = cell.waveformPeaks[i];
            float top = centerY - halfHeight * peak;
            float bottom = centerY + halfHeight * peak;
            
            canvas->drawRect(SkRect::MakeLTRB(x, top, x + peakWidth * 0.8f, bottom), wavePaint);
        }
    }
}

void SkiaGridComponent::drawMidiSparkline(SkCanvas* canvas, const SessionClipCell& cell, const SkRect& rect)
{
    // Draw MIDI note bars
    SkPaint notePaint;
    notePaint.setColor(SkColorSetARGB(200, 255, 180, 0));  // Amber for MIDI
    notePaint.setAntiAlias(true);
    
    // Dummy MIDI notes
    for (int i = 0; i < 8; ++i) {
        float x = rect.fLeft + (rect.width() * i / 8.0f);
        float noteHeight = rect.height() * (0.3f + std::sin(i * 0.7f) * 0.3f);
        float y = rect.fBottom - noteHeight;
        
        canvas->drawRect(SkRect::MakeXYWH(x + 1, y, rect.width() / 10.0f, noteHeight - 2), notePaint);
    }
    
    (void)cell;  // Silence unused warning
}

void SkiaGridComponent::drawGlowEffect(SkCanvas* canvas, const SkRRect& rrect, SkColor color)
{
    SkPaint glowPaint;
    glowPaint.setColor(color);
    glowPaint.setAntiAlias(true);
    glowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, GLOW_BLUR_SIGMA));
    
    canvas->drawRRect(rrect, glowPaint);
}

void SkiaGridComponent::drawInteractionZoneHighlight(SkCanvas* canvas, const SkRect& cellRect, InteractionZone zone)
{
    SkPaint highlightPaint;
    highlightPaint.setAntiAlias(true);
    highlightPaint.setColor(SkColorSetARGB(60, 255, 255, 255));
    
    SkRect zoneRect;
    
    switch (zone) {
        case InteractionZone::Gain:
            zoneRect = SkRect::MakeLTRB(
                cellRect.fLeft, cellRect.fTop,
                cellRect.fRight, cellRect.fTop + cellRect.height() * GAIN_ZONE_RATIO
            );
            break;
        case InteractionZone::LoopStart:
            zoneRect = SkRect::MakeLTRB(
                cellRect.fLeft, cellRect.fTop,
                cellRect.fLeft + cellRect.width() * LOOP_ZONE_RATIO, cellRect.fBottom
            );
            break;
        case InteractionZone::LoopEnd:
            zoneRect = SkRect::MakeLTRB(
                cellRect.fRight - cellRect.width() * LOOP_ZONE_RATIO, cellRect.fTop,
                cellRect.fRight, cellRect.fBottom
            );
            break;
        default:
            return;
    }
    
    canvas->drawRect(zoneRect, highlightPaint);
}
#endif  // ZENITH_USE_SKIA

//==============================================================================
// Hit Testing
//==============================================================================
juce::Point<int> SkiaGridComponent::getCellAt(float x, float y) const
{
    if (x < sceneHeaderWidth_ || y < trackHeaderHeight_) {
        return {-1, -1};
    }
    
    int trackIndex = static_cast<int>((x - sceneHeaderWidth_) / cellWidth_);
    int sceneIndex = static_cast<int>((y - trackHeaderHeight_) / cellHeight_);
    
    if (trackIndex >= 0 && trackIndex < numTracks_ && 
        sceneIndex >= 0 && sceneIndex < numScenes_) {
        return {trackIndex, sceneIndex};
    }
    
    return {-1, -1};
}

InteractionZone SkiaGridComponent::getZoneAt(float x, float y) const
{
    auto cellPos = getCellAt(x, y);
    if (cellPos.x < 0 || cellPos.y < 0) {
        return InteractionZone::None;
    }
    
    auto bounds = getCellBounds(cellPos.x, cellPos.y);
    float localX = x - bounds.getX();
    float localY = y - bounds.getY();
    float width = bounds.getWidth();
    float height = bounds.getHeight();
    
    // Check zones
    if (localY < height * GAIN_ZONE_RATIO) {
        return InteractionZone::Gain;
    }
    if (localX < width * LOOP_ZONE_RATIO) {
        return InteractionZone::LoopStart;
    }
    if (localX > width * (1.0f - LOOP_ZONE_RATIO)) {
        return InteractionZone::LoopEnd;
    }
    
    return InteractionZone::Trigger;
}

juce::Rectangle<float> SkiaGridComponent::getCellBounds(int trackIndex, int sceneIndex) const
{
    float x = sceneHeaderWidth_ + trackIndex * cellWidth_;
    float y = trackHeaderHeight_ + sceneIndex * cellHeight_;
    return {x, y, cellWidth_, cellHeight_};
}

//==============================================================================
// Mouse Interaction
//==============================================================================
void SkiaGridComponent::mouseMove(const juce::MouseEvent& e)
{
    auto newCell = getCellAt(static_cast<float>(e.x), static_cast<float>(e.y));
    auto newZone = getZoneAt(static_cast<float>(e.x), static_cast<float>(e.y));
    
    if (newCell != hoveredCell_ || newZone != hoveredZone_) {
        hoveredCell_ = newCell;
        hoveredZone_ = newZone;
        repaint();
    }
}

void SkiaGridComponent::mouseDown(const juce::MouseEvent& e)
{
    auto cellPos = getCellAt(static_cast<float>(e.x), static_cast<float>(e.y));
    auto zone = getZoneAt(static_cast<float>(e.x), static_cast<float>(e.y));
    
    if (cellPos.x < 0 || cellPos.y < 0) return;
    
    activeZone_ = zone;
    dragStartPos_ = e.position;
    
    auto* cell = getClipCell(cellPos.x, cellPos.y);
    if (cell) {
        dragStartValue_ = (zone == InteractionZone::Gain) ? cell->gainDb : 0.0f;
        
        // Select cell
        selectedCell_ = cellPos;
        repaint();
    }
}

void SkiaGridComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (activeZone_ == InteractionZone::None) return;
    
    auto* cell = getClipCell(selectedCell_.x, selectedCell_.y);
    if (!cell || !cell->hasClip) return;
    
    float deltaX = e.position.x - dragStartPos_.x;
    float deltaY = e.position.y - dragStartPos_.y;
    
    switch (activeZone_) {
        case InteractionZone::Gain:
            // Vertical drag = volume (inverted: drag up = louder)
            cell->gainDb = juce::jlimit(-60.0f, 12.0f, dragStartValue_ - deltaY * 0.5f);
            break;
            
        case InteractionZone::LoopStart:
            cell->loopStart = juce::jlimit(0.0f, cell->loopEnd - 0.01f, 
                                           cell->loopStart + deltaX * 0.002f);
            break;
            
        case InteractionZone::LoopEnd:
            cell->loopEnd = juce::jlimit(cell->loopStart + 0.01f, 1.0f,
                                         cell->loopEnd + deltaX * 0.002f);
            break;
            
        default:
            break;
    }
    
    repaint();
}

void SkiaGridComponent::mouseUp(const juce::MouseEvent& e)
{
    if (activeZone_ == InteractionZone::Trigger) {
        auto cellPos = getCellAt(static_cast<float>(e.x), static_cast<float>(e.y));
        if (cellPos.x >= 0 && cellPos.y >= 0) {
            triggerClip(cellPos.x, cellPos.y);
        }
    }
    
    activeZone_ = InteractionZone::None;
}

void SkiaGridComponent::mouseExit(const juce::MouseEvent&)
{
    hoveredCell_ = {-1, -1};
    hoveredZone_ = InteractionZone::None;
    repaint();
}

//==============================================================================
// Clip Actions
//==============================================================================
void SkiaGridComponent::triggerClip(int trackIndex, int sceneIndex)
{
    auto* cell = getClipCell(trackIndex, sceneIndex);
    if (cell && cell->hasClip) {
        // Toggle playing state
        cell->isPlaying = !cell->isPlaying;
        cell->isQueued = false;
        
        // TODO: Send to engine for actual playback
        DBG("Triggered clip at Track " << trackIndex << ", Scene " << sceneIndex);
        
        repaint();
    }
}

void SkiaGridComponent::triggerScene(int sceneIndex)
{
    for (int t = 0; t < numTracks_; ++t) {
        auto* cell = getClipCell(t, sceneIndex);
        if (cell && cell->hasClip) {
            cell->isQueued = true;
        }
    }
    
    DBG("Triggered Scene " << sceneIndex);
    repaint();
}

void SkiaGridComponent::stopAllClips()
{
    for (int t = 0; t < numTracks_; ++t) {
        for (int s = 0; s < numScenes_; ++s) {
            auto& cell = cells_[static_cast<size_t>(t)][static_cast<size_t>(s)];
            cell.isPlaying = false;
            cell.isQueued = false;
        }
    }
    repaint();
}

//==============================================================================
// Audio Thread Updates
//==============================================================================
void SkiaGridComponent::updatePlayingState(int trackIndex, int sceneIndex, bool playing)
{
    if (trackIndex >= 0 && trackIndex < numTracks_ && 
        sceneIndex >= 0 && sceneIndex < numScenes_) {
        cells_[static_cast<size_t>(trackIndex)][static_cast<size_t>(sceneIndex)].isPlaying = playing;
        repaint();
    }
}

void SkiaGridComponent::updatePlayheadPosition(int trackIndex, float normalizedPosition)
{
    if (trackIndex >= 0 && trackIndex < 32) {
        playheadPositions_[trackIndex].store(normalizedPosition);
    }
}

//==============================================================================
// Helpers
//==============================================================================
void SkiaGridComponent::rebuildCellCache()
{
    // Future: Use SkSurface to cache static elements
}

void SkiaGridComponent::invalidateBackgroundCache()
{
    backgroundDirty_ = true;
#ifdef ZENITH_USE_SKIA
    backgroundCache_.reset();
#endif
}

} // namespace zenith
