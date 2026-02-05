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

    TakeFolderComponent.cpp
    Created: 2025-12-24
    Author:  Zenith DAW

  ==============================================================================
*/


#include "TakeFolderComponent.h"
#include "../design-system/ZenithDesignSystem.h"
#include "../design-system/ZenithTheme.h"
#include "ArrangerGridUtils.h"
#include <core/SkCanvas.h>
#include <core/SkPaint.h>
#include <core/SkRRect.h>
#include <core/SkFont.h>

namespace zenith {

TakeFolderComponent::TakeFolderComponent(ProjectState &projectState,
                                         ArrangerGridUtils &gridUtils,
                                         juce::ValueTree folderNode)
    : projectState_(projectState), gridUtils_(gridUtils),
      folderNode_(folderNode) {
  setMouseCursor(juce::MouseCursor::NormalCursor);
  folderNode_.addListener(this);

  // Initialize cached fonts
  cachedFont_ = design::getSkFont(12.0f, design::FontWeight::Medium);
  cachedFontSmall_ = design::getSkFont(10.0f, design::FontWeight::Bold);
  
  updateState();
}

TakeFolderComponent::~TakeFolderComponent() {
  folderNode_.removeListener(this);
}

void TakeFolderComponent::updateState() {
  if (!folderNode_.isValid()) return;

  isExpanded_ = folderNode_[ProjectState::PROP_EXPANDED];
  activeTakeIndex_ = folderNode_[ProjectState::PROP_ACTIVE_TAKE];
  startBeats_ = folderNode_[ProjectState::PROP_START_BEATS];
  lengthBeats_ = folderNode_[ProjectState::PROP_LENGTH_BEATS];

  // Request repaint on state change
  repaint();
}

void TakeFolderComponent::resized() {
  // Bounds handled by parent, but we might need to update internal layout
}

void TakeFolderComponent::setZoomLevel(double pixelsPerBeat) {
  pixelsPerBeat_ = pixelsPerBeat;
  repaint();
}

void TakeFolderComponent::setHeightPerLane(int height) {
  laneHeight_ = height;
  repaint();
}

//==============================================================================
// Rendering
//==============================================================================

void TakeFolderComponent::paint(juce::Graphics &g) {
  // Skia rendering used - see drawSkia()
  juce::ignoreUnused(g);
}

void TakeFolderComponent::drawSkia(SkCanvas *canvas) {
  if (!folderNode_.isValid()) return;

  using namespace zenith::design;

  // Draw Background
  SkRect bounds = SkRect::MakeWH((float)getWidth(), (float)getHeight());
  SkPaint bgPaint;
  juce::Colour bgExp = ZenithTheme::Colors::bg_00;
  juce::Colour bgCol = ZenithTheme::Colors::bg_01;
  bgPaint.setColor(isExpanded_ ? SkColorSetARGB(255, bgExp.getRed(), bgExp.getGreen(), bgExp.getBlue()) : SkColorSetARGB(255, bgCol.getRed(), bgCol.getGreen(), bgCol.getBlue()));
  canvas->drawRect(bounds, bgPaint);

  if (isExpanded_) {
    drawExpanded(canvas);
  } else {
    drawCollapsed(canvas);
  }

  // Draw Outline
  SkPaint borderPaint;
  borderPaint.setStyle(SkPaint::kStroke_Style);
  juce::Colour border = ZenithTheme::Colors::border_default;
  borderPaint.setColor(SkColorSetARGB(100, border.getRed(), border.getGreen(), border.getBlue()));
  borderPaint.setStrokeWidth(1.0f);
  canvas->drawRect(bounds, borderPaint);
}

void TakeFolderComponent::drawCollapsed(SkCanvas *canvas) {
  using namespace design;
  
  auto takesNode = folderNode_.getChildWithName(ProjectState::ID_CLIPS);
  
  // Count comp regions (children of type ID_COMP_REGION)
  int numCompRegions = 0;
  for (const auto& child : folderNode_) {
    if (child.hasType(ProjectState::ID_COMP_REGION))
      numCompRegions++;
  }
  
  // If no comp regions defined, just draw label
  if (numCompRegions == 0) {
    SkPaint textPaint;
    juce::Colour txt = ZenithTheme::Colors::text_secondary;
    textPaint.setColor(SkColorSetARGB(txt.getAlpha(), txt.getRed(), txt.getGreen(), txt.getBlue()));
    textPaint.setAntiAlias(true);
    canvas->drawString("Comp (Empty)", 10, (float)getHeight() / 2 + 4, cachedFont_, textPaint);
    return;
  }
  
  // Draw each comp region with waveform from its assigned take
  for (const auto& regionNode : folderNode_) {
    if (!regionNode.hasType(ProjectState::ID_COMP_REGION))
      continue;
    double regionStart = regionNode[ProjectState::PROP_START];
    double regionLength = regionNode[ProjectState::PROP_LENGTH];
    int takeIndex = regionNode[ProjectState::PROP_TAKE_INDEX];
    
    // Calculate pixel bounds for this region
    float x = static_cast<float>((regionStart - startBeats_) * pixelsPerBeat_);
    float w = static_cast<float>(regionLength * pixelsPerBeat_);
    float h = static_cast<float>(getHeight());
    
    // Skip if off-screen
    if (x + w < 0 || x > getWidth()) continue;
    
    SkRect regionRect = SkRect::MakeXYWH(x, 2.0f, w, h - 4.0f);
    
    // Get the take for this region
    if (takeIndex >= 0 && takeIndex < takesNode.getNumChildren()) {
      auto clipNode = takesNode.getChild(takeIndex);
      
      // Draw waveform for this segment
      SkColor waveColor = SkColorSetARGB(255, 80, 200, 120); // Green for comp
      drawTakeWaveform(canvas, clipNode, regionRect, waveColor);
      
      // Draw subtle separator between regions
      SkPaint sepPaint;
      juce::Colour sep = ZenithTheme::Colors::border_subtle;
      sepPaint.setColor(SkColorSetARGB(sep.getAlpha(), sep.getRed(), sep.getGreen(), sep.getBlue()));
      sepPaint.setStrokeWidth(1.0f);
      canvas->drawLine(x, 0, x, h, sepPaint);
    }
  }
  
  // Draw "Comp" label overlay
  SkPaint labelBg;
  juce::Colour lbg = ZenithTheme::Colors::bg_03;
  labelBg.setColor(SkColorSetARGB(180, lbg.getRed(), lbg.getGreen(), lbg.getBlue()));
  canvas->drawRect(SkRect::MakeXYWH(0, 0, 50, 18), labelBg);
  
  SkPaint textPaint;
  juce::Colour success = ZenithTheme::Colors::success;
  textPaint.setColor(SkColorSetARGB(255, success.getRed(), success.getGreen(), success.getBlue()));
  textPaint.setAntiAlias(true);
  canvas->drawString("COMP", 4, 13, cachedFontSmall_, textPaint);
}

void TakeFolderComponent::drawExpanded(SkCanvas *canvas) {
  auto takesNode = folderNode_.getChildWithName(ProjectState::ID_CLIPS); // Takes are children
  int numTakes = takesNode.getNumChildren();
  
  // Header / Master Lane
  SkRect headerRect = SkRect::MakeXYWH(0, 0, (float)getWidth(), (float)headerHeight_);
  SkPaint headerPaint;
  juce::Colour hdr = ZenithTheme::Colors::bg_03;
  headerPaint.setColor(SkColorSetARGB(255, hdr.getRed(), hdr.getGreen(), hdr.getBlue()));
  canvas->drawRect(headerRect, headerPaint);
  
  // Draw Lanes
  int y = headerHeight_;
  juce::Colour lane1 = ZenithTheme::Colors::bg_01;
  juce::Colour lane2 = ZenithTheme::Colors::bg_02;
  
  for (int i = 0; i < numTakes; ++i) {
    SkRect laneRect = SkRect::MakeXYWH(0, (float)y, (float)getWidth(), (float)laneHeight_);
    
    // Lane Background (Alternating?)
    SkPaint laneBg;
    laneBg.setColor((i % 2 == 0) ? SkColorSetARGB(255, lane1.getRed(), lane1.getGreen(), lane1.getBlue()) : SkColorSetARGB(255, lane2.getRed(), lane2.getGreen(), lane2.getBlue()));
    canvas->drawRect(laneRect, laneBg);
    
    // Draw Take Waveform
    auto child = takesNode.getChild(i);
    SkRect contentRect = laneRect.makeInset(1.0f, 1.0f);
    
    SkColor waveColor = (activeTakeIndex_ == i) ? SkColorSetARGB(255, 100, 255, 100) : SkColorSetARGB(255, 150, 150, 150);
    drawTakeWaveform(canvas, child, contentRect, waveColor);
    
    // Draw Highlight if this take is active in a comp region
    if (activeTakeIndex_ == i) {
       SkPaint activePaint;
       activePaint.setColor(SkColorSetARGB(40, 0, 255, 0));
       canvas->drawRect(laneRect, activePaint);
    }

    y += laneHeight_;
  }
  
  // Draw Drag Area
  if (dragState_.active) {
    float startX = (float)(dragState_.startBeat - startBeats_) * (float)pixelsPerBeat_;
    float endX = (float)(getBeatAtX(dragState_.startPos.x) - startBeats_) * (float)pixelsPerBeat_;
    
    SkRect dragRect = SkRect::MakeXYWH(startX, (float)(headerHeight_ + dragState_.takeIndex * laneHeight_), endX - startX, (float)laneHeight_);
    
    SkPaint dragPaint;
    dragPaint.setColor(SkColorSetARGB(100, 255, 255, 0)); // Highlight yellow
    canvas->drawRect(dragRect, dragPaint);
  }
}

// Redefine rendering logic to actually use internal data
void TakeFolderComponent::drawTakeWaveform(SkCanvas* canvas, const juce::ValueTree& clipNode, SkRect bounds, SkColor color) {
    juce::String path = clipNode[ProjectState::PROP_AUDIO_FILE].toString();
    
    // Fallback if no audio file (e.g. MIDI) or empty
    if (path.isEmpty()) {
        SkPaint paint;
        paint.setColor(color);
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setStrokeWidth(1.0f);
        canvas->drawLine(bounds.left(), bounds.centerY(), bounds.right(), bounds.centerY(), paint);
        return;
    }

    const auto* cache = gridUtils_.getWaveformCache(path);
    if (!cache || !cache->isValid || cache->minPeaks.empty()) {
        // Trigger async waveform cache build
        gridUtils_.buildWaveformCache(path);

        // Draw temporary loading indicator (waveform will appear on next repaint after cache is ready)
        
        SkPaint paint;
        paint.setColor(color);
        paint.setStrokeWidth(1.0f);
        canvas->drawLine(bounds.left(), bounds.centerY(), bounds.right(), bounds.centerY(), paint);
        return;
    }

    SkPath wavePath;
    float centerY = bounds.centerY();
    float height = bounds.height();
    float amp = height * 0.5f * 0.9f; // 90% height
    
    double clipOffset = clipNode[ProjectState::PROP_OFFSET];
    
    // We iterate pixels in bounds
    int startPx = (int)bounds.left();
    int endPx = (int)bounds.right();
    
    // Optimization: Step size
    int step = 1; 
    
    wavePath.moveTo(bounds.left(), centerY);
    
    // Get project context for mapping
    double sampleRate = projectState_.getSampleRate();
    double bpm = projectState_.getTempo();
    if (bpm <= 0.0) bpm = 120.0;
    
    int samplesPerPixel = cache->samplesPerPixel;
    if (samplesPerPixel <= 0) samplesPerPixel = 256; // Fallback
    
    size_t numPeaks = cache->maxPeaks.size();
    if (numPeaks == 0) return;
    
    double secondsPerBeat = 60.0 / bpm;
    
    for (int x = startPx; x < endPx; x += step) {
        float relativePx = x - bounds.left();
        double relativeBeats = relativePx / pixelsPerBeat_;
        
        // Calculate beat position within the audio file
        // internal offset is usually start time in file (in beats for simplicity here,
        // or converted from seconds. Assuming offset is beats as per similar usage).
        double fileBeats = clipOffset + relativeBeats;
        
        if (fileBeats < 0) continue;
        
        // Convert to samples
        double fileSeconds = fileBeats * secondsPerBeat;
        int64_t fileSample = static_cast<int64_t>(fileSeconds * sampleRate);
        
        size_t index = static_cast<size_t>(fileSample / samplesPerPixel);
        
        if (index >= numPeaks) break; // End of file
        
        float minVal = cache->minPeaks[index];
        float maxVal = cache->maxPeaks[index];
        
        // Draw vertical line for min/max
        float yMin = centerY + minVal * amp;
        float yMax = centerY + maxVal * amp;
        
        wavePath.moveTo(x, yMin);
        wavePath.lineTo(x, yMax);
    }
    
    SkPaint paint;
    paint.setColor(color);
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(1.0f);
    paint.setAntiAlias(false); // Sharp lines for waveform
    canvas->drawPath(wavePath, paint);
}

//==============================================================================
// Interaction
//==============================================================================

void TakeFolderComponent::mouseDown(const juce::MouseEvent &e) {
  if (!isExpanded_) return;
  
  int y = e.getPosition().y;
  if (y < headerHeight_) return; // Clicked header
  
  int takeIndex = (y - headerHeight_) / laneHeight_;
  
  // Start Comp Region Drag
  dragState_.active = true;
  dragState_.startPos = e.getPosition();
  dragState_.startBeat = getBeatAtX(e.getPosition().x);
  dragState_.takeIndex = takeIndex;
  
  repaint();
}

void TakeFolderComponent::mouseDrag(const juce::MouseEvent &e) {
  if (dragState_.active) {
    repaint();
  }
}

void TakeFolderComponent::mouseUp(const juce::MouseEvent &e) {
  if (dragState_.active) {
    double endBeat = getBeatAtX(e.getPosition().x);
    double start = std::min(dragState_.startBeat, endBeat);
    double len = std::abs(endBeat - dragState_.startBeat);
    
    if (len > 0.0) {
        // Commit Comp Region to ProjectState
        projectState_.setCompRegion(folderNode_[ProjectState::PROP_ID], start, len, dragState_.takeIndex);
    }
    
    dragState_.active = false;
    repaint();
  }
}

//==============================================================================
// Helpers
//==============================================================================

double TakeFolderComponent::getBeatAtX(int x) const {
  if (pixelsPerBeat_ <= 0.0) return startBeats_;  // Prevent division by zero
  return startBeats_ + (x / pixelsPerBeat_);
}

int TakeFolderComponent::getTakeIndexAtY(int y) const {
  // Collapsed mode has no separate take lanes
  if (!isExpanded_) return activeTakeIndex_;
  
  // Header area
  if (y < headerHeight_) return -1;
  
  // Calculate which lane the y coordinate falls in
  int laneY = y - headerHeight_;
  int takeIndex = laneY / laneHeight_;
  
  // Validate against actual take count
  auto takesNode = folderNode_.getChildWithName(ProjectState::ID_CLIPS);
  if (!takesNode.isValid()) return -1;
  
  if (takeIndex >= 0 && takeIndex < takesNode.getNumChildren()) {
    return takeIndex;
  }
  
  return -1;
}

//==============================================================================
// ValueTree Listener
//==============================================================================

void TakeFolderComponent::valueTreePropertyChanged(juce::ValueTree &tree,
                                                   const juce::Identifier &property) {
  if (tree == folderNode_) {
    updateState();
  }
}

void TakeFolderComponent::valueTreeChildAdded(juce::ValueTree &parent,
                                              juce::ValueTree &child) {
    if (parent == folderNode_) repaint();
}

void TakeFolderComponent::valueTreeChildRemoved(juce::ValueTree &parent,
                                                juce::ValueTree &child, int) {
    if (parent == folderNode_) repaint();
}

} // namespace zenith
