#pragma once

#ifdef ZENITH_USE_SKIA
#include "skia/SkiaComponent.h"
class SkCanvas;
struct SkRect;
#endif

#include "../../include/ProjectState.h"
#ifdef ZENITH_USE_SKIA
#include "skia/SkiaCanvasComponent.h"
#endif
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

struct ClipView {
  juce::String clipId;
  juce::String trackId;
  double startBeats = 0.0;
  double lengthBeats = 4.0;
  bool isMidi = false;
  bool isSelected = false;
  juce::Rectangle<float> bounds;

  bool isInLeftResizeZone(juce::Point<float> p) const {
    return p.x >= bounds.getX() && p.x < bounds.getX() + 8.0f; // 8px grid: 5→8
  }
  bool isInRightResizeZone(juce::Point<float> p) const {
    return p.x >= bounds.getRight() - 8.0f &&
           p.x <= bounds.getRight(); // 8px grid: 5→8
  }
};

struct ClipDragState {
  juce::String clipId;
  double originalStartBeats;
  int originalTrackIndex;
};

#ifdef ZENITH_USE_SKIA
class ArrangerComponent : public zenith::SkiaCanvasComponent,
#else
class ArrangerComponent : public juce::Component,
#endif
                          public juce::TooltipClient,
                          private juce::Timer,
                          public juce::ValueTree::Listener {
public:
  ArrangerComponent(ProjectState &ps);
  ~ArrangerComponent() override;

#ifndef ZENITH_USE_SKIA
  void paint(juce::Graphics &g) override;
#endif
  void resized() override;

  void mouseDown(const juce::MouseEvent &e) override;
  void mouseDrag(const juce::MouseEvent &e) override;
  void mouseUp(const juce::MouseEvent &e) override;
  void mouseMove(const juce::MouseEvent &e) override;
  juce::String getTooltip() override;
  void mouseDoubleClick(const juce::MouseEvent &e) override;
  void mouseWheelMove(const juce::MouseEvent &e,
                      const juce::MouseWheelDetails &wheel) override;

  bool keyPressed(const juce::KeyPress &key) override;

  // ValueTree::Listener
  void valueTreePropertyChanged(juce::ValueTree &tree,
                                const juce::Identifier &property) override;
  void valueTreeChildAdded(juce::ValueTree &parent,
                           juce::ValueTree &child) override;
  void valueTreeChildRemoved(juce::ValueTree &parent, juce::ValueTree &child,
                             int index) override;
  void valueTreeChildOrderChanged(juce::ValueTree &parent, int oldIndex,
                                  int newIndex) override;
  void valueTreeParentChanged(juce::ValueTree &tree) override {}

#ifdef ZENITH_USE_SKIA
  void paintSkia(SkCanvas &canvas, const juce::Rectangle<int> &bounds) override;
#endif

private:
  void timerCallback() override {} // Unused for now

  ProjectState &projectState;

  juce::Array<ClipView> clipViews;
  juce::StringArray selectedClipIds;

  // View settings (8px grid: trackHeight 64, rulerHeight 32)
  double viewStartBeats = 0.0;
  double pixelsPerBeat = 40.0;
  float trackHeight = 64.0f; // 8px grid: 60->64
  float rulerHeight = 32.0f; // 8px grid: 30->32
  int firstVisibleTrackIndex = 0;

  // Drag state
  enum class DragMode {
    None,
    MoveClips,
    ResizeClipLeft,
    ResizeClipRight,
    Marquee
  };
  DragMode currentDragMode = DragMode::None;
  juce::Point<float> dragStartPoint;
  juce::Array<ClipDragState> clipDragStates;
  juce::String resizingClipId;
  double resizeOriginalStart = 0.0;
  double resizeOriginalLength = 0.0;
  juce::Rectangle<float> marqueeRect;

  // Grid
  double gridSnapBeats = 1.0;

  // Helpers
  void rebuildClipViews();
  void recomputeClipBounds();
  ClipView *findClipView(const juce::String &clipId);
  ClipView *findClipAtPoint(juce::Point<float> point);

  float beatsToX(double beats) const;
  double xToBeats(float x) const;
  float trackIndexToY(int trackIndex) const;
  int yToTrackIndex(float y) const;
  double snapToGrid(double beats) const;

  void clearSelection();
  void selectClip(const juce::String &clipId, bool addToSelection);
  void selectClipsInRect(juce::Rectangle<float> rect);
  bool isClipSelected(const juce::String &clipId) const;

  void createClipAtPoint(juce::Point<float> point);
  void deleteSelectedClips();
  void duplicateSelectedClips();

  void paintBackground(juce::Graphics &g);
  void paintTracks(juce::Graphics &g);
  void paintClips(juce::Graphics &g);
  void paintTimeRuler(juce::Graphics &g);
  void paintMarquee(juce::Graphics &g);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArrangerComponent)
};
