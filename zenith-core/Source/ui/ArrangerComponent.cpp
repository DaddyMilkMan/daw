// POLISH: spacing normalized to 8px grid (trackHeight 64, rulerHeight 32, clip
// text offset 8px)

#include "ArrangerComponent.h"
#ifdef ZENITH_USE_SKIA
    #include <include/core/SkCanvas.h>
    #include <include/core/SkPaint.h>
    #include <include/core/SkFont.h>
    #include <include/core/SkPath.h>
    #include <include/core/SkRRect.h>
    #include <include/effects/SkGradientShader.h>
    #include "../Source/ui/skia/SkiaTheme.h"
#endif
#include "skia/SkiaTheme.h"

#ifdef ZENITH_USE_SKIA
#include <include/core/SkFont.h>
#include <include/core/SkPaint.h>
#include <include/core/SkPath.h>
#include <include/core/SkRRect.h>
#include <include/effects/SkGradientShader.h>
#endif

//==============================================================================
ArrangerComponent::ArrangerComponent(ProjectState &ps) : projectState(ps) {
  projectState.state.addListener(this);
  rebuildClipViews();
}

ArrangerComponent::~ArrangerComponent() {
  projectState.state.removeListener(this);
}

void ArrangerComponent::paint(juce::Graphics &g) {
  SkiaCanvasComponent::paint(g);
}

void ArrangerComponent::resized() {
  SkiaCanvasComponent::resized();
  recomputeClipBounds();
}

#ifdef ZENITH_USE_SKIA
void ArrangerComponent::paintSkia(SkCanvas &canvas,
                                  const juce::Rectangle<int> &bounds) {
  using namespace zenith;
  auto &theme = SkiaTheme::getInstance();
  auto colors = theme.getColors();

  float width = (float)bounds.getWidth();
  float height = (float)bounds.getHeight();

  // BACKGROUND
  canvas.clear(colors.bg1);

  // TIME RULER
  SkRect rulerRect = SkRect::MakeXYWH(0, 0, width, rulerHeight);
  SkPaint rulerBgPaint;
  rulerBgPaint.setColor(colors.bg2);
  canvas.drawRect(rulerRect, rulerBgPaint);

  SkPaint rulerBorderPaint;
  rulerBorderPaint.setColor(colors.borderSubtle);
  rulerBorderPaint.setStyle(SkPaint::kStroke_Style);
  rulerBorderPaint.setStrokeWidth(1.0f);
  canvas.drawLine(0, rulerHeight, width, rulerHeight, rulerBorderPaint);

  auto &typo = SkiaTheme::getInstance().getTypography();
  SkFont rulerFont;
  rulerFont.setSize(typo.small.size);
  rulerFont.setEdging(SkFont::Edging::kAntiAlias);

  SkPaint rulerTextPaint;
  rulerTextPaint.setColor(colors.textMuted);
  rulerTextPaint.setAntiAlias(true);

  double currentBeat = viewStartBeats;
  float xPos = beatsToX(currentBeat);

  while (xPos < width) {
    int barNumber = (int)(currentBeat / 4) + 1;
    int beatInBar = ((int)currentBeat % 4) + 1;

    if (beatInBar == 1) {
      juce::String barText = juce::String(barNumber);
      canvas.drawString(barText.toRawUTF8(), xPos + 4, rulerHeight - 8,
                        rulerFont, rulerTextPaint);
    }

    currentBeat += 1.0;
    xPos = beatsToX(currentBeat);
  }

  // TRACK LANES
  float contentY = rulerHeight;
  auto tracksTree = projectState.state.getChildWithName("tracks");
  int numTracks = tracksTree.getNumChildren();

  for (int trackIdx = 0; trackIdx < numTracks; ++trackIdx) {
    float trackY = contentY + trackIdx * trackHeight;
    if (trackY > height)
      break;

    bool isEven = (trackIdx % 2 == 0);
    SkPaint trackBgPaint;
    trackBgPaint.setColor(isEven ? colors.bg1 : colors.bg2);
    canvas.drawRect(SkRect::MakeXYWH(0, trackY, width, trackHeight),
                    trackBgPaint);

    SkPaint separatorPaint;
    separatorPaint.setColor(colors.borderSubtle);
    separatorPaint.setStyle(SkPaint::kStroke_Style);
    separatorPaint.setStrokeWidth(1.0f);
    canvas.drawLine(0, trackY + trackHeight, width, trackY + trackHeight,
                    separatorPaint);
  }

  // GRID LINES
  SkPaint beatLinePaint;
  beatLinePaint.setColor(colors.borderSubtle);
  beatLinePaint.setStyle(SkPaint::kStroke_Style);
  beatLinePaint.setStrokeWidth(1.0f);

  SkPaint barLinePaint;
  barLinePaint.setColor(colors.borderStrong);
  barLinePaint.setStyle(SkPaint::kStroke_Style);
  barLinePaint.setStrokeWidth(1.0f);

  currentBeat = viewStartBeats;
  xPos = beatsToX(currentBeat);

  while (xPos < width) {
    int beatInBar = (int)currentBeat % 4;
    if (beatInBar == 0) {
      canvas.drawLine(xPos, contentY, xPos, height, barLinePaint);
    } else {
      canvas.drawLine(xPos, contentY, xPos, height, beatLinePaint);
    }
    currentBeat += 1.0;
    xPos = beatsToX(currentBeat);
  }

  // CLIPS
  for (const auto &clipView : clipViews) {
    if (!clipView.bounds.isEmpty()) {
      SkRect clipRect = SkRect::MakeXYWH(
          clipView.bounds.getX(), clipView.bounds.getY(),
          clipView.bounds.getWidth(), clipView.bounds.getHeight());

      SkColor clipColor =
          clipView.isMidi ? colors.clipHarmony : colors.clipLeads;
      if (clipView.isSelected)
        clipColor = colors.accentMain;

      SkPaint clipPaint;
      SkColor topColor = clipColor;
      SkColor bottomColor = SkColorSetARGB(
          SkColorGetA(clipColor), (int)(SkColorGetR(clipColor) * 0.7f),
          (int)(SkColorGetG(clipColor) * 0.7f),
          (int)(SkColorGetB(clipColor) * 0.7f));

      SkPoint gradientPoints[2] = {
          SkPoint::Make(clipRect.centerX(), clipRect.top()),
          SkPoint::Make(clipRect.centerX(), clipRect.bottom())};
      SkColor gradientColors[2] = {topColor, bottomColor};

      sk_sp<SkShader> gradient = SkGradientShader::MakeLinear(
          gradientPoints, gradientColors, nullptr, 2, SkTileMode::kClamp);
      clipPaint.setShader(gradient);
      clipPaint.setAntiAlias(true);

      SkRRect rrect;
      rrect.setRectXY(clipRect, 4.0f, 4.0f);
      canvas.drawRRect(rrect, clipPaint);

      SkPaint borderPaint;
      borderPaint.setColor(clipView.isSelected ? colors.accentMain
                                               : colors.borderStrong);
      borderPaint.setStyle(SkPaint::kStroke_Style);
      borderPaint.setStrokeWidth(clipView.isSelected ? 2.0f : 1.0f);
      borderPaint.setAntiAlias(true);
      canvas.drawRRect(rrect, borderPaint);

      if (clipRect.height() > 24) { // 8px grid: 20→24
        auto &typo = SkiaTheme::getInstance().getTypography();
        SkFont clipFont;
        clipFont.setSize(typo.body.size);
        clipFont.setEdging(SkFont::Edging::kAntiAlias);

        SkPaint clipTextPaint;
        clipTextPaint.setColor(colors.textStrong);
        clipTextPaint.setAntiAlias(true);

        juce::String clipName = clipView.isMidi ? "MIDI Clip" : "Audio Clip";
        canvas.drawString(clipName.toRawUTF8(),
                          clipRect.left() + 8, // 8px grid: 6→8
                          clipRect.top() + 16, clipFont, clipTextPaint);
      }
    }
  }

  // MARQUEE
  if (currentDragMode == DragMode::Marquee && !marqueeRect.isEmpty()) {
    SkPaint marqueePaint;
    marqueePaint.setColor(theme.getSelectionStyle().tintColor);
    SkRect marqueeSkRect =
        SkRect::MakeXYWH(marqueeRect.getX(), marqueeRect.getY(),
                         marqueeRect.getWidth(), marqueeRect.getHeight());
    canvas.drawRect(marqueeSkRect, marqueePaint);

    SkPaint marqueeBorderPaint;
    marqueeBorderPaint.setColor(colors.accentMain);
    marqueeBorderPaint.setStyle(SkPaint::kStroke_Style);
    marqueeBorderPaint.setStrokeWidth(1.0f);
    canvas.drawRect(marqueeSkRect, marqueeBorderPaint);
  }
}
#endif

void ArrangerComponent::mouseDown(const juce::MouseEvent &e) {
  if (!e.mods.isLeftButtonDown())
    return;

  auto point = e.position;
  dragStartPoint = point;

  auto *clip = findClipAtPoint(point);

  if (clip != nullptr) {
    if (clip->isInLeftResizeZone(point)) {
      currentDragMode = DragMode::ResizeClipLeft;
      resizingClipId = clip->clipId;
      resizeOriginalStart = clip->startBeats;
      resizeOriginalLength = clip->lengthBeats;
    } else if (clip->isInRightResizeZone(point)) {
      currentDragMode = DragMode::ResizeClipRight;
      resizingClipId = clip->clipId;
      resizeOriginalStart = clip->startBeats;
      resizeOriginalLength = clip->lengthBeats;
    } else {
      currentDragMode = DragMode::MoveClips;

      if (!isClipSelected(clip->clipId)) {
        if (!e.mods.isCommandDown())
          clearSelection();
        selectClip(clip->clipId, true);
      }

      clipDragStates.clear();
      for (const auto &clipId : selectedClipIds) {
        auto *selectedClip = findClipView(clipId);
        if (selectedClip) {
          ClipDragState state;
          state.clipId = clipId;
          state.originalStartBeats = selectedClip->startBeats;
          state.originalTrackIndex = yToTrackIndex(selectedClip->bounds.getY());
          clipDragStates.add(state);
        }
      }
    }
  } else {
    currentDragMode = DragMode::Marquee;
    marqueeRect = juce::Rectangle<float>();
    if (!e.mods.isCommandDown())
      clearSelection();
  }

  repaint();
}

void ArrangerComponent::mouseDrag(const juce::MouseEvent &e) {
  auto point = e.position;

  if (currentDragMode == DragMode::Marquee) {
    marqueeRect = juce::Rectangle<float>(dragStartPoint, point);
    selectClipsInRect(marqueeRect);
    repaint();
  }
}

void ArrangerComponent::mouseUp(const juce::MouseEvent &e) {
  currentDragMode = DragMode::None;
  marqueeRect = juce::Rectangle<float>();
  clipDragStates.clear();
  repaint();
}

void ArrangerComponent::mouseMove(const juce::MouseEvent &e) {}

juce::String ArrangerComponent::getTooltip() { return "Arranger View"; }
void ArrangerComponent::mouseDoubleClick(const juce::MouseEvent &e) {}

void ArrangerComponent::mouseWheelMove(const juce::MouseEvent &e,
                                       const juce::MouseWheelDetails &wheel) {
  if (e.mods.isCommandDown()) {
    // Zoom
    pixelsPerBeat *= (1.0 + wheel.deltaY * 0.5);
    pixelsPerBeat = juce::jlimit(10.0, 200.0, pixelsPerBeat);
  } else {
    // Scroll
    viewStartBeats -= wheel.deltaY * 4.0;
    viewStartBeats = juce::jmax(0.0, viewStartBeats);
  }

  recomputeClipBounds();
  repaint();
}

bool ArrangerComponent::keyPressed(const juce::KeyPress &key) {
  if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey) {
    deleteSelectedClips();
    return true;
  }
  return false;
}

//==============================================================================
// ValueTree Listener
//==============================================================================

void ArrangerComponent::valueTreePropertyChanged(
    juce::ValueTree &tree, const juce::Identifier &property) {
  rebuildClipViews();
  recomputeClipBounds();
  repaint();
}

void ArrangerComponent::valueTreeChildAdded(juce::ValueTree &parent,
                                            juce::ValueTree &child) {
  rebuildClipViews();
  recomputeClipBounds();
  repaint();
}

void ArrangerComponent::valueTreeChildRemoved(juce::ValueTree &parent,
                                              juce::ValueTree &child,
                                              int index) {
  rebuildClipViews();
  recomputeClipBounds();
  repaint();
}

void ArrangerComponent::valueTreeChildOrderChanged(juce::ValueTree &parent,
                                                   int oldIndex, int newIndex) {
  rebuildClipViews();
  recomputeClipBounds();
  repaint();
}

//==============================================================================
// Helper Methods
//==============================================================================

void ArrangerComponent::rebuildClipViews() {
  clipViews.clear();

  auto tracksTree = projectState.state.getChildWithName("tracks");
  for (int trackIdx = 0; trackIdx < tracksTree.getNumChildren(); ++trackIdx) {
    auto trackTree = tracksTree.getChild(trackIdx);
    juce::String trackId = trackTree.getProperty("id").toString();

    auto clipsTree = trackTree.getChildWithName("clips");
    for (int clipIdx = 0; clipIdx < clipsTree.getNumChildren(); ++clipIdx) {
      auto clipTree = clipsTree.getChild(clipIdx);

      ClipView view;
      view.clipId = clipTree.getProperty("id").toString();
      view.trackId = trackId;
      view.startBeats = clipTree.getProperty("startBeats", 0.0);
      view.lengthBeats = clipTree.getProperty("lengthBeats", 4.0);
      view.isMidi = (clipTree.getProperty("type").toString() == "midi");
      view.isSelected = isClipSelected(view.clipId);

      clipViews.add(view);
    }
  }
}

void ArrangerComponent::recomputeClipBounds() {
  auto tracksTree = projectState.state.getChildWithName("tracks");

  for (auto &clipView : clipViews) {
    int trackIdx = -1;
    for (int i = 0; i < tracksTree.getNumChildren(); ++i) {
      if (tracksTree.getChild(i).getProperty("id").toString() ==
          clipView.trackId) {
        trackIdx = i;
        break;
      }
    }

    if (trackIdx >= 0) {
      float x = beatsToX(clipView.startBeats);
      float y = trackIndexToY(trackIdx);
      float w = (float)(clipView.lengthBeats * pixelsPerBeat);
      float h = trackHeight - 4.0f;

      clipView.bounds = juce::Rectangle<float>(x, y + 2.0f, w, h);
    }
  }
}

ClipView *ArrangerComponent::findClipView(const juce::String &clipId) {
  for (auto &view : clipViews) {
    if (view.clipId == clipId)
      return &view;
  }
  return nullptr;
}

ClipView *ArrangerComponent::findClipAtPoint(juce::Point<float> point) {
  for (auto &view : clipViews) {
    if (view.bounds.contains(point))
      return &view;
  }
  return nullptr;
}

float ArrangerComponent::beatsToX(double beats) const {
  return (float)((beats - viewStartBeats) * pixelsPerBeat);
}

double ArrangerComponent::xToBeats(float x) const {
  return viewStartBeats + (x / pixelsPerBeat);
}

float ArrangerComponent::trackIndexToY(int trackIndex) const {
  return rulerHeight + trackIndex * trackHeight;
}

int ArrangerComponent::yToTrackIndex(float y) const {
  return (int)((y - rulerHeight) / trackHeight);
}

double ArrangerComponent::snapToGrid(double beats) const {
  return std::round(beats / gridSnapBeats) * gridSnapBeats;
}

void ArrangerComponent::clearSelection() {
  selectedClipIds.clear();
  for (auto &view : clipViews)
    view.isSelected = false;
}

void ArrangerComponent::selectClip(const juce::String &clipId,
                                   bool addToSelection) {
  if (!addToSelection)
    clearSelection();

  selectedClipIds.addIfNotAlreadyThere(clipId);
  auto *view = findClipView(clipId);
  if (view)
    view->isSelected = true;
}

void ArrangerComponent::selectClipsInRect(juce::Rectangle<float> rect) {
  for (auto &view : clipViews) {
    if (rect.intersects(view.bounds)) {
      selectClip(view.clipId, true);
    }
  }
}

bool ArrangerComponent::isClipSelected(const juce::String &clipId) const {
  return selectedClipIds.contains(clipId);
}

void ArrangerComponent::createClipAtPoint(juce::Point<float> point) {}
void ArrangerComponent::deleteSelectedClips() {}
void ArrangerComponent::duplicateSelectedClips() {}

void ArrangerComponent::paintBackground(juce::Graphics &g) {}
void ArrangerComponent::paintTracks(juce::Graphics &g) {}
void ArrangerComponent::paintClips(juce::Graphics &g) {}
void ArrangerComponent::paintTimeRuler(juce::Graphics &g) {}
void ArrangerComponent::paintMarquee(juce::Graphics &g) {}

#ifdef ZENITH_USE_SKIA
void ArrangerComponent::paintToSkia(SkCanvas* canvas, SkRect bounds)
{
    auto& theme = zenith::SkiaTheme::getInstance();
    canvas->clear(theme.getColors().bg1);

    SkPaint p;
    p.setColor(theme.getColors().textStrong);
    // TODO: Implement custom Skia rendering for ArrangerComponent
}
#endif